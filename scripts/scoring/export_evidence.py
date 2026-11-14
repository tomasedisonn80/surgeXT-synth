#!/usr/bin/env python3
"""Audit and archive complete, current Scoring 1000 technical evidence.

SPDX-License-Identifier: GPL-3.0-or-later

No audio is rendered or normalized here. Original JSON/CSV bytes are preserved.
The exporter refuses incomplete, stale, or internally inconsistent evidence.
Warnings remain visible; an export is not human musical or composer approval.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile

import numpy as np

import qa_bank as qa
import check_sustains as sustains


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'doc/scoring-1000/manifest.json'
GATES = ('1_native_definition', '2_default_audio', '3_performance',
         '4_expression_stress', '5_evidence_fingerprint')
TIMESTAMP = (1980, 1, 1, 0, 0, 0)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def encode(value):
    return (json.dumps(value, indent=2, ensure_ascii=True, allow_nan=False) + '\n').encode()


def close(a, b):
    return math.isfinite(float(a)) and math.isfinite(float(b)) and math.isclose(
        float(a), float(b), rel_tol=1e-9, abs_tol=1e-10)


class Inputs:
    """Remember exact input bytes and reject edits during collection/export."""

    def __init__(self):
        self.hashes = {}

    def read(self, path):
        path = Path(path).resolve()
        data = path.read_bytes()
        current = digest(data)
        require(self.hashes.get(path, current) == current, f'Input changed: {path}')
        self.hashes[path] = current
        return data

    def json(self, path):
        return json.loads(self.read(path), parse_constant=lambda value: self.invalid(value))

    @staticmethod
    def invalid(value):
        raise ValueError(f'Nonfinite JSON value: {value}')

    def hash(self, path):
        return digest(self.read(path))

    def check(self):
        for path, expected in self.hashes.items():
            require(qa.sha256(path) == expected, f'Input changed: {path}')


def validate_native(row, preset, expected_fixture):
    require(row['fixture'] == expected_fixture, f"{preset['id']}: fixture differs")
    native, metrics = row['native'], row['metrics']
    require(native.get('ok') and not row['failures'], f"{preset['id']}: failed native row")
    require(native['name'] == preset['name'], f"{preset['id']}: native name differs")
    require(Path(native['patch']).name == Path(preset['path']).name,
            f"{preset['id']}: native patch filename differs")
    require(native['notes'] == ','.join(map(str, expected_fixture['notes'])),
            f"{preset['id']}: native notes differ")
    for key in ('sample_rate', 'velocity', 'tempo', 'macros', 'modwheel', 'pressure', 'expression'):
        require(native[key] == expected_fixture[key], f"{preset['id']}: native {key} differs")
    for key in ('hold', 'seconds'):
        require(close(native[key], expected_fixture[key]), f"{preset['id']}: native {key} differs")
    require(native['channels'] == 2 and native['format'] == 'f32le-interleaved',
            f"{preset['id']}: unexpected audio format")
    require(native['seed'] == qa.CONFIG['seed'] and native['restores_global_gain'],
            f"{preset['id']}: wrong random seed or gain reference")
    require(native['frames'] == metrics['frames'] and metrics['frames'] > 0,
            f"{preset['id']}: native and Python frame counts differ")
    require(native['nonfinite'] == metrics['nonfinite_samples'] == 0,
            f"{preset['id']}: nonfinite samples")
    for key in ('peak', 'rms'):
        require(close(native[key], metrics[key]),
                f"{preset['id']}: native and Python {key} differ")
    require(close(native['mean'], sum(metrics['channel_dc']) / 2),
            f"{preset['id']}: native and Python DC means differ")
    for key in ('pcm_sha256', 'coarse_fingerprint'):
        require(re.fullmatch(r'[0-9a-f]{64}', metrics[key]) is not None,
                f"{preset['id']}: invalid {key}")
    failures, warnings = qa.audit_scenario(native, expected_fixture, metrics)
    require(not failures, f"{preset['id']}: measured technical failure: {failures}")
    require(all(warning in row['warnings'] for warning in warnings),
            f"{preset['id']}: measured warning was suppressed")


def audit_reports(directory, presets, binding, inputs, sustained=False):
    directory = directory.resolve()
    expected_ids = {p['id'] for p in presets}
    paths = {p.stem: p for p in (directory / 'presets').glob('*.json')}
    require(set(paths) == expected_ids, f'{directory}: incomplete or extra preset reports')
    summary = inputs.json(directory / 'summary.json')
    require(summary['binding'] == binding, f'{directory}: stale aggregate binding')
    require(summary['config'] == (sustains.CONFIG if sustained else qa.CONFIG),
            f'{directory}: aggregate configuration differs')
    require(summary.get('musical_audition') == 'pending' and
            summary.get('composer_endorsement') == 'not_claimed',
            f'{directory}: unsupported audition or endorsement claim')
    csv_rows = list(csv.DictReader(io.StringIO(inputs.read(directory / 'summary.csv').decode())))
    require(len(csv_rows) == 1000 and {r['id'] for r in csv_rows} == expected_ids,
            f'{directory}: incomplete or duplicate CSV rows')
    csv_by_id = {r['id']: r for r in csv_rows}
    recipes = sustains.recipe_lookup() if sustained else None
    counts = Counter()
    stats = {'max_sample_peak': 0., 'rendered_audio_hours': 0.,
             'native_peak_discrepancy_max': 0., 'native_rms_discrepancy_max': 0.,
             'hold_seconds_min': math.inf, 'hold_seconds_max': 0.}
    index, duplicate_groups, engines, git_ids = [], defaultdict(list), set(), set()
    archive = {}
    warning_ids = []
    for preset in presets:
        ident = preset['id']
        path = paths[ident]
        result = inputs.json(path)
        planned = qa.fixtures(preset)
        expected = {**binding, 'patch_sha256': preset['sha256'],
                    'preset_definition_sha256': qa.digest_json(preset)}
        if sustained:
            expanded = recipes[(preset['category_id'], preset['family'], preset['variant'])]
            planned = sustains.fixtures(preset, expanded)
            expected['expanded_recipe_sha256'] = qa.digest_json(expanded)
        expected['fixture_sha256'] = qa.digest_json(planned)
        require(result['binding'] == expected, f'{ident}: stale report binding')
        require(result['id'] == ident and result['patch_path'] == preset['path'] and
                result['name'] == preset['name'] and result['category'] == preset['category'],
                f'{ident}: report identity differs')
        require(result['status'] == 'pass' and result['native_exit_code'] == 0,
                f'{ident}: export requires successful native evidence')
        require(result.get('musical_audition') == 'pending' and
                result.get('composer_endorsement') == 'not_claimed',
                f'{ident}: unsupported audition or endorsement claim')
        rows = result['scenarios']
        require(len(rows) == len(planned), f'{ident}: incomplete scenarios')
        for row, fixture in zip(rows, planned):
            validate_native(row, preset, fixture)
            native, metrics = row['native'], row['metrics']
            engines.add(native['engine'])
            git_ids.add(native['git'])
            counts['native_scenarios'] += 1
            stats['max_sample_peak'] = max(stats['max_sample_peak'], metrics['peak'])
            stats['rendered_audio_hours'] += metrics['frames'] / fixture['sample_rate'] / 3600.
            stats['hold_seconds_min'] = min(stats['hold_seconds_min'], fixture['hold'])
            stats['hold_seconds_max'] = max(stats['hold_seconds_max'], fixture['hold'])
            for key in ('peak', 'rms'):
                name = f'native_{key}_discrepancy_max'
                stats[name] = max(stats[name], abs(native[key] - metrics[key]))
        warning_count = len(result.get('warnings', [])) + sum(len(r['warnings']) for r in rows)
        require(result['warning_count'] == warning_count, f'{ident}: wrong warning count')
        counts['warning_count'] += warning_count
        counts['presets_with_warnings'] += bool(warning_count)
        if warning_count:
            warning_ids.append(ident)
        if not sustained:
            require(tuple(result['gates']) == GATES and all(
                gate['status'] == 'pass' and not gate['failures']
                for gate in result['gates'].values()), f'{ident}: incomplete or failed five gates')
            counts['technical_gates'] += len(GATES)
            by_key = {r['fixture']['key']: r for r in rows}
            original_hash = by_key['default']['metrics']['pcm_sha256']
            repeated_hash = by_key['default_repeat']['metrics']['pcm_sha256']
            identical = original_hash == repeated_hash
            require(result['default_repeat']['bit_identical'] == identical,
                    f'{ident}: repeat identity claim differs')
            counts['bit_identical_repeats'] += identical
            duplicate_groups[original_hash].append(ident)
            require(list(result['macro_response']) == preset['macros'], f'{ident}: missing macros')
            for response in result['macro_response'].values():
                resolved = (response['waveform_relative_rms_delta'] > response['waveform_threshold'] or
                            response['feature_distance'] > response['feature_threshold'])
                require((response['status'] == 'measurable_change') == resolved,
                        f'{ident}: macro status contradicts its measurement')
                counts['measurable_macros'] += resolved
                counts['macro_checks'] += 1
        csv_row = csv_by_id[ident]
        require(csv_row['patch_sha256'] == preset['sha256'] and csv_row['status'] == 'pass' and
                int(csv_row['warnings']) == warning_count and csv_row['name'] == preset['name'] and
                csv_row['musical_audition'] == 'pending', f'{ident}: CSV differs from report')
        if not sustained:
            require(int(csv_row['scenario_count']) == len(rows) and
                    close(csv_row['max_scenario_peak'], max(r['metrics']['peak'] for r in rows)),
                    f'{ident}: CSV scenario aggregates differ')
        index.append({'id': ident, 'report_sha256': inputs.hashes[path.resolve()],
                      'patch_sha256': preset['sha256']})
        archive[f'presets/{ident}.json'] = path
        counts['tested_presets'] += 1
    require(len(engines) == len(git_ids) == 1, f'{directory}: mixed native engine identities')
    for key in ('tested_presets', 'native_scenarios', 'presets_with_warnings'):
        require(summary[key] == counts[key], f'{directory}: summary {key} differs')
    require(summary['passed'] == 1000 and summary['failed'] == 0 and not summary['failed_ids'],
            f'{directory}: summary pass count differs')
    warning_key = 'warnings' if sustained else 'warning_count'
    require(summary[warning_key] == counts['warning_count'], f'{directory}: summary warnings differ')
    duplicates = [ids for ids in duplicate_groups.values() if len(ids) > 1]
    if sustained:
        require(summary['complete_bank'] and summary['selected_presets'] == 1000 and
                summary['warning_ids'] == warning_ids, f'{directory}: sustain summary differs')
        for key in ('max_sample_peak', 'rendered_audio_hours', 'hold_seconds_min', 'hold_seconds_max'):
            require(close(summary[key], stats[key]), f'{directory}: summary {key} differs')
    else:
        require(summary['exact_default_audio_duplicate_groups'] == duplicates,
                f'{directory}: exact duplicate groups differ')
    totals = {**dict(counts), **stats, 'native_engine_identifiers': sorted(engines),
              'native_git_identifiers': sorted(git_ids), 'nonfinite_samples': 0,
              'warning_ids': warning_ids}
    if not sustained:
        totals['exact_default_audio_duplicate_groups'] = duplicates
    return totals, index, archive


def write_archive(path, entries, inputs):
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, source in sorted(entries.items()):
            info = zipfile.ZipInfo(name, TIMESTAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, inputs.read(source), compresslevel=9)
    with zipfile.ZipFile(path) as archive:
        require(len(archive.namelist()) == len(entries) and archive.testzip() is None,
                'Archive cardinality or CRC failed')
        for name, source in entries.items():
            require(digest(archive.read(name)) == inputs.hashes[source.resolve()],
                    f'Archive bytes differ: {name}')


def export(args):
    inputs, payload, archives = Inputs(), {}, {}
    source_paths = [Path(__file__), Path(qa.__file__), Path(sustains.__file__),
                    ROOT / 'scripts/scoring/build_bank.py', ROOT / 'scripts/scoring/gain_trims.json',
                    ROOT / 'scripts/scoring/validate_bank.py', ROOT / 'scripts/scoring/check_roundtrip.py',
                    ROOT / 'src/scoring-tools/ScoringRender.cpp', ROOT / 'src/scoring-tools/CMakeLists.txt',
                    ROOT / 'doc/scoring-1000/package.sha256', MANIFEST]
    source_paths.extend(sorted((ROOT / 'scripts/scoring/recipes').glob('*.json')))
    source_hashes = {str(p.resolve().relative_to(ROOT)): inputs.hash(p) for p in source_paths}
    manifest = inputs.json(MANIFEST)
    manifest_hash = inputs.hashes[MANIFEST]
    presets = sorted(manifest['presets'], key=lambda preset: preset['id'])
    require(len(presets) == 1000 and len({p['id'] for p in presets}) == 1000,
            'A complete, uniquely identified 1000-preset catalog is required')
    require(Counter(p['category_id'] for p in presets) == {f'{i:02}': 100 for i in range(1, 11)},
            'Catalog must contain ten categories of 100')
    for preset in presets:
        require(re.fullmatch(r'SC\d{2}-\d{3}', preset['id']) is not None, 'Invalid preset ID')
        path = (ROOT / preset['path']).resolve()
        require(path.is_relative_to(ROOT), 'Preset path escapes repository')
        require(inputs.hash(path) == preset['sha256'], f"{preset['id']}: current FXP hash differs")
    renderer_hash = inputs.hash(args.renderer)
    binding = {'renderer_sha256': renderer_hash, 'qa_script_sha256': inputs.hash(Path(qa.__file__)),
               'config_sha256': qa.digest_json(qa.CONFIG), 'numpy_version': np.__version__}
    primary, report_index, archives['native-preset-reports.zip'] = audit_reports(
        args.qa_dir, presets, binding, inputs)
    payload['native-summary.json'] = inputs.read(args.qa_dir / 'summary.json')
    payload['native-summary.csv'] = inputs.read(args.qa_dir / 'summary.csv')
    payload['report-index.json'] = encode({'schema': 1, 'manifest_sha256': manifest_hash,
                                         'reports': report_index})
    static = inputs.json(args.static_report)
    require(static['ok'] and static['complete_bank'] and static['regeneration_checked'] and
            static['manifest_sha256'] == manifest_hash and not static['errors'],
            'Static regeneration report is incomplete, stale, or failed')
    require({r['id']: r['sha256'] for r in static['presets']} ==
            {p['id']: p['sha256'] for p in presets} and len(static['presets']) == 1000 and
            all(r['status'] == 'pass' for r in static['presets']),
            'Static preset evidence differs')
    payload['static-validation.json'] = inputs.read(args.static_report)
    audits = {'primary': primary}
    if args.sustain_dir:
        sustain_binding = {
            'renderer_sha256': renderer_hash, 'script_sha256': inputs.hash(Path(sustains.__file__)),
            'qa_helper_sha256': binding['qa_script_sha256'],
            'qa_helper_config_sha256': binding['config_sha256'],
            'compiler_sha256': inputs.hash(Path(sustains.compiler.__file__)),
            'config_sha256': qa.digest_json(sustains.CONFIG), 'numpy_version': np.__version__}
        summary = inputs.json(args.sustain_dir / 'summary.json')
        require(summary['manifest_sha256'] == manifest_hash, 'Stale sustain aggregate catalog')
        audits['sustain'], index, archives['sustain-preset-reports.zip'] = audit_reports(
            args.sustain_dir, presets, sustain_binding, inputs, sustained=True)
        for extension in ('json', 'csv'):
            payload[f'sustain-summary.{extension}'] = inputs.read(args.sustain_dir / f'summary.{extension}')
        payload['sustain-report-index.json'] = encode(
            {'schema': 1, 'manifest_sha256': manifest_hash, 'reports': index})
    if args.roundtrip_report:
        roundtrip = inputs.json(args.roundtrip_report)
        require(roundtrip['catalog_sha256'] == manifest_hash and
                roundtrip['renderer_sha256'] == renderer_hash and
                roundtrip['audit_script_sha256'] == inputs.hash(ROOT / 'scripts/scoring/check_roundtrip.py'),
                'Stale native semantic roundtrip evidence')
        records = roundtrip['records']
        require(len(records) == 1000 and {r['id']: r['source_sha256'] for r in records} ==
                {p['id']: p['sha256'] for p in presets} and all(
                    r['status'] == 'pass' and not r['errors'] for r in records),
                'Incomplete or failed native semantic roundtrip evidence')
        require(roundtrip['status'] == 'pass' and roundtrip['inputs_stable_through_run'] and
                roundtrip['semantic_passes'] == roundtrip['presets_checked'] == 1000,
                'Native semantic roundtrip aggregate differs')
        audio = roundtrip['audio']
        require(len(audio) == roundtrip['audio_pairs'] == roundtrip['audio_bit_identical'] and all(
            r['bit_identical'] and r['source_audio_sha256'] == r['saved_audio_sha256'] for r in audio),
                'Native saved/original audio equality claim differs')
        payload['roundtrip-summary.json'] = inputs.read(args.roundtrip_report)
        audits['roundtrip'] = {'semantic_passes': len(records), 'bit_identical_audio_pairs': len(audio)}
    if args.staging_evidence:
        staging = inputs.json(args.staging_evidence)
        require(staging['catalog_sha256'] == manifest_hash and staging['status'] == 'pass' and
                staging['package_index_sha256'] == inputs.hash(ROOT / 'doc/scoring-1000/package.sha256'),
                'Stale or failed factory staging evidence')
        require(staging['renderer']['sha256_before'] == staging['renderer']['sha256_after'] == renderer_hash,
                'Factory staging renderer differs')
        for path, expected in staging['implementation_sha256'].items():
            require(inputs.hash(ROOT / path) == expected, f'Stale factory staging implementation: {path}')
        require(staging['staging']['staged_hash_matches'] == 1000 and
                staging['native_discovery']['unique_bank_paths'] == 1000 and
                staging['native_discovery']['missing'] == staging['native_discovery']['unexpected'] == 0,
                'Incomplete factory staging/discovery evidence')
        payload['factory-staging.json'] = inputs.read(args.staging_evidence)
        audits['factory_staging'] = {'staged_hash_matches': 1000, 'native_unique_bank_paths': 1000}
    extras = []
    for item in args.extra:
        name, separator, source = item.partition('=')
        require(separator and re.fullmatch(r'[A-Za-z0-9_-]+\.(json|csv|md|zip)', name) is not None,
                '--extra requires a simple destination filename followed by =source-path')
        require(name not in payload and name not in archives and name not in
                ('provenance.json', 'integrity-audit.json', 'checksums.json', 'README.md'),
                f'Duplicate or reserved export filename: {name}')
        payload[name] = inputs.read(Path(source))
        extras.append({'file': name, 'sha256': digest(payload[name]),
                       'scope': 'Supplementary source evidence; historical claims are not relabeled current.'})
    short_git = primary['native_git_identifiers'][0]
    base_commit = subprocess.check_output(
        ['git', 'rev-parse', '--verify', f'{short_git}^{{commit}}'], cwd=ROOT, text=True).strip()
    payload['provenance.json'] = encode({
        'schema': 2, 'manifest_sha256': manifest_hash, 'engine_base_commit': base_commit,
        'native_engine_identifier': primary['native_engine_identifiers'][0],
        'scope': 'Actual common DSP through the offline native renderer. The embedded engine base '
                 'identifier is not the later feature commit; executable and source hashes bind this evidence.',
        'renderer_sha256': renderer_hash, 'source_sha256': source_hashes,
        'master_reference_db': 0, 'normalization': 'none',
        'human_musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
        'supplementary_source_evidence': extras})
    payload['integrity-audit.json'] = encode({
        'schema': 1, 'manifest_sha256': manifest_hash, 'renderer_sha256': renderer_hash,
        'exporter_sha256': source_hashes['scripts/scoring/export_evidence.py'],
        'status': 'pass', 'audits': audits, 'musical_audition': 'pending',
        'composer_endorsement': 'not_claimed',
        'limits': ['This checks evidence identity and consistency; it does not rerender audio.',
                   'Peak values are unnormalized sample peaks, not reconstructed true peaks.',
                   'Five primary gates are technical checks, not five human listening passes.',
                   'Different definitions or audio hashes do not prove perceptual uniqueness.',
                   'The sustain supplement has a two-second tail and does not test complete release settlement.']})
    payload['README.md'] = (
        '# Scoring 1000 technical evidence\n\n'
        'The report ZIPs preserve the exact original per-preset JSON bytes. Each report index records '
        'its report SHA-256 and corresponding FXP SHA-256. `checksums.json` identifies all files in '
        'this export. `integrity-audit.json` recomputes totals and cross-checks native and Python '
        'measurements against the current catalog and fixtures. `provenance.json` binds the renderer, '
        'source files, recipes, gain adjustments and catalog. Raw JSON/CSV summaries are unchanged.\n\n'
        'The five primary gates cover native definition, default audio, performance, expression and '
        'stress, and evidence completeness. Optional sustained-note, semantic roundtrip, and factory '
        'staging evidence retain their own scopes. Historical correction records are supplementary '
        'source evidence; their earlier catalogs remain labeled as such.\n\n'
        'Human musical audition remains pending, and composer endorsement is not claimed. These '
        'measurements do not establish universal genre suitability, perceptual uniqueness, reconstructed '
        'true-peak safety, complete release settling at every macro endpoint, or DAW-host behavior.\n'
    ).encode()
    inputs.check()
    if args.verify_only:
        return {'manifest_sha256': manifest_hash, 'audits': audits, 'written': False}
    output = args.output.resolve()
    require(not output.is_relative_to(ROOT / 'resources/data'), 'Do not export into factory content')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='scoring-evidence-', dir=output.parent) as staging_dir:
        staging = Path(staging_dir)
        for name, entries in archives.items():
            write_archive(staging / name, entries, inputs)
        for name, data in payload.items():
            (staging / name).write_bytes(data)
        files = {p.name: {'sha256': qa.sha256(p), 'bytes': p.stat().st_size}
                 for p in sorted(staging.iterdir())}
        (staging / 'checksums.json').write_bytes(encode({
            'schema': 1, 'manifest_sha256': manifest_hash, 'files': files}))
        inputs.check()
        output.mkdir(parents=True, exist_ok=True)
        for path in staging.iterdir():
            os.replace(path, output / path.name)
    return {'manifest_sha256': manifest_hash, 'audits': audits,
            'written': True, 'output': str(output), 'files': files}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qa-dir', required=True, type=Path)
    parser.add_argument('--renderer', required=True, type=Path)
    parser.add_argument('--static-report', required=True, type=Path)
    parser.add_argument('--sustain-dir', type=Path)
    parser.add_argument('--roundtrip-report', type=Path)
    parser.add_argument('--staging-evidence', type=Path)
    parser.add_argument('--extra', action='append', default=[], metavar='NAME=PATH',
                        help='Preserve separate historical or supplementary evidence bytes')
    parser.add_argument('--output', type=Path, default=ROOT / 'doc/scoring-1000/evidence')
    parser.add_argument('--verify-only', action='store_true', help='Validate without writing files')
    args = parser.parse_args()
    try:
        result = export(args)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as exc:
        print(f'Evidence export refused: {exc}', file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    sys.exit(main())
