#!/usr/bin/env python3
"""Validate and compare two surge-cpu-bench reports using the standard library.

No timing threshold is imposed by default: shared runners are noisy. A successful
comparison establishes equivalent recorded fixtures and audio fingerprints; use
both PCM directory options to additionally verify the complete rendered bytes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import struct
import sys
from typing import Any


COMPARABLE_FIELDS = (
    "schema", "tool", "compiler", "configuration", "lua_requested", "has_lua",
    "lto_requested", "sample_rate", "block_size", "seed", "warmup_blocks",
    "repetitions", "format", "channels", "frames", "pcm_hash_algorithm",
    "fpu_guard", "timing", "patch",
)
ROOT_FIELDS = set(COMPARABLE_FIELDS) | {"git", "engine", "scenarios", "ok"}
SCENARIO_FIELDS = {"name", "requested_notes", "deterministic", "held", "tail", "runs"}
PHASE_FIELDS = {
    "wall_seconds", "median_seconds", "min_seconds", "max_seconds",
    "audio_seconds", "median_realtime_percent",
}
RUN_FIELDS = {"pcm_hash", "peak", "rms"}
NAME_PATTERN = re.compile(r"[A-Za-z0-9_-]+\Z")
HASH_PATTERN = re.compile(r"[0-9a-f]{16}\Z")
FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211
UINT64_MASK = (1 << 64) - 1


class ComparisonError(ValueError):
    """The inputs cannot support a valid performance/audio comparison."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ComparisonError(message)


