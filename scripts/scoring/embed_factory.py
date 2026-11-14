#!/usr/bin/env python3
"""Build/check the small factory payload linked into every Surge common engine.

Generation needs the zstd CLI; ordinary checks and builds need no extra Python
packages or compressor. The flat container is deliberately simple to bound and
validate in native code. No samples, generated audio, or duplicate gallery images
are included. See doc/scoring-1000/BUILT_IN_CONTENT.md.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAGIC = b"SXTEMB01"
MAX_BYTES = 64 * 1024 * 1024
RESOURCE = Path("resources/surge-shared/scoring-factory.sxtz")
INDEX = Path("resources/surge-shared/scoring-factory.json")
IDENTITY = Path("resources/surge-shared/scoring-factory.id")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def collect(root: Path) -> list[Path]:
    data = root / "resources/data"
    patches = sorted((data / "patches_factory/Scoring 1000").rglob("*.fxp"))
    skins = sorted((data / "skins").glob("reference-*.surge-skin"))
    if len(patches) != 1000 or len(skins) != 10:
        raise ValueError(f"Expected 1000 presets and 10 skins; got {len(patches)}, {len(skins)}")
    files = list(patches)
    for skin in skins:
        for required in ("skin.xml", "workspace.json"):
            if not (skin / required).is_file():
                raise ValueError(f"Missing skin resource: {skin / required}")
        files.extend(p for p in skin.rglob("*") if p.is_file())
    for path in files:
        rel = path.relative_to(data)
        if path.is_symlink() or not rel.as_posix().isascii():
            raise ValueError(f"Payload must use ordinary files with portable ASCII paths: {path}")
        if any(p in ("", ".", "..") for p in rel.parts):
            raise ValueError(f"Unsafe path: {rel}")
    return sorted(files, key=lambda p: p.relative_to(data).as_posix())


def container(root: Path) -> tuple[bytes, dict]:
    data = root / "resources/data"
    files = collect(root)
    payload = bytearray(MAGIC + struct.pack("<I", len(files)))
    entries = []
    for path in files:
        name = path.relative_to(data).as_posix()
        encoded = name.encode("ascii")
        body = path.read_bytes()
        payload.extend(struct.pack("<II", len(encoded), len(body)))
        payload.extend(encoded)
        payload.extend(body)
        entries.append({"path": name, "bytes": len(body), "sha256": digest(body)})
    if len(payload) > MAX_BYTES:
        raise ValueError(f"Embedded factory payload exceeds {MAX_BYTES} bytes")
    manifest = {
        "format": "SXTEMB01+zstd",
        "catalog_sha256": digest((root / "doc/scoring-1000/manifest.json").read_bytes()),
        "preset_count": 1000,
        "skin_count": 10,
        "entry_count": len(files),
        "uncompressed_bytes": len(payload),
        "payload_sha256": digest(payload),
        "entries": entries,
    }
    return bytes(payload), manifest


def check(root: Path) -> dict:
    payload, expected = container(root)
    actual = json.loads((root / INDEX).read_text(encoding="utf-8"))
    packed = (root / RESOURCE).read_bytes()
    expected["compressed_bytes"] = len(packed)
    expected["compressed_sha256"] = digest(packed)
    if actual != expected:
        raise ValueError("Embedded payload index is stale or changed; regenerate with embed_factory.py")
    if (root / IDENTITY).read_bytes() != (digest(payload) + "\n").encode("ascii"):
        raise ValueError("Embedded payload identity does not match the current source files")
    return {k: v for k, v in expected.items() if k != "entries"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--check", action="store_true", help="Check source/payload bindings without zstd")
    parser.add_argument("--zstd", default="zstd", help="zstd CLI used only for regeneration")
    args = parser.parse_args()
    root = args.root.resolve()
    try:
        if not args.check:
            payload, index = container(root)
            with tempfile.TemporaryDirectory(prefix="surge-embedded-") as tmp:
                source = Path(tmp) / "factory.bin"
                output = Path(tmp) / "factory.zst"
                source.write_bytes(payload)
                # A regular input file records the exact decompressed frame size.
                subprocess.run([args.zstd, "-q", "-19", "-T1", "--check", "-f",
                                str(source), "-o", str(output)], check=True)
                packed = output.read_bytes()
                decoded = subprocess.run([args.zstd, "-q", "-d", "-c", str(output)],
                                         check=True, stdout=subprocess.PIPE).stdout
                if decoded != payload:
                    raise ValueError("Compression roundtrip changed factory bytes")
            index["compressed_bytes"] = len(packed)
            index["compressed_sha256"] = digest(packed)
            (root / RESOURCE).write_bytes(packed)
            (root / IDENTITY).write_text(digest(payload) + "\n", encoding="ascii", newline="\n")
            (root / INDEX).write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8", newline="\n")
        print(json.dumps(check(root), indent=2))
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        print(f"Embedded factory content failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
