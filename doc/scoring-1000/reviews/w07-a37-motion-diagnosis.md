# Wave 7 / Agent 37: random-motion startup diagnosis

## Finding

The small measured Motion response in **SC01-066–070, Breath Harmonium**, comes
from the native Noise LFO's startup state and the short note window. It is **not
a disconnected modulation route**. Both the authored main LFO and its zero-default
companion use Noise, key triggering, phase zero and a nominal 0.16 Hz rate.

The full-bank results already showed all five variants responding above at least
one measurement threshold, but their three-second waveform changes were only
about 0.0025–0.0057 relative RMS. This audit traces the cause and compares bounded
changes on copies; it does not replace the musical judgment of a listener.

## Native mechanism

In [LFOModulationSource.cpp](../../../src/common/dsp/modulators/LFOModulationSource.cpp),
`attackFrom()` initializes the four Noise interpolation-history values by
multiplying generated random values by the starting phase. At phase zero all
four values are zero. Each phase wrap shifts the history and introduces one new
random value. `process_block()` then uses cubic interpolation through that history.

The [compiler](../../../scripts/scoring/build_bank.py) sets the main magnitude to
0.35 and the companion magnitude to zero. Motion adds 0.6 to both magnitudes and
two octaves to both unsynchronized rates. The companion's native source 19 routes
to voice VCA and pan; the main source 17 retains its authored filter, tremolo and
pan routes. These mappings are present in all five inspected FXPs.

At the nominal default rate, the first wrap is around 6.25 seconds. At Motion 1,
the rate is approximately 0.64 Hz, making the first wrap around 1.56 seconds.
After only one wrap, the central interpolation points are still zero; a small
cubic overshoot can appear before substantial random motion develops. Both LFOs
share this startup limitation. The native rate lookup is approximate, so these
periods are explanations of the timing, not sample-exact predictions.

## Bounded native comparison

Rendered **24 scenarios** using the actual compiled engine: Close and Bite, three
definitions, two held durations, and Motion 0/1. All used MIDI note 63, velocity 96,
48 kHz, tempo 120, Space 0, saved unity master, fresh engines and seed 1000. Each
render included one second after note-off. No PCM normalization or limiting was
applied.

The experimental copies changed exactly one companion parameter:

- **Sine companion:** `a_lfo2_shape` from Noise (4) to Sine (0).
- **Noise phase 0.5:** retained Noise but changed `a_lfo2_phase` from 0 to 0.5,
  avoiding an all-zero initial history.

Neither copy changed the main random LFO, its default routes, or the saved
companion magnitude. Measurements below compare Motion 1 against Motion 0 over
the same held-note window. Relative RMS is the RMS of the difference divided by
reference RMS; it is not a perceptual audibility rating.

| Preset | Definition | 3-second held delta | 12-second held delta |
|---|---|---:|---:|
| SC01-066 Close | Original Noise companion | 0.003747 | 0.057167 |
| SC01-066 Close | Sine companion | 0.081255 | 0.083442 |
| SC01-066 Close | Noise companion, phase 0.5 | 0.042522 | 0.061974 |
| SC01-070 Bite | Original Noise companion | 0.002507 | 0.049273 |
| SC01-070 Bite | Sine companion | 0.063190 | 0.075526 |
| SC01-070 Bite | Noise companion, phase 0.5 | 0.048739 | 0.059402 |

For the original definitions, the first 1.5 seconds were **byte-identical** at
Motion 0 and 1. The first changed sample appeared at approximately 1.566 seconds.
During the last six seconds of the 12-second original renders, relative deltas
were 0.076098 and 0.075091: sustained playing exposes the working random motion.

The sine copies produced first-second deltas of 0.074364 and 0.052324. All four
Motion-0 sine-copy renders were byte-identical to their original counterparts;
the four phase-0.5 default comparisons were also byte-identical. All 24 native
renders succeeded without nonfinite samples. The largest sample peak was
0.051273. These are bounded single-note results, not a replacement for the bank's
polyphony and control stress suite.

## Recommendation and scope

The parent selected **Noise companion phase 0.5 when the authored main shape is
Noise**. This is the smaller tested change and retains irregular movement while
avoiding the companion's all-zero initial history. Leave the main Noise LFO phase,
its authored routes and the companion's zero default magnitude unchanged. The
measured three-second deltas of 0.042522 and 0.048739 support this integration
direction without establishing artistic audibility.

A static scan found **95 affected presets across 21 families**: Keys 5, Pads 20,
Ensemble Hybrids 12, Drones and Air 24, Bass 4, and Percussion and Transitions 30.
This rule leaves all other shapes and tuned step sequences unchanged. Retest all
95 affected files after integration, including default preservation, Motion,
wheel, dense polyphony and combined controls. Update audits that previously
required the companion phase to match its main LFO; this exception is deliberate.

The sine companion remains a tested alternative if a consistent regular component
is desired: Motion would then add a deterministic sway to the existing random
texture. That is a broader musical change than the chosen phase adjustment.
With phase 0.5 the immediate amount still depends on the random history, so any
remaining weak cases need to be identified in the full rerun. Neither experiment
establishes that one texture is artistically preferable, and the two tested
presets do not certify the effect in all 95 cases.

## Evidence and provenance

Fixtures, untouched native PCM, experimental copies, native JSONL and detailed
measurements are retained in `/workspace/scratch/9118c5c72ad8/a37-motion/`.
`summary.json` includes each experimental patch hash and the full affected-ID list.
Only this report was added to the repository; no bank, compiler, renderer or QA
implementation was edited.

| Audited input | SHA-256 |
|---|---|
| Catalog | `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331` |
| Native renderer | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| LFO implementation | `ecbc8af269df53c5d895c948141f010a8571d4ff5338374e021b9a49c3a01ea6` |
| Compiler at inspection | `944b6d9207bd837ffa8c62870af576d069069b58060ef8d1cf15805ddd1f8bf0` |

Human audition, perceived quality, and composer endorsement are not claimed.
