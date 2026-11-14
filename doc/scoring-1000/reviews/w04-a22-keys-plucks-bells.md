# Wave 4 / agent 22: Keys, Plucks, Bells and Mallets design review

## Scope and method

Reviewed all **60 families and all 300 family/variant combinations** in categories 01–03. Read the repository `AGENTS.md`, recipe contract, recipes, and `build_bank.py`; inspected the native sine/FM oscillator implementations where waveform and ratio interpretation affected a finding. This is a **design and compiled-parameter audit**, not an audio audition or evidence of composer endorsement. No presets, recipes, or implementation files were edited.

For each of the 300 combinations, compiled the recipe in memory, then independently removed each requested variant transformation and recompiled. Equality of the resulting parameter XML identifies transformations that currently do nothing. This checks the actual builder mapping, not just whether two recipe dictionaries differ. It does not establish that every numerical difference is perceptually significant. No preliminary smoke measurements were used as current quality evidence.

The reviewer observed the current stereo Wide path, linear-to-cube-root mixer conversion, Space=0 dry/default=.4 routing, and the transient-aware Release macro. The findings below do not assume the earlier implementations.

## Result

The three categories have plausible, varied synthesis architectures and reasonably differentiated articulation targets. Every reviewed variant retains at least **four compiled non-FX transformation changes** after inactive transformations are excluded (minimum four in Keys, five in each other category). None is only a rename, reverb change, or gain change. This is a useful design baseline, not a musical quality pass.

Three description/design alignments merit correction before final catalog publication. The most significant is an additive organ variant whose stated upper-layer change points in the opposite direction from its actual level edit. Several other observations should become specific audition checks, rather than speculative preset redesigns.

## Actionable findings

### 1. Keys Glass intent does not fit the existing upper-octave organ/electric layers

**Strongest example: SC01-058 Drawbar Lantern Glass.** Its three oscillators are pure sines at octave 0, +1 and +2 with recipe levels .64, .25 and .11. Glass sets oscillator 2 to +1, where it already is, and reduces its level to .15. The fundamental-to-second-harmonic ratio therefore changes from .64:.25 to .64:.15. There is no FM oscillator for `fm_scale` to affect. The cutoff moves from 5100 to 9180 Hz, above all three partials through much of the middle register. Its stated “brighter upper layer” is consequently not represented by strengthening or raising that layer; the second partial is weakened. Longer decay and changed sustain/release remain real articulation changes.

The same redundant `osc2_octave: 1` plus .6 level multiplier applies to **SC01-038 Wool Electric Glass, SC01-063 Chapel Light Glass, SC01-088 Silver Desk Glass and SC01-093 Motor Keys Glass**. Their additional non-sine layers mean their overall brightness cannot be inferred from this alone, but the shared upper-layer explanation still does not describe the edit accurately.

**Action:** Give existing-upper-layer families an appropriate per-family Glass transform, or describe Glass as a leaner, more open articulation without promising a stronger upper layer. For Drawbar Lantern specifically, compare the +1-octave partial at .15 versus .30–.35 with a middle-register chord; choose the level by audition and retain headroom calibration. Do not transpose all Glass patches again.

### 2. Pluck Spark promises an upper layer while several patches strengthen a lower or same-register body

**SC02-043 Bone Tines Spark** increases oscillator 2 from .25 to .3625 at octave **−1**; that oscillator uses sine shape 1, whose native waveform retains the played-period fundamental. **SC02-088 Cork Mechanism Spark** similarly raises its plain sine sub-octave from .23 to .3335. Neither operation creates a more prominent upper layer. Both have brighter filter settings, but that is a different mechanism.

Other clear cases include **SC02-003 Silk Plectrum Spark** (same-octave triangle), **SC02-008 Cedar Thread Spark** (same-octave sine), **SC02-033 Glass Filament Spark** (same-octave pure sine), **SC02-038 Rubber Lantern Spark** (same-octave triangle), and **SC02-073 Iron Petal Spark** (same-octave triangle). These strengthen body/fundamental components while the cutoff or FM changes provide the brightness.

