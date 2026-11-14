# Wave 6 / Agent 34: Keys native audio review

## Outcome and scope

**All 100 Keys presets pass the five recorded technical gates: 500 gate results and 2,300 native render scenarios, with zero failures and zero warnings.** The review covers `SC01-001` through `SC01-100`, all 20 families and their five variants. All 800 macro endpoint responses are resolved above the runner’s measurement thresholds, and all 100 repeated defaults are byte-identical. These are technical findings; human musical audition and composer endorsement remain unclaimed.

The [machine-readable evidence](w06-a34-keys-audio-evidence.json) records all 100 input-report hashes, distribution summaries, focused comparisons, and provenance. The source is the unity-master generation of the [catalog](../manifest.json), not the superseded master-level run. Existing [native QA](../../../scripts/scoring/qa_bank.py) records were supplemented with six fresh-engine dry renders for the three closest family pairs. No preset, recipe, compiler, renderer, or main QA implementation was edited.

## Evidence integrity and coverage

For every preset I checked the actual FXP SHA-256 against the catalog and report, the normalized catalog-entry digest, renderer and QA-script digests, configuration digest, complete ordered fixture list, and fixture digest. Every native row agrees with the expected name, played notes, sample rate, velocity, hold/render durations, tempo, macro values, wheel, pressure, and expression. Frame counts agree with duration and sample rate, and all outputs report stereo interleaved float32 with serialized global gain restored. All five gate objects are present and passing. No report from the archived generation entered the review.

Each preset contains default/repeat, register endpoints, soft/hard velocity, a three-note chord, eight individual macro maxima, dry Space, the aggressive macro combination, wheel, pressure, two tempos, 44.1 kHz, and a 16-note stress chord. Thus all 100 presets have the same 23-scenario scope; the default reference is 48 kHz, velocity96, tempo120, saved0dB master. The tests cover defined endpoints and combinations, not every possible performance or downstream gain setting.

## Audio distributions

The following summaries are computed from the 100 default records. RMS values are linear full-scale amplitudes; full-render RMS includes release time, while active RMS covers the held-note window.

| Measurement | Minimum | Median | Maximum |
|---|---:|---:|---:|
| Sample peak | 0.023817 | 0.053267 | 0.090170 |
| Full-render RMS | 0.000905 | 0.004973 | 0.019386 |
| Active RMS | 0.001382 | 0.007626 | 0.031355 |
| Spectral centroid | 188.4 Hz | 438.5 Hz | 8,842.5 Hz |
| Mono energy ratio | 0.990683 | 0.999667 | 0.999969 |
| Final500ms RMS | 7.99e−15 | 1.89e−12 | 1.02e−9 |

Across all 2,300 scenarios, the largest sample peak is **0.770132**, from **SC01-058 Drawbar Lantern Glass**, in the 16-note stress fixture. The lowest mono-energy ratio is **0.733597**, from **SC01-064 Chapel Light Float**, with aggressive macros. The maximum absolute active-window DC is **0.0000252452**. There are zero reported nonfinite samples. All default tails have fallen at least142dB below their active RMS by the final measurement window; very small values approach the numerical floor and are not perceptual noise-floor measurements.

Hard-velocity active RMS exceeds soft-velocity RMS by **6.73–16.50dB**, with a median13.47dB. The category therefore has measured playing-dynamics response in these fixtures. Conservative polyphonic gain produces relatively quiet individual notes: **SC01-019 Ivory Frame Float** has the lowest default peak, and **SC01-030 Paper Hammers Bite** the lowest active RMS. Neither is below the protocol’s silence or quiet-default thresholds. This does not establish a commercial loudness standard; mix gain should be set using the intended phrase.

## Controls: corrected response and small margins

**SC01-048 Tin Toy Keys Glass** now measures a Contour waveform-relative RMS change of **0.355851** and spectral/envelope feature distance **0.140906**, against thresholds0.001 and0.002. The additional amplitude-decay route resolves the previously ineffective filter-only response. The current default sound remains the zero-Contour setting.

Two passing endpoints deserve focused musical judgment because their measured changes are small:

