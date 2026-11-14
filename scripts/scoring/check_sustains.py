#!/usr/bin/env python3
"""Measure long held notes and complete modulation cycles in the native engine.

SPDX-License-Identifier: GPL-3.0-or-later

This supplement does not replace the 23-scenario bank audit. It checks two
prolonged stress conditions per preset, without normalizing or repairing PCM.
Its two-second tail is not a complete release-settlement test or a listening pass.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import csv
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

import numpy as np

import build_bank as compiler
import qa_bank as qa

ROOT = Path(__file__).resolve().parents[2]
CONFIG = {
    'schema': 1,
    'sample_rate': 48000,
    'tempo_bpm': 120,
    'seed': 1000,
    'master_reference_db': 0.,
    'minimum_hold_seconds': 12.,
    'noise_cycles': 4,
    'step_count': 16,
    'other_cycles': 2,
    'tail_seconds': 2.,
    'native_block_frames': 32,
    'native_max_seconds': 120.,
    'fresh_engine_per_scenario': True,
    'render_timeout_seconds': 600,
    'peak_limit': qa.CONFIG['peak_limit'],
    'headroom_warning_peak': .8,
    'dc_limit': qa.CONFIG['dc_limit'],
    'silence_peak_floor': qa.CONFIG['silence_peak_floor'],
    'scope': 'Saved-default dense polyphony and aggressive-macro chord holds; '
             'single-note equivalents for mono presets. A bounded cycle sample, '
             'not exhaustive random-state, host-transport or complete-tail coverage.',
    'measurement': 'Unnormalized stereo float32 at saved unity master; sample peaks '
                   'rather than reconstructed true peaks. Musical audition remains pending.',
}


def recipe_lookup():
    result = {}
    for path in sorted(compiler.RECIPES.glob('*.json')):
        category = json.loads(path.read_text())
        for family in category['families']:
            for variant in category['variants']:
                result[(category['id'], family['name'], variant['name'])] = compiler.vary(family, variant)
    return result


def fixtures(preset, expanded):
    motion = expanded['motion']
    shape, rate = motion['shape'], float(motion['rate'])
    if not math.isfinite(rate) or rate <= 0:
        raise ValueError('Invalid motion rate')
    cycle_goal = (CONFIG['noise_cycles'] if shape == 'noise' else
                  CONFIG['step_count'] if shape == 'step' else CONFIG['other_cycles'])
    ordinary = {s['key']: s for s in qa.fixtures(preset)}
    stress_key = 'mono_velocity_stress' if preset['mono'] else 'polyphony_stress'
    planned = []
    for key, original, aggressive in (
            ('sustain_default_stress', ordinary[stress_key], False),
            ('sustain_aggressive', ordinary['macros_aggressive'], True)):
        hz = rate * (4. if aggressive and not motion.get('sync', False) else 1.)
        minimum = max(CONFIG['minimum_hold_seconds'], cycle_goal / hz)
        # Include a complete final block beyond the nominal cycle boundary.
        block = CONFIG['native_block_frames'] / CONFIG['sample_rate']
        hold = (math.ceil(minimum / block) + 1) * block
        if hold + CONFIG['tail_seconds'] > CONFIG['native_max_seconds']:
            raise ValueError('Required complete cycle exceeds the renderer duration limit')
        spec = qa.scenario(key, original['notes'], hold, CONFIG['tail_seconds'],
                           velocity=127, tempo=120, macros=original['macros'], gate='expression')
        spec['cycle_coverage'] = {
            'shape': shape, 'recipe_rate_hz_at_120_bpm': rate,
            'synchronized': bool(motion.get('sync', False)),
            'effective_rate_hz': hz, 'minimum_phase_cycles_or_steps': cycle_goal,
            'nominal_phase_cycles_or_steps_held': spec['hold'] * hz,
            'interpretation': 'Step shape counts individual steps; other shapes count LFO cycles.',
        }
        planned.append(spec)
    return planned


def check_native_definition(path, expanded):
    xml = compiler.read_patch(path)
    params = {node.tag: node for node in xml.find('parameters')}
    motion = expanded['motion']
    if float(params['volume'].get('value')) != 0.:
        raise ValueError('The held-note audit requires a saved unity master reference')
    if int(params['a_lfo0_shape'].get('value')) != compiler.SHAPES[motion['shape']]:
        raise ValueError('Native LFO shape differs from the recipe expansion')
    stored_rate = float(np.float32(float(params['a_lfo0_rate'].get('value'))))
    native_hz = 2 ** stored_rate
    if not math.isclose(native_hz, motion['rate'], rel_tol=1e-6):
        raise ValueError('Native LFO rate differs from the recipe expansion')
    if int(params['a_lfo0_rate'].get('temposync', '0')) != int(motion.get('sync', False)):
        raise ValueError('Native LFO synchronization differs from the recipe expansion')
    return {'native_lfo_rate_hz_at_120_bpm': native_hz,
            'native_lfo_shape': int(params['a_lfo0_shape'].get('value')),
            'master_db': 0.}


def run_preset(preset, expanded, renderer, output, binding, keep_audio, force):
    ident = preset['id']
    if not re.fullmatch(r'SC\d{2}-\d{3}', ident):
        raise ValueError('Invalid preset ID')
    source = (ROOT / preset['path']).resolve()
    if not source.is_relative_to(ROOT):
        raise ValueError('Preset input escapes the repository')
    source_hash = qa.sha256(source)
    planned = fixtures(preset, expanded)
    expected_binding = {
        **binding, 'patch_sha256': source_hash,
        'preset_definition_sha256': qa.digest_json(preset),
        'expanded_recipe_sha256': qa.digest_json(expanded),
        'fixture_sha256': qa.digest_json(planned),
    }
    destination = output / 'presets' / f'{ident}.json'
    if destination.exists() and not force:
        prior = json.loads(destination.read_text())
        if prior.get('binding') == expected_binding and prior.get('status') == 'pass':
            return prior, True
    started = time.monotonic()
    result = {
        'schema': 1, 'id': ident, 'name': preset['name'],
        'category': preset['category'], 'category_id': preset['category_id'],
        'patch_path': preset['path'], 'binding': expected_binding,
        'created_utc': datetime.now(timezone.utc).isoformat(),
        'status': 'fail', 'scenarios': [], 'failures': [], 'warning_count': 0,
        'musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
        'release_scope': 'Only two seconds after note-off; complete release settlement is not tested.',
    }
    work = output / 'work' / ident
    work.mkdir(parents=True, exist_ok=True)
    try:
        if source_hash != preset['sha256']:
            raise ValueError('Preset SHA256 differs from the catalog')
        result['definition'] = check_native_definition(source, expanded)
        batch_rows = []
        for spec in planned:
            columns = [str(source), str(work / f"{spec['key']}.f32"),
                       str(spec['sample_rate']), str(spec['seconds']), str(spec['hold']),
                       ','.join(map(str, spec['notes'])), str(spec['velocity']), str(spec['tempo']),
                       spec['macros'], '0', '0', '127']
            if any('\t' in value or '\r' in value or '\n' in value for value in columns):
                raise ValueError('Native TSV field contains a control character')
            batch_rows.append('\t'.join(columns))
        batch = work / 'scenarios.tsv'
        batch.write_bytes(('\n'.join(batch_rows) + '\n').encode())
        native_report = work / 'native.jsonl'
        with (work / 'native.log').open('w') as log:
            process = subprocess.run([
                str(renderer), '--batch', str(batch), '--report', str(native_report),
                '--data', str(ROOT / 'resources/data'), '--seed', str(CONFIG['seed']),
                '--fresh-engine', '1',
            ], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                timeout=CONFIG['render_timeout_seconds'], check=False)
        result['native_exit_code'] = process.returncode
        if process.returncode:
            result['failures'].append(f'Native renderer returned {process.returncode}')
        native_rows = [json.loads(line) for line in native_report.read_text().splitlines() if line.strip()]
        if len(native_rows) != len(planned):
            raise ValueError('Native scenario count differs from the plan')
        if qa.sha256(source) != source_hash:
            raise ValueError('Preset changed during the native audit')
        for native, spec in zip(native_rows, planned):
            row = {'fixture': spec, 'native': native, 'failures': [], 'warnings': []}
            raw = work / f"{spec['key']}.f32"
            if Path(native.get('out', '')).resolve() != raw.resolve():
                row['failures'].append('Native output identity differs from the fixture')
            if native.get('name') != preset['name']:
                row['failures'].append('Native patch identity differs from the catalog')
            if native.get('notes') != ','.join(map(str, spec['notes'])):
                row['failures'].append('Native notes differ from the fixture')
            if raw.exists():
                audio, metrics = qa.measure(raw, spec)
                del audio
                row['metrics'] = metrics
                failures, warnings = qa.audit_scenario(native, spec, metrics)
                row['failures'].extend(failures)
                row['warnings'].extend(warnings)
                if native.get('frames') != metrics['frames']:
                    row['failures'].append('Native frame count differs from measured PCM')
                if abs(float(native.get('peak', 0.)) - metrics['peak']) > 1e-6:
                    row['failures'].append('Native peak differs from measured PCM')
                if not keep_audio and not row['failures']:
                    raw.unlink()
            else:
                row['failures'].append('Native PCM output is missing')
            result['scenarios'].append(row)
        result['failures'].extend(
            f"{row['fixture']['key']}: {failure}"
            for row in result['scenarios'] for failure in row['failures'])
        result['status'] = 'fail' if result['failures'] else 'pass'
    except Exception as exc:
        result['fatal_error'] = f'{type(exc).__name__}: {exc}'
        result['status'] = 'fail'
    finally:
        result['elapsed_seconds'] = round(time.monotonic() - started, 3)
        result['warning_count'] = sum(len(row['warnings']) for row in result['scenarios'])
        qa.write_json(destination, result)
        if result['status'] == 'pass' and not keep_audio:
            shutil.rmtree(work)
    return result, False


def summarize(results, output, binding, manifest_hash, selected_total):
    rows = [row for result in results for row in result['scenarios']]
    summary = {
        'schema': 1, 'binding': binding, 'manifest_sha256': manifest_hash, 'config': CONFIG,
        'tested_presets': len(results), 'selected_presets': selected_total,
        'complete_bank': len(results) == 1000,
        'passed': sum(result['status'] == 'pass' for result in results),
        'failed': sum(result['status'] != 'pass' for result in results),
        'native_scenarios': len(rows),
        'warnings': sum(result['warning_count'] for result in results),
        'presets_with_warnings': sum(result['warning_count'] > 0 for result in results),
        'max_sample_peak': max((row.get('metrics', {}).get('peak', 0.) for row in rows), default=0.),
        'nonfinite_samples': sum(row.get('metrics', {}).get('nonfinite_samples', 0) for row in rows),
        'rendered_audio_hours': sum(row.get('metrics', {}).get('frames', 0) /
                                    row['fixture']['sample_rate'] for row in rows) / 3600.,
        'hold_seconds_min': min((row['fixture']['hold'] for row in rows), default=0.),
        'hold_seconds_max': max((row['fixture']['hold'] for row in rows), default=0.),
        'failed_ids': [result['id'] for result in results if result['status'] != 'pass'],
        'warning_ids': [result['id'] for result in results if result['warning_count']],
        'musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
        'created_utc': datetime.now(timezone.utc).isoformat(),
    }
    qa.write_json(output / 'summary.json', summary)
    with (output / 'summary.csv').open('w', newline='') as f:
        fields = ['id', 'name', 'category', 'status', 'patch_sha256', 'warnings',
                  'default_stress_hold_seconds', 'default_stress_peak',
                  'aggressive_hold_seconds', 'aggressive_peak', 'musical_audition']
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for result in results:
            by_key = {row['fixture']['key']: row for row in result['scenarios']}
            ordinary = by_key.get('sustain_default_stress', {})
            aggressive = by_key.get('sustain_aggressive', {})
            writer.writerow({
                'id': result['id'], 'name': result['name'], 'category': result['category'],
                'status': result['status'], 'patch_sha256': result['binding']['patch_sha256'],
                'warnings': result['warning_count'],
                'default_stress_hold_seconds': ordinary.get('fixture', {}).get('hold', ''),
                'default_stress_peak': ordinary.get('metrics', {}).get('peak', ''),
                'aggressive_hold_seconds': aggressive.get('fixture', {}).get('hold', ''),
                'aggressive_peak': aggressive.get('metrics', {}).get('peak', ''),
                'musical_audition': 'pending',
            })
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--renderer', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--workers', type=int, default=6, help='Native processes, 1..6')
    parser.add_argument('--ids', help='Comma-separated exact IDs for targeted reruns')
    parser.add_argument('--keep-audio', action='store_true')
    parser.add_argument('--force', action='store_true')
    args = parser.parse_args()
    if not 1 <= args.workers <= 6:
        parser.error('--workers must be between 1 and 6')
    renderer, output = args.renderer.resolve(), args.output.resolve()
    if not renderer.is_file() or not os.access(renderer, os.X_OK):
        parser.error('--renderer must name an executable')
    output.mkdir(parents=True, exist_ok=True)
    manifest_path = ROOT / 'doc/scoring-1000/manifest.json'
    catalog = json.loads(manifest_path.read_text())
    presets = catalog['presets']
    if len(presets) != 1000 or len({p['id'] for p in presets}) != 1000:
        parser.error('A complete, uniquely identified 1000-preset catalog is required')
    if args.ids:
        requested = set(args.ids.split(','))
        missing = requested - {p['id'] for p in presets}
        if missing:
            parser.error('Unknown IDs: ' + ', '.join(sorted(missing)))
        presets = [p for p in presets if p['id'] in requested]
    recipes = recipe_lookup()
    expanded = {p['id']: recipes[(p['category_id'], p['family'], p['variant'])] for p in presets}
    binding = {
        'renderer_sha256': qa.sha256(renderer), 'script_sha256': qa.sha256(Path(__file__)),
        'qa_helper_sha256': qa.sha256(Path(qa.__file__)),
        'qa_helper_config_sha256': qa.digest_json(qa.CONFIG),
        'compiler_sha256': qa.sha256(Path(compiler.__file__)),
        'config_sha256': qa.digest_json(CONFIG), 'numpy_version': np.__version__,
    }
    manifest_hash = qa.sha256(manifest_path)
    planned = {p['id']: fixtures(p, expanded[p['id']]) for p in presets}
    qa.write_json(output / 'plan.json', {
        'manifest_sha256': manifest_hash, 'binding': binding, 'config': CONFIG,
        'preset_count': len(presets), 'scenarios': planned,
    })
    total_audio = sum(s['seconds'] for pair in planned.values() for s in pair)
    print(f'Planned {len(presets)} presets / {2 * len(presets)} scenarios / '
          f'{total_audio / 3600:.3f} hours of native audio', flush=True)
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        pending = {pool.submit(run_preset, preset, expanded[preset['id']], renderer,
                               output, binding, args.keep_audio, args.force): preset
                   for preset in presets}
        for future in concurrent.futures.as_completed(pending):
            preset = pending[future]
            try:
                result, reused = future.result()
            except Exception as exc:
                result = {'id': preset['id'], 'name': preset['name'],
                          'category': preset['category'], 'category_id': preset['category_id'],
                          'binding': {**binding, 'patch_sha256': preset['sha256']},
                          'status': 'fail', 'scenarios': [], 'warning_count': 0,
                          'fatal_error': f'{type(exc).__name__}: {exc}',
                          'musical_audition': 'pending', 'composer_endorsement': 'not_claimed'}
                qa.write_json(output / 'presets' / f"{preset['id']}.json", result)
                reused = False
            results.append(result)
            peaks = [row.get('metrics', {}).get('peak', 0.) for row in result['scenarios']]
            print(f"{len(results)}/{len(presets)} {result['id']} {result['status']} "
                  f"warnings={result['warning_count']} peak={max(peaks, default=0.):.6g} "
                  f"{'cached' if reused else 'rendered'}", flush=True)
    results.sort(key=lambda result: result['id'])
    summary = summarize(results, output, binding, manifest_hash, len(presets))
    print(json.dumps({key: summary[key] for key in (
        'tested_presets', 'passed', 'failed', 'native_scenarios', 'warnings', 'max_sample_peak')},
        indent=2), flush=True)
    return 1 if summary['failed'] else 0


if __name__ == '__main__':
    sys.exit(main())