**Action:** Change the shared Spark intent to name the actual brighter filter/FM transient and stronger secondary layer, or add family-specific intents for the sub-octave cases. Preserve useful low-body options unless audition finds masking. Specifically audition Bone Tines Spark and Cork Mechanism Spark in a low-register chord against a bass part; the extra body could be desirable but should not be advertised as upper-register emphasis.

### 3. Tin Carousel's role misidentifies its oscillator pairing

**SC02-056–060 Tin Carousel** is described as a “paired-FM pluck.” Its first oscillator is FM2 and its second is a pulse oscillator at width .32. There are two FM modulators inside the first oscillator, but the two layered sound sources are FM plus pulse, and the role wording suggests a different architecture.

**Action:** Use “bright FM-and-pulse pluck” in the family role. The recipe itself need not change for this issue.

### 4. Shared motion floors overwrite authored quietness in most of these families

The builder uses `max(1.5, motion.amp)` and `max(.12, motion.pan)` with saved LFO magnitude .35. The amplitude floor replaces the recipe value in **53 of 60 families / 265 presets**; the pan floor replaces it in **55 families / 275 presets**. In Bells and Mallets, many authored amplitude values are exactly zero, yet the generated patch has nominal peak LFO amplitude depth .525 dB and pan depth .042. The effect is modest, but it is deliberately added movement where the recipe requested less or none. For example, **SC03-001–005 Amber Marimba** asks for amp 0 and pan .03; **SC01-026–030 Paper Hammers** asks for amp .2 and pan .04.

**Action:** Make this baseline movement an explicit part of the recipe contract or preserve the recipe's baseline depths and let Motion add movement. If retained, audition the shortest Close mallets and damped keys with Space=0 for unwanted note-to-note amplitude/pan changes. The current implementation is technically active; the unresolved question is whether its default movement serves the promised close articulation. This is not a claim that these sounds audibly fail.

## Inactive transformations: exact scope

These no-op fields do **not** make the resulting variants duplicates; the remaining transformations are sufficient numerically. They should not be counted as independent audible design changes in progress reports.

| Category | Inactive transform | Affected combinations | Reason |
|---|---|---:|---|
| Keys | `detune_scale` | 13 Float | Every oscillator detune is zero in those families. |
| Keys | `attack_scale` | 12 | Glass Hammer, Amber Clav, Tin Toy Keys and Pocket Reed: Close, Glass and Bite are already at the 4 ms floor. |
| Keys | `fm_scale` | 18 | Felt, Glass and Bite for Paper Hammers, Amber Clav, Drawbar Lantern, Chapel Light, Breath Harmonium and Motor Keys have no FM source. |
| Keys | `osc2_octave` | 5 Glass | Wool Electric, Drawbar Lantern, Chapel Light, Silver Desk and Motor Keys already use +1. |
| Plucks | `fm_scale` | 18 | Tender and Spark for Silk Plectrum, Cedar Thread, Paper Lattice, Rubber Lantern, Bone Tines, Woven Strings, Palm Circuit, Soft Pinwheel and Cork Mechanism have no FM source. |
| Plucks | `detune_scale` | 15 Cascade | Only Woven Strings, Palm Circuit, Soft Pinwheel, Velvet Mandolin and Tremor Lute have a nonzero authored detune to scale. |
| Plucks | `attack_scale` | 10 Spark | Harbor Harp, Paper Lattice, Clockwork Reed, Glass Filament, Bone Tines, Palm Circuit, Tin Carousel, Iron Petal, Velvet Mandolin and Cork Mechanism are already at 4 ms. |
| Plucks | `motion_shape` | 2 Shadow | Soft Pinwheel and Tremor Lute already use triangle. |
| Bells/Mallets | `attack_scale` | 10 | Hollow Slats, Cedar Knocks, Frost Celeste, Silver Nodes and Quiet Anvils: Close and Glass are already at 4 ms. |
| Bells/Mallets | `detune_scale` | 10 | Hollow Slats, Cedar Knocks, Ceramic Hands, Quiet Anvils and Teal Resonator: Glass and Shadow have no nonzero oscillator detune to scale. |

