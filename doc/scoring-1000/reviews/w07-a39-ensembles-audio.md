# Wave 7 / Agent 39: Ensemble Hybrids native audio review

## Outcome and scope

**All 100 reviewed Ensemble Hybrids presets pass their 500 recorded technical gates and 2,300 native scenarios.** All current input bindings validate, all 100 default repeats are byte-identical, and all 100 default PCM hashes differ. **796 of 800 macro responses resolve** above the runner's thresholds. Four Midnight Unison Motion responses remain unresolved; four Frost Divisi Motion responses are small enough to warrant the requested additional review.

This is an independent review of recorded native measurements, supplemented by three fresh dry renders. It is not a listening panel, an acoustic-realism evaluation, or composer endorsement. The [machine-readable evidence](w07-a39-ensembles-evidence.json) contains all 100 report and preset hashes, distributions, control findings, and supplemental measurements. The reviewed [catalog](../manifest.json) has SHA-256 `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`; later corrections require evidence for their new hashes.

## Integrity and coverage

For `SC05-001` through `SC05-100`, I checked the actual FXP hash against both catalog and report, the normalized catalog-entry digest, renderer and QA-source digests, configuration digest, complete ordered fixture list, and fixture digest. Every native row agrees with its expected name, input path, notes, rate, velocity, durations, tempo, macros, wheel, pressure, and expression. Frame counts agree with duration and sample rate. Native and independently measured peaks/RMS agree within numerical serialization tolerance.

All outputs report stereo float32, successful native execution, restored serialized gain, and zero nonfinite samples. I reapplied the current measurement failure rules to each stored metric set; none fail. Direct XML inspection also confirms unity master and native analog amplitude-envelope mode in all 100 files. Inputs remained stable through finalization.

Each preset has 23 scenarios: default/repeat, register endpoints, soft/hard velocity, chord, eight macro maxima, dry Space, combined aggressive controls, wheel, pressure, two tempos, 44.1 kHz, and dense polyphony. This establishes behavior within those conditions, not every possible performance or downstream gain setting.

## Audio distributions and limits

| Default measurement | Minimum | Median | Maximum |
|---|---:|---:|---:|
| Sample peak | 0.019485 | 0.056106 | 0.084225 |
| Full-render RMS | 0.001344 | 0.006766 | 0.020339 |
| Active RMS | 0.002039 | 0.010658 | 0.031866 |
| Spectral centroid | 149.6 Hz | 375.0 Hz | 1,919.4 Hz |
| Mono energy ratio | 0.755922 | 0.999237 | 0.999961 |
| Final 500 ms RMS | 2.22e−13 | 2.86e−10 | 1.01e−8 |

Across all scenarios, maximum sample peak is **0.727992**, from **SC05-065 Chapel Pipes Motion** under 16-note stress. Maximum absolute active/full channel DC is **0.000116489**. The lowest mono energy ratio is **0.329300**, from **SC05-004 Linen Bows Muted** with aggressive macros; default mono ratios are substantially higher. There is no integrity, headroom, DC, or mono-threshold failure.

Hard-velocity active RMS exceeds soft-velocity RMS by **7.48–17.20 dB**, median **12.38 dB**. The category therefore has measurable playing-dynamics response in these fixtures. **Felt Pizzicato Swell** has the quietest default peak; **Amber Mutes Section** has the lowest active RMS and highest centroid. These are concrete audition priorities, not evidence of defects or commercial loudness compliance. Different family test notes also limit cross-family spectral comparisons.

Every default tail is at least **117.9 dB** below active RMS at the final measurement window. Such small values approach numerical floors and are not perceptual noise-floor measurements. Maximum-Release fixtures have a separate limit: their tail observation is capped at six seconds. **Frost Divisi Swell** remains about **15.89 dB** below active RMS at that endpoint. The result supports a long release, not a claim that every maximum-release setting has fully settled.

## Motion corrections still required

| Presets | Control | Relative waveform change | Feature distance | Finding |
|---|---|---:|---:|---|
| SC05-096–099 Midnight Unison Close/Section/Swell/Muted | Motion | 0.000127–0.000202 | 0.0000075–0.0000301 | Unresolved under the declared protocol. |
| SC05-016–019 Frost Divisi Close/Section/Swell/Muted | Motion | 0.001306–0.001945 | 0.001096–0.002215 | Technically resolved, but below the additional 0.01/0.01 review screen. |

No other macro meets that small-response screen. The screen is an ordering aid, not a perceptual standard. These eight cases align with the separately diagnosed Noise-LFO startup behavior: their three-second fixtures contain little developed random motion. The planned correction gives only the zero-default Noise companion a nonzero start phase while preserving the authored main LFO. This report does not treat that future change as already tested. Re-render affected presets and compare their new hashes before closing these findings.

## Felt Pizzicato Swell: actual entrance

The [recipe](../../../scripts/scoring/recipes/05_ensemble_hybrids.json) now gives Swell a **nominal 0.4-second Attack setting**, amplitude decay 0.986 seconds, and String decay 0.86 instead of 0.81. Native [analog ADSR code](../../../src/common/dsp/modulators/ADSRModulationSource.h) charges a capacitor toward a threshold; the parameter is not a promise of a literal 400 ms acoustic rise.

I rendered Close, Section, and Swell with Space zero, MIDI note 65, velocity 96, 48 kHz, a three-second hold, seven-second duration, seed 1000, and a fresh engine per row. All three renders are finite and below the peak limit. Five-millisecond RMS bins show:

| Variant | First-second peak RMS bin | First 20 ms RMS | First 100 ms RMS |
|---|---:|---:|---:|
| Close | 0–5 ms | 0.019479 | 0.012023 |
| Section | 0–5 ms | 0.025037 | 0.016564 |
| Swell | 105–110 ms | 0.003455 | 0.009850 |

Swell first reaches 90% of its first-second local RMS peak in the **95–100 ms** bin. Its initial energy is substantially reduced and its maximum arrives later, supporting the softened-entrance description. These are waveform-window measurements, not exact envelope-state or perceptual onset times. The user guide's nominal-time wording is appropriate.

## Related variants and interpretation

All **200 within-family pairs** use matching active notes and hold times. The closest coarse-feature pair is **Felt Pizzicato Close/Section**, distance **0.009441** under the runner's spectral-plus-envelope comparison. Their dry signals still differ after constant-level matching: relative waveform difference **0.142827**, normalized correlation **0.989800**. RMS equalization occurred only in the comparison calculation; stored PCM remains unnormalized.

The source distinctions are concrete: nominal attack 5.2 versus 9.6 ms, filter cutoff 2,210 versus 3,510 Hz, secondary-layer level 0.1875 versus 0.3375, and release 0.208 versus 0.432 seconds. Thus the pair is not distinguished solely by ambience or overall gain. Its close feature profile still makes it a useful listener A/B priority. Different hashes and waveform differences do not prove that a listener values every variant.

String, brass, reed, organ, and choir names denote synthetic colors implemented by native oscillators and filtering. These measurements do not establish exposed acoustic or vocal realism. The actionable engineering follow-up is the eight Motion cases; final musical usefulness remains a separate judgment in a representative cue.

## Provenance

The native executable hash is `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`; QA source is `203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681`. The executable reports engine base `dfdd2f94a`; the renderer addition is separately hash-bound in the overall project evidence. Supplemental TSV, JSONL, logs, and raw PCM remain in `/workspace/scratch/9118c5c72ad8/a39-ensembles/`. Only this report and its evidence JSON were added to the repository. No preset, recipe, compiler, renderer, or QA implementation was changed.
