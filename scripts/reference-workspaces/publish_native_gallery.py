#!/usr/bin/env python3
"""Validate and copy unmodified native captures into the committed gallery.

The native workflow, not this script, establishes that the build and tests passed.
This script requires complete, clean, source-matched capture evidence and passing
logs, and never substitutes reference concepts for native screenshots.
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import hashlib
import html
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib

CANVAS = (1586, 992)
MAX_FILE_BYTES = 10 * 1024 * 1024
SKINS = (
    ("01-obsidian-precision", "Obsidian Precision", "01_Obsidian_Precision.png", "reference-01-obsidian"),
    ("02-ivory-atelier", "Ivory Atelier", "02_Ivory_Atelier.png", "reference-02-ivory"),
    ("03-spectral-observatory", "Spectral Observatory", "03_Spectral_Observatory.png", "reference-03-spectral"),
    ("04-bauhaus-modular", "Bauhaus Modular", "04_Bauhaus_Modular.png", "reference-04-bauhaus"),
    ("05-space-age-1977", "Space-Age 1977", "05_Space_Age_1977.png", "reference-05-space"),
    ("06-tokyo-industrial", "Tokyo Industrial", "06_Tokyo_Industrial.png", "reference-06-tokyo"),
    ("07-liquid-aurora", "Liquid Aurora", "07_Liquid_Aurora.png", "reference-07-aurora"),
    ("08-clearsignal-accessible", "ClearSignal Accessible", "08_ClearSignal_Accessible.png", "reference-08-clear"),
    ("09-signal-flow-cartographer", "Signal-Flow Cartographer", "09_Signal_Flow_Cartographer.png", "reference-09-flow"),
    ("10-live-performance-deck", "Live Performance Deck", "10_Live_Performance_Deck.png", "reference-10-live"),
)
EVIDENCE_FILES = (
    "capture-evidence.json",
    "build-provenance.txt",
    "native-tests.log",
    "native-capture.log",
    "audio-tap.log",
    "audio-tap-tsan.log",
    "manifest-validation.log",
    "schema-self-test.log",
)
SCOPE = (
    "Native JUCE component snapshots attached to the actual Surge XT processor. "
    "These are not OS desktop or DAW captures. Reference concepts and edited "
    "native states are labelled separately. Passing functional tests does not "
    "establish pixel-identical reference fidelity."
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def unique_object(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, "Duplicate JSON field: " + key)
        result[key] = value
    return result


def parse_json(data: bytes) -> object:
    def reject_constant(value: str) -> None:
        raise ValueError("Nonfinite JSON value: " + value)
    return json.loads(data, object_pairs_hook=unique_object, parse_constant=reject_constant)


def read_artifact(root: Path, name: str) -> bytes:
    path = root / name
    require(path.is_file() and not path.is_symlink(), "Missing or linked artifact: " + name)
    require(path.resolve().parent == root, "Artifact leaves its directory: " + name)
    size = path.stat().st_size
    require(0 < size <= MAX_FILE_BYTES, "Artifact size outside bounds: " + name)
    return path.read_bytes()


def verify_png(data: bytes, name: str) -> None:
    require(data.startswith(b"\x89PNG\r\n\x1a\n"), "Invalid PNG signature: " + name)
    offset = 8
    first = True
    saw_data = False
    while offset + 12 <= len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        end = offset + 12 + length
        require(end <= len(data), "Truncated PNG chunk: " + name)
        payload = data[offset + 8:offset + 8 + length]
        expected_crc = struct.unpack(">I", data[end - 4:end])[0]
        require(zlib.crc32(kind + payload) & 0xffffffff == expected_crc,
                "PNG checksum mismatch: " + name)
        if first:
            require(kind == b"IHDR" and length == 13, "Missing PNG header: " + name)
            require(struct.unpack(">II", payload[:8]) == CANVAS,
                    "PNG dimensions must be 1586 x 992: " + name)
            first = False
        else:
            require(kind != b"IHDR", "Duplicate PNG header: " + name)
        saw_data |= kind == b"IDAT" and length > 0
        offset = end
        if kind == b"IEND":
            require(length == 0 and saw_data and offset == len(data),
                    "Invalid PNG end or image data: " + name)
            return
    raise ValueError("Missing complete PNG end: " + name)


def source_blob(repo: Path, revision: str, path: str) -> bytes:
    result = subprocess.run(
        ["git", "-C", str(repo), "show", revision + ":" + path],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
    )
    require(result.returncode == 0, "Cannot read source-versioned file: " + path)
    return result.stdout


def passing_summary(data: bytes, name: str) -> dict:
    text = data.decode("utf-8")
    matches = re.findall(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    require(len(matches) == 1, "Missing unique passing test summary: " + name)
    require(not re.search(r"\bFAILED\b|fatal error:|ThreadSanitizer: data race", text),
            "Failure text in passing test log: " + name)
    assertions, cases = map(int, matches[0])
    require(assertions > 0 and cases > 0, "Empty test result: " + name)
    return {"testCases": cases, "assertions": assertions}


def validate_bundle(artifacts: Path, repo: Path, source: str, run_id: str, repository: str) -> dict:
    require(re.fullmatch(r"[0-9a-f]{40}", source) is not None, "Source must be a full commit SHA")
    require(re.fullmatch(r"[1-9][0-9]*", run_id) is not None, "Run ID must be a positive integer")
    require(re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository) is not None,
            "Repository must be owner/name")
    require(artifacts.is_dir() and not artifacts.is_symlink(), "Artifacts must be a real directory")
    artifacts = artifacts.resolve()
    repo = repo.resolve()
    files = {name: read_artifact(artifacts, name) for name in EVIDENCE_FILES}
    evidence = parse_json(files["capture-evidence.json"])
    require(isinstance(evidence, dict), "Capture evidence must be an object")
    require(evidence.get("schema") == 1 and evidence.get("complete") is True,
            "Capture evidence is not complete")
    require(evidence.get("sourceRevisionAtConfigure") == source,
            "Capture source differs from requested source")
    require(evidence.get("sourceDirtyAtConfigure") is False, "Capture source was dirty")
    require(evidence.get("expectedCaptures") == 10 and evidence.get("capturedCount") == 10,
            "Capture evidence must account for all ten skins")
    require(evidence.get("captureMethod") == "JUCE Component::createComponentSnapshot",
            "Unsupported native capture method")
    require(evidence.get("target") == "surge-xt-tests [reference-render]",
            "Unexpected native capture target")
    captures = evidence.get("captures")
    require(isinstance(captures, list) and len(captures) == 10,
            "Expected ten native capture records")
    require(all(isinstance(c, dict) and isinstance(c.get("id"), str) for c in captures),
            "Invalid capture record")
    by_id = {c["id"]: c for c in captures}
    require(len(by_id) == 10 and set(by_id) == {s[0] for s in SKINS},
            "Duplicate, missing or unknown skin IDs")
    run_url = "https://github.com/" + repository + "/actions/runs/" + run_id
    provenance = files["build-provenance.txt"].decode("utf-8")
    require("Source commit: " + source in provenance, "Build provenance source mismatch")
    require("Workflow run: " + run_url in provenance, "Build provenance run mismatch")
    validation = {
        "behavior": passing_summary(files["native-tests.log"], "native-tests.log"),
        "capture": passing_summary(files["native-capture.log"], "native-capture.log"),
    }
    for name in ("audio-tap.log", "audio-tap-tsan.log"):
        text = files[name].decode("utf-8")
        require("PASS: AudioTap boundaries" in text and
                "ThreadSanitizer: data race" not in text,
                "Missing passing audio-display stress result: " + name)
    require("PASS: 10 reference workspaces validated" in files["manifest-validation.log"].decode("utf-8"),
            "Missing ten-manifest validation result")
    require(files["schema-self-test.log"].decode("utf-8").startswith("PASS:"),
            "Missing schema self-test result")
    records = []
    for skin_id, title, reference_name, package in SKINS:
        capture = by_id[skin_id]
        native_name, edited_name = skin_id + ".png", skin_id + "-edited.png"
        manifest_path = "resources/data/skins/" + package + ".surge-skin/workspace.json"
        reference_path = "doc/reference-workspaces/references/" + reference_name
        require(capture.get("image") == native_name and capture.get("editedImage") == edited_name,
                "Capture image names differ from expected native files: " + skin_id)
        require(capture.get("manifest") == manifest_path, "Capture manifest mismatch: " + skin_id)
        require((capture.get("width"), capture.get("height")) == CANVAS,
                "Capture record dimensions differ from canvas: " + skin_id)
        changed = capture.get("changedPixelsAfterControlEdit")
        require(isinstance(changed, int) and not isinstance(changed, bool) and
                0 < changed <= CANVAS[0] * CANVAS[1], "No recorded native edit: " + skin_id)
        manifest = parse_json(source_blob(repo, source, manifest_path))
        require(manifest.get("id") == skin_id and manifest.get("reference") == reference_name,
                "Source manifest does not identify this reference: " + skin_id)
        reference = source_blob(repo, source, reference_path)
        verify_png(reference, reference_name)
        require((repo / reference_path).read_bytes() == reference,
                "Current reference bytes differ from captured source: " + reference_name)
        for name in (native_name, edited_name):
            files[name] = read_artifact(artifacts, name)
            verify_png(files[name], name)
        require(files[native_name] != files[edited_name], "Native edit image is unchanged: " + skin_id)
        records.append({
            "id": skin_id, "name": title, "native": native_name, "edited": edited_name,
            "reference": "../references/" + reference_name,
            "referenceSha256": sha256(reference),
            "nativeSha256": sha256(files[native_name]),
            "editedSha256": sha256(files[edited_name]),
            "manifest": manifest_path, "editedBinding": capture.get("editedBinding", ""),
            "normalizedBefore": capture.get("normalizedBefore"),
            "normalizedAfter": capture.get("normalizedAfter"),
            "recordedChangedPixels": changed,
        })
    require(sum(map(len, files.values())) <= 100 * 1024 * 1024, "Capture bundle exceeds 100 MiB")
    return {
        "source": source, "repository": repository, "runId": run_id, "runUrl": run_url,
        "commitUrl": "https://github.com/" + repository + "/commit/" + source,
        "generatedUtc": evidence.get("generatedUtc", ""),
        "canvas": list(CANVAS), "scope": SCOPE, "validation": validation,
        "captures": records, "files": files,
    }


def html_document(bundle: dict) -> str:
    esc = html.escape
    source, run = bundle["source"], bundle["runUrl"]
    links = "\n".join(
        '<a href="#' + c["id"] + '">' + c["id"][:2] + " " + esc(c["name"]) + "</a>"
        for c in bundle["captures"])
    sections = []
    for c in bundle["captures"]:
        title = c["id"][:2] + " / " + c["name"]
        sections.append(
            '<section id="' + c["id"] + '"><h2>' + esc(title) + '</h2>'
            '<div class="pair">'
            '<figure><a href="' + c["reference"] + '"><img src="' + c["reference"] +
            '" width="1586" height="992" loading="lazy" alt="Reference concept: ' + esc(c["name"]) +
            '"></a><figcaption><b>Reference concept</b> — supplied design target</figcaption></figure>'
            '<figure><a href="' + c["native"] + '"><img src="' + c["native"] +
            '" width="1586" height="992" loading="lazy" alt="Native baseline: ' + esc(c["name"]) +
            '"></a><figcaption><b>Native baseline</b> — compiled processor and real fixture</figcaption></figure>'
            '</div><details><summary>Native image after editing ' + esc(str(c["editedBinding"])) +
            '</summary><figure class="edited"><a href="' + c["edited"] + '"><img src="' +
            c["edited"] + '" width="1586" height="992" loading="lazy" alt="Edited native state: ' +
            esc(c["name"]) + '"></a><figcaption><b>Native edited state</b> — a real control callback; '
            'the capture recorded ' + str(c["recordedChangedPixels"]) +
            ' changed pixels.</figcaption></figure></details></section>')
    validation = bundle["validation"]
    return """<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ten native Surge XT reference workspaces</title>