| Preset/control | Relative waveform change | Feature distance | Interpretation |
|---|---:|---:|---|
| SC01-023 Glass Hammer Glass / Contour | 0.001554 | 0.0000501 | Just above the waveform threshold; filter motion may be subtle in a cue. |
| SC01-070 Breath Harmonium Bite / Motion | 0.002504 | 0.001325 | Small change on a brief articulated sound. |

These are not failures under the declared protocol. They also should not be presented as evidence that every macro endpoint is artistically obvious. A listener can determine whether stronger body-contour or faster early motion would improve those two variants. No threshold was lowered, and no audio was altered to make them pass.

## Closest related variants

All 100 default PCM hashes differ. All **200 within-family pairs** have matching active fixtures, allowing comparison of normalized spectral-band energy and normalized active envelopes. The three smallest combined distances are Amber Clav Close/Felt0.024457, Felt Ember Close/Felt0.033986, and Drawbar Lantern Felt/Glass0.035579. The score is spectral L2 plus half envelope L2; it is an ordering aid, not a psychoacoustic scale.

I rendered both members of each pair dry (`Space=0`), using the same note within a family, velocity96, 48 kHz, three-second hold, seven-second duration, seed1000, and a fresh engine for each row. All six native renders completed with finite audio and peaks below0.1. For analysis only, the first three seconds of each waveform were divided by their own RMS before subtraction. Saved PCM remained unnormalized.

| Pair | Equal-RMS waveform difference | Normalized correlation |
|---|---:|---:|
| Amber Clav Close / Felt | 0.435562 | 0.905143 |
| Felt Ember Close / Felt | 0.127051 | 0.991929 |
| Drawbar Lantern Felt / Glass | 0.155638 | 0.987888 |

The differences survive removal of effects and overall level. Their source changes also explain why these remain related variants:

- **Amber Clav:** nominal attack4→9.6ms, decay420→483ms, cutoff2635→1612Hz, and stronger String-layer weighting. In the bounded dry spectral window, energy above2kHz falls from4.30% to2.02%.
- **Felt Ember:** nominal attack7.2→21.6ms, decay1.7→1.955s, sustain0.108→0.084, and reduced FM amount. Both outputs remain strongly fundamental dominated; their distinction is principally envelope/body behavior. A louder Felt preset alone is not evidence of better quality.
- **Drawbar Lantern:** nominal attack21.6→5.85ms, decay345→390ms, sustain0.616→0.66, and different cutoff/partial balance. Both are three-partial organ colors, so similar spectral summaries are expected. Glass need not be brighter than every other variant; its increase is relative to the family’s starting voicing.

The dry comparison establishes technical differences beyond gain and ambience. It does not establish that a listener will value every distinction. These three pairs remain appropriate candidates for a blind, level-matched phrase comparison.

## Remaining musical review priorities

**SC01-050 Tin Toy Keys Bite** has the category’s highest default centroid,8.84kHz. Its FM3 source, fractional ratios, inharmonic tag, and bright Bite transformation make this a plausible intentional color. It is nevertheless a useful foreground-comfort audition target, especially in its upper range. The quieter Felt and Close variants provide alternate starting points; a measurement cannot decide which suits a scene.

No automatic correction is justified by the observed integrity, headroom, mono, tail, or velocity results. The small-response controls and the especially bright toy-key variant are the concrete follow-up priorities. Universal genre suitability, exposed acoustic realism, perceived uniqueness, and Hollywood adoption are outside these findings.

## Provenance and reproduction

| Input | SHA-256 |
|---|---|
| Catalog | `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331` |
| Native renderer | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| QA implementation | `203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681` |
| Compiler | `944b6d9207bd837ffa8c62870af576d069069b58060ef8d1cf15805ddd1f8bf0` |
| Renderer source | `d6e7d6ed947ce6af7a2c5f476c0cedb3e52e2d92b05195d2507b6855c024366c` |

The native executable identifies engine base`dfdd2f94a`; its separately hashed renderer addition was then uncommitted. The catalog remained stable through this review. Dry-probe TSV, native JSONL, logs and original PCM are retained in `/workspace/scratch/9118c5c72ad8/a34-keys/dry/`; the compact repository evidence records their hashes and conditions. Re-run the native CLI with `--batch batch.tsv --fresh-engine 1 --seed 1000 --data resources/data` and the same executable to reproduce those probes. Later changed preset hashes require fresh review; this report describes only its bound inputs.
