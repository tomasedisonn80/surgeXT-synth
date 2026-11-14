#!/usr/bin/env python3
"""Write the CMake packaging index from the complete Scoring 1000 manifest.

SPDX-License-Identifier: GPL-3.0-or-later
The manifest hash binds this compact index to the richer independently checked
catalog. No audio is changed. Build-time verification needs only CMake.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath


def write_package_index(manifest_path: Path) -> Path:
    manifest_path = Path(manifest_path).resolve()
    root = manifest_path.parents[2]
    catalog = json.loads(manifest_path.read_text(encoding="utf-8"))
    presets = catalog["presets"]
    if catalog.get("preset_count") != 1000 or len(presets) != 1000:
        raise ValueError("The packaging index requires the complete 1000-preset bank")
    if catalog.get("category_count") != 10:
        raise ValueError("The packaging index requires ten categories")
    rows = [(manifest_path.relative_to(root).as_posix(),
             hashlib.sha256(manifest_path.read_bytes()).hexdigest())]
    seen = set()
    categories = {}
    for entry in presets:
        relative = entry["path"]
        parsed = PurePosixPath(relative)
        if (parsed.is_absolute() or ".." in parsed.parts or
                any(c in relative for c in "\n\r\t;\\") or
                not relative.startswith("resources/data/patches_factory/Scoring 1000/") or
                not relative.endswith(".fxp")):
            raise ValueError(f"Unsafe preset path: {relative!r}")
        if relative.casefold() in seen:
            raise ValueError(f"Duplicate preset path: {relative}")
        seen.add(relative.casefold())
        actual = hashlib.sha256((root / relative).read_bytes()).hexdigest()
        if actual != entry["sha256"]:
            raise ValueError(f"Manifest hash differs from file: {relative}")
        category = entry["category_id"]
        categories[category] = categories.get(category, 0) + 1
        rows.append((relative, actual))
    if categories != {f"{i:02}": 100 for i in range(1, 11)}:
        raise ValueError(f"Unexpected category counts: {categories}")
    target = manifest_path.with_name("package.sha256")
    # The index is hashed and compared across checkout/build platforms. Bytes
    # avoid Windows text-mode newline translation even outside a Git checkout.
    target.write_bytes("".join(f"{sha}  {path}\n" for path, sha in rows).encode("utf-8"))
    return target


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).resolve().parents[2] / "doc/scoring-1000/manifest.json")
    args = parser.parse_args()
    print(write_package_index(args.manifest))


if __name__ == "__main__":
    main()
