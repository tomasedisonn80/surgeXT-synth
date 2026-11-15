"""Regression evidence contracts. All fixture audio is synthetic, not a native result."""
from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest

import compare_regression as comparator


PCM = struct.pack("<4f", 0.25, -0.25, 0.5, -0.5)


def fingerprint(data):
    value = comparator.FNV_OFFSET
    for byte in data:
        value = ((value ^ byte) * comparator.FNV_PRIME) & comparator.UINT64_MASK
    return f"{value:016x}"


def fixture(name="conditioner-44100", frames=2, data=PCM):
    return {"name": name, "file": name + ".f32", "frames": frames,
            "sample_rates": [44100, 48000, 96000], "peak": 0.5,
            "pcm_hash": fingerprint(data), "rate_change_blocks": [512, 768],
            "same_rate_reset_blocks": [640],
            "outside_callback_init_blocks": [] if name.startswith("lowcut-") else [448],
            "direct_storage_rate_reset_blocks": [704],
            "standard_tuning_table_reset_blocks": [832],
            "processing_rounding_ranges": [[944, 976], [976, 1008]]}


def manifest():
    return {"schema": 1, "tool": "surge-dsp-regression", "seed": 1801,
            "block_size": 1, "blocks": 1536, "release_block": 1024,
            "fpu_guard": True, "effect_init_rounding": "synthetic init policy",
            "table_reset_rounding": "synthetic table policy",
            "processing_rounding": "synthetic processing policy",
            "format": "f32le-interleaved", "channels": 2,
            "hash_algorithm": "fnv1a64-f32le", "ok": True,
            "fixtures": [fixture(name, 1536, PCM * 768)
                         for name in sorted(comparator.fixture_names())]}


class ManifestTests(unittest.TestCase):
    def reject(self, report):
        with self.assertRaises(comparator.ComparisonError):
            comparator.validate_manifest(report, "synthetic")

    def test_complete_known_suite_validates(self):
        self.assertEqual(len(comparator.validate_manifest(manifest(), "synthetic")), 63)

    def test_unknown_and_missing_fields_fail(self):
        for mutate in (lambda r: r.update(extra=True),
                       lambda r: r.pop("processing_rounding"),
                       lambda r: r["fixtures"][0].pop("processing_rounding_ranges")):
            value = manifest()
            mutate(value)
            self.reject(value)

    def test_missing_duplicate_or_substituted_fixture_fails(self):
        value = manifest()
        value["fixtures"].pop()
        self.reject(value)
        value = manifest()
        value["fixtures"][1] = deepcopy(value["fixtures"][0])
        self.reject(value)
        value = manifest()
        value["fixtures"][0].update(name="unknown", file="unknown.f32")
        self.reject(value)

    def test_path_traversal_and_mismatched_filenames_fail(self):
        for name, filename in (("../escape", "../escape.f32"),
                               ("safe", "../escape.f32"), ("a/b", "a/b.f32"),
                               ("a\\b", "a\\b.f32")):
            value = manifest()
            value["fixtures"][0].update(name=name, file=filename)
            self.reject(value)

    def test_invalid_frame_counts_and_boolean_integers_fail(self):
        for frames in (True, 0, -1, 1535, 1537, 1536.0):
            value = manifest()
            value["fixtures"][0]["frames"] = frames
            self.reject(value)
        for field in ("schema", "block_size", "seed", "channels"):
            value = manifest()
            value[field] = True
            self.reject(value)

    def test_nonfinite_silent_and_invalid_fingerprints_fail(self):
        for field, item in (("peak", float("nan")), ("peak", float("inf")),
                            ("peak", 0), ("pcm_hash", "a"), ("pcm_hash", 1)):
            value = manifest()
            value["fixtures"][0][field] = item
            self.reject(value)

    def test_invalid_event_order_ranges_and_rate_sets_fail(self):
        for field, events in (("rate_change_blocks", [768, 512]),
                              ("rate_change_blocks", [512, 512]),
                              ("same_rate_reset_blocks", [1536]),
                              ("same_rate_reset_blocks", [True]),
                              ("sample_rates", [44100, 44100, 44100]),
                              ("processing_rounding_ranges", [[976, 944], [976, 1008]]),
                              ("processing_rounding_ranges", [[944, 976], [960, 1008]])):
            value = manifest()
            value["fixtures"][0][field] = events
            self.reject(value)


class PCMTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.base, self.new = self.root / "baseline", self.root / "candidate"
        self.base.mkdir()
        self.new.mkdir()
        self.before, self.after = fixture(), fixture()
        for directory in (self.base, self.new):
            (directory / self.before["file"]).write_bytes(PCM)

    def compare(self):
        return comparator.verify_fixture(self.base, self.new, self.before, self.after)

    def test_full_bytes_fingerprint_peak_and_sha_verified(self):
        result = self.compare()
        self.assertTrue(result["identical"])
        self.assertEqual(result["bytes_per_file"], len(PCM))
        self.assertEqual(result["sha256"], hashlib.sha256(PCM).hexdigest())

    def test_missing_and_truncated_files_fail(self):
        path = self.new / self.after["file"]
        path.unlink()
        with self.assertRaises(comparator.ComparisonError):
            self.compare()
        path.write_bytes(PCM[:-1])
        with self.assertRaises(comparator.ComparisonError):
            self.compare()

    def test_same_fingerprint_metadata_does_not_replace_byte_comparison(self):
        (self.new / self.after["file"]).write_bytes(PCM[::-1])
        with self.assertRaisesRegex(comparator.ComparisonError, "PCM bytes differ"):
            self.compare()

    def test_identically_replaced_bytes_must_match_report_hash(self):
        for directory in (self.base, self.new):
            (directory / self.before["file"]).write_bytes(bytes(len(PCM)))
        with self.assertRaisesRegex(comparator.ComparisonError, "fingerprint"):
            self.compare()

    def test_each_reported_peak_and_hash_is_checked(self):
        self.after["peak"] = 0.25
        with self.assertRaisesRegex(comparator.ComparisonError, "candidate peak"):
            self.compare()
        self.after = fixture()
        self.after["pcm_hash"] = "0" * 16
        with self.assertRaisesRegex(comparator.ComparisonError, "candidate fingerprint"):
            self.compare()

    def test_nonfinite_pcm_rejected_even_with_matching_hash(self):
        for nonfinite in (float("nan"), float("inf")):
            data = struct.pack("<f", nonfinite) + PCM[4:]
            for directory in (self.base, self.new):
                (directory / self.before["file"]).write_bytes(data)
            self.before["pcm_hash"] = self.after["pcm_hash"] = fingerprint(data)
            with self.assertRaisesRegex(comparator.ComparisonError, "nonfinite PCM"):
                self.compare()

    def test_external_symlink_rejected(self):
        path = self.new / self.after["file"]
        path.unlink()
        path.symlink_to(self.base / self.before["file"])
        with self.assertRaisesRegex(comparator.ComparisonError, "outside"):
            self.compare()

    def test_output_aliases_cannot_overwrite_pcm(self):
        path = self.base / self.before["file"]
        for output in (path, self.root / "alias"):
            if output != path:
                os.link(path, output)
            with self.assertRaisesRegex(comparator.ComparisonError, "overwrite"):
                comparator.validate_output(output, (self.base, self.new))
        self.assertEqual(path.read_bytes(), PCM)


class IntegrationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.base, self.new = self.root / "baseline", self.root / "candidate"
        self.report = manifest()
        for directory in (self.base, self.new):
            directory.mkdir()
            (directory / "manifest.json").write_text(json.dumps(self.report))
            for entry in self.report["fixtures"]:
                (directory / entry["file"]).write_bytes(PCM * 768)

    def test_cli_writes_complete_comparison_and_preserves_inputs(self):
        output = self.root / "result.json"
        before = (self.base / "manifest.json").read_bytes()
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            status = comparator.main([str(self.base), str(self.new), "--output", str(output)])
        self.assertEqual(status, 0)
        result = json.loads(output.read_text())
        self.assertEqual(result["fixture_count"], 63)
        self.assertTrue(result["full_pcm_verified"])
        self.assertEqual(result["bytes_compared_per_side"], 63 * 1536 * 8)
        self.assertEqual((self.base / "manifest.json").read_bytes(), before)

    def test_mismatching_root_and_fixture_metadata_fail(self):
        value = deepcopy(self.report)
        value["processing_rounding"] = "different"
        (self.new / "manifest.json").write_text(json.dumps(value))
        with self.assertRaisesRegex(comparator.ComparisonError, "root metadata"):
            comparator.compare_directories(self.base, self.new)
        value = deepcopy(self.report)
        value["fixtures"][0]["rate_change_blocks"] = [513, 769]
        (self.new / "manifest.json").write_text(json.dumps(value))
        with self.assertRaisesRegex(comparator.ComparisonError, "fixture metadata"):
            comparator.compare_directories(self.base, self.new)

    def test_duplicate_json_fields_and_truncated_json_fail(self):
        for text in ('{"schema":1,"schema":1}', '{"schema":'):
            (self.new / "manifest.json").write_text(text)
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                self.assertEqual(comparator.main([str(self.base), str(self.new)]), 2)

    def test_extra_pcm_file_and_same_directory_fail(self):
        (self.new / "unrecorded.f32").write_bytes(PCM)
        with self.assertRaisesRegex(comparator.ComparisonError, "unexpected PCM"):
            comparator.compare_directories(self.base, self.new)
        with self.assertRaisesRegex(comparator.ComparisonError, "distinct"):
            comparator.compare_directories(self.base, self.base)


if __name__ == "__main__":
    unittest.main()
