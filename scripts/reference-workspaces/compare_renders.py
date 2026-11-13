#!/usr/bin/env python3
"""Compare supplied concept art with independently captured native JUCE renders.

No rendering, registration, cropping or recolouring of the native UI is performed.
The metrics describe raw, same-coordinate RGB differences, not implementation
quality, functional correctness, accessibility, or a percentage of fidelity.
Requires Pillow; NumPy is optional (Pillow computes the same metrics without it).
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import re
import sys
import tempfile
from pathlib import Path

try:
    import PIL
    from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageStat
except ImportError as exc:
    raise SystemExit("Pillow is required to compare PNG captures; no files were changed.") from exc

try:
    import numpy as np
except ImportError:
    np = None

SIZE = (1586, 992)
SPECS = (
    ("01-obsidian-precision", "01_Obsidian_Precision.png", "reference-01-obsidian"),
    ("02-ivory-atelier", "02_Ivory_Atelier.png", "reference-02-ivory"),
    ("03-spectral-observatory", "03_Spectral_Observatory.png", "reference-03-spectral"),
    ("04-bauhaus-modular", "04_Bauhaus_Modular.png", "reference-04-bauhaus"),
    ("05-space-age-1977", "05_Space_Age_1977.png", "reference-05-space"),
    ("06-tokyo-industrial", "06_Tokyo_Industrial.png", "reference-06-tokyo"),
    ("07-liquid-aurora", "07_Liquid_Aurora.png", "reference-07-aurora"),
    ("08-clearsignal-accessible", "08_ClearSignal_Accessible.png", "reference-08-clear"),
    ("09-signal-flow-cartographer", "09_Signal_Flow_Cartographer.png", "reference-09-flow"),
    ("10-live-performance-deck", "10_Live_Performance_Deck.png", "reference-10-live"),
)
CAPTURE_METHOD = "JUCE Component::createComponentSnapshot"
WARNING = (
    "Raw RGB difference is not a fidelity percentage or pass/fail criterion. Concept art and "
    "native captures may show different real patches, values, waveforms, fonts, window chrome, "
    "and platform antialiasing. Capture metadata is a declaration, not an authenticated "
    "attestation; independently check its source revision and successful workflow."
)


class InvalidEvidence(ValueError):
    """An input is malformed, mismatched, incomplete or unsafe to use."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise InvalidEvidence(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def no_duplicate_keys(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def finite_number(value: object) -> bool:
    return (isinstance(value, (int, float)) and not isinstance(value, bool)
            and math.isfinite(value))


def reject_nonfinite(token: str) -> None:
    raise InvalidEvidence(f"Nonfinite JSON constant: {token}")


def input_file(root: Path, name: str) -> Path:
    require(isinstance(name, str) and Path(name).name == name and "\\" not in name,
            "Capture/reference names must be plain file names")
    path = root / name
    resolved = path.resolve(strict=True)
    require(resolved.is_relative_to(root) and resolved.is_file(), f"Escaping/non-file input: {name}")
    require(resolved.stat().st_size <= 40 * 1024 * 1024, f"Input is unexpectedly large: {name}")
    return resolved


def load_png(path: Path) -> Image.Image:
    with Image.open(path) as source:
        require(source.format == "PNG", f"Not a PNG: {path.name}")
        require(source.size == SIZE, f"{path.name}: expected {SIZE}, found {source.size}")
        require(getattr(source, "n_frames", 1) == 1, f"Animated capture is not allowed: {path.name}")
        rgba = source.convert("RGBA")
        require(rgba.getchannel("A").getextrema() == (255, 255),
                f"Capture/reference must be opaque for visible-pixel comparison: {path.name}")
        return rgba.convert("RGB")


def pixel_metrics(a: Image.Image, b: Image.Image, *, use_numpy: bool = True) -> dict:
    require(a.size == b.size and a.mode == b.mode == "RGB", "Metric inputs must be matching RGB images")
    difference = ImageChops.difference(a, b)
    if np is not None and use_numpy:
        values = np.asarray(difference, dtype=np.float64)
        mae = float(values.mean())
        rmse = float(np.sqrt(np.square(values).mean()))
        changed = int(np.any(values != 0, axis=2).sum())
    else:
        stat = ImageStat.Stat(difference)
        channels = a.width * a.height * 3
        mae = sum(stat.sum) / channels
        rmse = math.sqrt(sum(stat.sum2) / channels)
        red, green, blue = difference.split()
        maximum = ImageChops.lighter(ImageChops.lighter(red, green), blue)
        changed = a.width * a.height - maximum.histogram()[0]
    return {
        "maeRgb0To255": mae,
        "rmseRgb0To255": rmse,
        "differentPixels": changed,
        "differentPixelFraction": changed / (a.width * a.height),
        "identicalRgbPixels": changed == 0,
    }


