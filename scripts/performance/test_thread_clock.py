"""Thread-clock contracts. Every timing and child process in this file is synthetic."""
from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import compare
import run_comparison as runner
import thread_clock
from test_compare import PCM
from test_run_comparison import one_report


def sidecar(wall=None, cpu=0.0007):
    wall = one_report() if wall is None else wall
    result = {field: wall[field] for field in thread_clock.METADATA}
    result.update(schema=1, tool="surge-cpu-bench-thread-clock",
                  clock=thread_clock.CLOCK_NAME, ok=True)
    result["scenarios"] = [{
        "name": scenario["name"],
        "held_audio_seconds": scenario["held"]["audio_seconds"],
        "tail_audio_seconds": scenario["tail"]["audio_seconds"],
        "pcm_hashes": [run["pcm_hash"] for run in scenario["runs"]],
        "held_thread_cpu_seconds": [cpu] * wall["repetitions"],
        "tail_thread_cpu_seconds": [cpu / 2] * wall["repetitions"],
    } for scenario in wall["scenarios"]]
    return result


class ValidationTests(unittest.TestCase):
    def setUp(self):
        self.wall = one_report()
        self.cpu = sidecar(self.wall)

    def reject(self):
        with self.assertRaises(compare.ComparisonError):
            thread_clock.validate_report(self.cpu, self.wall, "synthetic")

    def test_accepts_matching_metadata_audio_and_timings(self):
        result = thread_clock.validate_report(self.cpu, self.wall, "synthetic")
        self.assertEqual(set(result), {"init-1"})

    def test_rejects_missing_and_unknown_fields(self):
        for root in (True, False):
            for extra in (True, False):
                self.cpu = sidecar(self.wall)
                target = self.cpu if root else self.cpu["scenarios"][0]
                if extra:
                    target["invented"] = 1
                else:
                    del target["schema" if root else "pcm_hashes"]
                self.reject()

    def test_rejects_wrong_clock_tool_schema_and_success(self):
        for key, value in (("clock", "CLOCK_PROCESS_CPUTIME_ID"),
                           ("tool", "another-tool"), ("schema", True),
                           ("schema", 2), ("ok", False), ("ok", 1)):
            self.cpu = sidecar(self.wall)
            self.cpu[key] = value
            with self.subTest(key=key, value=value):
                self.reject()

    def test_rejects_metadata_mismatch_and_boolean_integer(self):
        for field in thread_clock.METADATA:
            for value in (self.wall[field] + 1, True):
                self.cpu = sidecar(self.wall)
                self.cpu[field] = value
                with self.subTest(field=field, value=value):
                    self.reject()

    def test_rejects_scenario_missing_extra_duplicate_and_invalid(self):
        for name in ("unexpected", "../outside", None, True):
            self.cpu = sidecar(self.wall)
            self.cpu["scenarios"][0]["name"] = name
            self.reject()
        self.cpu = sidecar(self.wall)
        self.cpu["scenarios"] *= 2
        self.reject()
        self.cpu["scenarios"] = []
        self.reject()

    def test_rejects_invalid_and_miscounted_cpu_samples(self):
        for field in ("held_thread_cpu_seconds", "tail_thread_cpu_seconds"):
            for values in ([], [0.1, 0.2], [0], [-1], [float("nan")],
                           [float("inf")], [True], ["0.1"], "not-an-array"):
                self.cpu = sidecar(self.wall)
                self.cpu["scenarios"][0][field] = values
                with self.subTest(field=field, values=values):
                    self.reject()

    def test_rejects_audio_duration_and_fingerprint_drift(self):
        for field, value in (("held_audio_seconds", 0.008),
                             ("tail_audio_seconds", 0),
                             ("pcm_hashes", []), ("pcm_hashes", ["0" * 16])):
            self.cpu = sidecar(self.wall)
            self.cpu["scenarios"][0][field] = value
            self.reject()

    def test_scenario_order_is_irrelevant(self):
        another = deepcopy(self.wall["scenarios"][0])
        another["name"] = "second"
        self.wall["scenarios"].append(another)
        self.cpu = sidecar(self.wall)
        self.cpu["scenarios"].reverse()
        self.assertEqual(len(thread_clock.validate_report(self.cpu, self.wall, "test")), 2)

    def test_recomputes_raw_array_medians_ranges_and_reduction(self):
        before = [sidecar(cpu=x) for x in (0.003, 0.001, 0.002)]
        after = [sidecar(cpu=x) for x in (0.001, 0.0008, 0.0016)]
        originals = deepcopy((before, after))
        result = thread_clock.compare_reports(before, after, [self.wall] * 3, [self.wall] * 3)
        self.assertEqual((before, after), originals)
        held = result["scenarios"][0]["held"]
        self.assertEqual(held["baseline_thread_cpu_seconds"], [0.003, 0.001, 0.002])
        self.assertEqual(held["candidate_range_seconds"], [0.0008, 0.0016])
        self.assertEqual(held["baseline_median_seconds"], 0.002)
        self.assertEqual(held["candidate_median_seconds"], 0.001)
        self.assertEqual(held["median_reduction_percent"], 50)
        self.assertEqual(held["candidate_thread_realtime_percent"], 25)
        self.assertNotIn("full_pcm_verified", result)

    def test_rejects_unmatched_round_counts_and_multi_repetition_round(self):
        for before, after, walls in (([], [], []), ([self.cpu], [], [self.wall]),
                                      ([self.cpu] * 31, [self.cpu] * 31, [self.wall] * 31)):
            with self.assertRaises(compare.ComparisonError):
                thread_clock.compare_reports(before, after, walls, walls)
        wall = deepcopy(self.wall)
        wall["repetitions"] = 2
        scenario = wall["scenarios"][0]
        scenario["runs"] *= 2
        for phase in ("held", "tail"):
            scenario[phase]["wall_seconds"] *= 2
        cpu = sidecar(wall)
        with self.assertRaises(compare.ComparisonError):
            thread_clock.compare_reports([cpu], [cpu], [wall], [wall])

    def test_rejects_changed_audio_or_engine_between_rounds(self):
        for field in ("git", "engine"):
            wall = deepcopy(self.wall)
            wall[field] = "another-engine"
            with self.assertRaises(compare.ComparisonError):
                thread_clock.compare_reports([self.cpu] * 2, [self.cpu] * 2,
                                              [self.wall, wall], [self.wall] * 2)
        wall = deepcopy(self.wall)
        wall["scenarios"][0]["runs"][0]["pcm_hash"] = "0" * 16
        with self.assertRaises(compare.ComparisonError):
            thread_clock.compare_reports([self.cpu], [sidecar(wall)], [self.wall], [wall])

    def test_rejects_timing_magnitudes_that_overflow_summaries(self):
        for before, after in ((1e-300, 1e300), (1e308, 1e308)):
            with self.subTest(before=before, after=after), self.assertRaises(compare.ComparisonError):
                thread_clock.compare_reports([sidecar(cpu=before)], [sidecar(cpu=after)],
                                              [self.wall], [self.wall])


class IntegrationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.binaries = {side: self.root / side for side in ("baseline", "candidate")}
        for side, path in self.binaries.items():
            path.write_text(side)
            path.chmod(0o755)
        self.output = self.root / "evidence"
        self.argv = ["--baseline", str(self.binaries["baseline"]),
                     "--candidate", str(self.binaries["candidate"]),
                     "--output-dir", str(self.output), "--rounds", "2", "--seconds", "0.004",
                     "--tail-seconds", "0.004", "--sample-rate", "16000",
                     "--scenario", "init-1", "--thread-cpu"]
        self.commands = []

    def child(self, command, **kwargs):
        self.commands.append(command)
        old = command[0] == str(self.binaries["baseline"])
        wall = one_report(0.001 if old else 0.0008)
        report_path = Path(command[command.index("--output") + 1])
        report_path.write_text(json.dumps(wall))
        thread_path = Path(command[command.index("--thread-output") + 1])
        thread_path.write_text(json.dumps(sidecar(wall, 0.0007 if old else 0.0005)))
        if "--pcm-dir" in command:
            pcm = Path(command[command.index("--pcm-dir") + 1])
            pcm.mkdir()
            (pcm / "init-1.f32").write_bytes(PCM)
        kwargs["stdout"].write(b"synthetic thread-clock child\n")
        return subprocess.CompletedProcess(command, 0)

    def execute(self, child=None):
        with mock.patch.object(runner, "source_description", return_value={"synthetic": True}), \
             mock.patch.object(runner, "machine_description", return_value={"synthetic": True}), \
             mock.patch.object(runner.subprocess, "run", side_effect=child or self.child), \
             redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            return runner.main(self.argv)

    def test_complete_run_keeps_wall_and_thread_evidence_distinct(self):
        self.assertEqual(self.execute(), 0)
        self.assertEqual([Path(command[0]).name for command in self.commands],
                         ["baseline", "candidate", "candidate", "baseline"])
        wall = json.loads((self.output / "comparison.json").read_text())
        cpu = json.loads((self.output / "thread-comparison.json").read_text())
        self.assertAlmostEqual(wall["scenarios"][0]["held"]["median_reduction_percent"], 20)
        self.assertAlmostEqual(cpu["scenarios"][0]["held"]["median_reduction_percent"], 200 / 7)
        self.assertTrue(cpu["full_pcm_verified"])
        self.assertEqual(cpu["wall_comparison"], "comparison.json")
        self.assertEqual(len(list(self.output.glob("round-*-thread.json"))), 4)
        provenance = json.loads((self.output / "provenance.json").read_text())
        self.assertTrue(provenance["ok"])
        self.assertEqual(provenance["thread_cpu_clock"], thread_clock.CLOCK_NAME)
        for command in provenance["commands"]:
            self.assertEqual(command["thread_report_sha256"],
                             runner.digest(self.output / command["thread_report"]))

    def test_missing_sidecar_fails_with_failure_provenance(self):
        def missing(command, **kwargs):
            result = self.child(command, **kwargs)
            Path(command[command.index("--thread-output") + 1]).unlink()
            return result
        self.assertEqual(self.execute(missing), 2)
        self.assertFalse(json.loads((self.output / "provenance.json").read_text())["ok"])
        self.assertFalse((self.output / "thread-comparison.json").exists())

    def test_wrong_pcm_binding_fails_before_summary(self):
        def corrupt(command, **kwargs):
            result = self.child(command, **kwargs)
            path = Path(command[command.index("--thread-output") + 1])
            cpu = json.loads(path.read_text())
            cpu["scenarios"][0]["pcm_hashes"] = ["0" * 16]
            path.write_text(json.dumps(cpu))
            return result
        self.assertEqual(self.execute(corrupt), 2)
        self.assertFalse((self.output / "thread-comparison.json").exists())

    def test_duplicate_json_fields_are_rejected(self):
        def duplicate(command, **kwargs):
            result = self.child(command, **kwargs)
            path = Path(command[command.index("--thread-output") + 1])
            value = path.read_text()
            path.write_text('{"schema":1,' + value[1:])
            return result
        self.assertEqual(self.execute(duplicate), 2)
        self.assertIn("duplicate JSON field", json.loads(
            (self.output / "provenance.json").read_text())["error"])

    def test_thread_clock_is_explicit_opt_in(self):
        self.assertTrue(runner.parse(self.argv).thread_cpu)
        self.assertFalse(runner.parse(self.argv[:-1]).thread_cpu)


if __name__ == "__main__":
    unittest.main()
