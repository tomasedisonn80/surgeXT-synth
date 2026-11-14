# Wave 8, agent 48: sustained notes and modulation cycles

## Purpose and result

The principal 23-scenario audit uses short active windows. Those windows do not
necessarily traverse a complete 16-step sequence or the startup and later
interpolation history of a slowly running Noise LFO. This supplement adds two
longer native renders for every preset, using the actual expanded recipe rate
to select the hold duration.

The corrected bank passes this supplement across **1,000 presets and 2,000
scenarios**, with **zero failures and zero warnings**. Its largest sample peak
is **0.7811339497566223**, below the 0.8 headroom warning threshold.

The first complete sweep identified ten headroom warnings and a largest sample
peak of 0.8896784782409668. All ten warnings occurred in the saved-default
dense-note fixture. Gain-only corrections were then applied and all ten changed
presets were rendered again with the exact same fixtures. Neither sweep
reported nonfinite audio, silence, excessive mean DC, or the documented
mono-energy warning.

These are measurements of native synthesized output. They do not constitute
human listening, certification of musical quality, or composer endorsement.

## Fixtures

The implementation is
[check_sustains.py](../../../scripts/scoring/check_sustains.py). It reuses the
musical note selections and measurement criteria from
[qa_bank.py](../../../scripts/scoring/qa_bank.py), and expands the authored
category recipes through
[build_bank.py](../../../scripts/scoring/build_bank.py).

| Fixture | Notes and velocity | Macro state |
| --- | --- | --- |
| `sustain_default_stress` | The principal audit's 16-note dense chord, or one note for a mono preset; velocity 127 | Saved defaults |
| `sustain_aggressive` | The principal audit's three-note chord, or one note for a mono preset; velocity 127 | Macros 0–5 at 1; Attack and Release at 0 |

Both fixtures use 48 kHz stereo float32, 120 BPM, expression CC11 at 127,
modulation wheel and channel pressure at zero, seed 1000, and a fresh native
engine for each scenario. The saved master is verified at 0 dB. The renderer
restores the preset's saved gain; the protective voice gain remains part of the
preset. Audio is measured without normalization, limiting, or PCM repair.

For an authored motion rate `r` in hertz at 120 BPM, the minimum held time is:

| Main motion shape | Saved-default minimum hold | Aggressive minimum hold |
| --- | --- | --- |
| Noise | `max(12, 4 / r)` seconds | `max(12, 4 / effective_rate)` seconds |
| Step | `max(12, 16 / r)` seconds | `max(12, 16 / effective_rate)` seconds |
| Other | `max(12, 2 / r)` seconds | `max(12, 2 / effective_rate)` seconds |

For the aggressive fixture, `effective_rate` is `4 * r` for unsynchronized
motion, reflecting the Motion macro's two-octave rate route. It remains `r`
for synchronized motion. Step coverage counts individual steps, so 16 steps
cover the authored loop. The script checks the FXP's saved LFO shape, rate,
synchronization flag, and master against the recipe before rendering.

The calculated time is rounded upward to a 32-frame native block boundary,
then extended by one block to pass the nominal final cycle boundary. Actual
holds span **12.000667–76.191333 seconds**. The plan covers 521 sine, 209 triangle,
118 step, 95 noise, 29 square, and 28 saw-shaped presets. The two fixtures per
preset represent approximately **8.6107 hours** of rendered stereo audio,
including a two-second tail after each hold. The final frame-count total is
8.61069925925926 hours.

The two-second tail is deliberately bounded. This supplement does **not** test
complete release settlement. It also does not exhaust all random seeds,
controller combinations, transport positions, note ranges, or host behavior.

## Measurement and acceptance

The whole float32 render is measured for finite output, sample peak, silence,
per-channel mean DC, and mono energy. Active-window and full-render DC are both
checked. The existing thresholds are unchanged:

| Measure | Criterion |
| --- | --- |
| Nonfinite samples | Any nonfinite sample fails |
| Sample peak | A peak at or above 0.98 fails |
| Additional headroom | A peak above 0.8 produces a warning |
| Silence | A peak at or below 0.00001 fails |
| Mean DC | Absolute mean at or above 0.003 in either channel or active window fails |
| Mono energy | A mono-energy ratio below 0.1 with RMS above 0.0001 produces a warning |

Peak values are sample peaks; reconstructed intersample true peaks are outside
this check. Native frame counts and peaks are cross-checked against the PCM
measurement. Failed rows retain raw audio for diagnosis. Successful rows retain
the complete numerical report and PCM SHA-256; raw audio is deleted unless
`--keep-audio` is requested.

## Initial warning evidence

