# Fidelity continuation — 13 November 2026

This renewed pass starts at `a0d016fd59ef9aee1c96a720c6a2c5a0b9a56f35` and
responds to the request for closer reference matching, sixty assignments in ten
waves, ordinary publication to `main`, and genuine native screenshots of all
ten skins. The [current ten-wave ledger](ten-wave-ledger.md) records every
numbered scope and report. Assignments 1–30 in this directory belong to this
pass. The [earlier ledger](../ten-wave-ledger.md), engineering log and previous
delivery records are historical; their assignments are not added here.

## Completed delivery — 60 of 60 assignments

**All sixty scoped assignments across ten waves are complete**, with exactly
one numbered report per assignment and six reports per wave. Work ran in batches
within the three-worker-plus-coordinator limit. Completion describes actual
implementation, review and documentation handoffs; native acceptance is supported
separately by the exact-source evidence below.

The final tested implementation is
`6028da1b15b3263e459bbd7742d2fd0572adcc89`, accepted by
[native run 43504150637](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43504150637).
Its twenty unmodified native baseline/edited images are published in gallery
commit `e2f26e4dc711e1127e66b9aae92b1e94afd15345`.
[Assignment 60](w10-a60-final-delivery.md) independently verified the complete
Linux native gate, source provenance and gallery publication. Subsequent
changes to the final records are documentation only and do not change this
tested implementation.

| Wave | Scope | Completed scopes and evidence boundary |
| --- | --- | --- |
| 1 | References, native gaps, architecture and validation | 6/6; original-image audits and inherited baseline review |
| 2 | Shared materials, controls, plots, headers and buttons | 6/6; native checkpoint `a362667` |
| 3 | Obsidian, Ivory, Spectral, Bauhaus, Space and Tokyo | 6/6; native checkpoint `2a5a621` after the compile hotfix |
| 4 | Aurora, ClearSignal, Flow, Live, XY and browser | 6/6; native checkpoint `2a5a621` |
| 5 | Audible sources, capture state, terrain, faders, matrix and type | 6/6; native checkpoint `0708bc7` |
| 6 | Independent native gate and five paired visual reviews | 6/6; each report retains its actual capture revision |
| 7 | Measured corrections across all ten skins | 6/6; native checkpoint `ea674878` |
| 8 | Placement, audio, source integrity, bounds, packaging and native gate | 6/6; exact-source acceptance in assignment 48 |
| 9 | Five fresh paired reviews and modulation-state correction | 6/6; reviewed `ea674878`; final corrective regressions pass on `6028da1` |
| 10 | Final paint, guide, ledger, adaptations and delivery acceptance | 6/6; final source, native tests, gallery and scoped visual corrections accepted |

## Exact native checkpoints

Each passing row includes strict manifest/schema validation, the real native
test executable, twenty 1586 × 992 baseline/edited PNGs, and separate normal and
ThreadSanitizer audio-tap passes. Counts are native assertions, not visual
similarity measurements. Gallery commits contain unmodified capture bytes.

| Scope | Tested source | Native run and result | Gallery commit |
| --- | --- | --- | --- |
| Waves 1–2 | `a362667839b8e1d742cf3af94bb8710fa3c6d1fd` | [43486541422](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43486541422): 42 behavior cases / 10,316,058 assertions; 24,100,990 capture assertions | `418151a6200a35f4a7522ef346f0bde9263ea3fb` |
| Waves 3–4, failed attempt | `411578ae5eda04e92628380d2123e28b8a20f3ea` | [43490570167](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43490570167): Spectral chassis rectangle compile failure; no native pass | None for this failed attempt |
| Waves 3–4, explicit `Rectangle<float>` hotfix | `2a5a621241e28e15eda57ff5be3bad6dc3f36ff3` | [43491379963](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43491379963): 43 behavior cases / 10,317,666 assertions; 24,100,990 capture assertions | `48725235d6576e19d05ff24b524bb90b5d91f2ae` |
| Wave 5 | `0708bc76bb194a127064cf18b8849cf77373a7e7` | [43493343847](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43493343847): 43 behavior cases / 10,317,666 assertions; 24,101,092 capture assertions | `facc04210a6c93e8ef846d31cce3e7c65649b3d0` |
| Waves 7–8 | `ea674878313006afa0b9d1a2b1d3495b69755627` | [43497923810](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43497923810): 44 behavior cases / 10,345,938 assertions; 24,101,112 capture assertions | `762d888f7159558db472836a8f9b5f015c1f9427` |
| Final corrections, failed behavior attempt | `21c2a82813c4e57332bff19cf134a78ac9fe013d` | [43502323088](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43502323088): build/capture/tap checks passed; behavior 46/47 cases, ten faulty restored-slider assertions | Gallery publication skipped |
| Final accepted implementation | `6028da1b15b3263e459bbd7742d2fd0572adcc89` | [43504150637](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43504150637): **47 behavior cases / 10,395,101 assertions; 24,101,112 capture assertions** | `e2f26e4dc711e1127e66b9aae92b1e94afd15345` |

