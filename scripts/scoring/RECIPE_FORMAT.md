# Scoring 1000 recipe contract

The bank is 10 categories × 20 authored families × 5 functional variations.
Families are original sound designs, not edits of third-party presets. The
CC0 Init Saw factory file is used only as a complete initialized state container.
Never imply that numerical testing establishes composer endorsement or acoustic
instrument realism. All orchestral, piano and voice descriptions are synthetic
evocations. The bank is a versatile collection; every individual sound is not
appropriate in every musical context.

Each category JSON in `scripts/scoring/recipes/` has `id` (01..10), `name`,
`prefix` (K/P/B/A/E/D/S/L/Q/T), `description`, `families` (exactly 20), and
`variants` (exactly 5). Give each family an original name of at most 18 ASCII
characters. Variant names should be at most 8 characters. No artist/film/product
names in presets. Author ONLY your assigned category file.

## Family schema

Required fields: `name`, `role` (specific musical use, one sentence), `range`
(two inclusive MIDI note numbers, at least 24 semitones), `tags` (3–6 descriptive
words), `oscillators` (1–3 objects), `amp` ([attack seconds, decay seconds,
sustain 0..1, release seconds]), `filter` (object below), `motion` (object below).

Oscillator object:

- `kind`: `saw`, `pulse`, `triangle`, `sine`, `fm2`, `fm3`, `modern`, `string`.
- `level`: linear 0.05..1.0; mixer is internally gain staged.
- `octave`: integer -2..2. `detune`: semitones -0.18..0.18 (keep tuned designs
  near +/-0.05). `unison`: 1..3, used only by saw/pulse/triangle/sine/modern.
- `shape`: pulse width 0.12..0.88 for pulse, sine waveform integer0..3 for sine;
  optional. Ordinary sine defaults to0.
- `fm_amount` 0..0.65, `ratio` 1..12, `fm_amount2`0..0.4, `ratio2`1..12,
  `feedback`0..0.15 for FM2/FM3; keep FM2 ratios integral. FM3 ratios may be
  fractional to create explicitly labelled inharmonic sounds.
- String only: `exciter`integer0..6, `decay`0.6..0.98, `stiffness`-0.4..0.4.
- Modern only: `saw`0..1, `pulse`0..1, `tri`0..1; at least one nonzero.

Filter object: `kind` = `lp12`, `lp24`, `hp12`, `bp12`; `hz`80..16000;
`resonance`0.02..0.5; `env`-24..48 semitones; `keytrack`0..1.
Optional `filter_env` is ADSR in seconds like `amp`, defaults to a short decay.
All envelopes attack>=0.004, release>=0.03; release<=5, attack<=3.
For unpitched effects label tags appropriately but keep the keyboard useful.

Motion object: `shape`=`sine`,`triangle`,`square`,`saw`,`noise`,`step`;
`rate`0.05..8 (Hz at120BPM); `sync`bool; `depth`0..18 filter semitones;
`amp`0..9 tremolo depth in dB; `pan`0..0.4. Optional `steps`:16 values -1..1
for shape=step. Steps should create explicit rhythmic identities, not random
lists. Optional `pitch`0..12 semitones (only use when musically justified).

Optional family fields: `noise`0..0.08, `noise_color`-1..1; `drift`0..0.15;
`reverb`0..0.25; `delay`0..0.2; `reverb_time`0.4..4 seconds;
`width`0.1..1; `mono`bool; `portamento`0.004..0.15 seconds;
`highpass`20..150Hz; `velocity`6..24dB response; `pitch_drop`-24..24
semitones assigned to filter envelope, for explicit drums/FX only.

Optional `variant_changes`, `variant_roles`, and `variant_intents` dictionaries
are keyed by variant name. They refine a shared variant for a particular family:
changes merge over the category transform; role and intent replace the inherited
text. Use them when the same transformation or description is inappropriate for
one architecture. These overrides must remain in the reproducible recipe.

## Five variants

Each variant is `{name, intent, changes}`. `intent` explains its actual playing
function. `changes` supports these scalar transformations:
`attack_scale`, `decay_scale`, `release_scale`, `sustain_scale`, `cutoff_scale`,
`motion_scale`, `motion_rate_scale`, `fx_scale`, `detune_scale`, `fm_scale`,
`noise_add`, `second_level_scale`, `resonance_add`, `pitch_motion_add`.
It also supports `sync` bool, `motion_shape` as above, `steps`16 values, and
`osc2_octave`integer -2..2. Use at least three meaningful, audible changes
per variant. One neutral/close variant may have only two. Do not vary gain or
transpose the whole patch to pad the count. Avoid changes that do nothing for
your family choices. Never rely solely on reverb/noise to distinguish variants.

`attack_min` and `attack_max` bound the transformed amplitude attack in seconds,
within the common 0.004–3 second range. `contour_amp` optionally routes Contour to amplitude decay in log2-time units and reduces sustain by up to0.12 when filter-only movement is ineffective. `string_decay_add` adds to both coupled
String decay settings through the oscillator's base decay (capped at 0.985).
These allow a true slower entrance or a crisp articulation where multiplying a
very short or long family attack would not achieve the variant's stated purpose.

The compiler preserves the recipe's default tremolo and pan modulation. A separate
zero-default companion LFO adds controllable tremolo and pan when Motion or the
wheel is raised. It follows the authored shape, rate and synchronization, without
changing the raw output used by tuned step sequences.

`gain_trims.json` records measured per-preset voice-VCA corrections and their
calibration evidence. Trims are engineering corrections, never additional variants
or audio-file normalization. Any gain or design change requires native QA of the
resulting FXP and refreshed manifest/package checksums.

## Shared player controls

The builder assigns eight consistent macro labels and actual audible routes:
Tone, Motion, Contour, Space, Color, Width, Attack, Release. Defaults remain
musically usable. Wheel controls brightness/motion; pressure adds vibrato.
Every patch carries its range, role, and variant intent in its comment and
catalog entry. Document synthetic instrument limitations in role descriptions
where relevant; do not repeat a disclaimer in every patch name.

QA must use the actual generated FXP and native engine. Recipe validity is
not proof of a good sound. Subsequent waves will inspect renders, controls,
clipping, silence, release, performance ranges, distinction and packaging.

Final gain bounds are−24 to+18dB in `gain_trims.json`; positive trims are measured
exceptions for quiet defaults with verified stress margin, not arbitrary loudness
normalization. The companion LFO follows the main shape/rate/sync but uses phase0.5
for Noise shapes to avoid a silent initial interpolation history. Its magnitude
remains zero at the saved setting, and the main Noise phase is unchanged.
