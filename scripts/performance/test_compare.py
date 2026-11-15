"""Comparator contract tests. All timing fixtures here are synthetic, not measurements."""

from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import importlib.util
import io
import json
from pathlib import Path
import statistics
import struct
import tempfile
import unittest


_SPEC = importlib.util.spec_from_file_location("surge_cpu_compare", Path(__file__).with_name("compare.py"))
compare = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(compare)


def fingerprint(data: bytes) -> str:
    result = 14695981039346656037
    for byte in data:
        result = ((result ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return f"{result:016x}"


PCM = struct.pack("<256f", *([0.25, -0.25] * 128))


def phase(times=(0.001, 0.0011, 0.0009)):
    return {
        "wall_seconds": list(times), "median_seconds": statistics.median(times),
        "min_seconds": min(times), "max_seconds": max(times), "audio_seconds": 0.004,
        "median_realtime_percent": 100 * statistics.median(times) / 0.004,
    }


def fixture():
    return {
        "schema": 1, "tool": "surge-cpu-bench", "git": "synthetic-before",
        "engine": "synthetic-not-measured", "compiler": "test-compiler",
        "configuration": "Release", "lua_requested": "puc", "has_lua": True,
        "lto_requested": True, "sample_rate": 16000, "block_size": 32, "seed": 1701,
        "warmup_blocks": 128, "repetitions": 3, "format": "f32le-interleaved",
        "channels": 2, "frames": 128, "pcm_hash_algorithm": "fnv1a64-f32le",
        "fpu_guard": True, "timing": "synthetic contract test", "patch": "",
        "scenarios": [{
            "name": "init-1", "requested_notes": 1, "deterministic": True,
            "held": phase(), "tail": phase((0.0005, 0.0006, 0.0004)),
            "runs": [{"pcm_hash": fingerprint(PCM), "peak": 0.25, "rms": 0.25}
                     for _ in range(3)],
        }],
        "ok": True,
    }


class ReportTests(unittest.TestCase):
    def setUp(self):
        self.baseline = fixture()
        self.candidate = deepcopy(self.baseline)
        self.candidate["git"] = "synthetic-after"

    def reject(self):
        with self.assertRaises(compare.ComparisonError):
            compare.compare_reports(self.baseline, self.candidate)

    def test_recomputes_reduction_and_ranges(self):
        self.candidate["scenarios"][0]["held"] = phase((0.0008, 0.0009, 0.0007))
        result = compare.compare_reports(self.baseline, self.candidate)
        self.assertTrue(result["ok"])
        self.assertFalse(result["full_pcm_verified"])
        held = result["scenarios"][0]["held"]
        self.assertAlmostEqual(held["median_reduction_percent"], 20)
        self.assertEqual(held["baseline_range_seconds"], [0.0009, 0.0011])
        self.assertAlmostEqual(held["candidate_realtime_percent"], 20)

    def test_metadata_mismatches_rejected(self):
        variants = {
            "compiler": "other-compiler", "configuration": "Debug", "lua_requested": "luajit",
            "has_lua": False, "lto_requested": False, "seed": 99, "warmup_blocks": 64,
            "timing": "different measurement", "patch": "different.fxp",
        }
        for key, value in variants.items():
            with self.subTest(key=key):
                self.candidate = deepcopy(self.baseline)
                self.candidate[key] = value
                self.reject()

    def test_unknown_or_missing_fields_rejected(self):
        self.candidate["unrecorded_option"] = True
        self.reject()
        self.candidate = deepcopy(self.baseline)
        del self.candidate["configuration"]
        self.reject()

    def test_false_success_or_determinism_rejected(self):
        self.candidate["ok"] = False
        self.reject()
        self.candidate = deepcopy(self.baseline)
        self.candidate["scenarios"][0]["deterministic"] = False
        self.reject()

    def test_repetition_count_mismatch_rejected(self):
        self.candidate["repetitions"] = 2
        self.reject()

    def test_scenario_set_missing_extra_and_duplicate_rejected(self):
        self.candidate["scenarios"] = []
        self.reject()
        self.candidate = deepcopy(self.baseline)
        extra = deepcopy(self.candidate["scenarios"][0])
        extra["name"] = "extra"
        self.candidate["scenarios"].append(extra)
        self.reject()
        extra["name"] = "init-1"
        self.reject()

    def test_scenario_order_is_irrelevant(self):
        extra = deepcopy(self.baseline["scenarios"][0])
        extra["name"] = "second"
        self.baseline["scenarios"].append(extra)
        self.candidate = deepcopy(self.baseline)
        self.candidate["scenarios"].reverse()
        self.assertTrue(compare.compare_reports(self.baseline, self.candidate)["ok"])

    def test_changed_note_count_rejected(self):
        self.candidate["scenarios"][0]["requested_notes"] = 16
        self.reject()

    def test_scenario_name_cannot_escape_pcm_directory(self):
        for name in ("../outside", "a/b", "a\\b", "", "."):
            with self.subTest(name=name):
                self.candidate["scenarios"][0]["name"] = name
                self.reject()

    def test_nonfinite_zero_negative_and_bool_timings_rejected(self):
        for value in (float("nan"), float("inf"), -1, 0, True, "0.1"):
            with self.subTest(value=value):
                self.candidate = deepcopy(self.baseline)
                self.candidate["scenarios"][0]["held"]["wall_seconds"][0] = value
                self.reject()

    def test_forged_timing_summary_rejected(self):
        for field in ("median_seconds", "min_seconds", "max_seconds", "median_realtime_percent"):
            with self.subTest(field=field):
                self.candidate = deepcopy(self.baseline)
                self.candidate["scenarios"][0]["held"][field] *= 0.5
                self.reject()

    def test_changed_duration_and_partial_blocks_rejected(self):
        self.candidate["frames"] += 1
        self.reject()
        self.candidate = deepcopy(self.baseline)
        self.candidate["scenarios"][0]["held"]["audio_seconds"] = 0.00401
        self.reject()

    def test_differing_hashes_within_or_across_reports_rejected(self):
        self.candidate["scenarios"][0]["runs"][1]["pcm_hash"] = "0000000000000000"
        self.reject()
        for run in self.candidate["scenarios"][0]["runs"]:
            run["pcm_hash"] = "0000000000000000"
        self.reject()

    def test_nonfinite_or_inconsistent_audio_statistics_rejected(self):
        self.candidate["scenarios"][0]["runs"][0]["rms"] = float("nan")
        self.reject()
        self.candidate["scenarios"][0]["runs"][0]["rms"] = 0.5
        self.reject()

    def test_bool_does_not_pass_integer_or_boolean_contracts(self):
        for key, value in (("schema", True), ("seed", True), ("has_lua", 1), ("ok", 1)):
            with self.subTest(key=key):
                self.candidate = deepcopy(self.baseline)
                self.candidate[key] = value
                self.reject()

    def test_threshold_optional_and_applies_to_tail(self):
        self.candidate["scenarios"][0]["tail"] = phase((0.001, 0.0012, 0.0008))
        self.assertTrue(compare.compare_reports(self.baseline, self.candidate)["ok"])
        gated = compare.compare_reports(self.baseline, self.candidate, max_regression_percent=10)
        self.assertFalse(gated["ok"])
        self.assertFalse(gated["scenarios"][0]["tail"]["regression_threshold_passed"])

    def test_invalid_threshold_rejected(self):
        for value in (float("nan"), float("inf"), -1):
            with self.subTest(value=value), self.assertRaises(compare.ComparisonError):
                compare.compare_reports(self.baseline, self.candidate, max_regression_percent=value)


class FileTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.base_dir = self.root / "base"
        self.new_dir = self.root / "new"
        self.base_dir.mkdir()
        self.new_dir.mkdir()
        self.base_pcm = self.base_dir / "init-1.f32"
        self.new_pcm = self.new_dir / "init-1.f32"
        self.base_pcm.write_bytes(PCM)
        self.new_pcm.write_bytes(PCM)

    def full_compare(self):
        return compare.compare_reports(fixture(), fixture(), baseline_pcm=self.base_dir,
                                       candidate_pcm=self.new_dir)

    def test_complete_pcm_comparison_binds_hash_and_length(self):
        result = self.full_compare()
        self.assertTrue(result["full_pcm_verified"])
        self.assertEqual(result["scenarios"][0]["pcm"]["bytes_per_file"], len(PCM))
        self.assertEqual(len(result["scenarios"][0]["pcm"]["sha256"]), 64)

    def test_one_pcm_directory_rejected(self):
        with self.assertRaises(compare.ComparisonError):
            compare.compare_reports(fixture(), fixture(), baseline_pcm=self.base_dir)

    def test_missing_truncated_or_changed_pcm_rejected(self):
        self.new_pcm.unlink()
        with self.assertRaises(compare.ComparisonError):
            self.full_compare()
        self.new_pcm.write_bytes(PCM[:-4])
        with self.assertRaises(compare.ComparisonError):
            self.full_compare()
        self.new_pcm.write_bytes(bytes(len(PCM)))
        with self.assertRaises(compare.ComparisonError):
            self.full_compare()

    def test_identically_replaced_pcm_still_must_match_report_hash(self):
        self.base_pcm.write_bytes(bytes(len(PCM)))
        self.new_pcm.write_bytes(bytes(len(PCM)))
        with self.assertRaises(compare.ComparisonError):
            self.full_compare()

    def test_nonfinite_pcm_rejected_even_with_matching_fingerprint(self):
        data = struct.pack("<f", float("nan")) + PCM[4:]
        self.base_pcm.write_bytes(data)
        self.new_pcm.write_bytes(data)
        report = fixture()
        for run in report["scenarios"][0]["runs"]:
            run["pcm_hash"] = fingerprint(data)
        with self.assertRaises(compare.ComparisonError):
            compare.compare_reports(report, report, baseline_pcm=self.base_dir, candidate_pcm=self.new_dir)

    def test_duplicate_json_fields_and_nan_rejected(self):
        report = self.root / "bad.json"
        for content in ('{"ok":true,"ok":false}', '{"value":NaN}'):
            with self.subTest(content=content):
                report.write_text(content, encoding="utf-8")
                with self.assertRaises(compare.ComparisonError):
                    compare.read_report(report)

    def test_cli_success_writes_json_and_preserves_inputs(self):
        before, after = self.root / "before.json", self.root / "after.json"
        for path in (before, after):
            path.write_text(json.dumps(fixture()), encoding="utf-8")
        output = self.root / "nested" / "comparison.json"
        stdout = io.StringIO()
        with redirect_stdout(stdout):
            status = compare.main([str(before), str(after), "--baseline-pcm", str(self.base_dir),
                                   "--candidate-pcm", str(self.new_dir), "--output", str(output)])
        self.assertEqual(status, 0)
        self.assertTrue(json.loads(output.read_text())["full_pcm_verified"])
        self.assertEqual(json.loads(stdout.getvalue()), json.loads(output.read_text()))
        original = before.read_bytes()
        with redirect_stderr(io.StringIO()), redirect_stdout(io.StringIO()):
            status = compare.main([str(before), str(after), "--output", str(before)])
        self.assertEqual(status, 2)
        self.assertEqual(before.read_bytes(), original)

    def test_cli_rejects_truncated_json(self):
        bad = self.root / "bad.json"
        bad.write_text('{"schema":', encoding="utf-8")
        with redirect_stderr(io.StringIO()), redirect_stdout(io.StringIO()):
            self.assertEqual(compare.main([str(bad), str(bad)]), 2)


if __name__ == "__main__":
    unittest.main()
