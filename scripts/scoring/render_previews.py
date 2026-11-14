#!/usr/bin/env python3
"""Render ten documented native audition excerpts and a compact overview.

SPDX-License-Identifier: GPL-3.0-or-later
Requires NumPy, FFmpeg/FFprobe and the actual surge-scoring-render executable.
Preview playback gain is separate from the unnormalized bank QA measurements.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
SAMPLE_RATE = 48000
SEED = 1000
GAP_SECONDS = .4
MAX_COMMON_GAIN = 4.
PLAYBACK_TARGET_PEAK = .7
PLAYBACK_PEAK_CEILING = .8

# Each row is one actual held note or simultaneous voicing, not a composed cue.
SELECTION = [
    ('SC01-007', [57, 60, 64], 2.5, 88, 'Intimate electric-key harmony'),
    ('SC02-014', [60, 64, 69], 2., 88, 'Resonant harp-like voicing'),
    ('SC03-022', [65, 69, 72], 3., 82, 'Soft mallet resonance'),
    ('SC04-003', [45, 52, 60], 4., 84, 'Open sustained pad harmony'),
    ('SC05-022', [55, 62, 67], 4., 92, 'Synthetic middle-string fifths'),
    ('SC06-001', [48, 55], 6., 78, 'Restrained drone and open fifth'),
    ('SC07-033', [40], 3., 100, 'Focused saw-and-sub bass'),
    ('SC08-092', [69], 4., 88, 'Rounded solo tone'),
    ('SC09-082', [48], 8., 96, 'Held-root major-pentatonic sequence'),
    ('SC10-052', [36], .7, 104, 'Low pitched impact and decay'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_json(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')
    temporary.replace(path)


def metrics(audio):
    if audio.ndim != 2 or audio.shape[1] != 2 or not len(audio):
        raise ValueError('Expected nonempty stereo audio')
    nonfinite = int(np.count_nonzero(~np.isfinite(audio)))
    if nonfinite:
        raise ValueError(f'Audio has {nonfinite} nonfinite samples')
    return {
        'frames': len(audio), 'sample_rate': SAMPLE_RATE, 'channels': 2,
        'duration_seconds': len(audio) / SAMPLE_RATE,
        'nonfinite_samples': nonfinite,
        'sample_peak': float(np.max(np.abs(audio))),
        'rms': float(np.sqrt(np.mean(np.square(audio, dtype=np.float64)))),
        'last_500ms_rms': float(np.sqrt(np.mean(np.square(
            audio[-SAMPLE_RATE // 2:], dtype=np.float64)))),
    }


def decode_and_check(path, ffmpeg, ffprobe, expected_frames, max_padding_frames=0):
    probe = json.loads(subprocess.check_output([
        ffprobe, '-v', 'error', '-select_streams', 'a:0',
        '-show_entries', 'stream=codec_name,sample_rate,channels,bits_per_raw_sample',
        '-of', 'json', str(path),
    ]))['streams'][0]
    if int(probe['sample_rate']) != SAMPLE_RATE or int(probe['channels']) != 2:
        raise ValueError(f'Unexpected encoded format: {probe}')
    decoded = subprocess.run([
        ffmpeg, '-v', 'error', '-i', str(path), '-map', '0:a:0',
        '-c:a', 'pcm_f32le', '-f', 'f32le', 'pipe:1',
    ], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout
    values = np.frombuffer(decoded, dtype='<f4')
    padding_frames = len(values) // 2 - expected_frames
    if len(values) % 2 or not 0 <= padding_frames <= max_padding_frames:
        raise ValueError(f'{path.name}: decoded frame count differs from the native fixture')
    audio = values.reshape(-1, 2)
    result = metrics(audio)
    if not 1e-5 < result['sample_peak'] < PLAYBACK_PEAK_CEILING:
        raise ValueError(f'{path.name}: decoded peak is outside the preview range')
    result.update(codec=probe['codec_name'], bits_per_raw_sample=probe.get('bits_per_raw_sample'),
                  sha256=digest(path), bytes=path.stat().st_size,
                  reference_frames=expected_frames, decoded_padding_frames=padding_frames)
    return audio, result


def render(renderer, output, work, ffmpeg, ffprobe):
    output.mkdir(parents=True, exist_ok=True)
    work.mkdir(parents=True, exist_ok=True)
    manifest_path = ROOT / 'doc/scoring-1000/manifest.json'
    manifest_sha = digest(manifest_path)
    catalog = json.loads(manifest_path.read_text())
    if catalog['preset_count'] != 1000 or catalog['category_count'] != 10:
        raise ValueError('Preview selection requires the complete Scoring 1000 catalog')
    by_id = {preset['id']: preset for preset in catalog['presets']}
    rows, selected, batch_rows = [], [], []
    for ident, notes, hold, velocity, purpose in SELECTION:
        preset = by_id[ident]
        patch = (ROOT / preset['path']).resolve()
        if not patch.is_relative_to(ROOT) or digest(patch) != preset['sha256']:
            raise ValueError(f'{ident}: preset path or hash is invalid')
        if not all(preset['range'][0] <= note <= preset['range'][1] for note in notes):
            raise ValueError(f'{ident}: preview notes are outside the suggested range')
        if preset['mono'] and len(notes) != 1:
            raise ValueError(f'{ident}: mono preview must use one note')
        tail = max(4., preset['amp'][3] * 3. + 2.)
        frames = round((hold + tail) * SAMPLE_RATE)
        seconds = frames / SAMPLE_RATE
        raw = work / f'{ident}.f32'
        fixture = {
            'notes': notes, 'hold_seconds': hold, 'total_seconds': seconds,
            'release_observation_seconds': seconds - hold,
            'velocity': velocity, 'tempo_bpm': 120, 'macro_assignments': '-',
            'saved_macro_values': [0, 0, 0, .4, 0, 0, 0, 0],
            'modwheel': 0, 'channel_pressure': 0, 'expression': 127,
            'sample_rate': SAMPLE_RATE, 'seed': SEED, 'fresh_engine': True,
        }
        columns = [str(patch), str(raw), str(SAMPLE_RATE), f'{seconds:.9f}',
                   str(hold), ','.join(map(str, notes)), str(velocity), '120', '-', '0', '0', '127']
        if any('\t' in column or '\n' in column for column in columns):
            raise ValueError('Unsafe TSV field')
        batch_rows.append('\t'.join(columns))
        selected.append((preset, raw, fixture, purpose, frames))
    if {preset['category_id'] for preset, *_ in selected} != {f'{i:02d}' for i in range(1, 11)}:
        raise ValueError('Previews must cover all ten categories exactly once')
    batch = work / 'previews.tsv'
    report = work / 'native.jsonl'
    batch.write_text('\n'.join(batch_rows) + '\n')
    report.unlink(missing_ok=True)
    with (work / 'native.log').open('w') as log:
        subprocess.run([
            str(renderer), '--batch', str(batch), '--report', str(report),
            '--data', str(ROOT / 'resources/data'), '--seed', str(SEED), '--fresh-engine', '1',
        ], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    native = [json.loads(line) for line in report.read_text().splitlines() if line.strip()]
    if len(native) != len(selected):
        raise ValueError('Native renderer did not return exactly ten reports')
    raw_audio = []
    for (preset, raw, fixture, purpose, frames), native_row in zip(selected, native):
        if not native_row.get('ok') or native_row.get('nonfinite') or native_row['name'] != preset['name']:
            raise ValueError(f"Native render failed or mismatched: {preset['id']}")
        if Path(native_row['out']).resolve() != raw.resolve() or native_row['frames'] != frames:
            raise ValueError('Native fixture identity differs from its requested output')
        audio = np.fromfile(raw, dtype='<f4').reshape(-1, 2)
        if len(audio) != frames:
            raise ValueError('Native PCM length differs from its fixture')
        raw_metrics = metrics(audio)
        if not 1e-5 < raw_metrics['sample_peak'] < .98:
            raise ValueError('Native preview is silent or exceeds the raw safety boundary')
        raw_audio.append(audio)
        native_row['patch'] = preset['path']
        native_row['out'] = raw.name
        rows.append({
            'id': preset['id'], 'name': preset['name'], 'category': preset['category'],
            'category_id': preset['category_id'], 'patch_path': preset['path'],
            'patch_sha256': preset['sha256'], 'purpose': purpose, 'fixture': fixture,
            'raw_pcm_sha256': digest(raw), 'raw_native_metrics': raw_metrics,
            'native_report': native_row,
        })
    if digest(manifest_path) != manifest_sha or any(
        digest(ROOT / preset['path']) != preset['sha256'] for preset, *_ in selected):
        raise ValueError('Input catalog or selected presets changed during rendering')

    maximum = max(row['raw_native_metrics']['sample_peak'] for row in rows)
    gain = min(MAX_COMMON_GAIN, PLAYBACK_TARGET_PEAK / maximum)
    encoded_audio, timeline = [], []
    position = 0
    gap = np.zeros((round(GAP_SECONDS * SAMPLE_RATE), 2), dtype='<f4')
    for index, (row, audio) in enumerate(zip(rows, raw_audio)):
        slug = '-'.join(row['name'].lower().split())
        filename = f"{row['category_id']}-{slug}.flac"
        scaled = (audio * gain).astype('<f4')
        scaled_path = work / f"{row['id']}-playback.f32"
        scaled.tofile(scaled_path)
        target = output / filename
        subprocess.run([
            ffmpeg, '-v', 'error', '-y', '-f', 'f32le', '-ar', str(SAMPLE_RATE), '-ac', '2',
            '-i', str(scaled_path), '-map_metadata', '-1', '-c:a', 'flac',
            '-sample_fmt', 's32', '-bits_per_raw_sample', '24', '-compression_level', '8',
            str(target),
        ], check=True, timeout=120)
        decoded, encoded = decode_and_check(target, ffmpeg, ffprobe, len(scaled))
        if encoded['codec'] != 'flac' or encoded['bits_per_raw_sample'] != '24':
            raise ValueError('Expected 24-bit FLAC')
        error = float(np.max(np.abs(decoded.astype(np.float64) - scaled)))
        if error > 2. / 2 ** 23:
            raise ValueError('FLAC differs by more than bounded 24-bit PCM conversion error')
        encoded['max_absolute_error_from_scaled_float32'] = error
        row['file'] = filename
        row['encoded_audio'] = encoded
        row['overview_start_seconds'] = position / SAMPLE_RATE
        timeline.append({'id': row['id'], 'name': row['name'], 'start_seconds': position / SAMPLE_RATE,
                         'duration_seconds': len(scaled) / SAMPLE_RATE})
        encoded_audio.append(scaled)
        position += len(scaled)
        if index != len(rows) - 1:
            encoded_audio.append(gap)
            position += len(gap)
    combined = np.concatenate(encoded_audio)
    combined_raw = work / 'overview-playback.f32'
    combined.tofile(combined_raw)
    overview = output / 'scoring-1000-overview.mp3'
    subprocess.run([
        ffmpeg, '-v', 'error', '-y', '-f', 'f32le', '-ar', str(SAMPLE_RATE), '-ac', '2',
        '-i', str(combined_raw), '-map_metadata', '-1', '-c:a', 'libmp3lame', '-q:a', '2',
        str(overview),
    ], check=True, timeout=120)
    # MP3 encoder/decoder padding can retain part of one 1152-sample frame.
    # Record it explicitly; the individual FLAC files require exact frame counts.
    _, overview_metrics = decode_and_check(overview, ffmpeg, ffprobe, len(combined),
                                          max_padding_frames=1152)
    result = {
        'schema': 1, 'title': 'Scoring 1000 native audition excerpts',
        'scope': 'Ten held-note or simultaneous-voicing excerpts; not complete composed cues, '
                 'not a listening panel and not the bank-wide QA evidence.',
        'manifest_sha256': manifest_sha, 'renderer_sha256': digest(renderer),
        'preview_script_sha256': digest(Path(__file__)),
        'ffmpeg_version': subprocess.check_output([ffmpeg, '-version'], text=True).splitlines()[0],
        'numpy_version': np.__version__,
        'playback_gain_linear': gain, 'playback_gain_db': 20 * math.log10(gain),
        'playback_gain_policy': 'One constant multiplier for every excerpt and the overview: '
                                'min(4, 0.70 / largest native excerpt peak). No per-clip '
                                'normalization, limiting, compression, fades or added effects.',
        'raw_measurement_scope': 'Unnormalized native stereo float32 at saved unity master; '
                                 'preview gain is applied only after these measurements.',
        'encoding': 'Individual FLAC files preserve converted 24-bit PCM; the MP3 overview is lossy.',
        'noteoff_timing': 'First native 32-sample block boundary at or after requested hold time.',
        'preview_count': len(rows), 'previews': rows,
        'overview': {'file': overview.name, 'gap_seconds': GAP_SECONDS,
                     'timeline': timeline, 'decoded_metrics': overview_metrics},
        'musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
    }
    write_json(output / 'previews.json', result)
    write_json(output / 'checksums.json', {
        'schema': 1, 'files': {path.name: digest(path) for path in sorted(output.iterdir())
                              if path.is_file() and path.name != 'checksums.json'},
    })
    print(json.dumps({
        'previews': len(rows), 'common_gain_db': result['playback_gain_db'],
        'largest_raw_peak': maximum,
        'largest_decoded_flac_peak': max(row['encoded_audio']['sample_peak'] for row in rows),
        'overview_seconds': overview_metrics['duration_seconds'],
        'overview_decoded_peak': overview_metrics['sample_peak'],
        'total_media_bytes': sum(row['encoded_audio']['bytes'] for row in rows) + overview_metrics['bytes'],
        'manifest_sha256': manifest_sha,
    }, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--renderer', required=True, type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'doc/scoring-1000/audio')
    parser.add_argument('--work-dir', type=Path, help='Retain native PCM and fixture logs here')
    parser.add_argument('--ffmpeg', default=shutil.which('ffmpeg'))
    parser.add_argument('--ffprobe', default=shutil.which('ffprobe'))
    args = parser.parse_args()
    renderer = args.renderer.resolve()
    if not renderer.is_file() or not args.ffmpeg or not args.ffprobe:
        parser.error('An actual native renderer, FFmpeg and FFprobe are required')
    if args.work_dir:
        render(renderer, args.output.resolve(), args.work_dir.resolve(), args.ffmpeg, args.ffprobe)
    else:
        with tempfile.TemporaryDirectory(prefix='surge-scoring-previews-') as temporary:
            render(renderer, args.output.resolve(), Path(temporary), args.ffmpeg, args.ffprobe)


if __name__ == '__main__':
    main()
