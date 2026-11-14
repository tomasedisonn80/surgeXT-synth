#!/usr/bin/env python3
"""Audit measured Scoring 1000 diversity without inventing a perceptual score.

SPDX-License-Identifier: GPL-3.0-or-later

Consumes the native QA runner's hash-bound JSON evidence, never regenerates or
changes sound files. Exact PCM collisions are actionable; different hashes and
small/large feature distances do not establish musical quality or uniqueness.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import copy
import csv
from datetime import datetime, timezone
import hashlib
import itertools
import json
import math
from pathlib import Path
import re
import sys

import numpy as np

import build_bank
import qa_bank

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'doc/scoring-1000/manifest.json'
MATCH_KEYS = ('notes', 'hold', 'sample_rate', 'velocity', 'tempo', 'macros',
              'modwheel', 'pressure', 'expression')
SHARED_BINDINGS = ('renderer_sha256', 'qa_script_sha256', 'config_sha256', 'numpy_version')


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')
    temporary.replace(path)


def recipe_sound_state(value):
    """Exclude explanatory and override metadata from expanded-recipe ablation."""
    metadata = {'name', 'role', 'tags', 'range', 'variant_intent',
                'variant_changes', 'variant_roles', 'variant_intents'}
    return {k: v for k, v in value.items() if k not in metadata}


def inactive_transforms():
    """List exact no-ops in expanded recipe state, not a claim about DSP audibility."""
    result, hashes = {}, {}
    for path in sorted((ROOT / 'scripts/scoring/recipes').glob('*.json')):
        hashes[str(path.relative_to(ROOT))] = sha256(path)
        category = json.loads(path.read_text())
        for family in category['families']:
            for variant in category['variants']:
                expanded = recipe_sound_state(build_bank.vary(family, variant))
                changes = {**variant['changes'],
                           **family.get('variant_changes', {}).get(variant['name'], {})}
                ineffective = []
                for key in changes:
                    fam, var = copy.deepcopy(family), copy.deepcopy(variant)
                    var['changes'].pop(key, None)
                    fam.get('variant_changes', {}).get(var['name'], {}).pop(key, None)
                    ablated = recipe_sound_state(build_bank.vary(fam, var))
                    if expanded == ablated:
                        ineffective.append(key)
                result[(category['id'], family['name'], variant['name'])] = sorted(ineffective)
    return result, hashes


def validated_entry(entry, path, shared, renderer_hash):
    """Reject stale or malformed evidence before it enters the comparison matrix."""
    original = path.read_bytes()
    report = json.loads(original)
    ident = entry['id']
    for key in ('id', 'name', 'family', 'variant', 'category', 'category_id'):
        if report.get(key) != entry.get(key):
            raise ValueError(f'{ident}: report {key} disagrees with catalog')
    patch = (ROOT / entry['path']).resolve()
    if not patch.is_relative_to(ROOT) or sha256(patch) != entry['sha256']:
        raise ValueError(f'{ident}: preset file does not match catalog hash')
    binding = report['binding']
    if binding.get('patch_sha256') != entry['sha256']:
        raise ValueError(f'{ident}: stale QA preset hash')
    if binding.get('preset_definition_sha256') != digest(entry):
        raise ValueError(f'{ident}: stale QA catalog-entry binding')
    if binding.get('qa_script_sha256') != sha256(Path(qa_bank.__file__)):
        raise ValueError(f'{ident}: QA script hash differs from current measurement protocol')
    if binding.get('config_sha256') != digest(qa_bank.CONFIG):
        raise ValueError(f'{ident}: QA configuration hash differs from current protocol')
    if renderer_hash is not None and binding.get('renderer_sha256') != renderer_hash:
        raise ValueError(f'{ident}: renderer hash differs from supplied executable')
    for key in SHARED_BINDINGS:
        if not binding.get(key):
            raise ValueError(f'{ident}: missing evidence binding {key}')
        if shared and binding[key] != shared[key]:
            raise ValueError(f'{ident}: mixed {key} in measured input set')
    expected = qa_bank.fixtures(entry)
    specs = [row['fixture'] for row in report['scenarios']]
    if specs != expected or binding.get('fixture_sha256') != digest(specs):
        raise ValueError(f'{ident}: fixture identity or protocol binding is stale')
    defaults = [row for row in report['scenarios'] if row['fixture']['key'] == 'default']
    if len(defaults) != 1:
        raise ValueError(f'{ident}: expected exactly one default render')
    row, fixture = defaults[0], defaults[0]['fixture']
    metrics, native = row['metrics'], row['native']
    if not native.get('ok') or native.get('name') != entry['name']:
        raise ValueError(f'{ident}: native default render is invalid')
    if metrics['nonfinite_samples'] or native.get('nonfinite', 0):
        raise ValueError(f'{ident}: nonfinite default audio cannot enter diversity comparisons')
    expected_frames = round(fixture['seconds'] * fixture['sample_rate'])
    if metrics['frames'] != expected_frames or native['frames'] != expected_frames:
        raise ValueError(f'{ident}: inconsistent default frame count')
    if native['notes'] != ','.join(map(str, fixture['notes'])):
        raise ValueError(f'{ident}: native note identity differs from default fixture')
    for field in ('sample_rate', 'hold', 'velocity', 'tempo', 'macros',
                  'modwheel', 'pressure', 'expression'):
        if native[field] != fixture[field]:
            raise ValueError(f'{ident}: native {field} differs from default fixture')
    if native.get('channels') != 2 or native.get('format') != 'f32le-interleaved':
        raise ValueError(f'{ident}: unexpected native PCM format')
    if not re.fullmatch(r'[0-9a-f]{64}', metrics.get('pcm_sha256', '')):
        raise ValueError(f'{ident}: invalid PCM fingerprint')
    vector = np.asarray(metrics['spectral_band_energy'] + metrics['normalized_active_envelope'], dtype=float)
    nband = len(qa_bank.CONFIG['spectral_band_edges_hz']) - 1
    if vector.shape != (nband + qa_bank.CONFIG['rms_envelope_bins'],) or not np.isfinite(vector).all():
        raise ValueError(f'{ident}: malformed feature vector')
    if (vector < 0).any() or abs(float(vector[:nband].sum()) - 1) > .0001:
        raise ValueError(f'{ident}: invalid normalized spectral energy')
    for field in ('peak', 'rms', 'active_rms', 'spectral_centroid_hz'):
        if not math.isfinite(metrics[field]) or metrics[field] < 0:
            raise ValueError(f'{ident}: invalid metric {field}')
    return {
        'id': ident, 'name': entry['name'], 'family': entry['family'],
        'category_id': entry['category_id'], 'category': entry['category'],
        'variant': entry['variant'], 'fixture': fixture, 'metrics': metrics,
        'vector': vector, 'qa_status': report['status'],
        'qa_report_sha256': hashlib.sha256(original).hexdigest(),
        'patch_sha256': entry['sha256'], 'binding': binding,
        'default_repeat': report.get('default_repeat', {}),
    }


def pair(a, b, distance=None):
    nband = len(qa_bank.CONFIG['spectral_band_edges_hz']) - 1
    spectral = float(np.linalg.norm(a['vector'][:nband] - b['vector'][:nband]))
    envelope = float(np.linalg.norm(a['vector'][nband:] - b['vector'][nband:]))
    matching = {key: a['fixture'][key] == b['fixture'][key] for key in MATCH_KEYS}
    return {
        'id_a': a['id'], 'id_b': b['id'], 'name_a': a['name'], 'name_b': b['name'],
        'family_a': a['family'], 'family_b': b['family'],
        'category_a': a['category_id'], 'category_b': b['category_id'],
        'same_family': (a['category_id'], a['family']) == (b['category_id'], b['family']),
        'feature_distance': spectral + .5 * envelope if distance is None else float(distance),
        'spectral_distance': spectral, 'envelope_distance': envelope,
        'matched_active_fixture': all(matching.values()),
        'same_played_notes': matching['notes'], 'same_hold_seconds': matching['hold'],
        'notes_a': a['fixture']['notes'], 'notes_b': b['fixture']['notes'],
        'hold_a': a['fixture']['hold'], 'hold_b': b['fixture']['hold'],
        'render_seconds_a': a['fixture']['seconds'], 'render_seconds_b': b['fixture']['seconds'],
        'same_full_fixture': a['fixture'] == b['fixture'],
        'exact_default_pcm_equal': a['metrics']['pcm_sha256'] == b['metrics']['pcm_sha256'],
        'peak_a': a['metrics']['peak'], 'peak_b': b['metrics']['peak'],
        'active_rms_a': a['metrics']['active_rms'], 'active_rms_b': b['metrics']['active_rms'],
        'active_rms_ratio_db': 20 * math.log10(max(b['metrics']['active_rms'], 1e-15) /
                                            max(a['metrics']['active_rms'], 1e-15)),
        'centroid_hz_a': a['metrics']['spectral_centroid_hz'],
        'centroid_hz_b': b['metrics']['spectral_centroid_hz'],
        'inactive_recipe_transforms_a': a.get('inactive_recipe_transforms', []),
        'inactive_recipe_transforms_b': b.get('inactive_recipe_transforms', []),
        'repeat_feature_distance_a': a.get('default_repeat', {}).get('spectral_envelope_feature_distance'),
        'repeat_feature_distance_b': b.get('default_repeat', {}).get('spectral_envelope_feature_distance'),
    }


def write_csv(path, rows):
    with path.open('w', newline='') as f:
        if not rows:
            return
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        for row in rows:
            writer.writerow({k: json.dumps(v, separators=(',', ':')) if isinstance(v, (list, dict)) else v
                             for k, v in row.items()})


def closest_unique(rows, limit):
    seen, selected = set(), []
    for row in sorted(rows, key=lambda p: (p['feature_distance'], p['id_a'], p['id_b'])):
        identity = tuple(sorted((row['id_a'], row['id_b'])))
        if identity not in seen:
            seen.add(identity)
            selected.append(row)
            if len(selected) == limit:
                break
    return selected


def analyze(args):
    manifest_bytes = args.manifest.read_bytes()
    catalog = json.loads(manifest_bytes)
    entries = catalog['presets']
    expected_ids = {e['id'] for e in entries}
    if len(entries) != 1000 or len(expected_ids) != 1000 or catalog.get('family_count') != 200:
        raise ValueError('The complete catalog must contain 1000 distinct presets and 200 families')
    if Counter(e['category_id'] for e in entries) != Counter({f'{i:02d}': 100 for i in range(1, 11)}):
        raise ValueError('The catalog must have ten categories of 100')
    catalog_families = Counter((e['category_id'], e['family']) for e in entries)
    if len(catalog_families) != 200 or set(catalog_families.values()) != {5}:
        raise ValueError('The catalog must have exactly200 families of five presets')
    files = {p.stem: p for p in (args.qa_dir / 'presets').glob('SC*.json')}
    missing, extra = sorted(expected_ids - files.keys()), sorted(files.keys() - expected_ids)
    errors = [f'Unexpected QA report IDs: {extra}'] if extra else []
    if missing and not args.allow_partial:
        errors.append(f'Complete audit requires all1000 QA reports; {len(missing)} are missing')
    annotation, recipe_hashes = inactive_transforms()
    renderer_hash = sha256(args.renderer) if args.renderer else None
    measured, shared = [], {}
    for entry in entries:
        if entry['id'] not in files:
            continue
        try:
            record = validated_entry(entry, files[entry['id']], shared, renderer_hash)
            if not shared:
                shared = {key: record['binding'][key] for key in SHARED_BINDINGS}
            record['inactive_recipe_transforms'] = annotation[(entry['category_id'], entry['family'], entry['variant'])]
            measured.append(record)
        except (KeyError, ValueError, OSError, TypeError) as exc:
            errors.append(str(exc))
    if args.manifest.read_bytes() != manifest_bytes:
        errors.append('Catalog changed while evidence was being read; rerun against stable inputs')
    if not measured:
        errors.append('No valid native default evidence available')
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    family_groups, pcm_groups = defaultdict(list), defaultdict(list)
    for record in measured:
        family_groups[(record['category_id'], record['family'])].append(record)
        pcm_groups[record['metrics']['pcm_sha256']].append(record['id'])
    collisions = [ids for ids in pcm_groups.values() if len(ids) > 1]
    within = [pair(a, b) for group in family_groups.values() for a, b in itertools.combinations(group, 2)]
    within.sort(key=lambda p: (p['feature_distance'], p['id_a'], p['id_b']))
    neighbors, matched_neighbors = [], []
    if len(measured) > 1:
        nband = len(qa_bank.CONFIG['spectral_band_edges_hz']) - 1
        vectors = np.stack([r['vector'] for r in measured])
        for i, record in enumerate(measured):
            differences = vectors - vectors[i]
            distances = np.linalg.norm(differences[:, :nband], axis=1) + .5 * np.linalg.norm(differences[:, nband:], axis=1)
            distances[i] = np.inf
            j = int(np.argmin(distances))
            neighbors.append(pair(record, measured[j], distances[j]))
            # Cross-family matching excludes note and held-window differences;
            # stereo phase, time-varying spectra and perceptual context remain unmodeled.
            candidates = [k for k, other in enumerate(measured)
                          if (other['category_id'], other['family']) != (record['category_id'], record['family'])
                          and all(other['fixture'][key] == record['fixture'][key] for key in MATCH_KEYS)]
            if candidates:
                j = min(candidates, key=lambda k: distances[k])
                matched_neighbors.append(pair(record, measured[j], distances[j]))
    compact = [{k: r[k] for k in ('id', 'name', 'family', 'category_id', 'variant', 'qa_status',
                                   'qa_report_sha256', 'patch_sha256', 'inactive_recipe_transforms')}
               | {key: r['metrics'][key] for key in ('peak', 'rms', 'active_rms', 'spectral_centroid_hz',
                                                    'pcm_sha256', 'spectral_band_energy', 'normalized_active_envelope')}
               | {'notes': r['fixture']['notes'], 'hold_seconds': r['fixture']['hold'],
                  'render_seconds': r['fixture']['seconds'], 'default_repeat': r['default_repeat']}
               for r in measured]
    qa_failures = [r['id'] for r in measured if r['qa_status'] != 'pass']
    summary = {
        'schema': 1, 'created_utc': datetime.now(timezone.utc).isoformat(),
        'status': ('invalid' if errors else 'blocked_by_qa_failure' if qa_failures
                   else 'review_required' if collisions else 'partial' if missing else 'complete'),
        'scope': 'Independent comparison of native QA default metrics; no artistic certification.',
        'manifest_sha256': hashlib.sha256(manifest_bytes).hexdigest(),
        'audit_script_sha256': sha256(Path(__file__)),
        'compiler_sha256': sha256(Path(build_bank.__file__)), 'recipe_sha256': recipe_hashes,
        'measurement_bindings': shared, 'expected_presets': 1000, 'validated_presets': len(measured),
        'missing_ids': missing, 'errors': errors, 'input_qa_failure_ids': qa_failures,
        'families_observed': len(family_groups),
        'complete_families': sum(len(g) == 5 for g in family_groups.values()),
        'within_family_pairs': len(within),
        'within_family_pairs_matched_active_fixture': sum(p['matched_active_fixture'] for p in within),
        'within_family_pairs_with_timing_or_pitch_confound': sum(not p['matched_active_fixture'] for p in within),
        'unique_default_pcm_hashes': len(pcm_groups), 'exact_default_pcm_collision_groups': collisions,
        'requires_exact_collision_review': bool(collisions),
        'nonidentical_default_repeat_ids': [r['id'] for r in measured
                                           if not r['default_repeat'].get('bit_identical', False)],
        'distance_definition': 'L2 distance of ten spectral energy fractions plus0.5 times L2 distance of twelve peak-normalized active RMS bins. It is a transparent ranking heuristic, not a calibrated perceptual distance.',
        'closest_within_family_matched_pairs': [p for p in within if p['matched_active_fixture']][:args.top],
        'closest_cross_family_matched_pairs': closest_unique(matched_neighbors, args.top),
        'closest_bankwide_pairs_with_possible_pitch_confound': closest_unique(neighbors, args.top),
        'limits': [
            'Spectral bands and envelope bins discard detail. Low distance is an audition priority, not proof of duplication; high distance is not proof of useful variation.',
            'Bank-wide nearest neighbors can be affected by played-note and adaptive-hold differences; matching flags are explicit. Even equal MIDI notes can produce different oscillator registers.',
            'Full PCM hashes include render length and release-tail length. Different hashes can arise solely from duration, gain, phase or tiny numerical differences.',
            'Normalized features reduce overall gain sensitivity but do not remove changes in envelopes, phase, effects, velocity response or time-varying spectra.',
            'Inactive transform annotations compare expanded recipe state, not native DSP ablation. Numerically active transformations may still be perceptually weak.',
            'The audit validates stored metric provenance and structure; it does not independently recompute spectra from deleted PCM.',
            'Human musical audition remains pending; no composer endorsement is claimed.',
        ],
        'musical_audition': 'pending', 'composer_endorsement': 'not_claimed',
    }
    write_json(output / 'summary.json', summary)
    write_json(output / 'preset_features.json', compact)
    write_csv(output / 'preset_features.csv', compact)
    write_csv(output / 'within_family_pairs.csv', within)
    write_csv(output / 'nearest_neighbors.csv', neighbors)
    write_csv(output / 'cross_family_matched_neighbors.csv', matched_neighbors)
    print(json.dumps({key: summary[key] for key in ('status', 'validated_presets', 'complete_families',
                     'within_family_pairs', 'within_family_pairs_matched_active_fixture',
                     'unique_default_pcm_hashes', 'exact_default_pcm_collision_groups', 'errors')}, indent=2))
    return 1 if errors or qa_failures else (2 if collisions else 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qa-dir', required=True, type=Path, help='Directory containing qa_bank.py presets/*.json')
    parser.add_argument('--output', required=True, type=Path, help='JSON/CSV analysis output directory')
    parser.add_argument('--manifest', type=Path, default=MANIFEST)
    parser.add_argument('--renderer', type=Path, help='Optional exact executable for additional binary hash validation')
    parser.add_argument('--allow-partial', action='store_true', help='Permit missing reports only; invalid/stale evidence still fails')
    parser.add_argument('--top', type=int, default=20, help='Number of ranked pairs to include in summary')
    args = parser.parse_args()
    if args.top < 1:
        parser.error('--top must be positive')
    try:
        return analyze(args)
    except (ValueError, OSError, KeyError, TypeError) as exc:
        print(f'Diversity audit failed: {type(exc).__name__}: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