There are **113 inactive transformation occurrences**, not 113 failed presets. Minimum total effective transformation counts, including FX, are Keys 5, Plucks 6, Bells/Mallets 6. Excluding the FX transform gives minima 4, 5, 5.

## Specific remaining audio-review targets

These are consequences of the design, not demonstrated audio defects.

- **SC01-056–060 Drawbar Lantern, top MIDI 91:** its +2-octave layer reaches MIDI 115. Check whether the upper partial remains useful and whether Glass is actually brighter than Close in both the middle and upper registers.
- **SC03-086–090 Quartz Gong, low MIDI 33:** its supporting octave reaches MIDI 21 (27.5 Hz), with lowcut 22 Hz. Check sub-bass weight and peak headroom in the documented lowest register; do not treat the keyboard range as a guarantee of ordinary chordal suitability.
- **SC03-026–030 Frost Celeste and SC03-076–080 Starlit Drops, top MIDI 96:** their octave layer reaches MIDI 108, with high FM ratios adding still higher components. Check the high-end timbre at both 44.1 and 48 kHz, especially Alloy/Glass. This is a timbral/aliasing audition target, not evidence of aliasing from recipe values alone.
- **SC01-099 Resonant Marrow Float:** requested amp decay 6.48 s is capped at 6 s. **SC03-087 Quartz Gong Bloom** requests 8.46 s and **SC03-088 Quartz Gong Glass** requests 6.11 s; both become 6 s. Bloom and Glass retain different attack/release/filter/FM settings, so they do not collapse, but “extended resonance” should be judged from the render, not the nominal multiplier.
- **SC03-049 Bronze Orchard Alloy, SC03-074 Quiet Anvils Alloy, SC03-089 Quartz Gong Alloy:** first FM amounts requested as .798, .874 and .76 all cap at .7. Each remains stronger than its base .42, .46 or .4 and is not inactive, but the intended 1.9× change is not realized. Check their high-register Alloy brightness and pitch anchoring before raising this ceiling.
- **SC02-041–045 Bone Tines and SC01-026–030 Paper Hammers:** their native String decay coefficients (.73 and .72) remain fixed across variants, while only the outer amp envelope's time changes. A larger amp decay cannot regenerate an already-decayed String excitation. Compare Close/Cascade or Close/Float dry and assess whether the continuing second layer supplies the intended longer body. The updated Release macro's separate String-decay route is acknowledged; this observation concerns the preset variants.

For family distinction, useful same-role comparison pairs are **Glass Filament / Rain Tines** (harmonic FM2 with ratios 2 and 7), **Woven Strings / Velvet Mandolin** (two modeled strings plus a third color), and **Glass Hammer / Pocket Musicbox** (near-integer FM3 ratios around 2 and 5). Their octave assignments, modulation amounts, envelopes and filters differ, so parameter inspection does not establish duplicates. A matched-velocity, matched-register, Space=0 comparison should decide whether their roles are perceptually distinct. Do not infer perceptual uniqueness from unique XML hashes.

## Full family coverage

Each row includes all five variants. IDs are within that category, prefixed `SC01-`, `SC02-` or `SC03-` respectively. Ranges are the authored inclusive MIDI key ranges, not every oscillator's transposed range.

### 01 Keys

Variants reviewed: Close, Felt, Glass, Float, Bite.

| IDs | Family | MIDI range |
|---|---|---|
| 001–005 | Felt Ember | 45–84 |
| 006–010 | Copper Tine | 40–88 |
| 011–015 | Velvet Reed | 43–84 |
| 016–020 | Ivory Frame | 48–88 |
| 021–025 | Glass Hammer | 48–88 |
| 026–030 | Paper Hammers | 45–81 |
| 031–035 | Amber Clav | 43–84 |
| 036–040 | Wool Electric | 40–83 |
| 041–045 | Weathered Upright | 45–86 |
| 046–050 | Tin Toy Keys | 57–93 |
| 051–055 | Hollow Console | 40–84 |
| 056–060 | Drawbar Lantern | 43–91 |
| 061–065 | Chapel Light | 43–86 |
| 066–070 | Breath Harmonium | 43–83 |
| 071–075 | Pocket Reed | 40–84 |
| 076–080 | Porcelain Desk | 52–91 |
| 081–085 | Noir Cabinet | 36–76 |
| 086–090 | Silver Desk | 45–88 |
| 091–095 | Motor Keys | 43–86 |
| 096–100 | Resonant Marrow | 40–84 |

