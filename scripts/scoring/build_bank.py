#!/usr/bin/env python3
"""Deterministically compile original scoring recipes to self-contained Surge FXP.

SPDX-License-Identifier: GPL-3.0-or-later
Native parameter values and current revision come from this repository's engine.
This compiler does not make an artistic-quality or endorsement claim.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import re
import struct
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
RECIPES = Path(__file__).parent / 'recipes'
BANK = ROOT / 'resources/data/patches_factory/Scoring 1000'
DOC = ROOT / 'doc/scoring-1000'
TEMPLATE = ROOT / 'resources/data/patches_factory/Templates/Init Saw.fxp'
TRIMS_FILE = Path(__file__).parent / 'gain_trims.json'
GAIN_TRIMS = json.loads(TRIMS_FILE.read_text())['trims'] if TRIMS_FILE.exists() else {}
MACROS = ['Tone', 'Motion', 'Contour', 'Space', 'Color', 'Width', 'Attack', 'Release']
FILTERS = {'lp12': 1, 'lp24': 2, 'hp12': 4, 'bp12': 6}
SHAPES = {'sine': 0, 'triangle': 1, 'square': 2, 'saw': 3, 'noise': 4, 'step': 7}
MODELS = {'saw': 0, 'pulse': 0, 'triangle': 8, 'sine': 1, 'fm2': 6, 'fm3': 5,
          'modern': 8, 'string': 9}


def clamp(v, lo, hi):
    return min(hi, max(lo, v))


def logtime(seconds):
    return clamp(math.log2(max(0.004, seconds)), -8, 5)


def frequency(hz):
    return clamp(12 * math.log2(hz / 440), -60, 70)


def read_patch(path):
    data = Path(path).read_bytes()
    if len(data) < 92 or data[:4] != b'CcnK' or data[8:12] != b'FPCh':
        raise ValueError(f'Invalid FXP header: {path}')
    size = struct.unpack_from('<I', data, 64)[0]
    return ET.fromstring(data[92:92 + size])


class Patch:
    def __init__(self):
        self.xml = read_patch(TEMPLATE)
        self.xml.set('revision', '31')
        self.params = self.xml.find('parameters')
        self.index = {p.tag: p for p in self.params}
        for p in self.params:
            for child in list(p):
                p.remove(child)

    def set(self, key, value, kind=None, **attrs):
        if not math.isfinite(float(value)):
            raise ValueError(f'Nonfinite parameter {key}')
        p = self.index.get(key)
        if p is None:
            p = ET.SubElement(self.params, key, type=str(kind if kind is not None else 2))
            self.index[key] = p
        if kind is not None:
            p.set('type', str(kind))
        p.set('value', str(int(value)) if p.get('type') == '0' else format(float(value), '.9g'))
        for k, v in attrs.items():
            p.set(k, str(v))
        return p

    def route(self, key, source, depth, source_index=0):
        if depth == 0:
            return
        p = self.index[key]
        if p.get('type') != '2':
            raise ValueError(f'Modulation must target a float: {key}')
        attrs = dict(source=str(source), depth=format(float(depth), '.9g'),
                     muted='0', source_index=str(source_index))
        if not key.startswith(('a_', 'b_')):
            attrs['source_scene'] = '0'
        ET.SubElement(p, 'modrouting', attrs)

    def oscillator(self, i, o):
        pre = f'a_osc{i}_'
        kind = o['kind']
        self.set(pre + 'type', MODELS[kind], 0)
        self.set(pre + 'octave', o.get('octave', 0), 0)
        self.set(pre + 'pitch', o.get('detune', 0), 2, extend_range=0)
        self.set(pre + 'keytrack', 1, 0)
        self.set(pre + 'retrigger', 1, 0)
        vals = [0., .5, .5, 0., 0., .08, 1]
        ints = {6}
        if kind == 'pulse':
            vals[0], vals[1] = 1., o.get('shape', .5)
        elif kind == 'sine':
            vals = [o.get('shape', 0), o.get('feedback', 0), 1, -60., 70., .08, 1]
            ints = {0, 2, 6}
        elif kind in ('fm2', 'fm3'):
            vals = [o.get('fm_amount', .12), o.get('ratio', 2),
                    o.get('fm_amount2', .03), o.get('ratio2', 1), 0., 0.,
                    o.get('feedback', 0)]
            ints = {1, 3} if kind == 'fm2' else set()
        elif kind in ('modern', 'triangle'):
            vals = ([0., 0., .8, .5, 0., .08, 1] if kind == 'triangle' else
                    [o.get('saw', .5), o.get('pulse', .15), o.get('tri', .2),
                     o.get('shape', .5), 0., .08, 1])
        elif kind == 'string':
            vals = [o.get('exciter', 0), .75, o.get('decay', .88),
                    o.get('decay', .88) - .03, .04, -.2, o.get('stiffness', .02)]
            ints = {0}
        if kind in ('saw', 'pulse', 'triangle', 'modern', 'sine'):
            vals[6] = o.get('unison', 1)
        for j, v in enumerate(vals):
            p = self.set(pre + f'param{j}', v, 0 if j in ints else 2)
            # No stale oscillator flags may leak from a different model.
            p.attrib = {k: p.attrib[k] for k in ('type', 'value')}
            p.set('extend_range', '0')
        if kind == 'sine':
            for j in (3, 4):
                self.index[pre + f'param{j}'].set('deactivated', '1')
        if kind in ('modern', 'triangle'):
            for j in (0, 1, 2):
                self.index[pre + f'param{j}'].set('deactivated', '0')
            self.index[pre + 'param2'].set('deform_type', '0')
        if kind == 'string':
            self.index[pre + 'param1'].set('deform_type', '34')
            self.index[pre + 'param6'].set('deform_type', '64')

    def save(self, path, name):
        xml = ET.tostring(self.xml, encoding='utf-8', xml_declaration=True)
        chunk = b'sub3' + struct.pack('<7I', len(xml), 0, 0, 0, 0, 0, 0) + xml
        header = struct.pack('>4sI4sI4sII28sI', b'CcnK', 0, b'FPCh', 1, b'cjs3',
                             1, 1, name.encode('ascii')[:27].ljust(28, b'\0'), len(chunk))
        path.parent.mkdir(parents=True, exist_ok=True)
        data = header + chunk
        path.write_bytes(data)
        return hashlib.sha256(data).hexdigest()


def vary(family, variant):
    f = copy.deepcopy(family)
    c = {**variant['changes'], **family.get('variant_changes', {}).get(variant['name'], {})}
    f['role'] = family.get('variant_roles', {}).get(variant['name'], family['role'])
    f['variant_intent'] = family.get('variant_intents', {}).get(variant['name'], variant['intent'])
    f['contour_amp'] = c.get('contour_amp', family.get('contour_amp', 0.))
    for index, key in ((0, 'attack'), (1, 'decay'), (2, 'sustain'), (3, 'release')):
        f['amp'][index] *= c.get(key + '_scale', 1)
    f['amp'][0] = clamp(f['amp'][0], .004, 3)
    f['amp'][0] = clamp(f['amp'][0], c.get('attack_min', .004), c.get('attack_max', 3))
    f['amp'][1] = clamp(f['amp'][1], .015, 6)
    f['amp'][2] = clamp(f['amp'][2], 0, 1)
    f['amp'][3] = clamp(f['amp'][3], .03, 5)
    f['filter']['hz'] = clamp(f['filter']['hz'] * c.get('cutoff_scale', 1), 80, 16000)
    f['filter']['resonance'] = clamp(f['filter'].get('resonance', .1) +
                                      c.get('resonance_add', 0), .02, .55)
    motion = f['motion']
    motion['depth'] = clamp(motion.get('depth', 3) * c.get('motion_scale', 1), 0, 18)
    motion['rate'] = clamp(motion.get('rate', .2) * c.get('motion_rate_scale', 1), .05, 8)
    motion['pitch'] = clamp(motion.get('pitch', 0) + c.get('pitch_motion_add', 0), 0, 12)
    for key in ('sync', 'steps'):
        if key in c:
            motion[key] = c[key]
    if 'motion_shape' in c:
        motion['shape'] = c['motion_shape']
    for key, default, hi in (('reverb', .1, .3), ('delay', .04, .25)):
        f[key] = clamp(f.get(key, default) * c.get('fx_scale', 1), 0, hi)
    f['noise'] = clamp(f.get('noise', 0) + c.get('noise_add', 0), 0, .08)
    for i, o in enumerate(f['oscillators']):
        o['detune'] = clamp(o.get('detune', 0) * c.get('detune_scale', 1), -.18, .18)
        if o['kind'] in ('fm2', 'fm3'):
            o['fm_amount'] = clamp(o.get('fm_amount', .12) * c.get('fm_scale', 1), .005, .7)
        if o['kind'] == 'string':
            o['decay'] = clamp(o.get('decay', .88) + c.get('string_decay_add', 0), .05, .985)
        if i == 1:
            o['level'] = clamp(o['level'] * c.get('second_level_scale', 1), .02, 1)
            if 'osc2_octave' in c:
                o['octave'] = c['osc2_octave']
    return f


def make_patch(f, category, name, variant):
    p = Patch()
    role = f['role']
    comment = (f"{role} Range MIDI {f['range'][0]}-{f['range'][1]}. "
               f"{f.get('variant_intent', variant['intent'])} Wheel: tone/motion; pressure: vibrato. "
               'Macros: Tone, Motion, Contour, Space, Color, Width, Attack, Release.')
    meta = p.xml.find('meta')
    meta.attrib = dict(name=name, category=f"Scoring 1000/{category['name']}",
                       comment=comment, author='Scoring 1000 Project',
                       license='CC0-1.0; original recipes; initialized from Surge CC0 Init Saw')
    tags = meta.find('tags')
    tags.clear()
    for tag in ['Scoring 1000', category['name'], *f['tags'], variant['name']]:
        ET.SubElement(tags, 'tag', tag=tag)
    # Use unity as an explicit measurement reference and keep protective gain
    # staging inside the saved voice path. User automation can still raise gain.
    p.set('volume', 0.)
    p.set('scenemode', 0, 0)
    p.set('polylimit', 16, 0)
    p.set('scene_active', 0, 0)
    p.set('fx_disable', 0, 0)
    p.set('fx_bypass', 0, 0)
    p.set('a_polymode', 1 if f.get('mono', False) else 0, 0)
    p.set('a_portamento', logtime(f.get('portamento', .004)))
    p.set('a_volume', .95)
    trim = float(GAIN_TRIMS.get(name, {}).get('db', 0.))
    if not math.isfinite(trim) or not -24 <= trim <= 18:
        raise ValueError(f'Unsafe gain trim for {name}: {trim}')
    p.set('a_vca_level', -6. + trim)
    p.set('a_vca_velsense', -abs(f.get('velocity', 14.)))
    p.set('a_drift', f.get('drift', .025))
    p.set('a_pan', 0.)
    p.set('a_pan2', clamp(f.get('width', .6), .1, 1.))
    p.set('a_lowcut', frequency(f.get('highpass', 25)), deactivated=0)
    p.set('a_fb_config', 7, 0)
    p.set('a_f_balance', -1.)
    p.set('a_fm_switch', 0, 0)
    p.set('a_feedback', 0.)
    level_sum = max(1., sum(o['level'] for o in f['oscillators']))
    for i in range(1, 4):
        p.set(f'a_mute_o{i}', int(i > len(f['oscillators'])), 0)
        p.set(f'a_solo_o{i}', 0, 0)
        p.set(f'a_route_o{i}', 1, 0)
        if i <= len(f['oscillators']):
            o = f['oscillators'][i - 1]
            p.oscillator(i, o)
            # Surge's mixer stores cube roots, not linear amplitudes (DSPUtils.h).
            p.set(f'a_level_o{i}', (o['level'] * .9 / level_sum) ** (1. / 3.))
    for label in ('ring12', 'ring23'):
        p.set(f'a_mute_{label}', 1, 0)
    p.set('a_mute_noise', int(f.get('noise', 0) <= 0), 0)
    p.set('a_level_noise', f.get('noise', 0) ** (1. / 3.))
    p.set('a_noisecol', f.get('noise_color', -.4))
    p.set('a_route_noise', 1, 0)
    flt = f['filter']
    p.set('a_filter1_type', FILTERS[flt['kind']], 0)
    p.set('a_filter1_subtype', 0, 0)
    p.set('a_filter1_cutoff', frequency(flt['hz']))
    p.set('a_filter1_resonance', flt.get('resonance', .1))
    p.set('a_filter1_envmod', flt.get('env', 0))
    p.set('a_filter1_keytrack', flt.get('keytrack', .5))
    p.set('a_filter2_type', 0, 0)
    for num, env in ((1, f['amp']), (2, f.get('filter_env', [.006, .6, .1, .4]))):
        for index, key in enumerate(('attack', 'decay', 'sustain', 'release')):
            p.set(f'a_env{num}_{key}', env[index] if key == 'sustain' else logtime(env[index]))
        p.set(f'a_env{num}_mode', 1, 0)
    m = f['motion']
    p.set('a_lfo0_shape', SHAPES[m.get('shape', 'sine')], 0)
    p.set('a_lfo0_rate', math.log2(m.get('rate', .2)), temposync=int(m.get('sync', False)))
    p.set('a_lfo0_magnitude', .35)
    p.set('a_lfo0_trigmode', 0, 0)
    p.set('a_lfo0_deform', 0.)
    p.set('a_lfo0_phase', 0.)
    p.set('a_lfo0_attack', -8.)
    p.set('a_lfo0_release', -2.)
    p.set('a_lfo0_unipolar', 0, 0)
    p.route('a_filter1_cutoff', 17, m.get('depth', 3))
    p.route('a_vca_level', 17, m.get('amp', 0))
    p.route('a_pan', 17, m.get('pan', 0))
    # Raw step output keeps programmed intervals in tune while Motion adjusts timbre.
    p.route('a_pitch', 17, m.get('pitch', 0), source_index=1 if m.get('shape') == 'step' else 0)
    if m.get('shape') == 'step':
        seq = ET.SubElement(p.xml.find('stepsequences'), 'sequence', scene='0', i='0',
                            loop_start='0', loop_end='15', shuffle='0', trigmask='0')
        steps = m.get('steps', [1, -.7, .2, -.5] * 4)
        if len(steps) != 16:
            raise ValueError('Step sequencer requires 16 steps')
        for i, v in enumerate(steps):
            seq.set(f's{i}', format(v, '.8g'))
    # A zero-default companion preserves authored subtle motion at the saved
    # setting. Motion adds tremolo/pan even when an open filter has little effect.
    for key, source in list(p.index.items()):
        if key.startswith('a_lfo0_'):
            target = 'a_lfo2_' + key[len('a_lfo0_'):]
            dest = p.set(target, float(source.get('value')), int(source.get('type')))
            dest.attrib = dict(source.attrib)
    p.set('a_lfo2_magnitude', 0.)
    if m.get('shape') == 'noise':
        # Native Noise at phase zero initializes an empty interpolation history.
        # A populated companion history makes short-note Motion responsive while
        # retaining irregular movement and the authored main LFO's saved behavior.
        p.set('a_lfo2_phase', .5)
    if m.get('shape') == 'step':
        companion = copy.deepcopy(seq)
        companion.set('i', '2')
        p.xml.find('stepsequences').append(companion)
    p.route('a_vca_level', 19, 1.5)
    p.route('a_pan', 19, .12)
    p.set('a_lfo1_shape', 0, 0)
    p.set('a_lfo1_rate', math.log2(5.2))
    p.set('a_lfo1_magnitude', 0.)
    p.set('a_lfo1_trigmode', 0, 0)
    p.route('a_lfo1_magnitude', 4, .7)
    p.route('a_pitch', 18, .2)
    p.route('a_pitch', 16, f.get('pitch_drop', 0))
    # Common serial global effects, fully self-contained and conservative feedback.
    for slot in range(1, 17):
        p.set(f'fx{slot}_type', 0, 0)
    p.set('fx7_type', 1, 0)
    delay = [-2., math.log2(.375), .22, .10, frequency(170), frequency(6500),
             -2., .015, 0., 0., 0., -2.]
    for j, val in enumerate(delay):
        p.set(f'fx7_p{j}', val, 0 if j == 9 else 2)
    p.index['fx7_p0'].set('temposync', '1')
    p.index['fx7_p1'].set('temposync', '1')
    p.index['fx7_p1'].set('deactivated', '0')
    p.index['fx7_p2'].set('deform_type', '1')
    for j in (4, 5):
        p.index[f'fx7_p{j}'].set('deactivated', '0')
    p.set('fx8_type', 11, 0)
    reverb = [-6., .1, math.log2(f.get('reverb_time', 1.5)), .85, .8, .25,
              .5, .55, -1., 0.]
    for j, val in enumerate(reverb):
        p.set(f'fx8_p{j}', val, 2)
    for entry, label in zip(p.xml.find('customcontroller'), MACROS):
        entry.set('label', label)
        entry.set('v', '0')
        entry.set('bipolar', '0')
        if label == 'Space':
            entry.set('v', '.4')
    p.route('a_filter1_cutoff', 7, 18 if flt['kind'] != 'hp12' else -15)
    p.route('a_lfo0_magnitude', 8, .6)
    p.route('a_lfo2_magnitude', 8, .6)
    if not m.get('sync', False):
        p.route('a_lfo0_rate', 8, 2.)
        p.route('a_lfo2_rate', 8, 2.)
    p.route('a_filter1_envmod', 9, 15)
    p.route('a_env2_decay', 9, 1.25)
    p.route('a_env1_decay', 9, f.get('contour_amp', 0.))
    if f.get('contour_amp', 0.):
        p.route('a_env1_sustain', 9, -min(.12, f['amp'][2]))
    # Space=0 is genuinely dry. A saved .4 recovers the recipe's intended ambience.
    p.route('fx8_p9', 10, max(.02, f.get('reverb', .1)) / .4)
    p.route('fx7_p10', 10, max(.008, f.get('delay', .04)) / .4)
    first = f['oscillators'][0]['kind']
    color = {'saw': (0, .55), 'pulse': (1, .22), 'triangle': (0, .5),
             'modern': (1, .45), 'sine': (1, .25), 'fm2': (0, .22),
             'fm3': (0, .22), 'string': (6, .18)}
    idx, dep = color[first]
    p.route(f'a_osc1_param{idx}', 11, dep)
    p.route(f'a_osc1_param{idx}', 7, dep * .25)
    p.route('a_pan2', 12, .5)
    p.route('fx8_p8', 12, 6.)
    p.route('fx7_p11', 12, 6.)
    p.route('a_env1_attack', 13, 2.8)
    p.route('a_env1_release', 14, 2.2)
    if f['amp'][2] < .3:
        p.route('a_env1_decay', 14, 1.5)
        for i, osc in enumerate(f['oscillators'], 1):
            if osc['kind'] == 'string':
                delta = max(0., min(.065, .985 - osc.get('decay', .88)))
                p.route(f'a_osc{i}_param2', 14, delta)
                p.route(f'a_osc{i}_param3', 14, delta)
    p.route('a_filter1_cutoff', 6, 9 if flt['kind'] != 'hp12' else -9)
    p.route('a_lfo0_magnitude', 6, .15)
    p.route('a_lfo2_magnitude', 6, .15)
    p.route('a_filter1_cutoff', 1, 6.)
    return p, comment


def build(only=None):
    DOC.mkdir(parents=True, exist_ok=True)
    categories = [json.loads(p.read_text()) for p in sorted(RECIPES.glob('*.json'))]
    if only:
        categories = [c for c in categories if c['id'] == only]
    entries = []
    names = set()
    for cat in categories:
        assert len(cat['families']) == 20 and len(cat['variants']) == 5, cat['name']
        number = 0
        folder = f"{cat['id']} {cat['name']}"
        expected = set()
        for family in cat['families']:
            for variant in cat['variants']:
                number += 1
                ident = f"SC{cat['id']}-{number:03d}"
                name = f"{family['name']} {variant['name']}"
                assert name not in names, name
                assert re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 _-]+', name), name
                names.add(name)
                f = vary(family, variant)
                patch, comment = make_patch(f, cat, name, variant)
                relative = Path(folder) / f'{ident} {name}.fxp'
                digest = patch.save(BANK / relative, name)
                expected.add(relative.name)
                entries.append(dict(id=ident, name=name, category=cat['name'],
                    category_id=cat['id'], family=family['name'], variant=variant['name'],
                    path=str((BANK / relative).relative_to(ROOT)), sha256=digest,
                    role=f['role'], range=f['range'], tags=f['tags'],
                    intent=f['variant_intent'], comment=comment,
                    mono=f.get('mono', False), amp=f['amp'],
                    gain_trim_db=float(GAIN_TRIMS.get(name, {}).get('db', 0.)),
                    tempo_sync=bool(f['motion'].get('sync', False)),
                    macros=MACROS, musical_audition='pending',
                    composer_endorsement='not_claimed'))
        for stale in (BANK / folder).glob('*.fxp'):
            if stale.name not in expected:
                stale.unlink()
    if only:
        print(f'Built {len(entries)} patches in category {only}; full catalog unchanged')
        return entries
    manifest = dict(schema=1, name='Scoring 1000', preset_count=len(entries),
                    category_count=len(categories), family_count=sum(len(c['families']) for c in categories),
                    patch_revision=31, license='CC0-1.0',
                    scope='Original synthesis; 200 families with five functional variants each. '
                          'No sampled acoustic realism or composer endorsement is claimed.',
                    template_sha256=hashlib.sha256(TEMPLATE.read_bytes()).hexdigest(),
                    presets=entries)
    (DOC / 'manifest.json').write_bytes(
        (json.dumps(manifest, indent=2, ensure_ascii=True) + '\n').encode('utf-8'))
    from write_package_index import write_package_index
    write_package_index(DOC / 'manifest.json')
    print(f'Built {len(entries)} presets / {len(categories)} categories')
    return entries


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--category', help='Compile one category without replacing the full catalog')
    args = parser.parse_args()
    build(args.category)
