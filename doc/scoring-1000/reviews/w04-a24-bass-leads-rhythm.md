# Wave 4, agent 24: bass, leads, pulses and transitions

## Scope and evidence

Reviewed all 80 authored families and all 20 category variant definitions in
categories 07–10, covering 400 generated presets. Read `AGENTS.md`,
`scripts/scoring/RECIPE_FORMAT.md`, the complete compiler
`scripts/scoring/build_bank.py`, relevant native LFO and play-mode implementation,
and the generated pitch routes and step values of all 20 pitched-sequence presets.
Applied `vary()` in memory to inspect the resulting envelope/filter values and
inactive transformations. No bank or code changes, render runs, full QA runs,
listening judgments, or composer-endorsement claims were made.

Evidence snapshot SHA-256:

| File | SHA-256 |
|---|---|
| `scripts/scoring/build_bank.py` | `d0efe5c2c47baf4447743496deda034f6a2a67caf2e0fe340857d49d79ba5a1c` |
| `scripts/scoring/recipes/07_bass.json` | `2d9abf27d160e29dbfa8fbe76da83ccb2a3d49c7afa167c1608885b5bb2c1960` |
| `scripts/scoring/recipes/08_leads.json` | `058a4cfeae1930836f6e7c3f2e99a292c542c1979b8af27f4f36436c1c7dca91` |
| `scripts/scoring/recipes/09_pulses_sequences.json` | `00201e90ee17730c29d0934b60a8d512c8ba0e428e6168a575421c2d19cb9d9b` |
| `scripts/scoring/recipes/10_percussion_transitions.json` | `d45d1af81cabb89ba58d46c8b4cd3edb05d500a0d4ce8bbe9be0d1876525afb1` |

Findings below distinguish definite implementation/description mismatches from
musical audition priorities. File hashes identify the inspected state if later
work changes the compiler or recipes.

## Actionable findings

### 1. Sparse overrides invalidate specific family descriptions

**Definite metadata mismatch; fix before presenting these as finished presets.**

Category 09's Sparse variant replaces every sequence with
`[1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0]`. This is a two-accent pattern with eight
steps between accents. It does not preserve the following inherited roles:

| Preset | Inherited claim | Actual Sparse pattern |
|---|---|---|
| `SC09-013 Copper Transit Sparse` | Three-three-two accent cycle | Eight-eight accent spacing |
| `SC09-048 Tilted Grid Sparse` | Five-five-six timbral accents | Eight-eight accent spacing |
| `SC09-083 Amber Ladder Sparse` | Major-pentatonic sequence | Root and octave only |
| `SC09-088 Night Thirds Sparse` | Minor-pentatonic figure | Root and octave only |
| `SC09-093 Open Fifths Sparse` | Root, fifth and octave | Root and octave; no fifth |
| `SC09-098 Prism Arcs Sparse` | Whole-tone figure | Root and octave only |

The root/octave pairs are members of the named scales, but no longer communicate
their characteristic major/minor/whole-tone identity. Sparse's own intent already
correctly distinguishes timbral and octave accents. Let variant metadata override
the family role, or make those family roles generic enough to remain true after
replacement. Preserve the current explicit statement that these are sustained
modulation patterns, not a MIDI arpeggiator or silent-rest sequence.

### 2. Compiler floors override authored bass centering and lead motion depths

**Definite parameter-contract mismatch; needs a deliberate authoring decision.**

`make_patch()` uses `max(1.5, motion.amp)` for VCA modulation and
`max(.12, motion.pan)` for pan modulation. Every lead family authors `amp=.15`
and `pan=.025`, but all 100 leads therefore compile with 1.5 dB and .12 routes:
ten times the authored tremolo depth and 4.8 times the authored pan depth before
the shared LFO magnitude is applied. With the saved .35 magnitude, those routes
contribute approximately ±.525 dB and ±.042 pan rather than the requested
±.0525 dB and ±.00875 pan.