### 02 Plucks

Variants reviewed: Close, Tender, Spark, Cascade, Shadow.

| IDs | Family | MIDI range |
|---|---|---|
| 001–005 | Silk Plectrum | 45–84 |
| 006–010 | Cedar Thread | 40–79 |
| 011–015 | Harbor Harp | 48–91 |
| 016–020 | Paper Lattice | 48–84 |
| 021–025 | Copper Plectrum | 45–81 |
| 026–030 | Clockwork Reed | 43–84 |
| 031–035 | Glass Filament | 55–96 |
| 036–040 | Rubber Lantern | 36–79 |
| 041–045 | Bone Tines | 43–84 |
| 046–050 | Woven Strings | 45–86 |
| 051–055 | Palm Circuit | 40–81 |
| 056–060 | Tin Carousel | 50–91 |
| 061–065 | Dusk Zither | 38–81 |
| 066–070 | Soft Pinwheel | 48–88 |
| 071–075 | Iron Petal | 48–84 |
| 076–080 | Velvet Mandolin | 48–91 |
| 081–085 | Water Threads | 48–91 |
| 086–090 | Cork Mechanism | 41–81 |
| 091–095 | Moon Marquetry | 45–88 |
| 096–100 | Tremor Lute | 43–84 |

### 03 Bells and Mallets

Variants reviewed: Close, Bloom, Glass, Alloy, Shadow.

| IDs | Family | MIDI range |
|---|---|---|
| 001–005 | Amber Marimba | 45–84 |
| 006–010 | Hollow Slats | 43–84 |
| 011–015 | Cedar Knocks | 40–79 |
| 016–020 | Rain Tines | 48–91 |
| 021–025 | Violet Vibra | 43–84 |
| 026–030 | Frost Celeste | 55–96 |
| 031–035 | Pocket Musicbox | 53–94 |
| 036–040 | Prism Chimes | 48–84 |
| 041–045 | Paper Glass | 52–89 |
| 046–050 | Bronze Orchard | 36–76 |
| 051–055 | Silver Nodes | 48–84 |
| 056–060 | Temple Signals | 38–79 |
| 061–065 | Ceramic Hands | 45–84 |
| 066–070 | Orbit Plates | 40–79 |
| 071–075 | Quiet Anvils | 39–79 |
| 076–080 | Starlit Drops | 55–96 |
| 081–085 | Warm Toybox | 48–89 |
| 086–090 | Quartz Gong | 33–72 |
| 091–095 | Pulse Lantern | 45–86 |
| 096–100 | Teal Resonator | 43–84 |

## Reviewed file fingerprints

These fingerprints identify the exact input revision for this read-only audit; later root-agent fixes can supersede the findings.

| File | SHA-256 |
|---|---|
| `scripts/scoring/recipes/01_keys.json` | `818de758ed5fc698ebe97fdecea87c6fc2fc94e7073be0ccc175b1d15c10efbc` |
| `scripts/scoring/recipes/02_plucks.json` | `e53f2241f9ee0946938f4007e6166c0b09848c66f44f1707a62ff96ef64657ad` |
| `scripts/scoring/recipes/03_bells_mallets.json` | `bdf8454024eb15718ad4bc1e4465ab3a18c53975ca29b935c45b8d37326a627c` |
| `scripts/scoring/build_bank.py` | `d0efe5c2c47baf4447743496deda034f6a2a67caf2e0fe340857d49d79ba5a1c` |

Native audio QA, human musical audition and composer endorsement are outside this review. No endorsement is claimed.