def validate_inputs(references: Path, captures: Path, expected_commit: str | None) -> tuple[dict, list]:
    require(references.is_dir() and captures.is_dir(), "Reference and capture directories must exist")
    evidence_path = input_file(captures, "capture-evidence.json")
    require(evidence_path.stat().st_size <= 1024 * 1024, "Capture evidence exceeds 1 MiB")
    evidence = json.loads(evidence_path.read_text(encoding="utf-8"),
                          object_pairs_hook=no_duplicate_keys, parse_constant=reject_nonfinite)
    require(isinstance(evidence, dict) and type(evidence.get("schema")) is int
            and evidence["schema"] == 1, "Unsupported evidence schema")
    revision = evidence.get("sourceRevisionAtConfigure")
    require(isinstance(revision, str) and re.fullmatch(r"[0-9a-f]{40}", revision) is not None,
            "A complete 40-character sourceRevisionAtConfigure is required")
    if expected_commit is not None:
        require(re.fullmatch(r"[0-9a-f]{40}", expected_commit) is not None,
                "--expect-commit must be a full lowercase 40-character commit SHA")
        require(revision == expected_commit, "Capture revision does not match --expect-commit")
    require(isinstance(evidence.get("sourceDirtyAtConfigure"), bool), "Missing boolean dirty-source flag")
    require(evidence.get("captureMethod") == CAPTURE_METHOD, "Unexpected or missing native capture method")
    require(finite_number(evidence.get("sampleRate")) and evidence["sampleRate"] > 0,
            "Missing positive capture sample rate")
    require(type(evidence.get("audioBlock")) is int and evidence["audioBlock"] > 0,
            "Missing positive capture block size")
    rows = evidence.get("captures")
    require(isinstance(rows, list) and len(rows) == len(SPECS), "Exactly ten native capture records are required")
    indexed = {}
    for row in rows:
        require(isinstance(row, dict) and isinstance(row.get("id"), str), "Malformed capture row")
        require(row["id"] not in indexed, "Duplicate native capture ID")
        indexed[row["id"]] = row
    require(set(indexed) == {entry[0] for entry in SPECS}, "Native capture IDs do not match all ten skins")
    checked = []
    for identifier, reference_name, package in SPECS:
        row = indexed[identifier]
        require(row.get("image") == identifier + ".png", f"Mismatched image mapping: {identifier}")
        require(row.get("editedImage") == identifier + "-edited.png", f"Mismatched edited image: {identifier}")
        manifest = row.get("manifest")
        require(isinstance(manifest, str) and manifest.replace("\\", "/") ==
                f"resources/data/skins/{package}.surge-skin/workspace.json",
                f"Mismatched manifest mapping: {identifier}")
        require(type(row.get("width")) is int and type(row.get("height")) is int
                and (row["width"], row["height"]) == SIZE, f"Mismatched evidence dimensions: {identifier}")
        require(row.get("kind") == "native JUCE Workspace component snapshot", f"Unexpected capture kind: {identifier}")
        require(type(row.get("resolvedManifestControls")) is int and row["resolvedManifestControls"] > 0,
                f"Missing resolved-control count: {identifier}")
        for key in ("normalizedBefore", "normalizedAfter"):
            require(finite_number(row.get(key)) and 0 <= row[key] <= 1, f"Invalid {key}: {identifier}")
        require(type(row.get("changedPixelsAfterControlEdit")) is int,
                f"Missing control-edit pixel count: {identifier}")
        ref = input_file(references, reference_name)
        native = input_file(captures, row["image"])
        edited = input_file(captures, row["editedImage"])
        reference_image, native_image, edited_image = (load_png(path) for path in (ref, native, edited))
        actual_changed = pixel_metrics(native_image, edited_image)["differentPixels"]
        require(actual_changed == row["changedPixelsAfterControlEdit"],
                f"Edited screenshot differs from declared pixel count: {identifier}")
        checked.append((row, ref, native, edited, reference_image, native_image))
    return evidence, checked


def font(size: int) -> ImageFont.ImageFont:
    try:
        return ImageFont.load_default(size=size)
    except TypeError:  # Older Pillow: still deterministic; no system font substitution.
        return ImageFont.load_default()


