# Wave 04, agent 23 — pads, ensemble hybrids, drones and air

## Scope and evidence

Independent read-only design review of all **60 families, 15 category-level variations and 300 expanded presets** in categories 04–06. Read `AGENTS.md`, `scripts/scoring/RECIPE_FORMAT.md`, all three recipe files, and the complete synthesis portion of `scripts/scoring/build_bank.py`. Inspected native FM3 ratio handling and LFO tempo handling where those affected interpretation. No bank, recipe, compiler or QA code was changed by this review.

Also read every actual FXP in these categories and checked it against the current manifest: **100 + 100 + 100 files; all 300 SHA-256 values matched; all 300 serialized parameter bodies differed**. Independently expanding all 300 recipes through the compiler also produced 300 different parameter bodies. These are structural checks, not evidence of 300 perceptually distinct or musically successful sounds.

**No audio audition was performed.** Native audio QA is a separate assignment. This review does not certify clipping, noise, pitch perception, mix usefulness, acoustic realism or composer endorsement. The manifest correctly retains `musical_audition: pending` and `composer_endorsement: not_claimed`.

## Findings requiring a design decision

### 1. Compiler motion floors replace much of the authored restraint

**Priority: medium. Affects 270 tremolo settings and 215 pan settings among the 300 reviewed presets.**

`make_patch()` routes LFO 0 to VCA with `max(1.5, motion.amp)` and to pan with `max(.12, motion.pan)`. The author-provided tremolo depth is below 1.5 in 54 of the 60 families, and pan depth is below .12 in 43. Variants do not modify those two fields. Consequently every ensemble family, every pad family, and 14 drone families have their requested tremolo depth replaced with the same value.

Examples are exact:

- **SC05-011–015 Silk Chamber:** authored tremolo .10 dB becomes 1.50 dB; pan .05 becomes .12.
- **SC05-061–065 Chapel Pipes:** authored tremolo .12 dB becomes 1.50 dB; pan .03 becomes .12.
- **SC06-026–030 Harbor Hum:** authored tremolo .25 dB becomes 1.50 dB; pan .02 becomes .12.
- **SC04-006–010 Paper Lantern:** authored tremolo .20 dB becomes 1.50 dB; pan .03 becomes .12.

With the saved LFO magnitude of .35, the nominal default VCA modulation depth is approximately .525 dB for these families, rather than, for example, Silk Chamber's authored .035 dB. This is not automatically excessive or unpleasant. It is, however, a concrete loss of the recipe's intended differentiation, particularly for restrained accompaniment and steady organ/foundation roles. It also means a static comparison of author values overstates actual motion diversity.

**Action:** honor the recipe depths, or explicitly make the floors part of the recipe contract and have authors review the resulting levels. If stronger modulation is needed for the Motion macro, increase it through that performance control rather than silently replacing the saved base character. A separate useful enhancement is a true still setting: currently Motion starts at zero while LFO magnitude remains .35, so the macro cannot remove the base movement.

### 2. Inherited role text contradicts a small number of actual variations

**Priority: medium.**

| Exact preset | Verified configuration | Mismatch | Action |
|---|---|---|---|
| **SC06-065 Clockless Room Ripple** | Saw LFO, tempo sync enabled, .56 Hz at 120 BPM; cutoff modulation depth reaches the 18-semitone cap | The saved family role promises “no fixed rhythmic pulse,” while Ripple deliberately introduces one | Allow variant-specific role text and describe the deliberate rhythmic version, or retain a noise waveform for this family |
| **SC06-100 Deep Listening Ripple** | Saw LFO, tempo sync enabled, .60 Hz at 120 BPM, 15.4-semitone cutoff route | The saved role describes “nearly motionless harmonic pedals,” although this version introduces regular timbral motion | Describe the moving variation separately; do not imply that the base family's stillness applies to every variation |
| **SC05-083 Felt Pizzicato Swell** | Actual FXP/manifest amp ADSR is .028 / .986 / .08 / .64 seconds | The generic Swell intent promises long entrances and crescendos, but a 28 ms attack is an articulated pluck, not a slow entrance | Call this a longer ringing articulation, or provide a family-specific attack floor and contour designed for an actual swell |
| **SC05-006 Cedar Section Close** | The secondary oscillator is a triangle one octave below the played pitch | Generic Close intent says “reduced upper-layer blend,” but this variant reduces a lower octave layer | Use “secondary layer” consistently, or specify the actual lower-register reduction |

Related review candidates are **SC05-043 Copper Fanfare Swell** (.077 s attack), **SC05-048 Amber Mutes Swell** (.1225 s), and **SC05-088 Walnut Marcato Swell** (.0875 s). These can be useful softer articulations. Their names and generic crescendo language should not be treated as proof of long orchestral-style swelling behavior.

