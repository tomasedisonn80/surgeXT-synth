#!/usr/bin/env python3
"""Build a validated, reproducible *data-only* ZIP for the ten native workspaces.

Requires Python 3.10+ and a source checkout. No network, third-party modules,
binary installation, reference-image copying, or screenshot fabrication occurs.
The modified Surge XT native renderer is required to use the packaged skins.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

import validate


PREFIX = "surge-xt-reference-workspaces"
ZIP_TIME = (1980, 1, 1, 0, 0, 0)
PACKAGE_NAME = re.compile(r"reference-[0-9]{2}-[a-z][a-z0-9-]*\.surge-skin\Z")
SOURCE_LIMIT = 8 * 1024 * 1024


class PackagingError(ValueError):
    """A package input or output failed an explicit safety/consistency check."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise PackagingError(message)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def json_bytes(value: object) -> bytes:
    return (json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n").encode("utf-8")


def read_source(path: Path, root: Path, limit: int = SOURCE_LIMIT) -> bytes:
    """Read bounded, regular inputs; do not silently package symlink targets."""
    relative = path.relative_to(root)
    current = root
    for part in relative.parts:
        current /= part
        require(not current.is_symlink(), f"Source symlinks are not packaged: {relative}")
    require(path.is_file(), f"Missing regular source file: {relative}")
    with path.open("rb") as stream:
        data = stream.read(limit + 1)
    require(len(data) <= limit, f"Source file exceeds {limit} bytes: {relative}")
    return data


def git_value(root: Path, *arguments: str) -> str | None:
    try:
        result = subprocess.run(["git", "-C", str(root), *arguments], check=True,
                                capture_output=True, text=True, timeout=15)
    except (OSError, subprocess.SubprocessError):
        return None
    return result.stdout.strip()


def aggregate_hash(files: dict[str, bytes]) -> str:
    # Hash names as well as content, using a line-oriented, unambiguous manifest.
    return digest("".join(f"{digest(data)}  {name}\n" for name, data in sorted(files.items()))
                  .encode("utf-8"))


def build_payload(root: Path) -> tuple[dict[str, bytes], dict]:
    root = root.resolve(strict=True)
    report = validate.validate_packages(root, allow_partial=False, strict_overlap=True)
    require(report["ok"], "Package validation failed: " + "; ".join(report["errors"]))

    payload: dict[str, bytes] = {}
    inputs: dict[str, bytes] = {}
    workspaces = []
    names: set[str] = set()
    ids: set[str] = set()
    styles: set[str] = set()
    skins = root / "resources/data/skins"
    packages = sorted(skins.glob("reference-*.surge-skin"))
    require(len(packages) == 10, f"Expected ten named reference packages, found {len(packages)}")

    for package in packages:
        require(bool(PACKAGE_NAME.fullmatch(package.name)), f"Unexpected package name: {package.name}")
        require(package.is_dir() and not package.is_symlink(), f"Invalid package directory: {package.name}")
        xml_path = package / "skin.xml"
        manifest_path = package / "workspace.json"
        xml_data = read_source(xml_path, root, validate.MAX_BYTES)
        manifest_data = read_source(manifest_path, root, validate.MAX_BYTES)
        require(b"<!DOCTYPE" not in xml_data.upper(), f"DTD is not allowed: {package.name}")
        xml = ET.fromstring(xml_data)
        tags = xml.findall("./globals/reference-workspace")
        require(xml.tag == "surge-skin" and xml.get("version") == "2", "Expected skin schema 2")
        require(len(tags) == 1 and tags[0].get("layout") == "workspace.json",
                f"Expected an adjacent workspace.json in {package.name}")
        require(all(xml.find(section) is not None for section in ("globals", "component-classes", "controls")),
                f"Missing skin XML section in {package.name}")
        name = xml.get("name", "").strip()
        require(bool(name) and name not in names, "Missing or duplicate skin display name")
        names.add(name)

        # Validate exactly the captured bytes, not merely a file read earlier.
        data = validate.load_bytes(manifest_data)
        require(not validate.overlaps(data["controls"]), f"Overlapping live controls in {package.name}")
        require(data["id"] not in ids and data["style"] not in styles, "Duplicate workspace ID/style")
        ids.add(data["id"])
        styles.add(data["style"])
        for path, raw in ((xml_path, xml_data), (manifest_path, manifest_data)):
            inputs[path.relative_to(root).as_posix()] = raw
            payload[f"skins/{package.name}/{path.name}"] = raw
        workspaces.append({"id": data["id"], "name": name, "style": data["style"],
                           "package": f"skins/{package.name}", "adaptations": data["adaptations"],
                           "manifestSha256": digest(manifest_data), "skinXmlSha256": digest(xml_data)})

    for source, destination in (("doc/reference-workspaces-user-guide.md", "README.md"),
                                ("LICENSE", "LICENSE")):
        raw = read_source(root / source, root)
        inputs[source] = raw
        payload[destination] = raw

    adaptations = ["# Reference-image adaptations", "",
                   "This data-only pack requires the modified native Surge XT build.",
                   "These notes are copied from the captured manifests, not a fidelity certification.", ""]
    for workspace in workspaces:
        adaptations.extend([f"## {workspace['name']}", ""])
        adaptations.extend(f"- {note}" for note in workspace["adaptations"])
        adaptations.append("")
    payload["ADAPTATIONS.md"] = ("\n".join(adaptations) + "\n").encode("utf-8")

    # The native implementation is not included in this ZIP; record its current
    # source bytes to distinguish a dirty snapshot from a compiled attestation.
    code_paths = sorted((root / "src/surge-xt/gui/reference").glob("*.[ch]*"))
    code_paths += [root / relative for relative in (
        "src/surge-xt/SurgeSynthEditor.cpp", "src/surge-xt/SurgeSynthEditor.h",
        "src/surge-xt/SurgeSynthProcessor.cpp", "src/surge-xt/SurgeSynthProcessor.h",
        "src/surge-xt/CMakeLists.txt", "scripts/reference-workspaces/validate.py",
        "scripts/reference-workspaces/package.py")]
    code = {path.relative_to(root).as_posix(): read_source(path, root) for path in code_paths}
    source_status = git_value(root, "status", "--porcelain", "--untracked-files=normal")
    provenance = {
        "schema": 1,
        "packageKind": "native-workspace-data-only",
        "requiredBuild": "Compatible modified tomasedisonn80/surgeXT-synth native renderer; not upstream skin-only compatibility",
        "source": {
            "repository": "https://github.com/tomasedisonn80/surgeXT-synth",
            "baseCommit": git_value(root, "rev-parse", "HEAD"),
            "baseTree": git_value(root, "rev-parse", "HEAD^{tree}"),
            "workingTreeDirty": None if source_status is None else bool(source_status),
            "packagedSourceSnapshotSha256": aggregate_hash(inputs),
            "nativeAndToolSourceSnapshotSha256": aggregate_hash(code),
            "nativeAndToolSourceFiles": {name: digest(raw) for name, raw in sorted(code.items())},
        },
        "validation": {"packageCount": len(workspaces), "strictControlOverlap": True,
                       "archivedManifestBytesRevalidated": True,
                       "nativeBuildOrRuntimeAttestation": "Not performed by this packager",
                       "pixelFidelityAssessment": "Not performed by this packager"},
        "archive": {"timestamp": "1980-01-01T00:00:00", "compression": "stored",
                    "sourceImagesIncluded": False, "screenshotsIncluded": False,
                    "binariesIncluded": False},
        "workspaces": workspaces,
        "payloadFiles": {name: digest(raw) for name, raw in sorted(payload.items())},
    }
    payload["PROVENANCE.json"] = json_bytes(provenance)
    payload["SHA256SUMS"] = "".join(f"{digest(raw)}  {name}\n" for name, raw in sorted(payload.items())).encode("utf-8")
    return payload, provenance


def archive_name(name: str) -> str:
    path = PurePosixPath(name)
    require(bool(name) and not path.is_absolute() and ".." not in path.parts and
            "\\" not in name and ":" not in name and "\x00" not in name,
            f"Unsafe archive path: {name!r}")
    return f"{PREFIX}/{path.as_posix()}"


def write_archive(destination: Path, payload: dict[str, bytes], force: bool = False) -> None:
    destination = destination.absolute()
    require(destination.suffix.lower() == ".zip", "Output filename must end in .zip")
    require(not destination.is_symlink(), "Refusing to replace an output symlink")
    require(not destination.exists() or (force and destination.is_file()),
            "Output already exists; use --force only to replace this specific ZIP")
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary: str | None = None
    try:
        descriptor, temporary = tempfile.mkstemp(prefix=".reference-pack-", suffix=".zip", dir=destination.parent)
        os.close(descriptor)
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_STORED, allowZip64=False) as archive:
            for name, raw in sorted(payload.items()):
                info = zipfile.ZipInfo(archive_name(name), date_time=ZIP_TIME)
                info.create_system = 3
                info.external_attr = (stat.S_IFREG | 0o644) << 16
                info.compress_type = zipfile.ZIP_STORED
                archive.writestr(info, raw)
        with zipfile.ZipFile(temporary) as archive:
            require(archive.testzip() is None, "Archive CRC verification failed")
        # Recheck just before publication. Never follow a replaced output symlink.
        require(not destination.is_symlink(), "Output became a symlink while packaging")
        require(not destination.exists() or (force and destination.is_file()), "Output appeared while packaging")
        if force:
            os.replace(temporary, destination)
        else:
            # Hard-link publication refuses a racing pre-existing destination.
            os.link(temporary, destination)
            os.unlink(temporary)
        temporary = None
    finally:
        if temporary is not None:
            os.unlink(temporary)


