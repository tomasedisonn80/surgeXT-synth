#!/usr/bin/env python3
"""Compare every sample byte in two surge-dsp-regression fixture directories.

This verifies the complete 63-fixture schema, metadata, finite PCM, fingerprints,
and measured peaks. Success is an audio-equivalence result for these fixtures;
it makes no timing, host compatibility, or binary provenance claim.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys
from typing import Any

from compare import (ComparisonError, FNV_OFFSET, FNV_PRIME, HASH_PATTERN,
                     NAME_PATTERN, UINT64_MASK, close, finite_number, integer,
                     object_fields, read_report, require)


ROOT_FIELDS = {
    "schema", "tool", "seed", "block_size", "blocks", "release_block", "fpu_guard",
    "effect_init_rounding", "table_reset_rounding", "processing_rounding", "format", "channels",
    "hash_algorithm", "fixtures", "ok",
}
EVENT_FIELDS = {
    "rate_change_blocks", "same_rate_reset_blocks", "outside_callback_init_blocks",
    "direct_storage_rate_reset_blocks", "standard_tuning_table_reset_blocks",
}
FIXTURE_FIELDS = {"name", "file", "frames", "sample_rates", "peak", "pcm_hash",
                  "processing_rounding_ranges"} | EVENT_FIELDS
RATES = (44100, 48000, 96000)


def fixture_names() -> set[str]:
    names = set()
    for rate in RATES:
        names.add(f"conditioner-{rate}")
        names.update(f"{effect}-{rate}-phase{phase}"
                     for effect in ("eq3", "eq11") for phase in range(8))
        names.update(f"lowcut-{slope}-{rate}" for slope in range(4))
    return names


def validate_manifest(manifest: Any, label: str) -> dict[str, dict[str, Any]]:
    manifest = object_fields(manifest, ROOT_FIELDS, label)
    require(type(manifest["schema"]) is int and manifest["schema"] == 1,
            f"{label}: unsupported schema")
    require(manifest["tool"] == "surge-dsp-regression", f"{label}: unexpected tool")
    require(manifest["ok"] is True and manifest["fpu_guard"] is True,
            f"{label}: successful guarded rendering is required")
    require(manifest["format"] == "f32le-interleaved" and
            type(manifest["channels"]) is int and manifest["channels"] == 2 and
            manifest["hash_algorithm"] == "fnv1a64-f32le",
            f"{label}: unsupported PCM format or fingerprint")
    integer(manifest["seed"], 1801, 1801, f"{label}.seed")
    block = integer(manifest["block_size"], 1, 65536, f"{label}.block_size")
    require(block & (block - 1) == 0, f"{label}: block size must be a power of two")
    blocks = integer(manifest["blocks"], 1536, 1536, f"{label}.blocks")
    integer(manifest["release_block"], 1024, 1024, f"{label}.release_block")
    for field in ("effect_init_rounding", "table_reset_rounding", "processing_rounding"):
        require(isinstance(manifest[field], str) and bool(manifest[field]),
                f"{label}.{field}: expected a nonempty description")
    fixtures = manifest["fixtures"]
    require(isinstance(fixtures, list) and len(fixtures) == 63,
            f"{label}: expected all 63 lifecycle fixtures")
    result = {}
    for index, fixture in enumerate(fixtures):
        where = f"{label}.fixtures[{index}]"
        fixture = object_fields(fixture, FIXTURE_FIELDS, where)
        name = fixture["name"]
        require(isinstance(name, str) and NAME_PATTERN.fullmatch(name) is not None,
                f"{where}: invalid fixture name")
        require(name not in result, f"{label}: duplicate fixture {name}")
        require(fixture["file"] == name + ".f32", f"{where}: unsafe or unexpected PCM filename")
        integer(fixture["frames"], block * blocks, block * blocks, f"{where}.frames")
        finite_number(fixture["peak"], f"{where}.peak", positive=True)
        require(fixture["peak"] >= 1e-6, f"{where}: unexpectedly silent fixture")
        require(isinstance(fixture["pcm_hash"], str) and
                HASH_PATTERN.fullmatch(fixture["pcm_hash"]) is not None,
                f"{where}: invalid PCM fingerprint")
        rates = fixture["sample_rates"]
        require(isinstance(rates, list) and len(rates) == 3 and
                all(type(rate) is int for rate in rates) and
                tuple(rates) in (RATES, RATES[1:] + RATES[:1], RATES[2:] + RATES[:2]),
                f"{where}: expected the three cyclic fixture sample rates")
        ranges = fixture["processing_rounding_ranges"]
        require(isinstance(ranges, list) and len(ranges) == 2,
                f"{where}.processing_rounding_ranges: expected two mode intervals")
        end = 0
        for interval in ranges:
            require(isinstance(interval, list) and len(interval) == 2,
                    f"{where}: expected [start, end) mode interval")
            start = integer(interval[0], 0, blocks - 1, f"{where}: mode interval start")
            stop = integer(interval[1], 1, blocks, f"{where}: mode interval end")
            require(end <= start < stop, f"{where}: reversed or overlapping mode intervals")
            end = stop
        for field in EVENT_FIELDS:
            events = fixture[field]
            require(isinstance(events, list), f"{where}.{field}: expected an array")
            for event in events:
                integer(event, 0, blocks - 1, f"{where}.{field}")
            require(events == sorted(set(events)), f"{where}.{field}: unordered or repeated events")
            expected_count = 2 if field == "rate_change_blocks" else 1
            if field == "outside_callback_init_blocks" and name.startswith("lowcut-"):
                expected_count = 0
            require(len(events) == expected_count,
                    f"{where}.{field}: expected {expected_count} events")
        result[name] = fixture
    require(set(result) == fixture_names(), f"{label}: fixture coverage differs from the 63-case suite")
    return result


def pcm_path(directory: Path, fixture: dict[str, Any]) -> Path:
    path = directory / fixture["file"]
    require(path.is_file(), f"Missing PCM file: {path}")
    require(path.resolve().parent == directory.resolve(),
            f"PCM file resolves outside its evidence directory: {path}")
    return path


def verify_fixture(baseline: Path, candidate: Path, before: dict[str, Any],
                   after: dict[str, Any]) -> dict[str, Any]:
    name = before["name"]
    expected = before["frames"] * 2 * 4
    paths = (pcm_path(baseline, before), pcm_path(candidate, after))
    for path in paths:
        require(path.stat().st_size == expected, f"{path}: expected {expected} PCM bytes")
    digest = hashlib.sha256()
    fingerprint = FNV_OFFSET
    offset, peak = 0, 0.0
    with paths[0].open("rb") as old, paths[1].open("rb") as new:
        while True:
            left, right = old.read(65536), new.read(65536)
            require(left == right, f"{name}: PCM bytes differ at chunk offset {offset}")
            if not left:
                break
            require(len(left) % 4 == 0, f"{name}: incomplete float sample")
            for sample, in struct.iter_unpack("<f", left):
                require(math.isfinite(sample), f"{name}: nonfinite PCM sample")
                peak = max(peak, abs(sample))
            for byte in left:
                fingerprint = ((fingerprint ^ byte) * FNV_PRIME) & UINT64_MASK
            digest.update(left)
            offset += len(left)
    require(offset == expected, f"{name}: PCM length changed during comparison")
    encoded = f"{fingerprint:016x}"
    for label, fixture in (("baseline", before), ("candidate", after)):
        require(fixture["pcm_hash"] == encoded, f"{name}: {label} fingerprint does not bind PCM")
        close(fixture["peak"], peak, f"{name}: {label} peak does not match PCM")
    return {"name": name, "file": before["file"], "frames": before["frames"],
            "metadata": dict(before),
            "bytes_per_file": expected, "sha256": digest.hexdigest(),
            "pcm_hash": encoded, "peak": peak, "identical": True}


def compare_directories(baseline: Path, candidate: Path) -> dict[str, Any]:
    baseline, candidate = baseline.resolve(strict=True), candidate.resolve(strict=True)
    require(baseline.is_dir() and candidate.is_dir(), "Expected two evidence directories")
    require(not os.path.samefile(baseline, candidate), "Expected distinct baseline/candidate directories")
    reports = [read_report(path / "manifest.json") for path in (baseline, candidate)]
    fixtures = [validate_manifest(report, label)
                for report, label in zip(reports, ("baseline", "candidate"))]
    for field in ROOT_FIELDS - {"fixtures"}:
        require(reports[0][field] == reports[1][field], f"Incompatible root metadata: {field}")
    for directory in (baseline, candidate):
        require({path.name for path in directory.glob("*.f32")} ==
                {fixture["file"] for fixture in fixtures[0].values()},
                f"{directory}: missing or unexpected PCM files")
    result = []
    for name in sorted(fixtures[0]):
        before, after = fixtures[0][name], fixtures[1][name]
        require(before == after, f"{name}: fixture metadata or audio fingerprint differs")
        result.append(verify_fixture(baseline, candidate, before, after))
    return {"schema": 1, "tool": "surge-dsp-regression-compare", "ok": True,
            "baseline_directory": str(baseline), "candidate_directory": str(candidate),
            "manifest_sha256": {
                label: hashlib.sha256((directory / "manifest.json").read_bytes()).hexdigest()
                for label, directory in (("baseline", baseline), ("candidate", candidate))},
            "metadata": {key: reports[0][key] for key in sorted(ROOT_FIELDS - {"fixtures"})},
            "fixture_count": len(result), "full_pcm_verified": True,
            "bytes_compared_per_side": sum(item["bytes_per_file"] for item in result),
            "fixtures": result,
            "scope": "Complete matching finite PCM bytes for the recorded lifecycle fixtures; no CPU or host claim."}


def validate_output(output: Path, directories: tuple[Path, Path]) -> None:
    inputs = [directory / "manifest.json" for directory in directories]
    inputs.extend(path for directory in directories for path in directory.glob("*.f32"))
    for path in inputs:
        require(output.resolve() != path.resolve() and
                not (output.exists() and path.exists() and os.path.samefile(output, path)),
                "Output must not overwrite an input manifest or PCM file")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path, help="Baseline fixture directory")
    parser.add_argument("candidate", type=Path, help="Candidate fixture directory")
    parser.add_argument("--output", type=Path, help="Save the complete comparison JSON")
    args = parser.parse_args(argv)
    try:
        if args.output:
            validate_output(args.output, (args.baseline, args.candidate))
        result = compare_directories(args.baseline, args.candidate)
        serialized = json.dumps(result, indent=2, allow_nan=False) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(serialized, encoding="utf-8")
        print(serialized, end="")
        return 0
    except (ComparisonError, OSError, json.JSONDecodeError, UnicodeError) as error:
        print(f"compare_regression.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