Seventeen bass families explicitly request zero pan motion, yet receive .12:
07/01–04, 07/06–13, 07/15–17, and 07/19–20. The other three bass families
(Quiet Bow, Cello Shadow, Low Lantern) request small nonzero pan motion and also
receive .12. Thus every bass variant has more pan motion than authored.
This does not prove an audible defect, but it defeats a specified centered-low
foundation and makes the recipe an inaccurate description of the output.

Respect zero/small authored values and establish audible Motion behavior through
purposeful family routes, or explicitly change the recipe contract and author the
larger values. Do not describe these bass patches as having zero pan modulation.
The `vary()` minimum filter-motion depth of 1 semitone also collapses distinct
small-motion transformations in Willow Breath and Harbor Whistle, among others;
retain this only as an explicit musical minimum, not an assumed active axis.

### 3. Global bass articulation copy exceeds the resulting slow-family envelopes

**Concrete envelope mismatch with the advertised playing use.**

| Family | Tight attack / decay | Pluck attack / decay |
|---|---:|---:|
| 07/05 Quiet Bow | 192 ms / 943 ms (`SC07-021`) | 156 ms / 624 ms (`SC07-024`) |
| 07/14 Cello Shadow | 104 ms / 715 ms (`SC07-066`) | 84.5 ms / 473 ms (`SC07-069`) |
| 07/18 Low Lantern | 96 ms / 1,040 ms (`SC07-086`) | 78 ms / 688 ms (`SC07-089`) |

Tight says it suits fast lines; Pluck promises fast decay and detached ostinatos.
Scaling a bowed or swelling envelope by .65 does not make a plucked onset.
Quiet Bow Pluck takes longer than a 120 BPM sixteenth note to reach full attack.
The patches may be useful soft articulated swells, but the generic copy is too
specific. Add family-specific attack/decay caps for those articulation variants,
or describe their actual slow, rounded articulation. If changed, inspect short
notes as well as held notes so the intended articulation is genuinely available.

### 4. Timing terminology needs an exact subdivision reference

**Documentation issue; the native tempo math itself is consistent.**

The native LFO computes step advancement from `2^rate * tempo/120`;
the compiler stores `log2(recipe_rate)`. Therefore a rate of 2 means two steps
per second at 120 BPM, i.e. one step per quarter-note beat. A 16-step sequence at
that rate spans 16 quarter-note beats. Rates 4 and 3 span eight and 10⅔ beats,
respectively. The whole pattern is not one LFO-rate cycle.

The Triplet multiplier is exactly 1.5. On a straight subdivision this produces
a conventional triplet relationship. On the already-three-step-per-second
families 09/04 Threaded Reed, 09/15 Hollow Dispatch and 09/18 Night Thirds,
their Triplet variants (`SC09-019`, `SC09-074`, `SC09-089`) run at 4.5 steps
per second: nine steps per four quarter-note beats. This is another 3:2
acceleration of an already triplet-related base, not a standard straight-grid
triplet subdivision. Current intent says “triplet-related,” which is defensible;
provide an explicit rate/subdivision field so users know what they are getting.

09/08 Linen Motor's role says “two-beat rhythmic thread,” but its main accents
occur four steps apart at a two-step-per-second base rate: four quarter beats
between main accents. Change that phrase to “gently repeating rhythmic thread”
or specify which beat unit is intended. The Sparse override should likewise be
described by its actual step spacing, not assumed bar length.

### 5. Mono lead Float variants are not single-trigger legato instruments

**Play-mode limitation worth resolving or describing.**

`mono=true` maps to native mode 1, `Mono`; native modes 2 and 4 separately provide
single-trigger behavior. Float promises spacious legato themes, yet all 15 mono
lead families remain retriggering Mono with lengthened attacks. Overlapping
notes therefore do not receive a guaranteed continuous envelope.

Affected Float presets: `SC08-004`, `009`, `014`, `019`, `024`, `029`, `039`,
`044`, `049`, `054`, `064`, `074`, `084`, `089`, and `094` (all use prefix
`SC08-`). Either support a per-variant single-trigger/fingered-portamento mode
for appropriate Float designs, or document that users must select that native
mode for continuous-envelope legato. A mono patch also cannot supply chords;
the poly alternatives listed below should be clear in the playing guide.

### 6. Transition pitch direction is correct, but release and duration need guidance

