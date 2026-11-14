#!/usr/bin/env python3
"""Independent category-09 interval and tempo audit, with a bounded native probe.

SPDX-License-Identifier: GPL-3.0-or-later
Pitch-class estimates are engineering observations, not a listening assessment.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import xml.etree.ElementTree as ET

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
SCALES = {
    'Amber Ladder': {0, 2, 4, 7, 9, 12},
    'Night Thirds': {0, 3, 5, 7, 10, 12},
    'Open Fifths': {0, 7, 12},
    'Prism Arcs': {0, 2, 4, 6, 8, 10, 12},
}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def xml(path):
    data = Path(path).read_bytes()
    assert data[:4] == b'CcnK' and data[60:64] == b'sub3'
    size = struct.unpack_from('<I', data, 64)[0]
    return ET.fromstring(data[92:92 + size])


def audit_static(manifest):
    rows = []
    rates = {}
    for entry in manifest['presets']:
        if entry['category_id'] != '09':
            continue
        path = ROOT / entry['path']
        assert digest(path) == entry['sha256'], entry['id']
        patch = xml(path)
        p = {n.tag: n for n in patch.find('parameters')}
        value = lambda key: float(p[key].get('value'))
        assert value('a_lfo0_shape') == 7
        assert p['a_lfo0_rate'].get('temposync') == '1'
        assert value('a_lfo0_deform') == 0 and p['a_lfo0_deform'].get('deform_type') == '0'
        assert value('a_lfo0_phase') == 0 and value('a_lfo0_trigmode') == 0
        assert not list(p['a_lfo0_rate']), 'Synchronized step rate has unexpected modulation'
        seq = next(n for n in patch.find('stepsequences')
                   if n.get('scene') == '0' and n.get('i') == '0')
        assert seq.get('loop_start') == '0' and seq.get('loop_end') == '15'
        assert seq.get('shuffle') == '0' and seq.get('trigmask') == '0'
        steps = [float(seq.get(f's{i}')) for i in range(16)]
        extra = next(n for n in patch.find('extralfo')
                     if n.get('scene') == '0' and n.get('i') == '0')
        assert extra.get('extraAmplitude') == '0'
        pitch = [n for n in p['a_pitch'] if n.get('source') == '17']
        intervals = None
        if entry['family'] in SCALES:
            assert len(pitch) == 1 and pitch[0].get('source_index') == '1'
            assert float(pitch[0].get('depth')) == 12 and pitch[0].get('muted') == '0'
            exact = [12 * step for step in steps]
            intervals = [round(v) for v in exact]
            assert max(abs(a - b) for a, b in zip(exact, intervals)) < 1e-6
            allowed = {0, 12} if entry['variant'] == 'Sparse' else SCALES[entry['family']]
            assert set(intervals) == allowed, (entry['id'], intervals)
            if entry['variant'] == 'Sparse':
                assert intervals == [12 if i in (0, 8) else 0 for i in range(16)]
                assert 'octave' in entry['role']
        else:
            assert not pitch
        rate = 2 ** value('a_lfo0_rate')
        rates.setdefault(entry['family'], {})[entry['variant']] = rate
        rows.append({'id': entry['id'], 'family': entry['family'], 'variant': entry['variant'],
                     'patch_sha256': entry['sha256'], 'steps_per_second_at_120_bpm': rate,
                     'steps_per_quarter_note': rate / 2,
                     'quarter_notes_per_16_step_cycle': 32 / rate,
                     'semitone_intervals': intervals})
    assert len(rows) == 100 and len(rates) == 20
    for family, variants in rates.items():
        assert len(variants) == 5
        for variant, rate in variants.items():
            factor = 1.5 if variant == 'Triplet' else 1.
            assert abs(rate / variants['Soft'] - factor) < 1e-7, (family, variant)
    assert sum(r['semitone_intervals'] is not None for r in rows) == 20
    return rows


def strongest_pitch_class(audio, sr, center, width=.08):
    """Estimate the strongest partial's class; deliberately do not infer its octave."""
    start = max(0, round((center - width / 2) * sr))
    segment = audio[start:start + round(width * sr)].mean(axis=1).astype(np.float64)
    segment -= segment.mean()
    spectrum = np.abs(np.fft.rfft(segment * np.hanning(len(segment)), n=65536))
    freqs = np.fft.rfftfreq(65536, 1 / sr)
    lo, hi = np.searchsorted(freqs, [90., 1100.])
    peak = lo + int(np.argmax(spectrum[lo:hi]))
    logs = np.log(np.maximum(spectrum[peak - 1:peak + 2], 1e-30))
    offset = .5 * (logs[0] - logs[2]) / (logs[0] - 2 * logs[1] + logs[2])
    frequency = (peak + offset) * sr / 65536
    midi = 69 + 12 * math.log2(frequency / 440.)
    return float(midi % 12), float(frequency)