def object_fields(value: Any, fields: set[str], where: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{where}: expected an object")
    require(set(value) == fields,
            f"{where}: field mismatch; missing={sorted(fields - set(value))}, "
            f"unexpected={sorted(set(value) - fields)}")
    return value


def integer(value: Any, lower: int, upper: int, where: str) -> int:
    require(type(value) is int and lower <= value <= upper,
            f"{where}: expected integer in {lower}..{upper}")
    return value


def finite_number(value: Any, where: str, *, positive: bool = False) -> float:
    require(type(value) in (int, float), f"{where}: expected a finite number")
    try:
        result = float(value)
    except (OverflowError, ValueError) as error:
        raise ComparisonError(f"{where}: expected a finite number") from error
    require(math.isfinite(result), f"{where}: expected a finite number")
    require(result > 0 if positive else result >= 0,
            f"{where}: expected {'positive' if positive else 'nonnegative'} number")
    return result


def close(actual: float, expected: float, where: str) -> None:
    # The native writer uses twelve significant digits. This accommodates that
    # serialization only; it is not an audio tolerance or a performance threshold.
    require(math.isclose(actual, expected, rel_tol=1e-9, abs_tol=1e-12),
            f"{where}: inconsistent summary ({actual} versus {expected})")


def validate_phase(phase: Any, repetitions: int, sample_rate: int, block_size: int,
                   where: str) -> None:
    phase = object_fields(phase, PHASE_FIELDS, where)
    times = phase["wall_seconds"]
    require(isinstance(times, list) and len(times) == repetitions,
            f"{where}.wall_seconds: expected {repetitions} timings")
    values = [finite_number(value, f"{where}.wall_seconds[{index}]", positive=True)
              for index, value in enumerate(times)]
    for name in PHASE_FIELDS - {"wall_seconds"}:
        finite_number(phase[name], f"{where}.{name}", positive=True)
    close(phase["median_seconds"], statistics.median(values), f"{where}.median_seconds")
    close(phase["min_seconds"], min(values), f"{where}.min_seconds")
    close(phase["max_seconds"], max(values), f"{where}.max_seconds")
    close(phase["median_realtime_percent"],
          100 * statistics.median(values) / phase["audio_seconds"],
          f"{where}.median_realtime_percent")
    blocks = phase["audio_seconds"] * sample_rate / block_size
    require(round(blocks) >= 1 and abs(blocks - round(blocks)) <= 1e-5,
            f"{where}.audio_seconds: duration is not a positive whole engine block count")


def validate_report(report: Any, label: str) -> dict[str, dict[str, Any]]:
    report = object_fields(report, ROOT_FIELDS, label)
    require(type(report["schema"]) is int and report["schema"] == 1,
            f"{label}: unsupported schema")
    require(report["tool"] == "surge-cpu-bench", f"{label}: unexpected tool")
    require(report["ok"] is True, f"{label}: benchmark did not report success")
    require(report["fpu_guard"] is True, f"{label}: plugin floating-point guard is required")
    for name in ("has_lua", "lto_requested"):
        require(type(report[name]) is bool, f"{label}.{name}: expected a boolean")
    for name in ("git", "engine", "compiler", "configuration", "timing"):
        require(isinstance(report[name], str) and bool(report[name]),
                f"{label}.{name}: expected a nonempty string")
    for name in ("lua_requested", "patch"):
        require(isinstance(report[name], str), f"{label}.{name}: expected a string")
    require(report["format"] == "f32le-interleaved" and report["channels"] == 2
            and type(report["channels"]) is int, f"{label}: unsupported PCM format")
    require(report["pcm_hash_algorithm"] == "fnv1a64-f32le",
            f"{label}: unsupported PCM fingerprint algorithm")
    rate = integer(report["sample_rate"], 12000, 192000, f"{label}.sample_rate")
    block = integer(report["block_size"], 1, 65536, f"{label}.block_size")
    require(block & (block - 1) == 0, f"{label}.block_size: expected a power of two")
    integer(report["seed"], 0, (1 << 32) - 1, f"{label}.seed")
    integer(report["warmup_blocks"], 0, 1000000, f"{label}.warmup_blocks")
    repeats = integer(report["repetitions"], 1, 30, f"{label}.repetitions")
    frames = integer(report["frames"], 2 * block, 120 * rate + 2 * block,
                     f"{label}.frames")
    require(frames % block == 0, f"{label}.frames: expected whole engine blocks")
    scenarios = report["scenarios"]
    require(isinstance(scenarios, list) and bool(scenarios),
            f"{label}.scenarios: expected a nonempty list")
    by_name = {}
    for index, scenario in enumerate(scenarios):
        where = f"{label}.scenarios[{index}]"
        scenario = object_fields(scenario, SCENARIO_FIELDS, where)
        name = scenario["name"]
        require(isinstance(name, str) and NAME_PATTERN.fullmatch(name) is not None,
                f"{where}.name: invalid scenario identifier")
        require(name not in by_name, f"{label}: duplicate scenario {name}")
        by_name[name] = scenario
        integer(scenario["requested_notes"], 0, 128, f"{where}.requested_notes")
        require(scenario["deterministic"] is True,
                f"{where}: repetitions are not deterministic")
        for phase in ("held", "tail"):
            validate_phase(scenario[phase], repeats, rate, block, f"{where}.{phase}")
        close(scenario["held"]["audio_seconds"] + scenario["tail"]["audio_seconds"],
              frames / rate, f"{where}: total rendered duration")
        runs = scenario["runs"]
        require(isinstance(runs, list) and len(runs) == repeats,
                f"{where}.runs: expected {repeats} runs")
        hashes = set()
        for run_index, run in enumerate(runs):
            run_where = f"{where}.runs[{run_index}]"
            run = object_fields(run, RUN_FIELDS, run_where)
            fingerprint = run["pcm_hash"]
            require(isinstance(fingerprint, str) and HASH_PATTERN.fullmatch(fingerprint) is not None,
                    f"{run_where}.pcm_hash: expected 16 lowercase hexadecimal digits")
            hashes.add(fingerprint)
            peak = finite_number(run["peak"], f"{run_where}.peak")
            rms = finite_number(run["rms"], f"{run_where}.rms")
            require(rms <= peak + max(1e-12, peak * 1e-9),
                    f"{run_where}: RMS cannot exceed peak")
        require(len(hashes) == 1, f"{where}: PCM fingerprints differ across repetitions")
    return by_name


def read_report(path: Path) -> dict[str, Any]:
    def reject_constant(value: str) -> None:
        raise ComparisonError(f"{path}: nonfinite JSON constant {value}")

    def unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result = {}
        for key, value in pairs:
            require(key not in result, f"{path}: duplicate JSON field {key}")
            result[key] = value
        return result

    with path.open(encoding="utf-8") as stream:
        return json.load(stream, parse_constant=reject_constant, object_pairs_hook=unique_object)


def verify_pcm(baseline_dir: Path, candidate_dir: Path, name: str, expected_bytes: int,
               fingerprint: str) -> dict[str, Any]:
    paths = [baseline_dir / f"{name}.f32", candidate_dir / f"{name}.f32"]
    for path in paths:
        require(path.is_file(), f"Missing PCM file: {path}")
        require(path.stat().st_size == expected_bytes,
                f"{path}: expected {expected_bytes} PCM bytes")
    hashes = [FNV_OFFSET, FNV_OFFSET]
    sha = hashlib.sha256()
    offset = 0
    with paths[0].open("rb") as base, paths[1].open("rb") as candidate:
        while True:
            chunks = [base.read(65536), candidate.read(65536)]
            require(chunks[0] == chunks[1], f"{name}: PCM byte mismatch at chunk offset {offset}")
            if not chunks[0]:
                break
            # Finiteness and fingerprint binding prevent unrelated, corrupted or
            # identically replaced PCM files from being accepted as report evidence.
            for sample, in struct.iter_unpack("<f", chunks[0]):
                require(math.isfinite(sample), f"{name}: nonfinite PCM sample")
            for index, chunk in enumerate(chunks):
                value = hashes[index]
                for byte in chunk:
                    value = ((value ^ byte) * FNV_PRIME) & UINT64_MASK
                hashes[index] = value
            sha.update(chunks[0])
            offset += len(chunks[0])
    require(offset == expected_bytes, f"{name}: PCM length changed during comparison")
    for index, value in enumerate(hashes):
        require(f"{value:016x}" == fingerprint,
                f"{paths[index]}: PCM does not match its report fingerprint")
    return {"bytes_per_file": expected_bytes, "sha256": sha.hexdigest(), "identical": True}


def compare_reports(baseline: dict[str, Any], candidate: dict[str, Any], *,
                    baseline_pcm: Path | None = None, candidate_pcm: Path | None = None,
                    max_regression_percent: float | None = None) -> dict[str, Any]:
    base_scenarios = validate_report(baseline, "baseline")
    new_scenarios = validate_report(candidate, "candidate")
    require((baseline_pcm is None) == (candidate_pcm is None),
            "Both baseline and candidate PCM directories are required together")
    if max_regression_percent is not None:
        finite_number(max_regression_percent, "max_regression_percent")
    for field in COMPARABLE_FIELDS:
        require(baseline[field] == candidate[field], f"Incompatible metadata field: {field}")
    require(base_scenarios.keys() == new_scenarios.keys(),
            "Scenario sets differ: "
            f"baseline_only={sorted(base_scenarios.keys() - new_scenarios.keys())}, "
            f"candidate_only={sorted(new_scenarios.keys() - base_scenarios.keys())}")
    results = []
    threshold_passed = True
    for name, before in base_scenarios.items():
        after = new_scenarios[name]
        require(before["requested_notes"] == after["requested_notes"],
                f"{name}: requested note count differs")
        fingerprint = before["runs"][0]["pcm_hash"]
        require(fingerprint == after["runs"][0]["pcm_hash"],
                f"{name}: baseline/candidate PCM fingerprints differ")
        row = {"name": name, "requested_notes": before["requested_notes"],
               "pcm_fingerprint": fingerprint}
        if baseline_pcm is not None:
            row["pcm"] = verify_pcm(baseline_pcm, candidate_pcm, name,
                                    baseline["frames"] * baseline["channels"] * 4, fingerprint)
        for phase in ("held", "tail"):
            old, new = before[phase], after[phase]
            require(old["audio_seconds"] == new["audio_seconds"],
                    f"{name}.{phase}: rendered duration differs")
            old_median = statistics.median(old["wall_seconds"])
            new_median = statistics.median(new["wall_seconds"])
            reduction = 100.0 * (1 - new_median / old_median)
            passed = (max_regression_percent is None or
                      new_median <= old_median * (1 + max_regression_percent / 100))
            threshold_passed = threshold_passed and passed
            row[phase] = {
                "baseline_wall_seconds": old["wall_seconds"],
                "candidate_wall_seconds": new["wall_seconds"],
                "baseline_median_seconds": old_median,
                "candidate_median_seconds": new_median,
                "baseline_range_seconds": [min(old["wall_seconds"]), max(old["wall_seconds"])],
                "candidate_range_seconds": [min(new["wall_seconds"]), max(new["wall_seconds"])],
                "audio_seconds": old["audio_seconds"],
                "baseline_realtime_percent": 100 * old_median / old["audio_seconds"],
                "candidate_realtime_percent": 100 * new_median / new["audio_seconds"],
                "median_reduction_percent": reduction,
                "regression_threshold_passed": passed,
            }
        results.append(row)
    return {
        "schema": 1, "tool": "surge-cpu-compare", "ok": threshold_passed,
        "baseline_git": baseline["git"], "candidate_git": candidate["git"],
        "metadata": {key: baseline[key] for key in COMPARABLE_FIELDS},
        "audio_fingerprints_match": True, "full_pcm_verified": baseline_pcm is not None,
        "max_regression_percent": max_regression_percent,
        "interpretation": "Positive median_reduction_percent means less measured wall time.",
        "limitations": [
            "Offline wall timing includes bounded PCM copy and transport work; it is not a host deadline test.",
            "Run on the same machine with matched build flags; shared-machine noise can exceed small changes.",
            "PCM fingerprints are noncryptographic; full_pcm_verified requires complete matching PCM files.",
        ],
        "scenarios": results,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--baseline-pcm", type=Path)
    parser.add_argument("--candidate-pcm", type=Path)
    parser.add_argument("--output", type=Path, help="Also save the comparison JSON to this file")
    parser.add_argument("--max-regression-percent", type=float,
                        help="Optional median wall-time regression gate, applied to both windows")
    args = parser.parse_args(argv)
    try:
        report = compare_reports(read_report(args.baseline), read_report(args.candidate),
                                 baseline_pcm=args.baseline_pcm, candidate_pcm=args.candidate_pcm,
                                 max_regression_percent=args.max_regression_percent)
        report["baseline_report"] = str(args.baseline)
        report["candidate_report"] = str(args.candidate)
        output = json.dumps(report, indent=2, allow_nan=False) + "\n"
        if args.output:
            # Never overwrite either input report during comparison.
            require(args.output.resolve() not in (args.baseline.resolve(), args.candidate.resolve()),
                    "Output must not overwrite an input report")
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(output, encoding="utf-8")
        print(output, end="")
        return 0 if report["ok"] else 1
    except (ComparisonError, OSError, json.JSONDecodeError, UnicodeError) as error:
        print(f"compare.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