**Confirmed mechanism with two concrete performance qualifications.**

Felt Drum, Low Cushion, Clay Pulse, Iron Footfall, Tension Thud, Drop Marker and
Circuit Clack use positive filter-envelope pitch depth with short attack and
decay to zero. This correctly gives a fast rise followed by a descending pitch
attack. No pitch-sign reversal was found.

Dawn Sweep (10/17, `SC10-081`–`085`) uses a +12-semitone pitch route and a
1.7-second filter-envelope attack to sustain 1: it rises while held. Falling Arc
(10/18, `SC10-086`–`090`) uses −12 semitones with a 1.2-second attack to
sustain 1: it descends while held. Both pitch envelopes release to zero over
.6 seconds. Releasing Dawn Sweep turns its pitch down toward the original note;
releasing Falling Arc turns its pitch up. Falling Arc's “soft landing” copy does
not disclose this rebound, and its amplitude releases range .44–1.4 seconds,
so the return can occur while the voice still sounds. Describe that return or
author a fade/hold relationship appropriate to a one-direction ending.

All five variants retain the same filter-envelope attack, and the Attack macro
only changes the amplitude attack. Consequently neither the variant's longer
entrance nor Attack changes the pitch-sweep duration. The existing “no
fixed-duration riser” disclaimer is correct. Add a guide to the filter-envelope
attack control, or an intentionally mapped sweep-duration control, before
claiming easy tempo-length riser adjustment.

## Register and play-mode assessment

All 20 category-07 families keep their lowest oscillator fundamental above the
20–25 Hz highpass within the authored playing range. Lowest fundamentals are
41.20–55.00 Hz. None uses an unlabelled octave-down oscillator that silently
moves its stated bass range below that highpass. This is a sound structural
choice, but not a measured loudness or acoustic-quality result.

Three category-10 low-register cases deserve targeted audition rather than an
automatic rejection. Iron Footfall's lowest octave-down layer is 24.50 Hz versus
a 27 Hz highpass; Gong Mist's is 25.96 Hz versus 30 Hz. Falling Arc at MIDI 38
finishes its octave-down layer at 18.35 Hz and its main layer at 36.71 Hz against
a 40 Hz highpass. This progressively removes low body at the bottom of its
advertised range. If a sustained low landing is intended, raise the suggested
range or adjust the lower layer/highpass; if fading low body is intended,
describe that behavior.

Polyphonic families are:

| Category | Polyphonic families; all remaining families are mono |
|---|---|
| 07 Bass | 04 Wood Finger; 05 Quiet Bow; 13 Soft Pedal; 17 Muted Current; 18 Low Lantern |
| 08 Leads | 07 Glass Cantabile; 12 Prism Voice; 14 Satin Harmonics; 16 Chalk Melody; 20 Open Passage |
| 09 Pulses | All except 03 Copper Transit, 05 Low Lantern and pitched-sequence families 17–20 |
| 10 Percussion | All 20 families; overlapping hits and held chords are available |

The four pitched-sequence families correctly use mono mode. All 20 corresponding
generated FXPs have `source_index=1`, pitch depth 12, and `extraAmplitude=0`.
Native `LFOModulationSource::process_block()` assigns that output the raw step
value. The major pentatonic steps produce 0/2/4/7/9/12 semitones; minor
pentatonic 0/3/5/7/10/12; open fifths 0/7/12; whole-tone
0/2/4/6/8/10/12. Motion's magnitude changes do not detune these programmed
intervals. This is a source/XML verification, not audio pitch measurement.

## Inactive variation axes

These do not establish duplicate presets: each listed variant has other active
changes. They must not be counted as audible axes on the affected families.

| Category | Families with no FM2/FM3 oscillator, so `fm_scale` is inactive | Variants using that inactive axis |
|---|---|---|
| 07 | 01, 04, 06, 07, 11, 13, 14, 15, 18 | Round, Edge, Pluck: 27 family/variant combinations |
| 08 | 01, 05, 08, 10, 11, 13, 17, 19 | Warm, Clear: 16 combinations |
| 09 | 01, 04, 05, 06, 08, 10, 11, 12, 13, 15, 17, 19 | Soft, Drive, Glass: 36 combinations |
| 10 | 02, 03, 07, 10, 11, 14, 20 | Body, Edge, Bloom: 21 combinations |

