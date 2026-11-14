#!/usr/bin/env python3
"""Audit Scoring 1000 semantics after native save/reload, with matched audio probes.

This checks the serialized synth state, not musical taste or composer endorsement.
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
from collections import Counter
import ctypes
import ctypes.util
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'doc/scoring-1000/manifest.json'
# These flags have zero defaults in SurgePatch::load_xml / Parameter initialization.
ZERO_FLAGS = {'temposync', 'extend_range', 'absolute', 'porta_const_rate',
              'porta_gliss', 'porta_retrigger', 'porta_curve', 'deform_type'}
EMPTY_ARBITRARY = (b'\xe2\x79\x10' + b''.join(bytes([len(f'fx{i}')]) +
                   f'fx{i}'.encode() + b'\xe2\x03\x00' for i in range(16)))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def f32(value):
    value = float(value)
    if not math.isfinite(value):
        raise ValueError('nonfinite serialized value')
    return struct.unpack('<f', struct.pack('<f', value))[0]


def unpack_arbitrary(blob):
    """Only the empty 16-slot FX map is allowed for this self-contained bank."""
    if not blob:
        return b''
    try:
        import zstandard
        raw = zstandard.ZstdDecompressor().decompress(blob, max_output_size=65536)
    except ImportError:
        lib = ctypes.util.find_library('zstd')
        if not lib:
            raise ValueError('Need Python zstandard or the native libzstd to audit saved payload')
        zstd = ctypes.CDLL(lib)
        zstd.ZSTD_getFrameContentSize.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        zstd.ZSTD_getFrameContentSize.restype = ctypes.c_ulonglong
        count = zstd.ZSTD_getFrameContentSize(blob, len(blob))
        if count != len(EMPTY_ARBITRARY):
            raise ValueError(f'Unexpected arbitrary payload expansion: {count} bytes')
        out = ctypes.create_string_buffer(count)
        zstd.ZSTD_decompress.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                        ctypes.c_void_p, ctypes.c_size_t]
        zstd.ZSTD_decompress.restype = ctypes.c_size_t
        actual = zstd.ZSTD_decompress(out, count, blob, len(blob))
        if actual != count:
            raise ValueError('Unable to decompress native arbitrary payload')
        raw = out.raw
    if raw != EMPTY_ARBITRARY:
        raise ValueError('Native arbitrary payload is not the empty FX map')
    return raw


def read_patch(path):
    data = Path(path).read_bytes()
    if len(data) < 92 or data[:4] != b'CcnK' or data[8:12] != b'FPCh':
        raise ValueError('Invalid FXP header')
    if data[16:20] != b'cjs3' or data[60:64] != b'sub3':
        raise ValueError('Unexpected native plugin/chunk identity')
    if struct.unpack_from('>I', data, 56)[0] != len(data) - 60:
        raise ValueError('FXP chunk length differs from actual bytes')
    size, *wavetables = struct.unpack_from('<7I', data, 64)
    if any(wavetables) or not 0 < size < 2 * 1024 * 1024 or size + 92 > len(data):
        raise ValueError('Unexpected wavetable payload or invalid XML size')
    root = ET.fromstring(data[92:92 + size])
    if root.tag != 'patch' or root.get('revision') != '31':
        raise ValueError('Unexpected patch revision')
    serialization = root.find('serialization')
    abs_size = int(serialization.get('absSize', '0')) if serialization is not None else 0
    payload = data[92 + size:]
    if abs_size != len(payload):
        raise ValueError('Arbitrary payload size mismatch')
    unpack_arbitrary(payload)
    return root


def numeric(value):
    try:
        return f32(value)
    except (ValueError, TypeError):
        return value


def tree(node):
    """Compare attribute/text values, with float32 storage precision and order-free children."""
    if node is None:
        return None
    children = [tree(c) for c in node]
    return (node.tag, tuple(sorted((k, numeric(v)) for k, v in node.attrib.items())),
            (node.text or '').strip(), tuple(sorted(children, key=repr)))


def routes(parameter):
    result = []
    for route in parameter:
        if route.tag != 'modrouting':
            raise ValueError('Unexpected parameter child')
        attrs = dict(route.attrib)
        attrs.setdefault('muted', '0')
        attrs.setdefault('source_index', '0')
        if not parameter.tag.startswith(('a_', 'b_')):
            attrs.setdefault('source_scene', '0')
        result.append(tuple(sorted((k, f32(v)) for k, v in attrs.items())))
    return sorted(result)


def compare(source, saved):
    errors, notes = [], Counter()
    a = {p.tag: p for p in source.find('parameters')}
    b = {p.tag: p for p in saved.find('parameters')}
    if len(a) != len(source.find('parameters')) or len(b) != len(saved.find('parameters')):
        errors.append('Duplicate parameter names')
    for name, p in a.items():
        fx = re.fullmatch(r'fx(\d+)_(?:type|p\d+)', name)
        fx_off = fx and f32(a[f'fx{fx[1]}_type'].get('value')) == 0
        if name not in b:
            if fx_off and not list(p):
                notes['omitted_disabled_fx_parameter'] += 1
                continue
            errors.append(f'{name}: omitted by native save')
            continue
        q = b[name]
        if p.get('type') != q.get('type'):
            errors.append(f'{name}: type {p.get("type")} -> {q.get("type")}')
        if f32(p.get('value')) != f32(q.get('value')):
            errors.append(f'{name}: value {p.get("value")} -> {q.get("value")}')
        for key in set(p.attrib) | set(q.attrib):
            if key in ('type', 'value'):
                continue
            pa, qa = p.get(key), q.get(key)
            if key in ZERO_FLAGS:
                pa, qa = pa or '0', qa or '0'
            if pa != qa:
                errors.append(f'{name}: {key} {p.get(key)} -> {q.get(key)}')
            elif p.get(key) != q.get(key):
                notes['zero_default_flag_materialized_or_omitted'] += 1
        if routes(p) != routes(q):
            errors.append(f'{name}: modulation routing changed')
    for name in set(b) - set(a):
        # Reverb2 declares ten useful controls; generic FX serialization has twelve.
        q = b[name]
        if (name in ('fx8_p10', 'fx8_p11') and
                f32(a.get('fx8_type').get('value')) == 11 and
                q.attrib == {'type': '0', 'value': '0'} and not list(q)):
            notes['unused_reverb2_parameter_materialized'] += 1
        else:
            errors.append(f'{name}: unexpected native parameter added')
    for tag in ('meta', 'extraoscdata', 'msegs', 'formulae', 'extralfo',
                'customcontroller', 'lfobanklabels', 'modwheel', 'compatability', 'dawExtraState'):
        if tree(source.find(tag)) != tree(saved.find(tag)):
            errors.append(f'{tag}: serialized state changed')
    na, nb = source.find('nonparamconfig'), saved.find('nonparamconfig')
    da, db = {n.tag: n for n in na}, {n.tag: n for n in nb}
    for name in set(da) | set(db):
        if name == 'detachSustainPedalFromVoicing' and name not in da:
            if db[name].attrib == {'v': '0'} and not list(db[name]):
                notes['default_attached_sustain_pedal_materialized'] += 1
                continue
        if tree(da.get(name)) != tree(db.get(name)):
            errors.append(f'nonparamconfig/{name}: state changed')
    def steps(root):
        output = {}
        for seq in root.find('stepsequences'):
            attrs = dict(seq.attrib)
            for i in range(16):
                attrs.setdefault(f's{i}', '0')
            # Older collapsed mask defaults are read then replaced by optional 16-bit fields.
            mask = int(attrs.pop('trigmask', '0'))
            for i, label in enumerate(('0to15', '16to31', '32to47')):
                value = attrs.pop('trigmask_' + label, None)
                if value is not None:
                    mask = (mask & ~(65535 << (16 * i))) | ((int(value) & 65535) << (16 * i))
            attrs['mask'] = str(mask)
            identity = (attrs.get('scene'), attrs.get('i'))
            if identity in output:
                raise ValueError('Duplicate step sequence identity')
            output[identity] = tuple(sorted((k, f32(v)) for k, v in attrs.items()))
        return output
    if steps(source) != steps(saved):
        errors.append('stepsequences: step/loop/shuffle/trigger state changed')
    # Save-time metadata has documented defaults and never changes host tempo in this test.
    ta, tb = source.find('tempoOnSave'), saved.find('tempoOnSave')
    if ta is None:
        if tb is None or f32(tb.get('v')) != 120:
            errors.append('tempoOnSave: expected implicit 120 BPM default')
        else:
            notes['default_save_tempo_materialized'] += 1
    elif tree(ta) != tree(tb):
        errors.append('tempoOnSave: stored tempo changed')
    expected_tags = {n.tag for n in source} | {'serialization', 'tempoOnSave'}
    for node in saved:
        if node.tag not in expected_tags:
            errors.append(f'Unexpected top-level state: {node.tag}')
    return errors, dict(notes)


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def native_batch(renderer, rows, output, stem, roundtrip=None):
    batch = output / (stem + '.tsv')
    report = output / (stem + '.jsonl')
    log = output / (stem + '.log')
    batch.write_text(''.join('\t'.join(map(str, row)) + '\n' for row in rows))
    report.unlink(missing_ok=True)
    command = [str(renderer), '--data', str(ROOT / 'resources/data'), '--batch', str(batch),
               '--report', str(report), '--fresh-engine', '1', '--seed', '1000']
    if roundtrip:
        command += ['--roundtrip-dir', str(roundtrip)]
    with log.open('w') as stream:
        result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT)
    records = [json.loads(line) for line in report.read_text().splitlines()] if report.exists() else []
    if result.returncode or len(records) != len(rows) or any(not r.get('ok') for r in records):
        raise ValueError(f'Native batch failed ({result.returncode}); inspect {log} and {report}')
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--renderer', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--ids', help='Comma-separated preset IDs; marks report as partial')
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--audio-per-category', type=int, default=2,
                        help='Evenly spaced matched audio comparisons per category; default 2')
    parser.add_argument('--keep-audio', action='store_true')
    args = parser.parse_args()
    if args.limit < 0 or args.audio_per_category < 0:
        parser.error('Counts must be nonnegative')
    started = time.time()
    renderer, output = args.renderer.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    saved_dir = output / 'native-saved'
    saved_dir.mkdir(exist_ok=True)
    catalog_hash, renderer_hash, script_hash = sha(MANIFEST), sha(renderer), sha(__file__)
    catalog = json.loads(MANIFEST.read_text())
    entries = catalog['presets']
    if args.ids:
        wanted = set(args.ids.split(','))
        entries = [e for e in entries if e['id'] in wanted]
        if {e['id'] for e in entries} != wanted:
            parser.error('Unknown requested preset ID')
    if args.limit:
        entries = entries[:args.limit]
    if not entries:
        parser.error('No selected presets')
    source_hashes = {e['id']: sha(ROOT / e['path']) for e in entries}
    if any(source_hashes[e['id']] != e['sha256'] for e in entries):
        raise ValueError('Source preset differs from catalog hash')
    rows = [[ROOT / e['path'], output / 'serialization-probe.f32', 48000, .01, .005,
             (e['range'][0] + e['range'][1]) // 2, 96, 120] for e in entries]
    native = native_batch(renderer, rows, output, 'serialization', roundtrip=saved_dir)
    records, aggregate = [], Counter()
    for entry, rendered in zip(entries, native):
        original, saved = ROOT / entry['path'], saved_dir / Path(entry['path']).name
        errors, notes = compare(read_patch(original), read_patch(saved))
        aggregate.update(notes)
        records.append({'id': entry['id'], 'source_sha256': source_hashes[entry['id']],
                        'saved_sha256': sha(saved), 'status': 'fail' if errors else 'pass',
                        'errors': errors, 'expected_serialization_changes': notes,
                        'native_engine': rendered['engine'], 'native_git': rendered['git']})
    audio_ids = set()
    groups = {}
    for e in entries:
        groups.setdefault(e['category_id'], []).append(e)
    for group in groups.values():
        count = min(len(group), args.audio_per_category)
        for index in range(count):
            audio_ids.add(group[round(index * (len(group) - 1) / max(1, count - 1))]['id'])
    audio_entries = [e for e in entries if e['id'] in audio_ids]
    audio_rows = []
    for e in audio_entries:
        for label, path in [('original', ROOT / e['path']),
                            ('saved', saved_dir / Path(e['path']).name)]:
            audio_rows.append([path, output / f'{e["id"]}-{label}.f32', 48000, 8, 4,
                               (e['range'][0] + e['range'][1]) // 2, 96, 120])
    audio = []
    if audio_rows:
        native_batch(renderer, audio_rows, output, 'audio')
        for e in audio_entries:
            a, b = [output / f'{e["id"]}-{label}.f32' for label in ('original', 'saved')]
            equal = a.read_bytes() == b.read_bytes()
            audio.append({'id': e['id'], 'bit_identical': equal,
                          'source_audio_sha256': sha(a), 'saved_audio_sha256': sha(b),
                          'frames': a.stat().st_size // 8, 'sample_rate': 48000,
                          'hold_seconds': 4, 'seconds': 8, 'seed': 1000})
            if not args.keep_audio:
                a.unlink(); b.unlink()
    stable = (sha(MANIFEST) == catalog_hash and sha(renderer) == renderer_hash and
              sha(__file__) == script_hash and all(sha(ROOT / e['path']) == source_hashes[e['id']]
                                                  for e in entries))
    passed = stable and all(r['status'] == 'pass' for r in records) and all(a['bit_identical'] for a in audio)
    report = {'schema': 1, 'status': 'pass' if passed else 'fail',
              'scope': 'all-1000' if len(entries) == 1000 and not args.ids and not args.limit else 'partial',
              'presets_checked': len(entries), 'semantic_passes': sum(r['status'] == 'pass' for r in records),
              'audio_pairs': len(audio), 'audio_bit_identical': sum(a['bit_identical'] for a in audio),
              'catalog_sha256': catalog_hash, 'renderer_sha256': renderer_hash,
              'audit_script_sha256': script_hash, 'inputs_stable_through_run': stable,
              'expected_serialization_changes': dict(aggregate),
              'elapsed_seconds': round(time.time() - started, 3),
              'human_audition': 'not_performed', 'composer_endorsement': 'not_claimed',
              'records': records, 'audio': audio}
    write_json(output / 'summary.json', report)
    (output / 'serialization-probe.f32').unlink(missing_ok=True)
    print(json.dumps({k: v for k, v in report.items() if k not in ('records', 'audio')}, indent=2))
    return 0 if passed else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError, ET.ParseError) as error:
        print(f'Roundtrip audit failed: {error}', file=sys.stderr)
        sys.exit(2)
