#!/usr/bin/env python3
"""Write the musician-facing family catalog from the exact preset manifest.

SPDX-License-Identifier: GPL-3.0-or-later
This describes authored metadata; it does not certify musical audition.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def cell(value: str) -> str:
    return value.replace('|', '\\|').replace('\n', ' ')


def catalog_text(manifest_bytes: bytes) -> str:
    manifest = json.loads(manifest_bytes)
    presets = manifest['presets']
    if len(presets) != 1000 or len({p['id'] for p in presets}) != 1000:
        raise ValueError('The family catalog requires 1,000 uniquely identified presets')
    categories = defaultdict(list)
    for preset in presets:
        categories[preset['category_id']].append(preset)
    if len(categories) != 10:
        raise ValueError('The family catalog requires ten categories')
    if any(p['musical_audition'] != 'pending' or
           p['composer_endorsement'] != 'not_claimed' for p in presets):
        raise ValueError('Review the catalog status prose before changing audition claims')
    mode_counts = Counter(p['mono'] for p in presets)

    lines = [
        '# Scoring 1000: family catalog', '',
        'Browse **200 synthesis families and all 1,000 preset IDs** below. Each',
        'category contains 20 families, each with five variants. The roles and',
        'suggested ranges come from the same catalog used to build the presets.',
        '',
        '[Download and install](DOWNLOADS.md) · [Player and genre guide](USER_GUIDE.md) ·',
        '[Native audio previews](AUDIO_PREVIEWS.md) · [Complete machine-readable catalog](manifest.json)',
        '',
        '## How to read the tables', '',
        'The five IDs in a family follow the variant order printed above its table.',
        'For example, **SC01-001–SC01-005** means Felt Ember Close, Felt, Glass,',
        'Float and Bite, in that order. The synth may display the musical name',
        'without the filename ID. Use both category and ID to disambiguate repeated',
        'family names, such as Low Lantern in Bass and in Pulses and Sequences.',
        '',
        'Ranges are inclusive **MIDI note numbers**, avoiding different host octave',
        'labels. They are suggested starting registers, not hard key limits. **Mono**',
        'presets use a retriggering single-note mode; **Poly** presets allow chords',
        'with a saved 16-voice limit. The bank has '
        f"**{mode_counts[True]} mono and {mode_counts[False]} poly presets**.",
        'Long releases can accumulate voices; set level against the intended phrase.',
        '',
        'All sounds are synthesized, including keyboard, ensemble and vocal-like',
        'impressions. The descriptions express intended musical roles. Human musical',
        'audition remains pending, and no composer endorsement or universal genre',
        'suitability is claimed. See the player guide for the eight macros, expression',
        'mapping, tempo behavior and scene-specific adaptation.',
        '',
        '## Categories', '',
    ]
    total_families = 0
    for ident, entries in sorted(categories.items()):
        name = entries[0]['category']
        anchor = ident + '-' + name.lower().replace(' ', '-')
        lines.append(f'- [{ident} {name}](#{anchor}) — 100 presets')
    for ident, entries in sorted(categories.items()):
        if len(entries) != 100:
            raise ValueError(f'Category {ident} does not contain 100 presets')
        name = entries[0]['category']
        families = defaultdict(list)
        for entry in entries:
            families[entry['family']].append(entry)
        if len(families) != 20:
            raise ValueError(f'Category {ident} does not contain 20 families')
        total_families += len(families)
        variant_order = [p['variant'] for p in next(iter(families.values()))]
        variant_intents = {}
        lines.extend(['', f'## {ident} {name}', '',
                      '**Variant order:** ' + ' → '.join(variant_order) + '.', '',
                      '| Variant | Usual intent |', '|---|---|'])
        for variant in variant_order:
            intents = Counter(p['intent'] for p in entries if p['variant'] == variant)
            intent = intents.most_common(1)[0][0]
            variant_intents[variant] = intent
            lines.append(f'| {cell(variant)} | {cell(intent)} |')
        lines.extend(['', '| Preset IDs | Family | MIDI range | Mode | Main musical role |',
                      '|---|---|---:|---|---|'])
        role_exceptions, intent_exceptions = [], defaultdict(list)
        for family, group in families.items():
            if len(group) != 5 or [p['variant'] for p in group] != variant_order:
                raise ValueError(f'Unexpected variant order for {ident}/{family}')
            if len({tuple(p['range']) for p in group}) != 1 or len({p['mono'] for p in group}) != 1:
                raise ValueError(f'Family-specific ranges or modes require a richer table: {family}')
            first, last = group[0], group[-1]
            first_number = int(first['id'].split('-')[1])
            expected_ids = [f'SC{ident}-{first_number + offset:03d}' for offset in range(5)]
            if [p['id'] for p in group] != expected_ids:
                raise ValueError(f'Family IDs are not a consecutive five-preset range: {family}')
            role = first['role']
            mode = 'Mono' if first['mono'] else 'Poly'
            bounds = f"{first['range'][0]}–{first['range'][1]}"
            lines.append(f"| {first['id']}–{last['id']} | {cell(family)} | {bounds} | {mode} | {cell(role)} |")
            for entry in group:
                if entry['role'] != role:
                    role_exceptions.append(entry)
                if entry['intent'] != variant_intents[entry['variant']]:
                    intent_exceptions[entry['intent']].append(entry)
        if role_exceptions:
            lines.extend(['', '**Role changes within these families:**', ''])
            for entry in role_exceptions:
                lines.append(f"- **{entry['id']} {entry['name']}:** {entry['role']}")
        if intent_exceptions:
            lines.extend(['', '**Family-specific variant details:**', ''])
            for intent, group in intent_exceptions.items():
                names = '; '.join(f"{p['id']} {p['name']}" for p in group)
                lines.append(f'- **{names}:** {intent}')
        if ident == '09':
            lines.extend(['', 'These are held-note modulation patterns, not a MIDI arpeggiator. Amber',
                          'Ladder, Night Thirds, Open Fifths and Prism Arcs add programmed pitch',
                          'intervals and are intended for one played root at a time. Their Sparse',
                          'variants replace the scale pattern with root/octave accents. Triplet',
                          'means a 3:2 speed increase relative to the family rate. See the',
                          '[sequence explanation](USER_GUIDE.md#pulses-harmony-and-tempo).'])
        if ident == '10':
            lines.extend(['', 'For Dawn Sweep and Falling Arc, the pitch envelope returns toward the',
                          'played note after note-off. Attack changes the amplitude entrance;',
                          'edit the native pitch-driving envelope to fit a precise transition.',
                          'See the [transition guidance](USER_GUIDE.md#shape-transitions-and-connected-phrases).'])
    if total_families != 200:
        raise ValueError('Expected 200 category-qualified families')
    lines.extend(['', '## Catalog identity', '',
                  'This document is generated by [write_catalog.py](../../scripts/scoring/write_catalog.py)',
                  'from the exact [manifest](manifest.json). All family roles, IDs, ranges,',
                  'variant order and exceptions above derive from that file.', '',
                  'Manifest SHA-256: `' + hashlib.sha256(manifest_bytes).hexdigest() + '`.', '',
                  'Regenerate after changing the bank:', '', '```bash',
                  'python scripts/scoring/write_catalog.py', '```', ''])
    return '\n'.join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=ROOT / 'doc/scoring-1000/manifest.json')
    parser.add_argument('--output', type=Path, default=ROOT / 'doc/scoring-1000/CATALOG.md')
    args = parser.parse_args()
    before = args.manifest.read_bytes()
    result = catalog_text(before)
    if args.manifest.read_bytes() != before:
        raise RuntimeError('Manifest changed while the family catalog was generated')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result.encode('utf-8'))
    print(f'Wrote 200 families / 1,000 IDs to {args.output}')


if __name__ == '__main__':
    main()
