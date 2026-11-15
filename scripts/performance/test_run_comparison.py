"""Paired runner contracts; all child timings/audio here are synthetic fixtures."""
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

import run_comparison as runner
from test_compare import fixture, PCM, phase


def one_report(seconds=0.001):
    report = fixture()
    report["sample_rate"] = 16000
    report["repetitions"] = 1
    scenario = report["scenarios"][0]
    scenario["runs"] = scenario["runs"][:1]
    scenario["held"] = phase((seconds,))
    scenario["tail"] = phase((seconds / 2,))
    return report


class MergeTests(unittest.TestCase):
    def test_merges_raw_timings_and_recalculates_statistics(self):
        reports = [one_report(value) for value in (0.003, 0.001, 0.002)]
        original = deepcopy(reports)
        merged = runner.merge_reports(reports, "test")
        self.assertEqual(reports, original)
        self.assertEqual(merged["repetitions"], 3)
        held = merged["scenarios"][0]["held"]
        self.assertEqual(held["wall_seconds"], [0.003, 0.001, 0.002])
        self.assertEqual(held["median_seconds"], 0.002)
        self.assertEqual(held["median_realtime_percent"], 50.0)
        self.assertEqual(len(merged["scenarios"][0]["runs"]), 3)

    def test_rejects_empty_or_excessive_rounds(self):
        for reports in ([], [one_report()] * 31):
            with self.assertRaises(runner.RunnerError):
                runner.merge_reports(reports, "test")

    def test_rejects_multiple_repetitions_in_single_round(self):
        report = one_report()
        report["repetitions"] = 3
        scenario = report["scenarios"][0]
        scenario["runs"] *= 3
        for phase_name in ("held", "tail"):
            scenario[phase_name]["wall_seconds"] *= 3
        with self.assertRaises(runner.RunnerError):
            runner.merge_reports([report], "test")

    def test_rejects_audio_drift_between_rounds(self):
        changed = one_report()
        changed["scenarios"][0]["runs"][0]["pcm_hash"] = "0" * 16
        with self.assertRaises(runner.compare.ComparisonError):
            runner.merge_reports([one_report(), changed], "test")

    def test_rejects_changed_engine_identity(self):
        for field in ("git", "engine"):
            changed = one_report()
            changed[field] = "changed"
            with self.subTest(field=field), self.assertRaises(runner.RunnerError):
                runner.merge_reports([one_report(), changed], "test")

    def test_rejects_changed_metadata_or_scenarios(self):
        for mutation in (lambda r: r.update(seed=13),
                         lambda r: r["scenarios"][0].update(name="other")):
            changed = one_report()
            mutation(changed)
            with self.assertRaises(runner.compare.ComparisonError):
                runner.merge_reports([one_report(), changed], "test")

    def test_rejects_invalid_nonfinite_and_false_success(self):
        for mutation in (lambda r: r.update(ok=False),
                         lambda r: r["scenarios"][0]["held"]["wall_seconds"].__setitem__(0, float("nan")),
                         lambda r: r["scenarios"][0].update(deterministic=False)):
            changed = one_report()
            mutation(changed)
            with self.assertRaises(runner.compare.ComparisonError):
                runner.merge_reports([changed], "test")

    def test_order_alternates_without_reordering_pair_members(self):
        self.assertEqual([runner.execution_order(i) for i in range(4)],
                         [("baseline", "candidate"), ("candidate", "baseline"),
                          ("baseline", "candidate"), ("candidate", "baseline")])


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.baseline = self.root / "baseline"
        self.candidate = self.root / "candidate"
        for path in (self.baseline, self.candidate):
            path.write_bytes(("synthetic " + path.name).encode())
            path.chmod(0o755)
        self.output = self.root / "evidence"
        self.argv = ["--baseline", str(self.baseline), "--candidate", str(self.candidate),
                     "--output-dir", str(self.output), "--rounds", "3", "--sample-rate", "16000",
                     "--scenario", "init-1", "--seconds", "0.004", "--tail-seconds", "0.004"]
        self.commands = []

    def tearDown(self):
        self.temporary.cleanup()

    def child(self, command, **kwargs):
        self.commands.append(command)
        report = one_report(0.001 if command[0] == str(self.baseline) else 0.0008)
        report["git"] = "baseline-source" if command[0] == str(self.baseline) else "candidate-source"
        path = Path(command[command.index("--output") + 1])
        path.write_text(json.dumps(report))
        if "--pcm-dir" in command:
            pcm_dir = Path(command[command.index("--pcm-dir") + 1])
            pcm_dir.mkdir(parents=True)
            (pcm_dir / "init-1.f32").write_bytes(PCM)
        kwargs["stdout"].write(b"synthetic child log\n")
        return subprocess.CompletedProcess(command, 0)

    def execute(self, child=None, argv=None):
        with mock.patch.object(runner, "source_description", return_value={"synthetic": True}), \
             mock.patch.object(runner, "machine_description", return_value={"synthetic": True}), \
             mock.patch.object(runner.subprocess, "run", side_effect=child or self.child), \
             redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            return runner.main(self.argv if argv is None else argv)

    def test_complete_paired_run_retains_evidence_and_full_pcm(self):
        self.assertEqual(self.execute(), 0)
        sides = [Path(command[0]).name for command in self.commands]
        self.assertEqual(sides, ["baseline", "candidate", "candidate", "baseline", "baseline", "candidate"])
        self.assertEqual(sum("--pcm-dir" in command for command in self.commands), 2)
        provenance = json.loads((self.output / "provenance.json").read_text())
        self.assertTrue(provenance["ok"])
        self.assertEqual(len(provenance["commands"]), 6)
        self.assertEqual(provenance["binaries"]["baseline"]["sha256"], runner.digest(self.baseline))
        result = json.loads((self.output / "comparison.json").read_text())
        self.assertTrue(result["full_pcm_verified"])
        self.assertAlmostEqual(result["scenarios"][0]["held"]["median_reduction_percent"], 20)
        merged = json.loads((self.output / "baseline.json").read_text())
        self.assertEqual(merged["repetitions"], 3)
        self.assertEqual(len(list(self.output.glob("round-*.log"))), 6)
        self.assertIn("offline", (self.output / "summary.txt").read_text())

    def test_rejects_same_binary_and_symlink_alias(self):
        for candidate in (self.baseline, self.root / "alias"):
            if candidate != self.baseline:
                candidate.symlink_to(self.baseline)
            argv = list(self.argv)
            argv[argv.index("--candidate") + 1] = str(candidate)
            self.assertEqual(self.execute(argv=argv), 2)
        self.assertFalse(self.output.exists())

    def test_rejects_hard_link_alias(self):
        alias = self.root / "hard-link"
        alias.hardlink_to(self.baseline)
        argv = list(self.argv)
        argv[argv.index("--candidate") + 1] = str(alias)
        self.assertEqual(self.execute(argv=argv), 2)

    def test_does_not_overwrite_existing_evidence(self):
        self.output.mkdir()
        marker = self.output / "keep.txt"
        marker.write_text("keep")
        self.assertEqual(self.execute(), 2)
        self.assertEqual(marker.read_text(), "keep")
        self.assertEqual(self.commands, [])

    def test_child_failure_preserves_log_and_failure_provenance(self):
        def fail(command, **kwargs):
            kwargs["stdout"].write(b"known child failure")
            return subprocess.CompletedProcess(command, 7)
        self.assertEqual(self.execute(fail), 2)
        provenance = json.loads((self.output / "provenance.json").read_text())
        self.assertFalse(provenance["ok"])
        self.assertEqual(provenance["commands"][0]["returncode"], 7)
        self.assertIn("known child failure", (self.output / "round-01-baseline.log").read_text())
        self.assertFalse((self.output / "comparison.json").exists())

    def test_child_timeout_is_bounded_and_recorded(self):
        def timeout(command, **kwargs):
            self.assertEqual(kwargs["timeout"], 300)
            raise subprocess.TimeoutExpired(command, kwargs["timeout"])
        self.assertEqual(self.execute(timeout), 2)
        self.assertIn("timed out", json.loads((self.output / "provenance.json").read_text())["error"])

    def test_rejects_nonconforming_requested_fixture(self):
        def wrong(command, **kwargs):
            result = self.child(command, **kwargs)
            path = Path(command[command.index("--output") + 1])
            report = json.loads(path.read_text())
            report["scenarios"][0]["name"] = "not-requested"
            path.write_text(json.dumps(report))
            return result
        self.assertEqual(self.execute(wrong), 2)
        self.assertFalse((self.output / "comparison.json").exists())

    def test_rejects_binary_change_between_rounds(self):
        def changed(command, **kwargs):
            result = self.child(command, **kwargs)
            if len(self.commands) == 2:
                self.baseline.write_bytes(b"modified while measuring")
            return result
        self.assertEqual(self.execute(changed), 2)
        self.assertIn("binary changed", json.loads((self.output / "provenance.json").read_text())["error"])

    def test_rejects_later_audio_mismatch(self):
        def mismatch(command, **kwargs):
            result = self.child(command, **kwargs)
            if len(self.commands) == 3:
                path = Path(command[command.index("--output") + 1])
                report = json.loads(path.read_text())
                report["scenarios"][0]["runs"][0]["pcm_hash"] = "1" * 16
                path.write_text(json.dumps(report))
            return result
        self.assertEqual(self.execute(mismatch), 2)
        self.assertFalse((self.output / "comparison.json").exists())

    def test_rejects_corrupted_first_pair_pcm(self):
        def corrupt(command, **kwargs):
            result = self.child(command, **kwargs)
            if len(self.commands) == 2:
                (self.output / "candidate-pcm/init-1.f32").write_bytes(b"bad PCM")
            return result
        self.assertEqual(self.execute(corrupt), 2)
        self.assertFalse(json.loads((self.output / "provenance.json").read_text())["ok"])

    def test_rejects_invalid_numeric_cli_arguments(self):
        for option, value in (("--rounds", "0"), ("--rounds", "31"), ("--seconds", "nan"),
                              ("--tail-seconds", "inf"), ("--sample-rate", "1")):
            argv = list(self.argv)
            argv[argv.index(option) + 1] = value
            with self.subTest(option=option, value=value), redirect_stderr(io.StringIO()), \
                 self.assertRaises(SystemExit):
                runner.parse(argv)


if __name__ == "__main__":
    unittest.main()
