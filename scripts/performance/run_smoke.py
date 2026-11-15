#!/usr/bin/env python3
"""Run bounded native correctness smoke checks, without a CPU speed threshold.

Every invocation uses fresh temporary PCM directories. Retained JSON and logs
describe one build's repeatability and fixture integrity, not baseline equivalence.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from compare import ComparisonError, read_report, require, validate_report, verify_pcm


BENCHMARK_SCENARIOS = {
    "idle", "init-1", "init-8", "init-16", "lowcut-steep", "unused-formula",
    "active-formula", "conditioner", "eq3", "eq11",
}


def native(command: list[str], log: Path, commands: list[list[str]]) -> None:
    commands.append(command)
    with log.open("w", encoding="utf-8") as stream:
        completed = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT,
                                   timeout=180, check=False)
    require(completed.returncode == 0,
            f"Native command exited {completed.returncode}; see {log.name}")


def verify_benchmark(directory: Path) -> dict:
    report = read_report(directory / "benchmark.json")
    scenarios = validate_report(report, "smoke benchmark")
    require(set(scenarios) == BENCHMARK_SCENARIOS, "Benchmark scenario coverage changed")
    require(report["repetitions"] == 2, "Smoke requires two deterministic repetitions")
    pcm = directory / "benchmark-pcm"
    evidence = {}
    for name, scenario in scenarios.items():
        # Reading the same output twice checks its length, finiteness and reported
        # fingerprint. This is deliberately not represented as a before/after test.
        binding = verify_pcm(pcm, pcm, name, report["frames"] * report["channels"] * 4,
                             scenario["runs"][0]["pcm_hash"])
        evidence[name] = {"bytes": binding["bytes_per_file"], "sha256": binding["sha256"]}
    return {"scenario_count": len(scenarios), "repetitions": 2, "pcm": evidence}


def verify_regression(directory: Path) -> dict:
    pcm = directory / "regression"
    manifest = read_report(pcm / "manifest.json")
    require(manifest.get("schema") == 1 and manifest.get("tool") == "surge-dsp-regression",
            "Unexpected regression manifest")
    require(manifest.get("ok") is True and manifest.get("fpu_guard") is True,
            "Regression did not report guarded, successful rendering")
    require(manifest.get("format") == "f32le-interleaved" and manifest.get("channels") == 2
            and manifest.get("hash_algorithm") == "fnv1a64-f32le",
            "Unsupported regression PCM format")
    fixtures = manifest.get("fixtures")
    require(isinstance(fixtures, list) and len(fixtures) == 63,
            "Expected all 63 effect and lowcut lifecycle fixtures")
    evidence = {}
    for fixture in fixtures:
        require(isinstance(fixture, dict), "Invalid regression fixture")
        name = fixture.get("name")
        require(isinstance(name, str) and name and all(c.isalnum() or c == "-" for c in name),
                "Invalid regression fixture name")
        require(name not in evidence and fixture.get("file") == name + ".f32",
                "Duplicate fixture or unexpected PCM path")
        frames = fixture.get("frames")
        require(type(frames) is int and frames == manifest["blocks"] * manifest["block_size"],
                f"{name}: unexpected frame count")
        binding = verify_pcm(pcm, pcm, name, frames * 2 * 4, fixture.get("pcm_hash"))
        evidence[name] = {"bytes": binding["bytes_per_file"], "sha256": binding["sha256"]}
    return {"fixture_count": len(fixtures), "pcm": evidence}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", required=True, type=Path)
    parser.add_argument("--regression", required=True, type=Path)
    parser.add_argument("--evidence-dir", required=True, type=Path)
    args = parser.parse_args()
    args.evidence_dir.mkdir(parents=True, exist_ok=True)
    # Replace only this helper's previous outputs, so a failed rerun cannot
    # accidentally retain a successful manifest from an earlier executable.
    for name in ("benchmark.json", "benchmark.log", "regression.log",
                 "regression-manifest.json", "summary.json"):
        (args.evidence_dir / name).unlink(missing_ok=True)
    commands: list[list[str]] = []
    summary = {
        "schema": 1, "tool": "surge-cpu-smoke", "ok": False,
        "baseline_equivalence_checked": False, "performance_threshold_applied": False,
        "scope": "Single-build deterministic benchmark and native fixture integrity",
    }
    with tempfile.TemporaryDirectory(prefix="surge-cpu-smoke-") as temporary:
        directory = Path(temporary)
        try:
            benchmark = args.benchmark.resolve(strict=True)
            regression = args.regression.resolve(strict=True)
            summary["binary_sha256"] = {
                "benchmark": hashlib.sha256(benchmark.read_bytes()).hexdigest(),
                "regression": hashlib.sha256(regression.read_bytes()).hexdigest(),
            }
            native([str(benchmark), "--output", str(directory / "benchmark.json"),
                    "--pcm-dir", str(directory / "benchmark-pcm"), "--repetitions", "2",
                    "--seconds", "0.15", "--tail-seconds", "0.15", "--sample-rate", "48000"],
                   directory / "benchmark.log", commands)
            summary["benchmark"] = verify_benchmark(directory)
            native([str(regression), "--output-dir", str(directory / "regression")],
                   directory / "regression.log", commands)
            summary["regression"] = verify_regression(directory)
            summary["ok"] = True
        except (ComparisonError, OSError, ValueError, KeyError, TypeError,
                subprocess.TimeoutExpired) as error:
            summary["error"] = str(error)
            print(f"run_smoke.py: {error}", file=sys.stderr)
        finally:
            # Never retain the PCM or remove unrelated content from the evidence
            # directory. Fresh temporary rendering makes repeated CTest runs safe.
            for source, name in ((directory / "benchmark.json", "benchmark.json"),
                                 (directory / "benchmark.log", "benchmark.log"),
                                 (directory / "regression.log", "regression.log"),
                                 (directory / "regression/manifest.json", "regression-manifest.json")):
                if source.is_file():
                    shutil.copyfile(source, args.evidence_dir / name)
            summary["commands"] = commands
            (args.evidence_dir / "summary.json").write_text(
                json.dumps(summary, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    if summary["ok"]:
        print("Passed 10 benchmark scenarios with two identical repetitions and 63 finite DSP fixtures.")
        print("No baseline equivalence or CPU improvement is claimed by this smoke check.")
    return 0 if summary["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