def label(draw: ImageDraw.ImageDraw, position: tuple[int, int], text: str, size: int = 18) -> None:
    draw.text(position, text, font=font(size), fill=(236, 240, 244))


def comparison(references: Path, captures: Path, output: Path, expected_commit: str | None = None) -> dict:
    references, captures = references.resolve(strict=True), captures.resolve(strict=True)
    require(not output.is_symlink(), "Output cannot be a symbolic link")
    output = output.resolve()
    require(not output.exists(), "Output already exists; choose a fresh directory (nothing is overwritten)")
    for source in (references, captures):
        require(not output.is_relative_to(source) and not source.is_relative_to(output),
                "Output and input directories must not overlap")
    evidence, checked = validate_inputs(references, captures, expected_commit)
    output.parent.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": 1,
        "comparisonMethod": "Same-coordinate, unregistered, unscaled opaque RGB pixels in PNG encoding",
        "captureMethod": evidence["captureMethod"],
        "sourceRevisionAtConfigure": evidence["sourceRevisionAtConfigure"],
        "sourceDirtyAtConfigure": evidence["sourceDirtyAtConfigure"],
        "expectedCommitMatched": expected_commit is not None,
        "captureEvidenceSha256": sha256(captures / "capture-evidence.json"),
        "pillowVersion": PIL.__version__,
        "metricsBackend": "NumPy " + np.__version__ if np is not None else "Pillow ImageStat",
        "interpretation": WARNING,
        "warnings": [],
        "comparisons": [],
    }
    if evidence["sourceDirtyAtConfigure"]:
        report["warnings"].append("Capture configuration reported a dirty source tree; the commit alone is not a complete source attestation.")
    thumb_width, thumb_height = 480, 300
    row_height, margin = 338, 16
    paired_sheet = Image.new("RGB", (2 * thumb_width + 3 * margin, 72 + 10 * row_height), (21, 25, 30))
    native_sheet = Image.new("RGB", (2 * thumb_width + 3 * margin, 72 + 5 * row_height), (21, 25, 30))
    paired_draw, native_draw = ImageDraw.Draw(paired_sheet), ImageDraw.Draw(native_sheet)
    label(paired_draw, (margin, 12), "REFERENCE (left) / NATIVE RENDER (right)", 22)
    label(native_draw, (margin, 12), "NATIVE JUCE RENDERS - no reference art composited", 22)
    for draw in (paired_draw, native_draw):
        label(draw, (margin, 43), "Source: " + evidence["sourceRevisionAtConfigure"], 15)
    # Only fresh, fixed-name outputs are written; all images are validated first.
    with tempfile.TemporaryDirectory(prefix=".reference-comparison-", dir=output.parent) as temporary:
        staging = Path(temporary) / "result"
        staging.mkdir()
        for index, (row, ref_path, native_path, edited_path, reference_image, native_image) in enumerate(checked):
            identifier = row["id"]
            metrics = pixel_metrics(reference_image, native_image)
            if metrics["identicalRgbPixels"]:
                report["warnings"].append(identifier + ": reference and native RGB pixels are identical; verify independent native origin, not only visual similarity.")
            if row["changedPixelsAfterControlEdit"] <= 0:
                report["warnings"].append(identifier + ": control-edit capture shows no changed pixels; inspect the native test result.")
            record = {
                "id": identifier, "reference": ref_path.name, "native": native_path.name,
                "referenceSha256": sha256(ref_path), "nativeSha256": sha256(native_path),
                "editedNativeSha256": sha256(edited_path), "width": SIZE[0], "height": SIZE[1],
                "patch": row.get("patch", "unspecified"), "audioFixture": row.get("audioFixture", "unspecified"),
                "verifiedEditedPixelCount": row["changedPixelsAfterControlEdit"],
                "metrics": metrics,
                "sideBySide": identifier + "-comparison.png", "absoluteDifference": identifier + "-difference.png",
            }
            report["comparisons"].append(record)
            pair = Image.new("RGB", (SIZE[0] * 2 + 24, SIZE[1] + 66), (21, 25, 30))
            draw = ImageDraw.Draw(pair)
            label(draw, (12, 8), "REFERENCE CONCEPT - " + identifier, 22)
            label(draw, (SIZE[0] + 12, 8), "NATIVE JUCE RENDER - " + evidence["sourceRevisionAtConfigure"][:12], 22)
            pair.paste(reference_image, (8, 56))
            pair.paste(native_image, (SIZE[0] + 16, 56))
            pair.save(staging / record["sideBySide"], compress_level=6)
            diff = Image.new("RGB", (SIZE[0], SIZE[1] + 48), (21, 25, 30))
            label(ImageDraw.Draw(diff), (10, 10), "ABSOLUTE RGB DIFFERENCE - unamplified, not a native screenshot", 20)
            diff.paste(ImageChops.difference(reference_image, native_image), (0, 48))
            diff.save(staging / record["absoluteDifference"], compress_level=6)
            thumb_ref = reference_image.resize((thumb_width, thumb_height), Image.Resampling.LANCZOS)
            thumb_native = native_image.resize((thumb_width, thumb_height), Image.Resampling.LANCZOS)
            y = 72 + index * row_height
            label(paired_draw, (margin, y), identifier, 17)
            paired_sheet.paste(thumb_ref, (margin, y + 26))
            paired_sheet.paste(thumb_native, (2 * margin + thumb_width, y + 26))
            x = margin + (index % 2) * (thumb_width + margin)
            y = 72 + (index // 2) * row_height
            label(native_draw, (x, y), identifier, 17)
            native_sheet.paste(thumb_native, (x, y + 26))
        paired_sheet.save(staging / "comparison-contact-sheet.png", compress_level=6)
        native_sheet.save(staging / "native-contact-sheet.png", compress_level=6)
        (staging / "comparison-report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False, allow_nan=False) + "\n", encoding="utf-8")
        (staging / "README.txt").write_text(
            "REFERENCE CONCEPT versus NATIVE JUCE RENDER\n\n" + WARNING + "\n\n"
            "Full-resolution pairs preserve original pixels. Contact sheets alone are resized.\n"
            "Difference images are absolute RGB differences without contrast amplification.\n"
            "No crop, geometric alignment, reference overlay, or native-image editing was applied.\n"
            "Input files were not modified. See comparison-report.json for hashes and provenance.\n",
            encoding="utf-8")
        staging.rename(output)
    return report


def self_test() -> None:
    """Synthetic data only: correctness/error paths, never actual UI evidence."""
    with tempfile.TemporaryDirectory(prefix="surge-comparison-selftest-") as temporary:
        root = Path(temporary)
        references, captures = root / "references", root / "captures"
        references.mkdir()
        captures.mkdir()
        revision = "a" * 40
        evidence = {"schema": 1, "captureMethod": CAPTURE_METHOD, "sourceRevisionAtConfigure": revision,
                    "sourceDirtyAtConfigure": False, "sampleRate": 48000, "audioBlock": 512, "captures": []}
        for index, (identifier, reference, package) in enumerate(SPECS):
            source = Image.new("RGB", SIZE, (24 + index * 12, 37 + index * 3, 65))
            source.save(references / reference)
            native = source.copy()
            native.putpixel((7, 9), (25 + index * 12, 37 + index * 3, 65))
            native.save(captures / (identifier + ".png"))
            edited = native.copy()
            edited.putpixel((11, 13), (26 + index * 12, 37 + index * 3, 65))
            edited.save(captures / (identifier + "-edited.png"))
            evidence["captures"].append({"id": identifier, "image": identifier + ".png", "editedImage": identifier + "-edited.png",
                "manifest": f"resources/data/skins/{package}.surge-skin/workspace.json", "width": SIZE[0], "height": SIZE[1],
                "kind": "native JUCE Workspace component snapshot", "resolvedManifestControls": 1,
                "normalizedBefore": .2, "normalizedAfter": .4, "changedPixelsAfterControlEdit": 1,
                "patch": "SYNTHETIC TEST FIXTURE - NOT A SYNTH CAPTURE"})
        evidence_file = captures / "capture-evidence.json"
        def save(data: dict) -> None:
            evidence_file.write_text(json.dumps(data), encoding="utf-8")
        save(evidence)
        before = {str(path): sha256(path) for folder in (references, captures) for path in folder.iterdir()}
        report = comparison(references, captures, root / "output", revision)
        assert len(report["comparisons"]) == 10
        assert len(list((root / "output").iterdir())) == 24
        m = report["comparisons"][0]["metrics"]
        assert m["differentPixels"] == 1
        assert math.isclose(m["maeRgb0To255"], 1 / (SIZE[0] * SIZE[1] * 3))
        assert math.isclose(m["rmseRgb0To255"], math.sqrt(1 / (SIZE[0] * SIZE[1] * 3)))
        assert before == {str(path): sha256(path) for folder in (references, captures) for path in folder.iterdir()}
        comparison(references, captures, root / "repeat-output", revision)
        first_hashes = {path.name: sha256(path) for path in (root / "output").iterdir()}
        repeat_hashes = {path.name: sha256(path) for path in (root / "repeat-output").iterdir()}
        assert first_hashes == repeat_hashes
        a, b = load_png(references / SPECS[0][1]), load_png(captures / (SPECS[0][0] + ".png"))
        assert pixel_metrics(a, b) == pixel_metrics(a, b, use_numpy=False)
        failures = 0
        def rejects(call) -> None:
            nonlocal failures
            try:
                call()
            except (InvalidEvidence, ValueError, OSError):
                failures += 1
            else:
                raise AssertionError("Expected malformed/unsafe input to be rejected")
        changes = (
            lambda d: d["captures"].pop(),
            lambda d: d["captures"][0].update(id=d["captures"][1]["id"]),
            lambda d: d["captures"][0].update(image="../outside.png"),
            lambda d: d["captures"][0].update(manifest="other/workspace.json"),
            lambda d: d["captures"][0].update(width=800),
            lambda d: d["captures"][0].update(changedPixelsAfterControlEdit=999),
            lambda d: d.update(sourceRevisionAtConfigure="short"),
            lambda d: d.update(sourceDirtyAtConfigure="false"),
            lambda d: d.update(captureMethod="generated mockup"),
        )
        for change in changes:
            mutated = copy.deepcopy(evidence)
            change(mutated)
            save(mutated)
            rejects(lambda: validate_inputs(references, captures, revision))
        save(evidence)
        rejects(lambda: validate_inputs(references, captures, "b" * 40))
        rejects(lambda: comparison(references, captures, references / "output", revision))
        rejects(lambda: comparison(references, captures, root / "output", revision))
        saved = (captures / (SPECS[0][0] + ".png")).read_bytes()
        Image.new("RGB", (32, 32)).save(captures / (SPECS[0][0] + ".png"))
        rejects(lambda: validate_inputs(references, captures, revision))
        (captures / (SPECS[0][0] + ".png")).write_bytes(saved)
        Image.new("RGBA", SIZE, (0, 0, 0, 128)).save(captures / (SPECS[0][0] + ".png"))
        rejects(lambda: validate_inputs(references, captures, revision))
        (captures / (SPECS[0][0] + ".png")).write_bytes(saved)
        reference_file = references / SPECS[0][1]
        reference_bytes = reference_file.read_bytes()
        outside = root / "outside.png"
        outside.write_bytes(reference_bytes)
        reference_file.unlink()
        reference_file.symlink_to(outside)
        rejects(lambda: validate_inputs(references, captures, revision))
        reference_file.unlink()
        reference_file.write_bytes(reference_bytes)
        evidence_file.write_text('{"schema":1,"schema":1}', encoding="utf-8")
        rejects(lambda: validate_inputs(references, captures, revision))
        evidence_file.write_text('{"schema":1,"unknown":NaN}', encoding="utf-8")
        rejects(lambda: validate_inputs(references, captures, revision))
        print(f"PASS: synthetic ten-ID mapping, dimensions, metrics/backends, 24 deterministic outputs, unchanged inputs; {failures} rejection cases")
        print("Synthetic tests are not reference-fidelity or native-render evidence.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--references", type=Path, help="Directory containing the ten original reference PNGs")
    parser.add_argument("--captures", type=Path, help="Native artifact directory containing PNGs and capture-evidence.json")
    parser.add_argument("--output", type=Path, help="Fresh output directory; existing directories are never overwritten")
    parser.add_argument("--expect-commit", help="Optional full commit SHA independently established from CI")
    parser.add_argument("--self-test", action="store_true", help="Exercise synthetic fixtures only; creates no permanent outputs")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
            return 0
        if not all((args.references, args.captures, args.output)):
            parser.error("--references, --captures, and --output are required unless --self-test is used")
        report = comparison(args.references, args.captures, args.output, args.expect_commit)
        print(f"Compared {len(report['comparisons'])} native captures; source {report['sourceRevisionAtConfigure']}")
        print(WARNING)
        for warning in report["warnings"]:
            print("WARNING:", warning)
        return 0
    except (InvalidEvidence, ValueError, OSError) as error:
        print(f"Comparison refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
