#!/usr/bin/env python3
"""Run matched Surge CPU benchmarks in alternating AB/BA order.

Raw one-repetition reports and logs are retained. Complete PCM is saved for the
first pair only, then verified against the merged, deterministic reports. The
reported reduction describes offline wall time, not host deadline reliability.
With --thread-cpu, separate Linux render-thread CPU reports and summaries are
also retained; the wall-time and complete first-pair audio checks still run.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import time
from typing import Any

import compare
import thread_clock


class RunnerError(ValueError):
    """The requested paired experiment cannot produce valid evidence."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RunnerError(message)


def digest(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def identity(path: Path) -> dict[str, Any]:
    return {"path": str(path.resolve()), "bytes": path.stat().st_size,
            "sha256": digest(path)}


def write_json(path: Path, value: Any) -> None:
    # Rename a complete document so an interrupted run does not leave valid-looking
    # truncated provenance or summaries. Raw child reports are retained as emitted.
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    temporary.replace(path)


def execution_order(round_index: int) -> tuple[str, str]:
    return ("baseline", "candidate") if round_index % 2 == 0 else ("candidate", "baseline")


def merge_reports(reports: list[dict[str, Any]], label: str) -> dict[str, Any]:
    require(1 <= len(reports) <= 30, f"{label}: expected 1..30 round reports")
    first = reports[0]
    first_scenarios = compare.validate_report(first, f"{label} round 1")
    require(first["repetitions"] == 1, f"{label}: each round must contain one repetition")
    scenario_maps = [first_scenarios]
    for index, report in enumerate(reports[1:], 2):
        current = compare.validate_report(report, f"{label} round {index}")
        # The normal comparator checks matching parameters, audio and scenario sets.
        compare.compare_reports(first, report)
        require(report["git"] == first["git"] and report["engine"] == first["engine"],
                f"{label}: engine identity changed between rounds")
        scenario_maps.append(current)
    merged = deepcopy(first)
    merged["repetitions"] = len(reports)
    for scenario in merged["scenarios"]:
        name = scenario["name"]
        matching = [mapping[name] for mapping in scenario_maps]
        scenario["runs"] = [deepcopy(item["runs"][0]) for item in matching]
        scenario["deterministic"] = True  # compare_reports above verified every fingerprint.
        for phase in ("held", "tail"):
            timings = [item[phase]["wall_seconds"][0] for item in matching]
            seconds = scenario[phase]["audio_seconds"]
            scenario[phase] = {
                "wall_seconds": timings, "median_seconds": statistics.median(timings),
                "min_seconds": min(timings), "max_seconds": max(timings),
                "audio_seconds": seconds,
                "median_realtime_percent": 100 * statistics.median(timings) / seconds,
            }
    compare.validate_report(merged, f"{label} merged")
    return merged


def machine_description() -> dict[str, Any]:
    result: dict[str, Any] = {
        "system": platform.system(), "release": platform.release(),
        "machine": platform.machine(), "processor": platform.processor(),
        "logical_cpus_reported": os.cpu_count(), "python": platform.python_version(),
    }
    if hasattr(os, "sched_getaffinity"):
        try:
            result["process_cpu_affinity"] = sorted(os.sched_getaffinity(0))
        except OSError:
            pass
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.is_file():
        # Hardware labels are contextual metadata, not proof of exclusive CPU use.
        allowed = {"vendor_id", "model name", "cpu family", "model", "stepping",
                   "Hardware", "CPU implementer", "CPU architecture", "CPU part"}
        values: dict[str, set[str]] = {}
        for line in cpuinfo.read_text(errors="replace").splitlines():
            key, separator, value = line.partition(":")
            if separator and key.strip() in allowed:
                values.setdefault(key.strip(), set()).add(value.strip())
        result["cpuinfo_labels"] = {key: sorted(value) for key, value in values.items()}
    return result


def source_description() -> dict[str, Any]:
    root = Path(__file__).resolve().parents[2]
    paths = ["src/performance/CpuBenchmark.cpp", "src/performance/CMakeLists.txt",
             "scripts/performance/compare.py", "scripts/performance/run_comparison.py",
             "scripts/performance/thread_clock.py"]
    result: dict[str, Any] = {
        "root": str(root),
        "files": {name: identity(root / name) for name in paths if (root / name).is_file()},
        "scope": "Current working-tree harness/scripts; these hashes do not attest binary build inputs.",
    }
    try:
        result["git_head"] = subprocess.check_output(
            ["git", "-C", str(root), "rev-parse", "HEAD"], text=True,
            stderr=subprocess.DEVNULL, timeout=10).strip()
        diff = subprocess.check_output(
            ["git", "-C", str(root), "diff", "--binary", "HEAD"],
            stderr=subprocess.DEVNULL, timeout=10)
        result["tracked_diff_sha256"] = hashlib.sha256(diff).hexdigest()
        result["tracked_diff_empty"] = not diff
    except (OSError, subprocess.SubprocessError):
        result["git_metadata_available"] = False
    return result


def bounded_int(lower: int, upper: int):
    def parse(value: str) -> int:
        try:
            number = int(value)
        except ValueError as error:
            raise argparse.ArgumentTypeError("expected an integer") from error
        if not lower <= number <= upper:
            raise argparse.ArgumentTypeError(f"expected {lower}..{upper}")
        return number
    return parse


def positive_seconds(value: str) -> float:
    try:
        number = float(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("expected finite seconds") from error
    if not math.isfinite(number) or not 0 < number <= 30:
        raise argparse.ArgumentTypeError("expected finite seconds greater than zero and <= 30")
    return number


def parse(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--rounds", type=bounded_int(1, 30), default=7)
    parser.add_argument("--seconds", type=positive_seconds, default=2.0)
    parser.add_argument("--tail-seconds", type=positive_seconds, default=1.0)
    parser.add_argument("--sample-rate", type=bounded_int(12000, 192000), default=48000)
    parser.add_argument("--scenario", default="all")
    parser.add_argument("--data", type=Path)
    parser.add_argument("--patch", type=Path)
    parser.add_argument("--thread-cpu", action="store_true",
                        help="Also request and compare Linux render-thread CPU clock reports")
    return parser.parse_args(argv)


def prepare(args: argparse.Namespace) -> tuple[dict[str, Path], Path]:
    binaries = {side: getattr(args, side).resolve() for side in ("baseline", "candidate")}
    for side, binary in binaries.items():
        require(binary.is_file() and os.access(binary, os.X_OK),
                f"{side}: executable is missing or not executable: {binary}")
    require(not os.path.samefile(binaries["baseline"], binaries["candidate"]),
            "Baseline and candidate must be distinct executable files, not aliases")
    if args.data is not None:
        args.data = args.data.resolve()
        require(args.data.is_dir(), f"Data directory does not exist: {args.data}")
    if args.patch is not None:
        args.patch = args.patch.resolve()
        require(args.patch.is_file(), f"Patch does not exist: {args.patch}")
        require(args.scenario in ("all", "patch"), "--patch requires --scenario patch or all")
    output = args.output_dir.resolve()
    require(not output.exists() or (output.is_dir() and not any(output.iterdir())),
            f"Output directory must be absent or empty: {output}")
    output.mkdir(parents=True, exist_ok=True)
    return binaries, output


def summary_text(result: dict[str, Any], rounds: int) -> str:
    lines = [f"Paired offline comparison: {rounds} rounds, alternating AB/BA order.",
             "First-pair complete PCM verified; every round has the same recorded audio fingerprint.",
             "Positive reduction means less measured wall time; shared-machine noise remains possible."]
    for item in result["scenarios"]:
        lines.append(f"{item['name']}: held {item['held']['median_reduction_percent']:+.3f}%, "
                     f"tail {item['tail']['median_reduction_percent']:+.3f}% median reduction")
    if rounds == 1:
        lines.append("One round is a smoke check and cannot establish timing repeatability.")
    return "\n".join(lines) + "\n"


def validate_requested_fixture(report: dict[str, Any], args: argparse.Namespace) -> None:
    require(report["sample_rate"] == args.sample_rate, "Benchmark ignored requested sample rate")
    require(report["patch"] == (str(args.patch) if args.patch is not None else ""),
            "Benchmark report does not identify the requested patch")
    expected_name = "patch" if args.patch is not None else args.scenario
    if expected_name != "all":
        require([item["name"] for item in report["scenarios"]] == [expected_name],
                "Benchmark did not run exactly the requested scenario")
    block = report["block_size"]
    for phase, requested in (("held", args.seconds), ("tail", args.tail_seconds)):
        expected = math.ceil(requested * args.sample_rate / block) * block / args.sample_rate
        for scenario in report["scenarios"]:
            compare.close(scenario[phase]["audio_seconds"], expected,
                          f"{scenario['name']}.{phase}: requested duration")


def run(args: argparse.Namespace) -> dict[str, Any]:
    binaries, output = prepare(args)
    thread_enabled = getattr(args, "thread_cpu", False)
    records = {side: identity(binary) for side, binary in binaries.items()}
    patch_identity = identity(args.patch) if args.patch is not None else None
    provenance: dict[str, Any] = {
        "schema": 1, "tool": "surge-cpu-paired-runner", "ok": False,
        "started_utc": datetime.now(timezone.utc).isoformat(), "cwd": str(Path.cwd()),
        "machine": machine_description(), "binaries": records, "patch": patch_identity,
        "source": source_description(), "rounds": args.rounds, "commands": [],
        "order": [list(execution_order(index)) for index in range(args.rounds)],
        "limitations": [
            "OS/CPU labels do not prove exclusive hardware or fixed clock frequency.",
            "Ambient user defaults and external tuning services must be controlled by the operator.",
            "Requested report flags are not a substitute for retaining effective build commands.",
            "Only first-round PCM is retained; subsequent audio equality uses noncryptographic fingerprints.",
        ],
    }
    if thread_enabled:
        provenance["thread_cpu_clock"] = thread_clock.CLOCK_NAME
        provenance["limitations"].append(
            "Render-thread CPU time excludes scheduling waits and other threads, but is not a cycle count.")
    write_json(output / "provenance.json", provenance)
    reports: dict[str, list[dict[str, Any]]] = {"baseline": [], "candidate": []}
    thread_reports: dict[str, list[dict[str, Any]]] = {"baseline": [], "candidate": []}
    try:
        for index in range(args.rounds):
            for side in execution_order(index):
                require(identity(binaries[side]) == records[side], f"{side}: binary changed during run")
                if patch_identity is not None:
                    require(identity(args.patch) == patch_identity, "Patch changed during run")
                stem = f"round-{index + 1:02d}-{side}"
                report_path = output / f"{stem}.json"
                command = [str(binaries[side]), "--output", str(report_path), "--repetitions", "1",
                           "--seconds", str(args.seconds), "--tail-seconds", str(args.tail_seconds),
                           "--sample-rate", str(args.sample_rate), "--scenario", args.scenario]
                if index == 0:
                    command += ["--pcm-dir", str(output / f"{side}-pcm")]
                if args.data is not None:
                    command += ["--data", str(args.data)]
                if args.patch is not None:
                    command += ["--patch", str(args.patch)]
                thread_path = output / f"{stem}-thread.json"
                if thread_enabled:
                    command += ["--thread-output", str(thread_path)]
                entry: dict[str, Any] = {"round": index + 1, "side": side,
                                         "argv": command, "log": f"{stem}.log"}
                provenance["commands"].append(entry)
                write_json(output / "provenance.json", provenance)
                start = time.monotonic()
                with (output / f"{stem}.log").open("wb") as log:
                    # The CLI bounds audio duration; additionally bound each process.
                    completed = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                               check=False, timeout=300)
                entry["process_wall_seconds"] = time.monotonic() - start
                entry["returncode"] = completed.returncode
                write_json(output / "provenance.json", provenance)
                require(completed.returncode == 0,
                        f"{stem}: benchmark failed ({completed.returncode}); inspect {stem}.log")
                report = compare.read_report(report_path)
                compare.validate_report(report, stem)
                require(report["repetitions"] == 1, f"{stem}: expected exactly one repetition")
                validate_requested_fixture(report, args)
                if thread_enabled:
                    thread_report = compare.read_report(thread_path)
                    thread_clock.validate_report(thread_report, report, stem)
                    thread_reports[side].append(thread_report)
                    entry["thread_report"] = thread_path.name
                    entry["thread_report_sha256"] = digest(thread_path)
                reports[side].append(report)
                # Fail promptly if a later process rendered different audio or metadata.
                compare.compare_reports(reports[side][0], report)
            compare.compare_reports(reports["baseline"][-1], reports["candidate"][-1])
        merged = {side: merge_reports(items, side) for side, items in reports.items()}
        for side, report in merged.items():
            require(identity(binaries[side]) == records[side], f"{side}: binary changed during run")
            write_json(output / f"{side}.json", report)
        if patch_identity is not None:
            require(identity(args.patch) == patch_identity, "Patch changed during run")
        result = compare.compare_reports(merged["baseline"], merged["candidate"],
                                         baseline_pcm=output / "baseline-pcm",
                                         candidate_pcm=output / "candidate-pcm")
        write_json(output / "comparison.json", result)
        text = summary_text(result, args.rounds)
        if thread_enabled:
            cpu_result = thread_clock.compare_reports(
                thread_reports["baseline"], thread_reports["candidate"],
                reports["baseline"], reports["candidate"])
            cpu_result["wall_comparison"] = "comparison.json"
            cpu_result["full_pcm_verified"] = result["full_pcm_verified"]
            write_json(output / "thread-comparison.json", cpu_result)
            cpu_text = thread_clock.summary_text(cpu_result)
            (output / "thread-summary.txt").write_text(cpu_text, encoding="utf-8")
            text += "\n" + cpu_text
        (output / "summary.txt").write_text(text, encoding="utf-8")
        provenance["ok"] = True
        provenance["finished_utc"] = datetime.now(timezone.utc).isoformat()
        write_json(output / "provenance.json", provenance)
        print(text, end="")
        return result
    except (RunnerError, compare.ComparisonError, OSError, ValueError,
            subprocess.SubprocessError) as error:
        provenance["error"] = str(error)
        provenance["finished_utc"] = datetime.now(timezone.utc).isoformat()
        write_json(output / "provenance.json", provenance)
        raise


def main(argv: list[str] | None = None) -> int:
    args = parse(argv)
    try:
        run(args)
        return 0
    except (RunnerError, compare.ComparisonError, OSError, ValueError,
            subprocess.SubprocessError) as error:
        print(f"run_comparison.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
