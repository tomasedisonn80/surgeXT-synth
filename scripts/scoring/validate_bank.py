#!/usr/bin/env python3
"""Independently validate Scoring 1000 source or installed factory resources.

Static integrity and exact synthesis-definition checks are not audio auditions.
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import re
import struct
import sys
import tempfile
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
DATA_PREFIX = PurePosixPath('resources/data')
BANK_SUFFIX = PurePosixPath('patches_factory/Scoring 1000')
LABELS = ['Tone', 'Motion', 'Contour', 'Space', 'Color', 'Width', 'Attack', 'Release']
SELF_CONTAINED_OSCILLATORS = {0, 1, 5, 6, 8, 9}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def finite(value, what):
    try:
        number = float(value)
    except (TypeError, ValueError) as error:
        raise ValueError(f'{what} is not a number') from error
    require(math.isfinite(number), f'{what} is not finite')
    return number


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_fxp(path):
    """Parse the actual container without importing the preset compiler."""
    data = path.read_bytes()
    require(92 <= len(data) <= 2 * 1024 * 1024, 'FXP length outside bank limits')
    (magic, byte_size, kind, version, plugin, plugin_version, programs,
     program_name, chunk_size) = struct.unpack('>4sI4sI4sII28sI', data[:60])
    require((magic, kind, plugin) == (b'CcnK', b'FPCh', b'cjs3'), 'Incorrect FXP magic')
    require((version, plugin_version, programs) == (1, 1, 1), 'Incorrect FXP version/program count')
    require(byte_size in (0, len(data) - 8), 'Incorrect FXP byteSize')
    require(chunk_size == len(data) - 60, 'FXP chunkSize does not match file length')
    require(data[60:64] == b'sub3', 'Missing native sub3 patch header')
    xml_size, *wave_sizes = struct.unpack('<7I', data[64:92])
    require(not any(wave_sizes), 'Bank must not contain wavetable payload dependencies')
    require(xml_size > 0 and 92 + xml_size == len(data),
            'XML length mismatch or unexpected trailing payload')
    xml = data[92:]
    require(b'<!DOCTYPE' not in xml.upper() and b'<!ENTITY' not in xml.upper(),
            'DTD/entity declarations are not permitted')
    try:
        patch = ET.fromstring(xml)
    except ET.ParseError as error:
        raise ValueError(f'Invalid XML: {error}') from error
    require(patch.tag == 'patch' and patch.get('revision') == '31', 'Expected patch revision 31')
    require(patch.find('parameters') is not None, 'Missing parameter state')
    require(patch.find('meta') is not None, 'Missing patch metadata')
    return data, patch, program_name


def parameter_map(patch):
    params = list(patch.find('parameters'))
    result = {p.tag: p for p in params}
    require(len(params) == len(result), 'Duplicate parameter XML tags')
    require(len(params) >= 100, 'Incomplete initialized parameter state')
    for p in params:
        require(p.get('type') in ('0', '1', '2'), f'Invalid parameter type: {p.tag}')
        value = finite(p.get('value'), p.tag)
        if p.get('type') in ('0', '1'):
            require(value.is_integer(), f'Noninteger value for integral parameter {p.tag}')
        for child in p:
            require(child.tag == 'modrouting', f'Unknown parameter child {p.tag}/{child.tag}')
            require(p.get('type') == '2', f'Modulation targets nonfloat parameter {p.tag}')
            source = finite(child.get('source'), f'{p.tag} modulation source')
            require(source.is_integer() and 1 <= source <= 40, f'Invalid modulation source for {p.tag}')
            finite(child.get('depth'), f'{p.tag} modulation depth')
            require(child.get('muted', '0') in ('0', '1'), f'Invalid route mute flag for {p.tag}')
            index = finite(child.get('source_index', '0'), f'{p.tag} source index')
            require(index.is_integer() and 0 <= index <= 2, f'Invalid source index for {p.tag}')
            if 7 <= source <= 14:
                require(index == 0, f'Macro route has a nonzero source index: {p.tag}')
            if 'source_scene' in child.attrib:
                require(child.get('source_scene') in ('0', '1'), f'Invalid source scene for {p.tag}')
    return result


def scalar(params, name, fallback=None):
    if name not in params:
        if fallback is not None:
            return fallback
        raise ValueError(f'Missing parameter {name}')
    return finite(params[name].get('value'), name)


def live_routes(parameter):
    return [r for r in parameter.findall('modrouting')
            if r.get('muted', '0') == '0' and finite(r.get('depth'), 'modulation depth') != 0]


def active_keys(params):
    """Select connected scene-A synthesis state, resolving LFO modulation dependencies.

    This removes the inactive scene, muted oscillator/noise/ring inputs, disabled
    filters, unused LFOs and disabled oscillator controls. It does not purport to
    estimate perceptual distance between nonidentical definitions.
    """
    require(scalar(params, 'scenemode') == 0 and scalar(params, 'scene_active') == 0,
            'Bank contract requires single scene A')
    live_osc = {i for i in range(1, 4) if scalar(params, f'a_mute_o{i}') == 0
                and scalar(params, f'a_level_o{i}') > 0}
    require(live_osc, 'No active oscillator signal path')
    for i in live_osc:
        model = scalar(params, f'a_osc{i}_type')
        require(model in SELF_CONTAINED_OSCILLATORS, f'Oscillator {i} needs unsupported/external input')
        if model == 9:
            require(0 <= scalar(params, f'a_osc{i}_param0') <= 6,
                    f'String oscillator {i} must use a self-contained burst exciter')
    noise = scalar(params, 'a_mute_noise') == 0 and scalar(params, 'a_level_noise') > 0
    rings = {label for label in ('ring12', 'ring23') if scalar(params, f'a_mute_{label}') == 0}
    filters = {i for i in (1, 2) if scalar(params, f'a_filter{i}_type') != 0
               and params[f'a_filter{i}_type'].get('deactivated', '0') == '0'}
    require(1 in filters, 'Tone macro requires an active first filter')
    result = set()
    for name, p in params.items():
        if not name.startswith('a_') or re.match(r'a_lfo\d+_', name) or name.startswith('a_send_fx_'):
            continue
        osc = re.match(r'a_osc([123])_', name)
        mixer = re.match(r'a_(?:level|mute|solo|route)_o([123])$', name)
        if (osc and int(osc[1]) not in live_osc) or (mixer and int(mixer[1]) not in live_osc):
            continue
        if not noise and (name in ('a_noisecol', 'a_level_noise', 'a_mute_noise',
                                   'a_solo_noise', 'a_route_noise')):
            continue
        if any(name.endswith('_' + ring) and ring not in rings for ring in ('ring12', 'ring23')):
            continue
        flt = re.match(r'a_filter([12])_', name)
        if flt and int(flt[1]) not in filters:
            continue
        if not 2 in filters and name in ('a_f2_cf_is_offset', 'a_f2_link_resonance'):
            continue
        if name.startswith('a_ws_') and scalar(params, 'a_ws_type', 0) == 0:
            continue
        if name == 'a_fm_depth' and scalar(params, 'a_fm_switch', 0) == 0:
            continue
        if p.get('deactivated', '0') == '1' and not live_routes(p):
            continue
        result.add(name)
    # The filter envelope is connected by hardwired filter envmod or a route.
    filter_env = any(scalar(params, f'a_filter{i}_envmod') != 0 or
                     live_routes(params[f'a_filter{i}_envmod']) for i in filters)
    filter_env |= any(int(r.get('source')) == 16 for name in result for r in live_routes(params[name]))
    if not filter_env:
        result = {name for name in result if not name.startswith('a_env2_')}
    used_lfos = set()
    while True:
        old = len(result)
        for name in list(result):
            for route in live_routes(params[name]):
                source = int(route.get('source'))
                if 17 <= source <= 28:
                    index = source - 17
                    used_lfos.add(index)
                    result.update(k for k in params if k.startswith(f'a_lfo{index}_'))
        if len(result) == old:
            break
    return result, live_osc, used_lfos


def canonical_number(value):
    return round(finite(value, 'canonical numeric attribute'), 8)


def canonical_attrs(attrs):
    # All attributes in parameter, route and connected step/LFO state are numeric.
    # Do not silently retain NaN/Inf as text if conversion fails.
    return [(key, canonical_number(value)) for key, value in sorted(attrs.items())]


def active_signature(patch, params, keys, live_osc, used_lfos):
    """Ignore names, global gain, FX and uniform transpose when detecting duplicates."""
    state = []
    gains = {f'a_level_o{i}': scalar(params, f'a_level_o{i}') ** 3 for i in live_osc}
    total = sum(gains.values())
    pitch_origin = (12 * scalar(params, f'a_osc{min(live_osc)}_octave') +
                    scalar(params, f'a_osc{min(live_osc)}_pitch'))
    ignored_base = {'a_volume', 'a_vca_level', 'a_pitch', 'a_octave'}
    for name in sorted(keys):
        p = params[name]
        attrs = dict(p.attrib)
        routes = [canonical_attrs(r.attrib) for r in live_routes(p)]
        if name in ignored_base:
            attrs.pop('value', None)
            if not routes:
                continue
        if name in gains:
            attrs['value'] = str(gains[name] / total)
        pitch = re.fullmatch(r'a_osc([123])_pitch', name)
        octave = re.fullmatch(r'a_osc([123])_octave', name)
        if octave:
            continue
        if pitch:
            i = int(pitch[1])
            attrs['value'] = str(12 * scalar(params, f'a_osc{i}_octave') +
                                 scalar(params, name) - pitch_origin)
        state.append((name, canonical_attrs(attrs), sorted(routes)))
    for parent_name, child_name in (('stepsequences', 'sequence'), ('extralfo', 'lfo')):
        parent = patch.find(parent_name)
        if parent is not None:
            for node in parent.findall(child_name):
                if node.get('scene') == '0' and int(node.get('i', '-1')) in used_lfos:
                    state.append((parent_name, canonical_attrs(node.attrib)))
    encoded = json.dumps(state, sort_keys=True, separators=(',', ':')).encode()
    return digest(encoded)


def check_macros(patch, params, keys):
    container = patch.find('customcontroller')
    require(container is not None, 'Missing macro controller state')
    entries = container.findall('entry')
    require(len(entries) == 8, 'Expected exactly eight macro entries')
    by_index = {}
    for entry in entries:
        index = int(entry.get('i', '-1'))
        require(index not in by_index and 0 <= index < 8, 'Duplicate/invalid macro index')
        require(entry.get('label') == LABELS[index], f'Wrong label for macro {index + 1}')
        require(entry.get('bipolar') == '0', f'Macro {index + 1} must be unipolar')
        require(0 <= finite(entry.get('v'), 'macro default') <= 1, 'Invalid macro default')
        by_index[index] = entry
    targets = set(keys)
    for slot in range(1, 17):
        if scalar(params, f'fx{slot}_type', 0) != 0:
            targets.update(k for k in params if k.startswith(f'fx{slot}_p'))
    routed = {int(route.get('source')) - 7 for key in targets
              for route in live_routes(params[key]) if 7 <= int(route.get('source')) <= 14}
    require(routed == set(range(8)), f'Unrouted audible macro indices: {sorted(set(range(8)) - routed)}')
    require(scalar(params, 'fx7_type') == 1 and scalar(params, 'fx8_type') == 11,
            'Unexpected Space-controlled global effect layout')
    for target in ('fx7_p10', 'fx8_p9'):
        require(scalar(params, target) == 0, f'Space=0 cannot be dry: nonzero base {target}')
        routes = live_routes(params[target])
        require(routes and all(int(r.get('source')) == 10 for r in routes),
                f'Space=0 cannot be proven dry: {target} has other/no live routing')
        require(all(0 < finite(r.get('depth'), target) <= 1 for r in routes),
                f'Invalid Space mix depth on {target}')


def check_metadata(entry, patch, program_name):
    meta = patch.find('meta')
    name = entry['name']
    require(meta.get('name') == name, 'Manifest and native patch names differ')
    require(program_name == name.encode('ascii')[:27].ljust(28, b'\0'), 'FXP program name mismatch')
    require(meta.get('category') == 'Scoring 1000/' + entry['category'], 'Native category mismatch')
    require(meta.get('comment') == entry['comment'], 'Native comment differs from catalog')
    require(meta.get('author') and 'CC0' in meta.get('license', ''), 'Missing author/CC0 license')
    require(all(isinstance(entry.get(k), str) and entry[k].strip()
                for k in ('role', 'intent', 'family', 'variant', 'comment')), 'Incomplete musical metadata')
    bounds = entry.get('range')
    require(isinstance(bounds, list) and len(bounds) == 2 and
            all(isinstance(n, int) and not isinstance(n, bool) for n in bounds) and
            0 <= bounds[0] < bounds[1] <= 127 and bounds[1] - bounds[0] >= 24,
            'Invalid documented keyboard range')
    tags = entry.get('tags')
    require(isinstance(tags, list) and len(tags) >= 3 and
            all(isinstance(tag, str) and tag.strip() for tag in tags), 'Missing musical tags')
    tag_node = meta.find('tags')
    native_tags = set() if tag_node is None else {x.get('tag') for x in tag_node.findall('tag')}
    require(set(tags) | {'Scoring 1000', entry['category'], entry['variant']} <= native_tags,
            'Native tags do not include catalog tags')
    require(entry.get('macros') == LABELS, 'Incorrect catalog macro list')
    require(entry.get('composer_endorsement') == 'not_claimed', 'Unsubstantiated endorsement status')
    require(entry.get('musical_audition') == 'pending',
            'Static bank contract cannot substantiate an audition claim')
    amp = entry.get('amp')
    require(isinstance(amp, list) and len(amp) == 4, 'Missing envelope documentation')
    for i, value in enumerate(amp):
        finite(value, f'amp[{i}]')


def rebuild_hashes(root, entries):
    """Optional deterministic authoring check; all output stays in a temporary directory."""
    module_file = root / 'scripts/scoring/build_bank.py'
    spec = importlib.util.spec_from_file_location('scoring_bank_regeneration', module_file)
    compiler = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(compiler)
    categories = {c['id']: c for c in (json.loads(p.read_text()) for p in
                  sorted((root / 'scripts/scoring/recipes').glob('*.json')))}
    failures = []
    with tempfile.TemporaryDirectory(prefix='scoring-regeneration-') as temp:
        for entry in entries:
            try:
                category = categories[entry['category_id']]
                family = next(x for x in category['families'] if x['name'] == entry['family'])
                variant = next(x for x in category['variants'] if x['name'] == entry['variant'])
                patch, comment = compiler.make_patch(compiler.vary(family, variant), category,
                                                      entry['name'], variant)
                out = Path(temp) / 'current.fxp'
                actual = patch.save(out, entry['name'])
                require(actual == entry['sha256'], 'Generated bytes differ from manifest')
                require(comment == entry['comment'], 'Generated comment differs from manifest')
            except (KeyError, StopIteration, ValueError, AssertionError) as error:
                failures.append(f"{entry.get('id', '?')}: regeneration failed: {error}")
    return failures


def validate(root=ROOT, manifest_path=None, data_root=None, allow_partial=False, check_generated=False):
    root = Path(root).resolve()
    manifest_path = Path(manifest_path or root / 'doc/scoring-1000/manifest.json')
    data_root = Path(data_root or root / 'resources/data').resolve()
    bank = data_root / str(BANK_SUFFIX)
    errors, records = [], []
    manifest_data = manifest_path.read_bytes()
    manifest = json.loads(manifest_data)
    entries = manifest.get('presets', [])
    require(isinstance(entries, list), 'Manifest presets must be a list')
    require(manifest.get('schema') == 1 and manifest.get('patch_revision') == 31,
            'Unsupported manifest schema/patch revision')
    require(manifest.get('name') == 'Scoring 1000', 'Wrong bank identity')
    counts = Counter(e.get('category_id') for e in entries)
    complete = len(entries) == 1000 and set(counts) == {f'{n:02}' for n in range(1, 11)}
    if not allow_partial and not complete:
        errors.append(f'Expected 1000 presets in categories 01..10; found {len(entries)} in {len(counts)}')
    if not entries or len(entries) > 1000 or any(n != 100 for n in counts.values()):
        errors.append(f'Every present category must contain 100 entries: {dict(counts)}')
    if manifest.get('preset_count') != len(entries) or manifest.get('category_count') != len(counts):
        errors.append('Manifest preset/category totals disagree with entries')
    ids, names, paths = set(), set(), set()
    families = defaultdict(Counter)
    signatures = defaultdict(list)
    for entry in entries:
        ident = entry.get('id', '?')
        record = {'id': ident, 'sha256': entry.get('sha256'), 'status': 'fail'}
        try:
            require(re.fullmatch(r'SC(?:0[1-9]|10)-(?:0[0-9][0-9]|100)', ident) is not None,
                    'Invalid preset ID')
            require(ident[-3:] != '000', 'Preset number must start at 001')
            category = entry['category_id']
            require(ident[2:4] == category, 'Preset ID/category mismatch')
            require(ident not in ids, 'Duplicate preset ID')
            ids.add(ident)
            name_key = entry['name'].casefold()
            require(name_key not in names, 'Duplicate preset name')
            names.add(name_key)
            relative = PurePosixPath(entry['path'])
            require(not relative.is_absolute() and '..' not in relative.parts and '\\' not in entry['path'],
                    'Unsafe manifest path')
            suffix = relative.relative_to(DATA_PREFIX)
            local = suffix.relative_to(BANK_SUFFIX)
            require(len(local.parts) == 2 and local.parts[0] == f"{category} {entry['category']}" and
                    local.name == f"{ident} {entry['name']}.fxp", 'Unexpected factory category/file path')
            require(str(local).casefold() not in paths, 'Duplicate/case-colliding manifest path')
            paths.add(str(local).casefold())
            file_path = data_root / str(suffix)
            require(file_path.is_file() and not file_path.is_symlink(), 'Missing file or symbolic-link preset')
            require(file_path.resolve().is_relative_to(bank.resolve()), 'File escapes bank directory')
            data, patch, program_name = read_fxp(file_path)
            require(re.fullmatch('[0-9a-f]{64}', entry['sha256']) is not None, 'Invalid SHA-256 field')
            require(digest(data) == entry['sha256'], 'Preset SHA-256 differs from manifest')
            check_metadata(entry, patch, program_name)
            params = parameter_map(patch)
            keys, live_osc, used_lfos = active_keys(params)
            check_macros(patch, params, keys)
            signature = active_signature(patch, params, keys, live_osc, used_lfos)
            signatures[signature].append(ident)
            families[category][entry['family']] += 1
            record.update(status='pass', active_definition_sha256=signature,
                          active_oscillators=len(live_osc), active_lfos=len(used_lfos))
        except (ValueError, KeyError, TypeError, OverflowError, UnicodeError, OSError) as error:
            record['error'] = str(error)
            errors.append(f'{ident}: {error}')
        records.append(record)
    actual_files = [p for p in bank.rglob('*') if p.is_file() and p.suffix.casefold() == '.fxp'] \
        if bank.is_dir() else []
    actual_paths = {str(p.relative_to(bank)).casefold() for p in actual_files}
    if len(actual_files) != len(actual_paths):
        errors.append('Factory files contain case-colliding duplicate paths')
    if actual_paths != paths:
        errors.append(f'Factory file set mismatch: missing={sorted(paths - actual_paths)[:8]}, '
                      f'unlisted={sorted(actual_paths - paths)[:8]}')
    for category, family_counts in families.items():
        if len(family_counts) != 20 or any(n != 5 for n in family_counts.values()):
            errors.append(f'Category {category}: expected 20 families with five variants each')
    if manifest.get('family_count') != sum(len(c) for c in families.values()):
        errors.append('Manifest family total disagrees with validated entries')
    template = root / 'resources/data/patches_factory/Templates/Init Saw.fxp'
    if template.is_file() and digest(template.read_bytes()) != manifest.get('template_sha256'):
        errors.append('Initialized template SHA-256 differs from the manifest')
    duplicates = [group for group in signatures.values() if len(group) > 1]
    for group in duplicates:
        errors.append(f'Identical active synthesis definitions excluding gain/FX/names/transpose: {group}')
    if check_generated:
        errors.extend(rebuild_hashes(root, entries))
    return {
        'schema': 1, 'ok': not errors, 'complete_bank': complete,
        'validation_scope': 'static_definition_and_resource_integrity',
        'manifest_sha256': digest(manifest_data),
        'preset_count': len(entries), 'file_count': len(actual_files),
        'category_counts': dict(sorted(counts.items())),
        'unique_active_definitions': len(signatures), 'duplicate_groups': duplicates,
        'regeneration_checked': check_generated, 'installed_data_root_checked': str(data_root),
        'limits': ['Different parameter definitions do not establish perceptual uniqueness.',
                   'Native rendering, musical audition and composer endorsement are outside this check.'],
        'errors': errors, 'presets': records,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT, help='Repository root for manifest and recipes')
    parser.add_argument('--manifest', type=Path, help='Override manifest for independent/negative fixtures')
    parser.add_argument('--data-root', type=Path, help='Installed resource directory containing patches_factory')
    parser.add_argument('--allow-partial', action='store_true', help='Allow complete 100-preset category subsets')
    parser.add_argument('--check-generated', action='store_true', help='Rebuild only in a temporary directory')
    parser.add_argument('--output', type=Path, help='Write full per-preset JSON report here')
    args = parser.parse_args()
    try:
        report = validate(args.root, args.manifest, args.data_root, args.allow_partial, args.check_generated)
    except (OSError, ValueError, KeyError, TypeError) as error:
        report = {'schema': 1, 'ok': False, 'complete_bank': False, 'errors': [str(error)]}
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + '\n')
    concise = {key: value for key, value in report.items() if key != 'presets'}
    print(json.dumps(concise, indent=2))
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    sys.exit(main())