The initial manifest was
`8396df6694b92e644f027e90014be629e3c3f542c9d9403c69cb72a0b5b6bee1`.
The following later peaks were absent from the shorter audit's warning set:

| Preset | Name | Hold, seconds | Initial peak |
| --- | --- | ---: | ---: |
| SC04-046 | Winter Fold Close | 72.728 | 0.800554693 |
| SC04-061 | Lantern Organ Close | 12.000667 | 0.862136781 |
| SC04-062 | Lantern Organ Open | 12.000667 | 0.816203237 |
| SC04-063 | Lantern Organ Bloom | 12.000667 | 0.819301128 |
| SC04-065 | Lantern Organ Veil | 12.000667 | 0.817189693 |
| SC06-026 | Harbor Hum Near | 30.77 | 0.889678478 |
| SC06-028 | Harbor Hum Open | 30.77 | 0.872960806 |
| SC06-029 | Harbor Hum Drift | 40.000667 | 0.870417178 |
| SC06-096 | Deep Listening Near | 26.667333 | 0.809503496 |
| SC07-062 | Soft Pedal Round | 12.000667 | 0.844616890 |

Original complete reports for these ten presets, together with the full plan,
summary JSON, and summary CSV, are preserved under
`../sustain-final/before-headroom-corrections/`. The combined
`warning-evidence.json` SHA-256 is
`28fa9b8ac0c69d89ae83612c1c5312bd94e66dc299109631299ef8933524e9c0`.

## Binding and reproduction

Each result binds the exact FXP hash, catalog entry, expanded recipe, fixture,
native executable, audit script, compiler script, QA helper script, both
configuration objects, and NumPy version. A cached pass is reused only when
that entire binding matches. The manifest hash is included in the plan and
summary, allowing unchanged per-preset reports to survive a targeted gain
correction without accepting changed preset bytes.

| Input | SHA-256 |
| --- | --- |
| Native renderer | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| Sustain script | `d473c76bfe5208e65bcc97f6935c169e0878cb008324a731420e524ac96d90cd` |
| QA helper | `203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681` |
| Compiler script | `f04372e5711525838a003e1b0235a9513d77f52b962036918e4262388af39ae2` |

Run from the repository root:

```sh
python scripts/scoring/check_sustains.py \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --output ../sustain-final --workers 6
```

The worker limit is six. The output includes `plan.json`, a resumable report for
every preset under `presets/`, `summary.json`, and `summary.csv`. `--ids` selects
exact preset IDs for targeted work, while a subsequent full invocation restores
the complete-bank summary and verifies every cached binding.

## Final corrected rerender

The root agent applied gain-only changes to the ten flagged presets. A full
invocation with two native workers then reused exactly **990** unchanged,
matching reports and rendered the **ten** changed presets again. This produced
20 new native scenarios, preserving the 2,000-scenario final coverage. The
compiler, helper, renderer, and sustain script hashes remained unchanged.

The final manifest SHA-256 is
`ddbb4781424430cc950daf6d610ec7025c743d7c435b404747c5821e65ec9369`.

| Final measure | Result |
| --- | --- |
| Presets passed | 1,000 / 1,000 |
| Native scenarios represented | 2,000 |
| Failures | 0 |
| Warnings | 0 |
| Nonfinite samples | 0 |
| Largest sample peak | 0.7811339497566223, SC07-064, saved-default stress |
| Largest absolute channel/active-window mean DC | 0.00010781144568979819 |
| Smallest mono-energy ratio | 0.21419469260355545 |
| Corrected ten-preset peak range | 0.7199852466583252–0.720000684261322 |

An additional integrity inspection verified every current FXP against the
manifest, every report's complete dependency and fixture binding, and the native
renderer argument echoes for all 2,000 scenarios. Note sets, sample rate, hold,
duration, velocity, tempo, macros, controllers, seed, format, frame count,
identity, and restored gain reference matched. It also verified that each held
duration meets its documented cycle/step goal.

Final machine-readable evidence is in `../sustain-final/summary.json`,
`summary.csv`, `integrity.json`, and the 1,000 individual `presets/*.json`
reports. The summary JSON SHA-256 is
`5086b1d9a3b8b9dd6e9a96d4d010b3de4dab4b27ff14b0d00d5b9ffadf680c8e`;
the summary CSV SHA-256 is
`10db41e99b748180a6c14197d0904b395958bcea425a43150f02a569477dbe7f`.
The ordered per-preset report-hash digest in `integrity.json` is
`0e31748637013f5d755a0bb25d7d51c3ad9dad55cba6795853f0088514227585`.

Musical audition remains pending. The result establishes the documented
bounded native sustain/cycle checks under the tested gain and controller
conditions; it does not establish universal suitability across productions or
the behavior of every possible later random history.