def self_test(root: Path) -> None:
    payload, provenance = build_payload(root)
    with tempfile.TemporaryDirectory(prefix="surge-reference-pack-test-") as directory:
        test_root = Path(directory)
        first, second = test_root / "first.zip", test_root / "second.zip"
        write_archive(first, payload)
        write_archive(second, payload)
        require(first.read_bytes() == second.read_bytes(), "Identical captured inputs produced different ZIP bytes")
        with zipfile.ZipFile(first) as archive:
            require(len(archive.infolist()) == 25, "Expected exactly 20 skin files and five package documents")
            for entry in archive.infolist():
                require(entry.date_time == ZIP_TIME and entry.compress_type == zipfile.ZIP_STORED,
                        "Non-reproducible ZIP entry metadata")
                require(entry.external_attr >> 16 == stat.S_IFREG | 0o644, "Unexpected ZIP file mode")
            for line in payload["SHA256SUMS"].decode().splitlines():
                expected, name = line.split("  ", 1)
                require(digest(archive.read(archive_name(name))) == expected, f"Hash mismatch: {name}")
        try:
            write_archive(first, payload)
        except PackagingError:
            pass
        else:
            raise PackagingError("An existing output was overwritten without --force")
        # Exercise the force path only against this newly created temporary ZIP.
        write_archive(first, payload, force=True)
        require(first.read_bytes() == second.read_bytes(), "Forced replacement changed archive bytes")
        link = test_root / "linked.zip"
        try:
            link.symlink_to(first.name)
        except (OSError, NotImplementedError):
            # Some Windows environments do not grant symlink creation. The
            # normal implementation guard remains active there.
            pass
        else:
            for operation in (lambda: write_archive(link, payload, force=True),
                              lambda: read_source(link, test_root)):
                try:
                    operation()
                except PackagingError:
                    continue
                raise PackagingError("A source/output symlink was accepted")
        for invalid in ("../escape", "/absolute", "C:/drive", "back\\slash", "nul\x00byte"):
            try:
                archive_name(invalid)
            except PackagingError:
                continue
            raise PackagingError(f"Unsafe ZIP path accepted: {invalid!r}")
    print(f"PASS: {provenance['validation']['packageCount']} validated skins; 25 archive entries; "
          "byte-identical repeated ZIPs; SHA-256/CRC/metadata verification; overwrite/path guards")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, help="destination .zip; data only, not a binary installer")
    parser.add_argument("--force", action="store_true", help="replace only the explicitly named existing ZIP")
    parser.add_argument("--self-test", action="store_true", help="validate and verify temporary deterministic archives")
    args = parser.parse_args()
    if args.self_test and args.output is not None:
        parser.error("--self-test and --output are separate operations")
    if not args.self_test and args.output is None:
        parser.error("--output is required unless --self-test is used")
    try:
        if args.self_test:
            self_test(args.root)
        else:
            payload, provenance = build_payload(args.root)
            write_archive(args.output, payload, args.force)
            print(json.dumps({"output": str(args.output.absolute()), "sha256": digest(args.output.read_bytes()),
                              "bytes": args.output.stat().st_size, "skinCount": len(provenance["workspaces"]),
                              "modifiedBuildRequired": True}, indent=2))
    except (PackagingError, validate.InvalidManifest, OSError, ValueError, ET.ParseError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