### 3. Two advertised bass ranges begin with predominantly subsonic fundamentals

**Priority: medium; a range-audition target, not an asserted render failure.**

**SC06-026–030 Harbor Hum** and **SC06-096–100 Deep Listening** both advertise MIDI 24–60. At MIDI 24, the dominant minus-one-octave oscillator fundamentals are MIDI 12, approximately **16.35 Hz**. Harbor Hum's principal sine is there; Deep Listening places both its sine and triangle there. Both apply a 20 Hz high-pass stage.

The smaller upper layers can still make the note audible, and low drones may intentionally use subharmonics. Nevertheless, the lower end of a claimed useful tonal range is being carried partly by a fundamental below ordinary full-range reproduction, further attenuated by the patch's low-cut. Deep Listening is especially dependent on the weaker upper FM layer at that extreme.

**Action:** audition MIDI 24, 28, 31 and 36 on limited-bandwidth monitoring and in mono. If the lower notes fail to communicate a stable foundation, raise the recommended lower limit or move the strongest sine to the played octave. Do not raise low bass gain solely to make those notes pass an RMS threshold.

## Design limitations to retain explicitly

### Vowel and choir descriptions are synthetic impressions, not modeled vowels

**SC04-036–040 Vowel Ribbon**, **SC06-016–020 Vowel Halo**, and **SC06-031–035 Distant Choir** use pulse/saw/FM mixtures and one resonant band-pass filter. The second filter is disabled; the compiler does not build independently placed multiple vocal-formant peaks, vowel morphs, phonemes or choir articulations. Color changes the first oscillator's waveform or FM amount, rather than a vocal-formant trajectory.

The existing descriptions are appropriately qualified: Vowel Ribbon is “clearly synthetic,” Vowel Halo is “synthetic vowel-colored,” and Distant Choir explicitly avoids vocal realism. **This is not a naming blocker.** Single-band nasal/breathy synthesis can be useful under dialogue or as a harmonic texture. Retain those qualifications and audition whether the names communicate the actual texture. If stronger choral identification becomes a requirement, author a genuine multi-resonance/formant architecture before strengthening the claims.

### Generic brightness transformations have a different meaning on high-pass families

**SC04-041–045 Sea Glass**, **SC04-076–080 Feather Bridge**, **SC06-036–040 Glass Air**, and **SC06-071–075 Silver Static** use `hp12`. Generic Close/Veil/Veiled transformations reduce the high-pass cutoff, which restores lower body; they do not attenuate the upper spectrum as an analogous low-pass operation would. Open raises the high-pass cutoff and removes more body.

Examples: Sea Glass has a 320 Hz base cutoff, 249.6 Hz in Close, 496 Hz in Open, and 176 Hz in Veil. Glass Air moves from 760 Hz to 440.8 Hz in Veiled and 1,368 Hz in Open. These can be musically sensible thicker/thinner variations. Revise shared intent text to describe body/thinness for these families, or add a separate upper-spectrum control if darkening is intended. The compiler's existing HP-aware Tone/wheel direction shows that filter polarity is already recognized elsewhere.

### Tempo-relative movement is not necessarily a conventional beat division

All **SC05 Motion** and **SC06 Ripple** variants enable tempo sync, but retain scaled decimal rates. Native LFO code multiplies the rate by tempo/120. This is actual host-relative behavior, yet rates such as Clockless Room Ripple's .56 Hz and Deep Listening Ripple's .60 Hz are not conventional straight, dotted or triplet divisions at 120 BPM. Their cycles occupy approximately 3.57 and 3.33 beats respectively.

This is not a defect if intentional drifting or cross-rhythm is wanted. For repeatable cut-to-picture or conventional ostinato support, select explicit musical divisions or disclose the nonstandard cycle. “Tempo-related” is a fair claim; “locked to eighth notes” would not be supported by these settings.

## Transform and architecture audit

No complete variation is a no-op. The 300 actual FXP parameter bodies are unique. The variations change envelopes, filter settings, layer balance and/or motion in addition to effects, so they are not merely renamed files or reverb-only duplicates.

There are **56 individual transformations that do nothing** because the relevant family lacks FM or starts with zero detune: 30 in Pads, 8 in Ensemble Hybrids and 18 in Drones and Air. They do not invalidate the other changes in those variants, but should not be counted as extra synthesis diversity:

- Pads: `fm_scale` does nothing in Open, Drift and Veil for **Amber Canopy (001–005), Paper Lantern (006–010), Reed Horizon (016–020), Hollow Velvet (021–025), Winter Fold (046–050), Linen Current (051–055), Distant Arcade (056–060), Lantern Organ (061–065), Feather Bridge (076–080), Hearth Mist (086–090)**. All IDs use prefix SC04; affected variation positions are 2, 4 and 5 within each family.
- Ensemble Hybrids: `fm_scale` does nothing for **SC05-004 Linen Bows Muted, SC05-009 Cedar Section Muted, SC05-014 Silk Chamber Muted, SC05-039 Slate Trombones Muted, SC05-064 Chapel Pipes Muted, SC05-069 Hollow Stops Muted, SC05-074 Silver Accord Muted**. `detune_scale` does nothing for **SC05-062 Chapel Pipes Section**, because all three authored detunes are zero.
- Drones and Air: `fm_scale` does nothing in Veiled, Open and Ripple for **Quiet Horizon (001–005), Breath Lantern (006–010), Reed Nebula (021–025), Harbor Hum (026–030), Low Cloud (041–045), Hollow Distance (066–070)**. All IDs use prefix SC06; affected variation positions are 2, 3 and 5 within each family.

Action: omit irrelevant changes or add per-family substitutions if these transformations are meant to represent five distinct musical redesigns. Keep the existing valid transformations; there is no reason to discard the whole variants.

Envelope clamps affect **SC04-048 Winter Fold Bloom** (5.025 s release becomes 5), **SC04-068 Cobalt Swell Bloom** (3.06 s attack becomes 3; 5.4 s release becomes 5), and **SC04-098 Slow Orbit Bloom** (5.775 s release becomes 5). These preserve the broad slow-pad roles and are not defects. They should be disclosed if source recipe calculations are presented as exact rendered times.

Detune values are generally restrained. I found no evidence of grossly detuned families marketed as neutral tuned instruments. **SC04-096–100 Slow Orbit** and **SC06-071–085 Silver Static, Metal Weather and Orbital Dust** correctly disclose their inharmonic role. **SC04-081–085 Spectral Silk** and **SC05-096–100 Midnight Unison** use a 1.5 FM ratio; half-integer ratios create octave-related subharmonic spacing, so fractional ratios alone are not proof of atonality. **SC05-016–020 Frost Divisi** uses 2.01, a deliberately near-integer shimmer that merits audition at high Color settings rather than an automatic failure.

## Complete family coverage

Each cell covers all five variations. Prefixes are SC04, SC05 and SC06 respectively. Pad variants are Close/Open/Bloom/Drift/Veil; ensemble variants Close/Section/Swell/Muted/Motion; drone variants Near/Veiled/Open/Drift/Ripple.

| ID suffix range | 04 Pads | 05 Ensemble Hybrids | 06 Drones and Air |
|---|---|---|---|
| 001–005 | Amber Canopy | Linen Bows | Quiet Horizon |
| 006–010 | Paper Lantern | Cedar Section | Breath Lantern |
| 011–015 | Glacier Loom | Silk Chamber | Velvet Current |
| 016–020 | Reed Horizon | Frost Divisi | Vowel Halo |
| 021–025 | Hollow Velvet | Ember Violas | Reed Nebula |
| 026–030 | Porcelain Tide | Bronze Swell | Harbor Hum |
| 031–035 | Copper Bow | Lantern Horns | Distant Choir |
| 036–040 | Vowel Ribbon | Slate Trombones | Glass Air |
| 041–045 | Sea Glass | Copper Fanfare | Low Cloud |
| 046–050 | Winter Fold | Amber Mutes | Amber Cavern |
| 051–055 | Linen Current | Breath Reeds | Porcelain Mist |
| 056–060 | Distant Arcade | Willow Reeds | Floating Ember |
| 061–065 | Lantern Organ | Chapel Pipes | Clockless Room |
| 066–070 | Cobalt Swell | Hollow Stops | Hollow Distance |
| 071–075 | Soft Circuit | Silver Accord | Silver Static |
| 076–080 | Feather Bridge | Glass Strings | Metal Weather |
| 081–085 | Spectral Silk | Felt Pizzicato | Orbital Dust |
| 086–090 | Hearth Mist | Walnut Marcato | Uneasy Bloom |
| 091–095 | Prism Current | Reed Crescendo | Friction Veil |
| 096–100 | Slow Orbit | Midnight Unison | Deep Listening |

## Disposition

No structurally identical presets or entire nonfunctional variations were found in this scope. The clearest corrections are inherited role wording and the compiler's silent replacement of subtle motion values. The strongest remaining listening targets are low-end audibility, the recognizability of the synthetic choir impressions, short-family Swell articulation, and differences between closely related sustained architectures. Structural validity and the honest synthetic descriptions support continued native QA and musical audition; they do not establish Hollywood adoption or suitability for every genre.