Attack scaling is clamped back to the same 4 ms value for bass families 03 and
10 in Tight/Edge/Pluck, and percussion families 01, 02, 04, 06, 11, 13, 14,
16 in Close/Edge: 22 inactive attack transformations. Anchor Sub's
Tight/Round/Edge/Pluck FX scaling also has no effect on generated mix routes
because the compiler floors reverb at .02 and delay at .008. These four variants
have the same ambience amounts as each other and as the floored base design.

“Upper layer” in generic variant copy should be “secondary layer.” Specific
counterexamples are lead families Reed Lantern, Soft Voltage and Moonlit
Triangle, and pulse families Orbit Dots, Hollow Dispatch, Amber Ladder and Open
Fifths: their second oscillator is explicitly a sine one octave below the root.
Changing its level changes lower foundation, not an upper layer.

## Complete family coverage

The family ordinal in the first column identifies the same ordinal within each
category. Its five preset suffixes are shown once; prepend `SC07-`, `SC08-`,
`SC09-` or `SC10-` for the corresponding category. Thus ordinal 03 covers
`SC07-011`–`SC07-015`, `SC08-011`–`SC08-015`, and so on.

| Ordinal / suffixes | 07 Bass | 08 Leads | 09 Pulses and Sequences | 10 Percussion and Transitions |
|---|---|---|---|---|
| 01 / 001–005 | Velvet Root | Amber Ribbon | Quiet Engine | Felt Drum |
| 02 / 006–010 | Anchor Sub | Reed Lantern | Paper Clock | Timber Knock |
| 03 / 011–015 | Copper Pick | Willow Breath | Copper Transit | Low Cushion |
| 04 / 016–020 | Wood Finger | Silver Thread | Threaded Reed | Copper Tap |
| 05 / 021–025 | Quiet Bow | Velvet Needle | Low Lantern | Paper Brush |
| 06 / 026–030 | Reed Floor | Hollow Compass | Slate Meter | Glass Tick |
| 07 / 031–035 | Transit Saw | Glass Cantabile | Orbit Dots | Clay Pulse |
| 08 / 036–040 | Hollow Step | Copper Call | Linen Motor | Iron Footfall |
| 09 / 041–045 | Rubber Coil | Harbor Whistle | Glass Repeater | Silk Shaker |
| 10 / 046–050 | Glass Kernel | Soft Voltage | Tilted Grid | Air Swish |
| 11 / 051–055 | Brushed Wire | Cedar Solo | Velvet Spokes | Tension Thud |
| 12 / 056–060 | Ember Grit | Prism Voice | Breath Relay | Sand Rattle |
| 13 / 061–065 | Soft Pedal | Woven Reed | Timber Circuit | Drop Marker |
| 14 / 066–070 | Cello Shadow | Satin Harmonics | Nickel Steps | Hollow Block |
| 15 / 071–075 | Pulse Engine | Narrow Horizon | Hollow Dispatch | Gong Mist |
| 16 / 076–080 | Bronze Growl | Chalk Melody | Distant Relay | Circuit Clack |
| 17 / 081–085 | Muted Current | Gold Arc | Amber Ladder | Dawn Sweep |
| 18 / 086–090 | Low Lantern | Porcelain Line | Night Thirds | Falling Arc |
| 19 / 091–095 | Felt Motor | Moonlit Triangle | Open Fifths | Breath Lift |
| 20 / 096–100 | Open Circuit | Open Passage | Prism Arcs | Velvet Arrival |

All five variants were inspected in their authored order: Bass
Tight/Round/Edge/Pluck/Pulse; Leads Near/Warm/Clear/Float/Phrase; Pulses
Soft/Drive/Sparse/Triplet/Glass; Percussion Close/Body/Edge/Drift/Bloom.

**Disposition:** recipe/compiler review complete for the assigned 400 presets.
Resolve the concrete metadata and parameter-contract findings, then use the
planned native renders and musical audition for sound-quality decisions.
