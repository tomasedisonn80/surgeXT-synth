"""Validate optional render-thread CPU clocks against their wall/audio reports.

These measurements charge CPU time to the rendering thread. They are neither
hardware cycle counts nor a claim about real-time scheduling or host deadlines.
"""
from __future__ import annotations

import math
import statistics
from typing import Any

import compare


CLOCK_NAME = "CLOCK_THREAD_CPUTIME_ID"
METADATA = ("sample_rate", "repetitions", "frames", "block_size", "channels", "seed")
ROOT_FIELDS = set(METADATA) | {"schema", "tool", "clock", "scenarios", "ok"}
SCENARIO_FIELDS = {"name", "held_audio_seconds", "tail_audio_seconds", "pcm_hashes",
                   "held_thread_cpu_seconds", "tail_thread_cpu_seconds"}


def validate_report(report: Any, wall: dict[str, Any], label: str) -> dict[str, dict[str, Any]]:
    """Bind one sidecar to the exact fixture/audio metadata in its wall report."""
    wall_scenarios = compare.validate_report(wall, f"{label} wall report")
    report = compare.object_fields(report, ROOT_FIELDS, label)
    compare.require(type(report["schema"]) is int and report["schema"] == 1,
                    f"{label}: unsupported thread-clock schema")
    compare.require(report["tool"] == "surge-cpu-bench-thread-clock" and
                    report["clock"] == CLOCK_NAME and report["ok"] is True,
                    f"{label}: expected a successful render-thread CPU clock report")
    for field in METADATA:
        compare.require(type(report[field]) is type(wall[field]) and report[field] == wall[field],
                        f"{label}: thread/wall metadata mismatch: {field}")
    scenarios = report["scenarios"]
    compare.require(isinstance(scenarios, list) and bool(scenarios),
                    f"{label}: expected a nonempty scenario array")
    result = {}
    repetitions = wall["repetitions"]
    for index, scenario in enumerate(scenarios):
        where = f"{label}.scenarios[{index}]"
        scenario = compare.object_fields(scenario, SCENARIO_FIELDS, where)
        name = scenario["name"]
        compare.require(isinstance(name, str) and name in wall_scenarios,
                        f"{where}: scenario is absent from its wall report")
        compare.require(name not in result, f"{label}: duplicate scenario {name}")
        matching = wall_scenarios[name]
        hashes = scenario["pcm_hashes"]
        compare.require(isinstance(hashes, list) and
                        hashes == [run["pcm_hash"] for run in matching["runs"]],
                        f"{where}: thread/wall PCM fingerprints differ")
        for phase in ("held", "tail"):
            seconds = compare.finite_number(scenario[f"{phase}_audio_seconds"],
                                            f"{where}.{phase}_audio_seconds", positive=True)
            compare.close(seconds, matching[phase]["audio_seconds"],
                          f"{where}: {phase} audio duration")
            timings = scenario[f"{phase}_thread_cpu_seconds"]
            compare.require(isinstance(timings, list) and len(timings) == repetitions,
                            f"{where}.{phase}: expected {repetitions} CPU timings")
            for position, value in enumerate(timings):
                compare.finite_number(value, f"{where}.{phase}[{position}]", positive=True)
        result[name] = scenario
    compare.require(set(result) == set(wall_scenarios),
                    f"{label}: thread/wall scenario coverage differs")
    return result


def compare_reports(baseline: list[dict[str, Any]], candidate: list[dict[str, Any]],
                    baseline_wall: list[dict[str, Any]],
                    candidate_wall: list[dict[str, Any]]) -> dict[str, Any]:
    """Recompute CPU summaries from single-repetition sidecars in round order."""
    count = len(baseline)
    compare.require(1 <= count <= 30 and
                    len(candidate) == len(baseline_wall) == len(candidate_wall) == count,
                    "Thread-clock comparison requires 1..30 matched rounds")
    maps = {}
    for side, reports, walls in (("baseline", baseline, baseline_wall),
                                 ("candidate", candidate, candidate_wall)):
        maps[side] = []
        for index, (report, wall) in enumerate(zip(reports, walls), 1):
            mapping = validate_report(report, wall, f"{side} thread round {index}")
            compare.require(wall["repetitions"] == 1,
                            f"{side} thread round {index}: expected one repetition")
            compare.compare_reports(baseline_wall[0], wall)
            compare.require(wall["git"] == walls[0]["git"] and
                            wall["engine"] == walls[0]["engine"],
                            f"{side}: engine identity changed between thread-clock rounds")
            maps[side].append(mapping)
    scenarios = []
    for name in sorted(maps["baseline"][0]):
        scenario: dict[str, Any] = {"name": name}
        for phase in ("held", "tail"):
            field = f"{phase}_thread_cpu_seconds"
            before = [item[name][field][0] for item in maps["baseline"]]
            after = [item[name][field][0] for item in maps["candidate"]]
            old, new = statistics.median(before), statistics.median(after)
            seconds = maps["baseline"][0][name][f"{phase}_audio_seconds"]
            summary = {
                "audio_seconds": seconds,
                "baseline_thread_cpu_seconds": before,
                "candidate_thread_cpu_seconds": after,
                "baseline_median_seconds": old, "candidate_median_seconds": new,
                "baseline_range_seconds": [min(before), max(before)],
                "candidate_range_seconds": [min(after), max(after)],
                "baseline_thread_realtime_percent": 100 * old / seconds,
                "candidate_thread_realtime_percent": 100 * new / seconds,
                "median_reduction_percent": 100 * (1 - new / old),
            }
            compare.require(all(math.isfinite(summary[field]) for field in
                                ("baseline_thread_realtime_percent", "candidate_thread_realtime_percent",
                                 "median_reduction_percent")),
                            f"{name}.{phase}: CPU timing magnitudes overflow derived statistics")
            scenario[phase] = summary
        scenarios.append(scenario)
    return {
        "schema": 1, "tool": "surge-cpu-thread-comparison", "ok": True,
        "clock": CLOCK_NAME, "rounds": count,
        "metadata": {field: baseline_wall[0][field]
                     for field in METADATA if field != "repetitions"},
        "audio_fingerprints_bound": True,
        "reduction": "100 * (1 - median(candidate) / median(baseline))",
        "scope": ("CPU charged to the rendering thread over matching held/released windows; "
                  "includes engine work, preallocated PCM copies, transport advancement and "
                  "clock-read overhead. Excludes time descheduled and other threads. "
                  "Not hardware cycles, a DAW meter, or a real-time deadline guarantee."),
        "scenarios": scenarios,
    }


def summary_text(result: dict[str, Any]) -> str:
    lines = [f"Render-thread CPU comparison ({CLOCK_NAME}):",
             "Positive reduction means less CPU time charged to the rendering thread.",
             "CPU frequency/cache effects remain; this is not a host-deadline measurement."]
    for scenario in result["scenarios"]:
        lines.append(f"{scenario['name']}: held {scenario['held']['median_reduction_percent']:+.3f}%, "
                     f"tail {scenario['tail']['median_reduction_percent']:+.3f}% CPU median reduction")
    return "\n".join(lines) + "\n"