The ten failures at `21c2a828` compared the legal displayed slider step `0.618`
with raw processor normalization `0.6175675392`. The test now requires exact
equality with the declared range's `snapToLegalValue` result; no tolerance or
product behavior was changed. That failed workflow is retained as history and
is not final acceptance. The same successor also reduces Bauhaus blue-paper
coarse amplitude from `1.5` to `0.25` after an actual-image mottling finding.

[Assignments 31](w06-a31-native-gate.md) and
[48](w08-a48-native-acceptance.md) independently checked the earlier gates.
For the final source, [assignment 60](w10-a60-final-delivery.md) checked all
**30 gallery-file byte counts/hashes**, all **20 native PNGs at 1586 × 992**,
all **ten exact baseline/edited changed-pixel counts**, and the actual source
and fixture provenance. All **forty active audio peaks** were finite and in
`(0.0001, 1.0)`, spanning `0.040551204234362–0.654747426509857`.
All **ten resets** remained below `1e-6`; the maximum was
`6.643543049023037e-9`. Normal and ThreadSanitizer AudioTap runs separately
passed. The capture used a clean source and the real processor at 48 kHz /
512-sample blocks, recorded at `2026-11-13T20:47:19.186Z`.

## Accepted final corrections and visual scope

The five paired reviews retain their measured conclusions and genuine engine
adaptations. The final coordinator review accepted Spectral, Bauhaus and Aurora
hero changes and edited ClearSignal state on `6028da1`. A55's earlier actual
hero measurements remain explicitly tied to the separately retained, failed
`21c2a828` workflow; they are not relabelled as final-source measurements.

| Assignment | Completed outcome and acceptance |
| --- | --- |
| [54](w09-a54-modulation-state.md) | Per-refresh route text and contention-safe same-scene cache/selection; both new native regressions pass. Final ClearSignal physical text follows the real 115.14 Hz edit while normalized depth stays +0.64. |
| [55](w10-a55-final-hero-polish.md) | Shared Spectral source/floor camera, brighter open Bauhaus wires and continuous Aurora outer haze. The native terrain regression passes; final coordinator visual review accepts the intended improvements. |
| [56](w10-a56-final-material-clearance.md) | Final actual-image review accepts smoother blue paper without dark mottling and visible Width glyph/tick separation. Remaining texture/palette differences are measured explicitly. |
| [57](w10-a57-user-guide.md) | Guide covers the modified binary/data requirement, four revised source families, genuine controls and native adaptations. |
| [58](w10-a58-completion-ledger.md) | Report-backed 60/60 ledger and exact-source record reconciled; historical ledgers preserved. |
| [59](w10-a59-adaptation-compatibility.md) | Independent adaptation review complete; its route-contention and FX-label findings are corrected in the accepted source. |
| [60](w10-a60-final-delivery.md) | Independent exact-source native, audio, image, provenance and published-gallery verification passed. |

The final blue filter strip retains median RGB **40/81/111**. Blue-channel
residual RMS after removing each region's broad gradient is **0.887**, **0.760**
and **0.867** in the two reported Scene areas and Filter 2 header gap, versus
reference **0.715**, **0.657** and **0.921**. It is smoother and less varied than
the concept. Width lettering ends at y101, with visible background at y102
before the unchanged graduation. These are scoped measurements, not a whole-skin
fidelity score. The coordinator also corrected ClearSignal's heading to
`FX SLOTS (1–8)` and Live's to `Effects (8)`; the guide describes the actual
first-eight native FX scope.

All controls, meters, presets, routes and signal displays remain attached to the
real Surge XT processor. Original reference pixels are comparison inputs only;
fixed decorative materials and artwork remain distinct from measured state.
Native source topology, capability adaptations and some font/material
residuals remain. **Literal pixel identity is not claimed.**

## Separate platform status

At `2026-11-13 20:49:09 UTC`, exact-source
[platform workflow 43504150486](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43504150486)
had passed version setup; six Linux/Windows build jobs were in progress and
macOS was queued. This is the recorded A60 observation, not a completed
all-platform/package or installer-publication claim. DAW/device operation is
also separate from the passing Linux native component/offline-processor gate.

See the [user guide](../../reference-workspaces-user-guide.md) and
[native gallery](../native-gallery/README.md) for operation and published evidence.
