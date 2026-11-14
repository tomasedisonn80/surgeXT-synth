# Wave 9 / agent 51: complete native diversity review

## Scope and evidence

The complete analysis covers **1,000 presets in 200 families**, with five related
versions per family. It checks native measurement provenance, identifies exact
default-audio collisions, ranks numerically similar pairs, and investigates five
selected pairs with ten additional native dry renders. It does not assign an
artistic quality score or certify that every preset is perceptually unique.

The [evidence record](w09-a51-final-diversity-evidence.json) identifies the exact
catalog, preset, recipe, compiler, measurement, renderer and audit hashes. The
[comparison archive](w09-a51-diversity-data.zip) preserves all per-preset feature
records, all 2,000 within-family comparisons, all nearest-neighbor tables, the
complete summary, and the targeted dry-render protocol and measurements.

This final snapshot binds catalog
`67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`.
It includes a complete analysis refresh and ten fresh dry renders after the
description corrections below. All ten dry PCM hashes are identical to their
pre-correction counterparts; the changes preserve the measured audio.

The measured renderer is the actual Surge common engine, SHA-256
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.
Neither presets nor engine code were changed by this reviewer. The parent applied
the narrow description correction discussed below and reran its native checks.

## Complete-bank results

| Check | Result |
|---|---:|
| Current hash-bound QA reports accepted | 1,000 / 1,000 |
| Complete five-variant families | 200 / 200 |
| Within-family pairs | 2,000 |
| Within-family pairs with matching active playing conditions | 1,941 |
| Within-family pairs with different adaptive hold durations | 59 |
| Within-family pairs with different played MIDI notes | 0 |
| Presets with a matching cross-family comparison candidate | 949 |
| Distinct full default PCM hashes | 1,000 |
| Exact default PCM collision groups | 0 |
| Nonidentical fresh default repeats | 0 |
| Stale, malformed or failed input reports | 0 |

The remaining 51 presets have no cross-family candidate with every required
active fixture field equal. The audit explicitly lists those IDs; it does not
silently describe all 1,000 cross-family comparisons as controlled comparisons.

Distance is the Euclidean difference between ten normalized spectral-energy
fractions, plus half the Euclidean difference between twelve normalized active
RMS-envelope bins. This is a transparent ranking heuristic, not a calibrated
perceptual scale. It largely reduces overall-gain sensitivity while retaining
envelope, spectrum and effects differences. The primary matched shortlist uses
equal played notes, hold time, sample rate, velocity, tempo and controller states.

The closest within-family pair is **Felt Drum Close / Felt Drum Edge**, distance
0.003237. Other close examples include Rubber Lantern Close / Spark, Palm Circuit
Close / Tender, Felt Motor Edge / Pulse, and Felt Pizzicato Close / Section. The
earlier category reviews already investigated several of those families. Related
articulations are expected to share an instrument identity; a small distance
alone does not establish a defect or justify changing the sound.

The leading matched cross-family pairs are **Paper Hammers Glass / Copper
Plectrum Close**, distance 0.016184, and **Ivory Frame Bite / Warm Toybox Alloy**,
distance 0.019712. Their default active RMS differences are approximately 7.00 dB
and 11.34 dB respectively. Those level differences are reported separately and
do not inflate the normalized ranking features into evidence of timbral variety.

The unconstrained bank-wide shortlist also places **Felt Drum Bloom / Clay Pulse
Drift** close together. Their original fixtures played different MIDI notes, so
the supplemental comparison below fixes that difference explicitly.

## Focused dry comparisons

Ten fresh renders examine five specific pairs at 48 kHz, velocity 96, 120 BPM,
three seconds held and eight seconds total. Each pair uses the same played note,
seed 1000, fresh engine initialization, saved controls and **Space = 0**. Native
output is retained unnormalized in the working evidence. Analytical gain fitting
is used only when comparing the resulting arrays.

For short hits, a twelve-bin envelope over three seconds can hide the relevant
first few milliseconds. The supplemental analysis therefore measures 5 ms RMS
bins over the first 600 ms and overlapping spectra over the first second. The
gain-removed waveform residual fits one scalar to one signal and measures the
remaining difference relative to the reference energy. Phase and random detail
still affect it; it is not an audibility or listener-preference score.

| Pair | Common MIDI note | Gain-removed waveform residual | Early-envelope residual |
|---|---:|---:|---:|
| Felt Drum Close / Edge | 54 | 0.1462 | 0.1258 |
| Felt Motor Edge / Pulse | 48 | 0.0872 | 0.0645 |
| Paper Hammers Glass / Copper Plectrum Close | 63 | 0.3211 | 0.1483 |
| Ivory Frame Bite / Warm Toybox Alloy | 68 | 0.7977 | 0.2934 |
| Felt Drum Bloom / Clay Pulse Drift | 54 | 0.8078 | 0.2501 |

All ten native renders succeed with finite output. Their largest sample peak is
0.091682, and no pair has identical PCM. The residuals establish measured dry
differences that remain after fitting overall gain; they do not establish that
listeners would regard every difference as musically useful.

Felt Drum Close accumulates 90% of its dry signal energy by approximately 50.21 ms;
Edge reaches that point at 63.73 ms. Their short envelope shapes therefore differ
even though their broad spectral fractions are particularly similar. Felt Motor
Edge / Pulse remains the most subtle of the five targeted dry waveform comparisons.
Both pairs remain sensible priorities for a human phrase-level A/B review.

## Recipe operations and description correction

The expanded-recipe ablation identifies **364 inactive individual operations
across 331 presets**. These include FM edits applied to sources without an FM
amount, attack edits limited by the minimum attack setting, and assignments that
repeat an existing value. The count does not mean that 331 complete variants are
unchanged: other source, balance, envelope, filter or motion edits remain active.
Conversely, an active numeric edit does not guarantee a clearly audible change.

This review identified a narrow description issue in eight percussion Edge
variants: **SC10-003, 008, 018, 028, 053, 063, 068 and 078**. Their authored and
expanded nominal attack values are both 0.004 seconds because the lower clamp
prevents the requested scaling from shortening them. The generic word “faster”
is therefore unsupported for those specific attacks. Their other sound changes
remain intact. The applied correction removes that word from those eight variant
intents; it does not redesign the voices. This review verified the corrected
descriptions and refreshed the complete catalog-bound evidence. The exact final
catalog and measurement bindings are recorded in the evidence file.

## Reproduction and practical limits

From the repository root, with the final primary QA directory available:

```sh
python scripts/scoring/analyze_diversity.py \
  --qa-dir ../qa-scoring-final \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --output ../diversity-final
```

The archived targeted probe script records its original working paths, command,
fixtures, random seed and exact input hashes. Its path constants can be adapted
to reproduce the ten renders elsewhere. The archive contains measurements and
PCM hashes, not the full raw audio arrays.

Different PCM hashes can arise from duration, phase, gain or tiny numerical
changes. Broad spectral bands omit harmonic detail, pitch tracking, stereo image
and musical phrasing. The main ranking consumes retained QA metrics rather than
independently recomputing every default spectrum from raw PCM. Its additional
ten renders address selected close cases, not every possible note, velocity,
macro combination, mix context or playback system.

The results support complete technical coverage and a concrete audition shortlist.
**Human musical audition remains pending; composer endorsement, Hollywood use,
acoustic realism and universal genre suitability are not claimed.**
