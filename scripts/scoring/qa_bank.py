#!/usr/bin/env python3
"""Render and measure every Scoring 1000 patch with the native Surge engine.

SPDX-License-Identifier: GPL-3.0-or-later

This is technical evidence, not a listening panel, a musical-quality score, or
composer endorsement. It never normalizes, limits, or repairs measured audio.
Results are reusable only for the same patch, renderer, QA script and fixtures.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import csv
from datetime import datetime, timezone
import hashlib
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

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'doc/scoring-1000/manifest.json'
SCHEMA = 1
CONFIG = {
    'schema': SCHEMA,
    'sample_rate': 48000,
    'secondary_sample_rate': 44100,
    'seed': 1000,
    'fresh_engine_per_scenario': True,
    'master_reference_db': 0.,
    'polyphony_stress_voices': 16,
    'peak_limit': .98,
    'silence_peak_floor': .00001,
    'dc_limit': .003,
    'default_hold_min_seconds': 3,
    'default_tail_min_seconds': 4,
    'release_time_multiplier': 3,
    'release_time_margin_seconds': 2,
    'macro_attack_multiplier': 2 ** 2.8,
    'macro_release_multiplier': 2 ** 2.2,
    'macro_defaults': [0, 0, 0, .4, 0, 0, 0, 0],
    'macro_response_waveform_floor': .001,
    'macro_response_feature_floor': .002,
    'repeat_uncertainty_multiplier': 3,
    'spectral_fft_size': 8192,
    'spectral_windows': 4,
    'spectral_band_edges_hz': [0, 40, 80, 160, 320, 640, 1280, 2560, 5120, 10240, 24001],
    'rms_envelope_bins': 12,
    'render_timeout_seconds': 300,
    'measurement': 'Unnormalized stereo float32 at the saved unity (0 dB) master reference; '
                   'sample peaks, not reconstructed true peaks. User-raised master gain is outside this fixture.',
    'audition': 'Human musical audition remains pending; no composer endorsement is claimed.',
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def digest_json(value) -> str:
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def write_json(path: Path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')
    temporary.replace(path)


def rms(a) -> float:
    return float(np.sqrt(np.mean(np.square(a, dtype=np.float64)))) if a.size else 0.


def db(value) -> float:
    return 20 * math.log10(max(float(value), 1e-12))


def scenario(key, notes, hold, tail=1., velocity=96, tempo=120, macros='-', rate=48000,
             wheel=0, pressure=0, gate='expression'):
    return {
        'key': key, 'notes': list(notes), 'hold': round(hold, 6),
        'seconds': round(hold + tail, 6), 'velocity': velocity, 'tempo': tempo,
        'macros': macros, 'sample_rate': rate, 'modwheel': wheel, 'pressure': pressure,
        'expression': 127, 'gate': gate,
    }


def fixtures(preset):
    lo, hi = preset['range']
    if not (0 <= lo < hi <= 127) or hi - lo < CONFIG['polyphony_stress_voices'] - 1:
        raise ValueError('Declared range cannot contain the required polyphony stress fixture')
    mid = (lo + hi) // 2
    attack, _, _, release = preset['amp']
    hold = max(3., attack + 1.)
    tail = max(4., release * 3. + 2.)
    chord_root = max(lo, min(mid, hi - 7))
    chord = [mid] if preset['mono'] else [chord_root, chord_root + 4, chord_root + 7]
    # A deliberately dense cluster exercises every declared voice, including the
    # sustained overlap a musical passage can create. It is not an audition chord.
    stress_root = max(lo, min(mid - 7, hi - CONFIG['polyphony_stress_voices'] + 1))
    stress = [mid] if preset['mono'] else list(range(stress_root,
                                                    stress_root + CONFIG['polyphony_stress_voices']))
    rows = [
        scenario('default', [mid], hold, tail, gate='default'),
        scenario('default_repeat', [mid], hold, tail, gate='default'),
        scenario('range_low', [lo], hold, gate='performance'),
        scenario('range_high', [hi], hold, gate='performance'),
        scenario('velocity_soft', [mid], hold, velocity=32, gate='performance'),
        scenario('velocity_hard', [mid], hold, velocity=127, gate='performance'),
        scenario('poly_chord' if not preset['mono'] else 'mono_single', chord, hold,
                 gate='performance'),
    ]
    for macro in range(8):
        macro_hold = max(hold, attack * CONFIG['macro_attack_multiplier'] + 1.) if macro == 6 else hold
        macro_tail = max(1., min(6., release * CONFIG['macro_release_multiplier'])) if macro == 7 else 1.
        rows.append(scenario(f'macro_{macro}_max', [mid], macro_hold, macro_tail,
                             macros=f'{macro}=1'))
    rows += [
        scenario('space_dry', [mid], hold, macros='3=0'),
        scenario('macros_aggressive', chord, hold, velocity=127,
                 macros='0=1,1=1,2=1,3=1,4=1,5=1,6=0,7=0'),
        scenario('wheel_max', [mid], hold, wheel=127),
        scenario('pressure_max', [mid], hold, pressure=127),
        scenario('tempo_60', [mid], hold, tempo=60),
        scenario('tempo_180', [mid], hold, tempo=180),
        scenario('sample_rate_44100', chord, hold, rate=44100),
        scenario('polyphony_stress' if not preset['mono'] else 'mono_velocity_stress',
                 stress, hold, velocity=127),
    ]
    return rows


def measure(audio_path: Path, spec):
    audio = np.fromfile(audio_path, dtype='<f4')
    expected = round(spec['seconds'] * spec['sample_rate'])
    if audio.size != expected * 2:
        raise ValueError(f'Expected {expected * 2} float samples; received {audio.size}')
    audio = audio.reshape(-1, 2)
    finite = np.isfinite(audio)
    nonfinite = int(audio.size - np.count_nonzero(finite))
    if nonfinite:
        # Preserve the original PCM and count; this copy only makes diagnostics serializable.
        analysed = np.where(finite, audio, 0.)
    else:
        analysed = audio
    sr = spec['sample_rate']
    active_end = min(len(audio), max(1, round(spec['hold'] * sr)))
    active = analysed[:active_end]
    trailing = analysed[-min(len(audio), round(.5 * sr)):]
    stereo_rms = rms(analysed)
    active_rms = rms(active)
    mono_rms = rms(analysed.mean(axis=1))
    tail_rms = rms(trailing)
    nfft = min(CONFIG['spectral_fft_size'], 2 ** int(math.log2(max(2, len(active)))))
    window = np.hanning(nfft)[:, None]
    power = np.zeros(nfft // 2 + 1)
    for start in np.linspace(0, max(0, len(active) - nfft), CONFIG['spectral_windows']).astype(int):
        segment = active[start:start + nfft].astype(np.float64)
        if len(segment) < nfft:
            segment = np.pad(segment, ((0, nfft-len(segment)), (0, 0)))
        power += np.square(np.abs(np.fft.rfft(segment * window, axis=0))).sum(axis=1)
    frequency = np.fft.rfftfreq(nfft, 1. / sr)
    energy = float(power.sum())
    bands = [float(power[(frequency >= lo) & (frequency < hi)].sum() / max(energy, 1e-30))
             for lo, hi in zip(CONFIG['spectral_band_edges_hz'][:-1], CONFIG['spectral_band_edges_hz'][1:])]
    centroid = float(np.dot(frequency, power) / max(energy, 1e-30))
    env = [rms(part) for part in np.array_split(active, CONFIG['rms_envelope_bins'])]
    normalized_env = [float(v / max(max(env), 1e-30)) for v in env]
    normalized_bands = [round(v, 9) for v in bands]
    metrics = {
        'frames': len(audio), 'nonfinite_samples': nonfinite,
        'peak': float(np.max(np.abs(analysed))) if analysed.size else 0.,
        'rms': stereo_rms, 'active_rms': active_rms,
        'channel_rms': [rms(analysed[:, i]) for i in range(2)],
        'channel_dc': [float(np.mean(analysed[:, i], dtype=np.float64)) for i in range(2)],
        'active_channel_dc': [float(np.mean(active[:, i], dtype=np.float64)) for i in range(2)],
        'tail_500ms_rms': tail_rms,
        'tail_to_active_db': db(tail_rms / max(active_rms, 1e-12)),
        'mono_energy_ratio': (mono_rms / max(stereo_rms, 1e-12)) ** 2,
        'spectral_centroid_hz': centroid, 'spectral_band_energy': normalized_bands,
        'active_rms_envelope': env, 'normalized_active_envelope': normalized_env,
        'pcm_sha256': sha256(audio_path),
        'coarse_fingerprint': digest_json({
            'bands': [round(v, 4) for v in bands],
            'envelope': [round(v, 3) for v in normalized_env],
        }),
    }
    return audio, metrics


def compare_waveforms(reference, changed):
    count = min(len(reference), len(changed))
    a, b = reference[:count], changed[:count]
    return rms(b.astype(np.float64) - a) / max(rms(a), 1e-12)


def compare_features(reference, changed):
    # Spectral energy fractions reduce sensitivity to phase; envelope distance also
    # catches useful articulation changes. This is not a psychoacoustic distance.
    a = np.array(reference['spectral_band_energy'])
    b = np.array(changed['spectral_band_energy'])
    ea = np.array(reference['normalized_active_envelope'])
    eb = np.array(changed['normalized_active_envelope'])
    return float(np.linalg.norm(a - b) + .5 * np.linalg.norm(ea - eb))


def audit_scenario(native, spec, metrics):
    failures, warnings = [], []
    if not native.get('ok'):
        failures.append('Native renderer did not report success')
    if metrics['nonfinite_samples'] or native.get('nonfinite', 0):
        failures.append('Nonfinite output samples')
    if metrics['peak'] >= CONFIG['peak_limit']:
        failures.append(f"Peak {metrics['peak']:.6g} exceeds {CONFIG['peak_limit']} safety limit")
    if metrics['peak'] <= CONFIG['silence_peak_floor']:
        failures.append('Output is silent or below the documented peak floor')
    if max(abs(v) for v in metrics['channel_dc'] + metrics['active_channel_dc']) >= CONFIG['dc_limit']:
        failures.append('Mean DC exceeds the documented per-channel limit')
    if metrics['peak'] > .8:
        warnings.append('Less than 1.94 dB sample-peak headroom in this fixture')
    if metrics['mono_energy_ratio'] < .1 and metrics['rms'] > .0001:
        warnings.append('Mono sum loses more than 10 dB of energy; inspect stereo design')
    if spec['key'] == 'default':
        if metrics['tail_500ms_rms'] > max(.002, metrics['active_rms'] * .25):
            failures.append('Released default tail remains too energetic after the full release fixture')
        elif metrics['tail_to_active_db'] > -30 and metrics['tail_500ms_rms'] > .0001:
            warnings.append('Default release tail has not fallen 30 dB below active RMS')
        if metrics['peak'] < .01:
            warnings.append('Quiet default peak; inspect intended role and gain staging')
    return failures, warnings


def gate(rows, scope):
    failures = [f"{r['fixture']['key']}: {v}" for r in rows for v in r['failures']]
    warnings = [f"{r['fixture']['key']}: {v}" for r in rows for v in r['warnings']]
    return {'status': 'fail' if failures else 'pass', 'scope': scope,
            'failures': failures, 'warnings': warnings}


def run_preset(preset, renderer, output, binding, keep_audio, force):
    ident = preset['id']
    if not re.fullmatch(r'SC\d{2}-\d{3}', ident):
        raise ValueError(f'Invalid preset ID: {ident}')
    input_path = (ROOT / preset['path']).resolve()
    if not input_path.is_relative_to(ROOT):
        raise ValueError('Preset path escapes repository')
    actual_hash = sha256(input_path)
    planned = fixtures(preset)
    expected_binding = {**binding, 'patch_sha256': actual_hash,
                        'fixture_sha256': digest_json(planned),
                        'preset_definition_sha256': digest_json(preset)}
    final_path = output / 'presets' / f'{ident}.json'
    if final_path.exists() and not force:
        previous = json.loads(final_path.read_text())
        if previous.get('binding') == expected_binding and previous.get('status') == 'pass':
            return previous, True
    started = time.monotonic()
    result = {
        'schema': SCHEMA, 'id': ident, 'name': preset['name'],
        'category': preset['category'], 'category_id': preset['category_id'],
        'family': preset['family'], 'variant': preset['variant'],
        'patch_path': preset['path'], 'binding': expected_binding,
        'musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
        'measurement_scope': CONFIG['measurement'],
        'created_utc': datetime.now(timezone.utc).isoformat(),
        'status': 'fail', 'scenarios': [], 'gates': {}, 'warnings': [],
    }
    work = output / 'work' / ident
    work.mkdir(parents=True, exist_ok=True)
    try:
        if actual_hash != preset['sha256']:
            raise ValueError('Patch SHA256 differs from manifest; rebuild and validate the catalog')
        rows = []
        for spec in planned:
            raw = work / f"{spec['key']}.f32"
            columns = [str(input_path), str(raw), str(spec['sample_rate']), str(spec['seconds']),
                       str(spec['hold']), ','.join(map(str, spec['notes'])), str(spec['velocity']),
                       str(spec['tempo']), spec['macros'], str(spec['modwheel']),
                       str(spec['pressure']), str(spec['expression'])]
            if any('\t' in v or '\n' in v or '\r' in v for v in columns):
                raise ValueError('Native TSV field contains a control character')
            rows.append('\t'.join(columns))
        batch = work / 'scenarios.tsv'
        report = work / 'native.jsonl'
        batch.write_text('\n'.join(rows) + '\n')
        with (work / 'native.log').open('w') as log:
            process = subprocess.run([
                str(renderer), '--batch', str(batch), '--report', str(report),
                '--roundtrip-dir', str(work / 'roundtrip'),
                '--data', str(ROOT / 'resources/data'), '--seed', str(CONFIG['seed']),
                '--fresh-engine', '1',
            ], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                timeout=CONFIG['render_timeout_seconds'], check=False)
        native_rows = [json.loads(line) for line in report.read_text().splitlines() if line.strip()]
        if len(native_rows) != len(planned):
            raise ValueError(f'Native report has {len(native_rows)} rows; expected {len(planned)}')
        if sha256(input_path) != actual_hash:
            raise ValueError('Patch changed while native scenarios were rendering; discard this evidence')
        result['native_exit_code'] = process.returncode
        original_audio, repeat_audio = None, None
        by_key = {}
        for native, spec in zip(native_rows, planned):
            row = {'fixture': spec, 'native': native, 'failures': [], 'warnings': []}
            if Path(native.get('out', '')).name != f"{spec['key']}.f32":
                row['failures'].append('Native report output identity disagrees with its fixture')
            if not native.get('ok'):
                row['failures'].append(native.get('error', 'Native load/render failure'))
            raw = work / f"{spec['key']}.f32"
            if raw.exists():
                audio, metrics = measure(raw, spec)
                row['metrics'] = metrics
                failures, warnings = audit_scenario(native, spec, metrics)
                row['failures'].extend(failures)
                row['warnings'].extend(warnings)
                if native.get('name') != preset['name']:
                    row['failures'].append('Native loaded patch name differs from the manifest')
                if spec['key'] == 'default':
                    original_audio = audio
                elif spec['key'] == 'default_repeat':
                    repeat_audio = audio
                elif original_audio is not None and spec['sample_rate'] == 48000:
                    if spec['key'] == 'macro_6_max':
                        # Match the note-on window: longer held notes must not masquerade
                        # as a successful Attack-control response after baseline note-off.
                        end = round(min(planned[0]['hold'], spec['hold']) * spec['sample_rate'])
                        row['waveform_delta_from_default'] = compare_waveforms(original_audio[:end], audio[:end])
                        row['comparison_window'] = 'Common pre-note-off samples only'
                    else:
                        row['waveform_delta_from_default'] = compare_waveforms(original_audio, audio)
                if not keep_audio:
                    raw.unlink()
            else:
                row['failures'].append('Native PCM output is missing')
            result['scenarios'].append(row)
            by_key[spec['key']] = row
        if original_audio is not None and repeat_audio is not None:
            repeat_waveform = compare_waveforms(original_audio, repeat_audio)
            repeat_feature = compare_features(by_key['default']['metrics'],
                                              by_key['default_repeat']['metrics'])
            result['default_repeat'] = {
                'waveform_relative_rms_delta': repeat_waveform,
                'spectral_envelope_feature_distance': repeat_feature,
                'bit_identical': by_key['default']['metrics']['pcm_sha256'] ==
                                 by_key['default_repeat']['metrics']['pcm_sha256'],
            }
            if repeat_waveform > .01:
                result['warnings'].append('Repeated default audio is not waveform-deterministic; '
                                          'macro comparisons use measured repeat uncertainty')
            responses = {}
            for i, label in enumerate(preset['macros']):
                row = by_key[f'macro_{i}_max']
                if 'metrics' not in row:
                    responses[label] = {'status': 'not_measured'}
                    continue
                wd = row.get('waveform_delta_from_default', 0.)
                # Attack's adaptive hold differs; unaligned whole-hold features would
                # be confounded by performance timing, so use the matched PCM window.
                fd = 0. if i == 6 else compare_features(by_key['default']['metrics'], row['metrics'])
                waveform_threshold = max(CONFIG['macro_response_waveform_floor'],
                    repeat_waveform * CONFIG['repeat_uncertainty_multiplier'])
                feature_threshold = max(CONFIG['macro_response_feature_floor'],
                    repeat_feature * CONFIG['repeat_uncertainty_multiplier'])
                measurable = wd > waveform_threshold or fd > feature_threshold
                responses[label] = {
                    'status': 'measurable_change' if measurable else 'not_resolved_above_repeat',
                    'waveform_relative_rms_delta': wd, 'feature_distance': fd,
                    'waveform_threshold': waveform_threshold, 'feature_threshold': feature_threshold,
                    'interpretation': 'Measured change is not certification of useful audibility.',
                }
                if not measurable:
                    row['warnings'].append(f'{label} response not resolved above repeat/measurement '
                                           'threshold; targeted musical review is required')
            result['macro_response'] = responses
            soft = by_key['velocity_soft'].get('metrics', {}).get('active_rms')
            hard = by_key['velocity_hard'].get('metrics', {}).get('active_rms')
            if soft is not None and hard is not None:
                result['velocity_hard_to_soft_db'] = db(hard / max(soft, 1e-12))
                if hard <= soft * 1.05:
                    by_key['velocity_hard']['warnings'].append(
                        'Hard velocity does not exceed soft RMS by 5%; inspect expression behavior')
        definition_failures = []
        if process.returncode:
            definition_failures.append(f'Native renderer exited with code {process.returncode}')
        if any(not row.get('ok') or not row.get('roundtrip') for row in native_rows):
            definition_failures.append('At least one native load/save/reload did not succeed')
        result['gates']['1_native_definition'] = {
            'status': 'fail' if definition_failures else 'pass',
            'scope': 'Manifest SHA, native load, metadata identity, native save and reload. '
                     'Full semantic serialization equality belongs to the separate static audit.',
            'failures': definition_failures, 'warnings': [],
        }
        result['gates']['2_default_audio'] = gate(
            [r for r in result['scenarios'] if r['fixture']['gate'] == 'default'],
            'Unnormalized default integrity, sample headroom, DC, and released-tail behavior.')
        result['gates']['3_performance'] = gate(
            [r for r in result['scenarios'] if r['fixture']['gate'] == 'performance'],
            'Declared register endpoints, soft/hard velocity, and chord or mono-note rendering.')
        result['gates']['4_expression_stress'] = gate(
            [r for r in result['scenarios'] if r['fixture']['gate'] == 'expression'],
            'Macro endpoints, dry Space, combined controls, MIDI expression, tempo, '
            '44.1 kHz, and sixteen-note polyphony or mono hard-velocity integrity. '
            'Unresolved response measurements remain warnings, not artistic approval.')
        complete = len(result['scenarios']) == len(planned) and all('metrics' in r for r in result['scenarios'])
        result['gates']['5_evidence_fingerprint'] = {
            'status': 'pass' if complete else 'fail',
            'scope': 'Complete per-fixture evidence and audio fingerprints. Bank-wide semantic '
                     'diversity and installed package verification are separate required audits.',
            'failures': [] if complete else ['Required fixture evidence is incomplete'], 'warnings': [],
        }
        result['status'] = 'pass' if all(g['status'] == 'pass' for g in result['gates'].values()) else 'fail'
    except Exception as exc:
        result['fatal_error'] = f'{type(exc).__name__}: {exc}'
        result['status'] = 'fail'
    finally:
        result['elapsed_seconds'] = round(time.monotonic() - started, 3)
        result['warning_count'] = len(result['warnings']) + sum(len(r['warnings']) for r in result['scenarios'])
        write_json(final_path, result)
        # Keep failure logs and roundtrips for diagnosis. Successful complete runs
        # retain detailed metrics but do not consume unbounded raw-audio storage.
        if result['status'] == 'pass' and not keep_audio:
            shutil.rmtree(work)
    return result, False


def summarize(results, output, binding):
    groups = {}
    for result in results:
        for row in result['scenarios']:
            if row['fixture']['key'] == 'default' and 'metrics' in row:
                groups.setdefault(row['metrics']['pcm_sha256'], []).append(result['id'])
    duplicates = [ids for ids in groups.values() if len(ids) > 1]
    summary = {
        'schema': SCHEMA, 'binding': binding, 'config': CONFIG,
        'tested_presets': len(results), 'passed': sum(r['status'] == 'pass' for r in results),
        'failed': sum(r['status'] != 'pass' for r in results),
        'native_scenarios': sum(len(r['scenarios']) for r in results),
        'presets_with_warnings': sum(r['warning_count'] > 0 for r in results),
        'warning_count': sum(r['warning_count'] for r in results),
        'exact_default_audio_duplicate_groups': duplicates,
        'duplicate_interpretation': 'Exact audio collisions require review; different hashes '
                                    'do not prove perceptual distinctness.',
        'scope': 'Five automated technical gates. Independent static, diversity, package, '
                 'and listening review must be reported separately.',
        'musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
        'failed_ids': [r['id'] for r in results if r['status'] != 'pass'],
        'created_utc': datetime.now(timezone.utc).isoformat(),
    }
    write_json(output / 'summary.json', summary)
    with (output / 'summary.csv').open('w', newline='') as f:
        fields = ['id', 'name', 'category', 'status', 'patch_sha256', 'warnings',
                  'default_peak', 'default_rms', 'default_tail_rms', 'max_scenario_peak',
                  'scenario_count', 'unresolved_macros', 'musical_audition']
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for result in results:
            default = next((r.get('metrics', {}) for r in result['scenarios']
                            if r['fixture']['key'] == 'default'), {})
            writer.writerow({
                'id': result['id'], 'name': result['name'], 'category': result['category'],
                'status': result['status'], 'patch_sha256': result['binding']['patch_sha256'],
                'warnings': result['warning_count'], 'default_peak': default.get('peak', ''),
                'default_rms': default.get('rms', ''), 'default_tail_rms': default.get('tail_500ms_rms', ''),
                'max_scenario_peak': max((r.get('metrics', {}).get('peak', 0) for r in result['scenarios']), default=0),
                'scenario_count': len(result['scenarios']),
                'unresolved_macros': ';'.join(k for k, v in result.get('macro_response', {}).items()
                                             if v['status'] != 'measurable_change'),
                'musical_audition': 'pending',
            })
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--renderer', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--workers', type=int, default=4, help='Bounded native processes (1..8)')
    parser.add_argument('--limit', type=int, help='Test only the first N selected entries')
    parser.add_argument('--ids', help='Comma-separated exact IDs; rejects missing IDs')
    parser.add_argument('--keep-audio', action='store_true')
    parser.add_argument('--force', action='store_true', help='Ignore valid successful cached results')
    args = parser.parse_args()
    if not 1 <= args.workers <= 8:
        parser.error('--workers must be between 1 and 8')
    if args.limit is not None and args.limit <= 0:
        parser.error('--limit must be positive')
    renderer, output = args.renderer.resolve(), args.output.resolve()
    if not renderer.is_file() or not os.access(renderer, os.X_OK):
        parser.error('--renderer must name an executable native renderer')
    output.mkdir(parents=True, exist_ok=True)
    catalog = json.loads(MANIFEST.read_text())
    presets = catalog['presets']
    if args.ids:
        selected = set(args.ids.split(','))
        missing = selected - {p['id'] for p in presets}
        if missing:
            parser.error('Unknown preset IDs: ' + ', '.join(sorted(missing)))
        presets = [p for p in presets if p['id'] in selected]
    if args.limit:
        presets = presets[:args.limit]
    binding = {
        'renderer_sha256': sha256(renderer), 'qa_script_sha256': sha256(Path(__file__)),
        'config_sha256': digest_json(CONFIG), 'numpy_version': np.__version__,
    }
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        futures = {pool.submit(run_preset, p, renderer, output, binding,
                               args.keep_audio, args.force): p for p in presets}
        for completed in concurrent.futures.as_completed(futures):
            try:
                result, reused = completed.result()
            except Exception as exc:
                preset = futures[completed]
                result = {
                    'schema': SCHEMA, 'id': preset['id'], 'name': preset.get('name', ''),
                    'category': preset.get('category', ''), 'category_id': preset.get('category_id', ''),
                    'binding': {**binding, 'patch_sha256': preset.get('sha256', '')},
                    'status': 'fail', 'scenarios': [], 'gates': {}, 'warnings': [],
                    'warning_count': 0, 'musical_audition': 'pending',
                    'composer_endorsement': 'not_claimed',
                    'fatal_error': f'{type(exc).__name__}: {exc}',
                }
                reused = False
                # Manifest IDs normally pass run_preset's guard; use a safe basename
                # even when reporting an adversarial malformed manifest entry.
                basename = re.sub(r'[^A-Za-z0-9_-]', '_', str(preset['id']))
                write_json(output / 'presets' / f'{basename}.json', result)
            results.append(result)
            print(f"{len(results)}/{len(presets)} {result['id']} {result['status']} "
                  f"warnings={result['warning_count']} {'cached' if reused else 'rendered'}", flush=True)
    results.sort(key=lambda r: r['id'])
    if sha256(renderer) != binding['renderer_sha256']:
        for result in results:
            result['status'] = 'fail'
            result['fatal_error'] = 'Renderer binary changed during the run; all evidence must be regenerated'
            basename = re.sub(r'[^A-Za-z0-9_-]', '_', str(result['id']))
            write_json(output / 'presets' / f'{basename}.json', result)
    summary = summarize(results, output, binding)
    print(json.dumps({k: summary[k] for k in ('tested_presets', 'passed', 'failed',
                                            'native_scenarios', 'presets_with_warnings')}, indent=2))
    return 1 if summary['failed'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