def native_probe(manifest, renderer, output):
    entry = next(p for p in manifest['presets'] if p['id'] == 'SC09-081')
    rows = []
    for tempo in (60, 120, 180):
        for motion in (0, 1):
            path = output / f'amber-{tempo}-motion{motion}.f32'
            rows.append('\t'.join((str(ROOT / entry['path']), str(path), '48000', '10',
                                   '9', '60', '96', str(tempo), f'1={motion},3=0')))
    batch, report = output / 'batch.tsv', output / 'native.jsonl'
    batch.write_text('\n'.join(rows) + '\n')
    with (output / 'native.log').open('w') as log:
        subprocess.run([str(renderer), '--batch', str(batch), '--report', str(report),
                        '--data', str(ROOT / 'resources/data'), '--seed', '1000',
                        '--fresh-engine', '1'], cwd=ROOT, check=True,
                       stdout=log, stderr=subprocess.STDOUT, timeout=180)
    native = [json.loads(line) for line in report.read_text().splitlines()]
    assert len(native) == 6 and all(n['ok'] and n['nonfinite'] == 0 for n in native)
    seq = next(n for n in xml(ROOT / entry['path']).find('stepsequences')
               if n.get('scene') == '0' and n.get('i') == '0')
    expected = [round(float(seq.get(f's{i}')) * 12) % 12 for i in range(16)]
    measurements = []
    for n in native:
        audio = np.fromfile(n['out'], dtype='<f4').reshape(-1, 2)
        assert np.isfinite(audio).all()
        duration = 120 / (4 * n['tempo'])
        observed = [strongest_pitch_class(audio, 48000, (i + .5) * duration)
                    for i in range(16)]
        errors = [abs((pitch - wanted + 6) % 12 - 6) * 100
                  for (pitch, _), wanted in zip(observed, expected)]
        # Dense pitch-class tracking measures transition times without assuming
        # sample-accurate control/oscillator smoothing or perceived intonation.
        hop = .01
        times = np.arange(.06, 16 * duration - .03, hop)
        classes = [round(strongest_pitch_class(audio, 48000, t, width=.06)[0]) % 12
                   for t in times]
        changes = [float(times[i]) for i in range(1, len(times)) if classes[i] != classes[i - 1]]
        residuals = [min(abs(t - i * duration) for t in changes)
                     for i in range(1, 16) if expected[i] != expected[i - 1]]
        assert max(errors) < 5., 'Illustrative center pitch class differs by at least five cents'
        assert max(residuals) <= .02, 'Illustrative transition differs by more than 20 ms'
        measurements.append({
            'tempo': n['tempo'], 'motion': 1 if '1=1' in n['macros'] else 0,
            'space': 0, 'step_seconds_from_native_rate': duration,
            'estimated_pitch_classes': [round(p, 5) for p, _ in observed],
            'expected_pitch_classes': expected, 'max_center_class_error_cents': max(errors),
            'max_transition_residual_seconds': max(residuals),
            'median_transition_residual_seconds': float(np.median(residuals)),
            'measurement_hop_seconds': hop,
            'pitch_class_limit_cents': 5., 'transition_limit_seconds': .02,
            'peak': n['peak'], 'pcm_sha256': digest(n['out']),
            'scope': 'Strongest-partial pitch classes, octave folded; not independent octave or hearing certification.'})
    return {'preset_id': entry['id'], 'patch_sha256': entry['sha256'],
            'renderer_sha256': digest(renderer), 'measurements': measurements}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--renderer', type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    manifest_path = ROOT / 'doc/scoring-1000/manifest.json'
    manifest = json.loads(manifest_path.read_text())
    result = {'schema': 1, 'manifest_sha256': digest(manifest_path),
              'script_sha256': digest(__file__), 'static': audit_static(manifest),
              'musical_audition': 'pending', 'composer_endorsement': 'not_claimed'}
    if args.renderer:
        result['native_probe'] = native_probe(manifest, args.renderer.resolve(), output)
    assert digest(manifest_path) == result['manifest_sha256'], 'Catalog changed during audit'
    (output / 'sequence-audit.json').write_text(json.dumps(result, indent=2, allow_nan=False) + '\n')
    print(json.dumps({'static_presets': len(result['static']), 'tuned_presets': 20,
                      'native_renders': len(result.get('native_probe', {}).get('measurements', []))}))


if __name__ == '__main__':
    main()
