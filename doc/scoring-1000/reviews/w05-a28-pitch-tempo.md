# Wave 5 / agent 28: pitched sequence and tempo audit

## Scope and outcome

Independently inspected **all 100 Pulses and Sequences FXPs**, including **all 20
pitched variants across four families**, and compared their serialized behavior
with the native LFO implementation. A bounded native probe rendered Amber Ladder
Soft at 60, 120 and 180 BPM with Motion at zero and one: **six renders, 96 measured
step centers**. No interval-routing or tempo defect was found in the reviewed
bank. This report does not certify musical listening or professional adoption.

The reusable [sequence audit](../../../scripts/scoring/check_sequences.py) does
not import the bank compiler. It parses the actual FXPs, verifies their manifest
hashes, checks the complete category, and optionally renders the six native
fixtures. That independence makes it useful for detecting later compiler changes.

## Programmed intervals and Motion

The active pitch route is source17, source_index1, depth12 semitones. The extra
LFO amplitude mode is0 (unscaled) in every pitched patch. In the native
[LFOModulationSource implementation](../../../src/common/dsp/modulators/LFOModulationSource.cpp),
output0 multiplies the waveform by the LFO envelope and magnitude; output1 is the
raw waveform unless extra amplitude scaling is enabled. Consequently, raising
Motion changes filter, gain and pan movement without proportionally detuning
these programmed intervals. The independent audit verifies the relevant source,
index, depth, mode, waveform and step data, rather than relying on macro labels.

| Family | Preset IDs | Non-Sparse interval set, semitones above played root |
|---|---|---|
| Amber Ladder | SC09-081–085 | 0, 2, 4, 7, 9, 12: major pentatonic |
| Night Thirds | SC09-086–090 | 0, 3, 5, 7, 10, 12: minor pentatonic |
| Open Fifths | SC09-091–095 | 0, 7, 12: root, fifth and octave |
| Prism Arcs | SC09-096–100 | 0, 2, 4, 6, 8, 10, 12: whole tone |

Each Sparse variant instead stores octave accents at steps0 and8, with the root
on the remaining14 steps. Its corrected role explicitly describes that exception.
Maximum step serialization error is below one millionth of a semitone. This is
parameter precision, not a claim about every oscillator's acoustic intonation.

All100 presets use16 steps, loop positions0–15, zero shuffle, zero deformation,
and trigger mask0. They modulate an existing held voice: they do not generate
MIDI notes, insert guaranteed silent rests, or retrigger the amplitude/filter
ADSR for each step. The16 other families have no pitch route from this sequencer.
The companion Motion LFO does not introduce a second pitch route.

## Tempo and the meaning of a step

Surge's native processing advances one sequence step per completed LFO phase.
For these files, step frequency is:

`steps per second = 2^(stored log2 rate) × BPM / 120`

The tempo ratio comes from the native
[SurgeSynthesizer implementation](../../../src/common/SurgeSynthesizer.cpp).
No macro routes to synchronized LFO rate in this category. At120BPM there are two
quarter notes per second, so **a sixteen-step sequence is not automatically a
sixteenth-note pattern**.

| Authored rate at120BPM | Families | Steps per quarter note | Step duration | Triplet-variant rate |
|---|---:|---:|---|---:|
| 1Hz | 1 | 0.5 | Half note | 1.5Hz |
| 2Hz | 5 | 1 | Quarter note | 3Hz |
| 3Hz | 3 | 1.5 | Quarter-note triplet | 4.5Hz |
| 4Hz | 11 | 2 | Eighth note | 6Hz |

Every Triplet variant applies exactly a3:2 acceleration relative to its family;
the other four variants preserve that family's base rate. For the three3Hz
families,4.5Hz gives nine steps per four quarter notes. It is therefore correct
that the updated guide promises a relative speed change rather than a universal
triplet subdivision or one-bar loop. Copper Transit and Tilted Grid retain their
3–3–2 and5–5–6 accent identities outside Sparse; their Sparse role overrides now
match the replaced pattern. Linen Motor now describes its four-step accent thread.

The stored trigger mode is0, which native `SurgeStorage.h` defines as free running.
Sequence phase can therefore relate to host song position. This audit's render
fixture starts PPQ at zero and advances it; it does not test every DAW's seek,
loop, stop, restart or automation behavior. The guide correctly keeps those
session-level operations outside the measured claim.

## Native probe and observations

Fixture: SC09-081 Amber Ladder Soft, MIDI60, velocity96,48kHz stereo float32,
10 seconds total,9 seconds held, seed1000, fresh engine per scenario. Space was
set to zero to prevent delayed prior notes from confounding interval estimation.
Motion was tested at0 and1; no PCM normalization or limiter was applied.

A Hann-windowed spectral peak estimate measures the strongest partial between
90 and1100Hz at each of the first16 step centers. Frequencies are octave folded:
this measurement independently checks pitch classes, while the octave routing is
covered by the structural audit above. It must not be described as a complete
fundamental-frequency or perceived-intonation test.

| BPM | Expected seconds per step | Largest center pitch-class error, either Motion setting | Largest transition residual |
|---|---:|---:|---:|
| 60 | 0.500000 | 0.040 cents | Below10ms analysis-grid resolution |
| 120 | 0.250000 | 0.228 cents | Below10ms analysis-grid resolution |
| 180 | 0.166667 | 0.357 cents | 6.7ms |

All96 centers matched the expected classes within the declared5-cent diagnostic
threshold. Transitions matched the expected timing within the declared20ms
threshold on a10ms tracking grid. The largest observed residual was6.7ms; numerical
zeros on grid-aligned transitions must not be interpreted as sample-accurate
latency measurements. Motion changed the rendered PCM and peak level while
preserving those class and timing observations. All six files were finite, with
peaks between0.03685 and0.04501.

## Reproduction and evidence binding

```sh
python scripts/scoring/check_sequences.py \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --output ../a28-sequence-audit
```

The output retains the exact fixture TSV, native JSONL, raw float32 renders and a
machine-readable `sequence-audit.json` containing all100 static records, six
measurements and hashes. A changed catalog causes the audit to fail if it changes
during execution. Rerun against later bank versions rather than reusing this
report's measurements for different bytes.

- Reviewed manifest SHA256: `a5408903059ac1d15540fb5700b2d8ce40a42ebe6344a37ab4edf46dbab1b1db`.
- Probe preset SHA256: `4a45e017399f1b8cf5e86b2422236028dfa1c9c0e7edf5238e4cf0f82d5333fd`.
- Renderer SHA256: `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.

Only the audit script and this review were authored. No preset, recipe, renderer,
main QA runner or compiler was changed.
