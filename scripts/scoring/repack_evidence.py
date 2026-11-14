#!/usr/bin/env python3
"""Losslessly compact the primary report ZIP for transport with bounded storage.

The JSON bodies, IDs and report-index hashes do not change. ZIP member names are
flattened from presets/SCxx-xxx.json to SCxx-xxx.json. All members use the standard
ZIP LZMA method and can be read by Python's ordinary zipfile module. This CLI uses
three deterministic encoder profiles and keeps the smallest encoding per report.
The custom compressor is installed only within this single-threaded CLI process;
no custom decoder or third-party Python package is required.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import lzma
from pathlib import Path
import re
import struct
import zipfile


class CompactLzma:
    def __init__(self):
        self.data = bytearray()

    def compress(self, data):
        self.data.extend(data)
        if len(self.data) > 1024 * 1024:
            raise ValueError("One evidence report exceeds the 1 MiB packing limit")
        return b""

    def flush(self):
        candidates = []
        for lc in (0, 2, 3):
            filters = {"id": lzma.FILTER_LZMA1,
                       "preset": 9 | lzma.PRESET_EXTREME,
                       "dict_size": 262144, "lc": lc, "lp": 0, "pb": 0}
            props = lzma._encode_filter_properties(filters)
            # APPNOTE 6.3.2 section 5.8: version, property size, properties,
            # followed by the raw LZMA stream with its end-of-stream marker.
            encoded = (struct.pack("<BBH", 9, 4, len(props)) + props +
                       lzma.compress(bytes(self.data), format=lzma.FORMAT_RAW,
                                     filters=[filters]))
            candidates.append((len(encoded), lc, encoded))
        return min(candidates, key=lambda value: (value[0], value[1]))[2]


def repack(source: Path, destination: Path):
    if source.resolve() == destination.resolve():
        raise ValueError("Use a separate output path; the source is retained")
    hashes = {}
    original_compressor = zipfile.LZMACompressor
    try:
        zipfile.LZMACompressor = CompactLzma
        with zipfile.ZipFile(source) as src, zipfile.ZipFile(
                destination, "w", compression=zipfile.ZIP_LZMA) as dst:
            members = src.infolist()
            if len(members) != 1000:
                raise ValueError("Expected exactly 1000 original primary reports")
            for item in sorted(members, key=lambda item: item.filename):
                if not re.fullmatch(r"(?:presets/)?SC\d{2}-\d{3}\.json", item.filename):
                    raise ValueError(f"Unexpected report path: {item.filename}")
                name = Path(item.filename).name
                if name in hashes or item.file_size > 1024 * 1024:
                    raise ValueError(f"Duplicate or oversized report: {item.filename}")
                data = src.read(item)
                hashes[name] = hashlib.sha256(data).hexdigest()
                info = copy.copy(item)
                info.filename = info.orig_filename = name
                info.compress_type = zipfile.ZIP_LZMA
                dst.writestr(info, data)
    finally:
        zipfile.LZMACompressor = original_compressor
    with zipfile.ZipFile(destination) as dst:
        if set(dst.namelist()) != set(hashes):
            raise ValueError("Packed report names differ")
        for name, expected in hashes.items():
            if hashlib.sha256(dst.read(name)).hexdigest() != expected:
                raise ValueError(f"Packing changed original report bytes: {name}")
    return {"report_count": len(hashes), "bytes": destination.stat().st_size,
            "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(),
            "all_original_report_bytes_unchanged": True,
            "member_layout": "SCxx-xxx.json"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    print(json.dumps(repack(args.source, args.destination), indent=2))


if __name__ == "__main__":
    main()