<style>
:root{color-scheme:dark;--bg:#0d141d;--panel:#15212c;--text:#e7edf1;--muted:#a9bac6;--line:#314450;--accent:#8ee0c0}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.5 system-ui,sans-serif}
header,main,footer{max-width:1800px;margin:auto;padding:32px clamp(18px,3vw,54px)}
header{border-bottom:1px solid var(--line)}h1{font-size:clamp(27px,3vw,46px);line-height:1.15;margin:8px 0 18px}
h2{font-size:24px;margin:0 0 18px}p{max-width:95ch;color:var(--muted)}a{color:var(--accent);text-decoration-thickness:1px;text-underline-offset:3px}
.eyebrow{color:var(--accent);font-size:12px;letter-spacing:.16em;text-transform:uppercase}
.provenance{display:flex;gap:12px 28px;flex-wrap:wrap;margin:22px 0}
nav{display:flex;flex-wrap:wrap;gap:8px}nav a{border:1px solid var(--line);padding:7px 11px;border-radius:5px;text-decoration:none;font-size:13px}
section{padding:28px 0 36px;border-bottom:1px solid var(--line);scroll-margin-top:20px}
.pair{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:18px}
figure{margin:0;min-width:0}img{display:block;width:100%;height:auto;border:1px solid var(--line);border-radius:4px;background:#081017}
figcaption{padding:10px 2px 14px;color:var(--muted);font-size:13px}figcaption b{color:var(--text)}
details{background:var(--panel);border:1px solid var(--line);border-radius:5px;margin-top:6px}
summary{cursor:pointer;padding:12px 16px}.edited{padding:0 16px 8px;max-width:1300px;margin:auto}
a:focus-visible,summary:focus-visible{outline:3px solid var(--accent);outline-offset:4px}
footer{font-size:14px;color:var(--muted)}.source{overflow-wrap:anywhere}
@media(max-width:850px){.pair{grid-template-columns:1fr}header,main,footer{padding:22px 16px}}
</style></head><body><header>
<div class="eyebrow">Surge XT / native evidence</div><h1>Ten reference workspaces</h1>
<p>Actual JUCE component snapshots from the tested synthesizer, each 1586 × 992 pixels.
The left image is the supplied concept; the right is the compiled native baseline.
Open an image for its original resolution, or expand the edited native state.</p>
<div class="provenance"><a class="source" href=""" + '"' + esc(bundle["commitUrl"]) + '">' + \
        esc(source) + '</a><a href="' + esc(run) + '">Native workflow run ' + bundle["runId"] + \
        '</a><a href="capture-evidence.json">Capture evidence</a><a href="gallery-manifest.json">Image hashes</a></div><p>' + \
        str(validation["behavior"]["testCases"]) + " behavior cases / " + \
        format(validation["behavior"]["assertions"], ",") + " assertions; " + \
        format(validation["capture"]["assertions"], ",") + \
        ' native capture assertions. Normal and ThreadSanitizer audio-display checks passed.</p><nav aria-label="Workspaces">' + \
        links + '</nav></header><main>' + "\n".join(sections) + \
        '</main><footer><p>' + esc(SCOPE) + \
        '</p><p><a href="../../reference-workspaces-user-guide.md">Native controls and adaptation guide</a> · ' + \
        '<a href="build-provenance.txt">Build provenance</a> · <a href="native-tests.log">Behavior log</a> · ' + \
        '<a href="native-capture.log">Capture log</a></p></footer></body></html>\n'


def markdown_document(bundle: dict) -> str:
    validation = bundle["validation"]
    lines = [
        "# Native screenshot gallery", "",
        "Twenty unmodified native PNGs: a baseline and a real edited state for each of the ten skins.",
        "Each image is 1586 × 992 pixels.", "",
        "**Source:** [" + bundle["source"] + "](" + bundle["commitUrl"] + ")  ",
        "**Native verification:** [workflow run " + bundle["runId"] + "](" + bundle["runUrl"] + ")  ",
        "**Capture time:** " + str(bundle["generatedUtc"]), "",
        "Behavior: " + str(validation["behavior"]["testCases"]) + " cases / " +
        format(validation["behavior"]["assertions"], ",") + " assertions. Capture: " +
        format(validation["capture"]["assertions"], ",") +
        " assertions. Normal and ThreadSanitizer audio-display checks passed.", "",
        "[Open the comparison HTML](index.html) after downloading this directory with its sibling references directory. "
        "This README also renders the native gallery directly on GitHub.", "",
        "[Capture evidence](capture-evidence.json) · [Build provenance](build-provenance.txt) · "
        "[Image hashes](gallery-manifest.json) · [Native control guide](../../reference-workspaces-user-guide.md)", "",
        SCOPE, "",
        "| Skin | Reference concept | Native baseline | Native edited state |",
        "| --- | --- | --- | --- |",
    ]
    for c in bundle["captures"]:
        lines.append("| " + c["id"][:2] + " " + c["name"] + " | [Reference](" +
                     c["reference"] + ") | [Native](" + c["native"] +
                     ") | [Edited](" + c["edited"] + ") |")
    for c in bundle["captures"]:
        lines.extend(["", "## " + c["id"][:2] + " " + c["name"], "",
                      "![Native baseline: " + c["name"] + "](" + c["native"] + ")", "",
                      "[Reference concept](" + c["reference"] + ") · [Native after editing " +
                      str(c["editedBinding"]) + "](" + c["edited"] + ")"])
    return "\n".join(lines) + "\n"


def write_gallery(bundle: dict, output: Path) -> None:
    require(not output.is_symlink(), "Output directory must not be a symlink")
    require(not output.exists() or output.is_dir(), "Output must be a directory")
    files = dict(bundle["files"])
    files["index.html"] = html_document(bundle).encode("utf-8")
    files["README.md"] = markdown_document(bundle).encode("utf-8")
    manifest = {key: value for key, value in bundle.items() if key != "files"}
    manifest["schema"] = 1
    manifest["files"] = {name: {"bytes": len(data), "sha256": sha256(data)}
                         for name, data in sorted(files.items())}
    files["gallery-manifest.json"] = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    output.mkdir(parents=True, exist_ok=True)
    # Validation completes before any output is written. Replace each managed
    # file through a sibling temporary file so an existing symlink is not followed.
    # Unknown files are preserved; the publisher starts from a clean checkout.
    for name, data in sorted(files.items()):
        fd, temporary = tempfile.mkstemp(prefix=".native-gallery-", dir=output)
        try:
            with os.fdopen(fd, "wb") as stream:
                stream.write(data)
            os.replace(temporary, output / name)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--source", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--repository", default="tomasedisonn80/surgeXT-synth")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    try:
        bundle = validate_bundle(args.artifacts, args.repo_root, args.source, args.run_id, args.repository)
        require(args.output.resolve() != args.artifacts.resolve(),
                "Gallery output must differ from the input artifact")
        write_gallery(bundle, args.output)
    except (OSError, ValueError, KeyError, TypeError, UnicodeError) as error:
        print("Gallery rejected: " + str(error), file=sys.stderr)
        return 2
    print("Validated and copied 20 native PNGs; source " + args.source +
          "; workflow run " + args.run_id + ". Reference images remain separately labelled.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
