# Wave 10 assignment 59 — coherent native filter preview refresh

> Curated historical assignment report. The production correction and its native regression passed at both 4dbb414 and 453df569. The later addenda preserve exact-source plot counts and native sample deltas. Earlier pending statements describe implementation handoff, not current native verification. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Result

The stale edited plots were caused by workspace refresh cadence. The native filter cache already includes the inputs required for the displayed base response. Before this change, controls and the cutoff marker read current native values on each refresh, while the response curve was copied from the cache only every eighth refresh. At the normal 20 Hz workspace timer this allowed approximately 400 ms of response lag. The capture fixture issued one refresh after its real parameter edit and four audio blocks, so it could export the old response with the new knob/value/marker.

The production update now reads the existing cached filter responses on every workspace refresh. The expensive native sweep remains conditional on an actual cache-key change. No capture delay, repeated-refresh workaround, synthetic response, audio modification, or RNG manipulation was introduced.

## Source and evidence reviewed

- Baseline native source: `a9d0d0a838632daf178255561def4622cd208258`.
- Baseline captures: `native-artifact-a9d0d0a`.
- Shared repository HEAD when the final five-file review began: gallery commit `575d63db237a8ff8e4be170a3de2f76d011015f7`.
- Production sources reviewed: `Workspace.cpp`, `Workspace.h`, `PreviewCurves.cpp`, and `PreviewCurves.h`.
- Existing native coverage reviewed: `XTTestReferencePreviews.cpp` and `XTTestReferenceWorkspaces.cpp`.
- Agent 56 independently identified the eight-tick cadence and zero-delta Aurora/Live plots; this assignment reproduced the pixel counts across all ten native capture pairs.

Changed pixels inside each manifest's actual filter-1 plot rectangle, comparing the existing baseline and edited a9 captures:

| Workspace | Plot [x, y, width, height] | Changed pixels |
|---|---|---:|
| Obsidian | [552, 163, 271, 114] | 153 |
| Ivory | [526, 176, 237, 138] | 581 |
| Spectral | [495, 523, 212, 135] | 155 |
| Bauhaus | [999, 403, 205, 85] | 153 |
| Space-Age | [1210, 194, 230, 74] | 152 |
| Tokyo | [1138, 162, 233, 107] | 156 |
| Aurora | [1272, 145, 181, 54] | 0 |
| ClearSignal | [480, 166, 696, 212] | 310 |
| Flow | [1325, 432, 236, 134] | 153 |
| Live | [478, 723, 180, 44] | 0 |

The miniature Aurora and Live plots have no cutoff marker or dynamic numeric footer. Their zero deltas isolate the stale response. The other small deltas are consistent with the separately drawn marker/labels and the source-level cadence finding; they do not prove a changed response. These counts are liveness evidence, not reference-fidelity measurements.

The full baseline measurements, including changed-pixel bounding boxes, are retained in `analysis-only/w10_a59_a9_filter_plot_deltas.json`.

## Production change

A bounded patch was provided as `analysis-only/w10_a59_workspace_filter_refresh.patch` and applied by the root agent, which owns concurrent cosmetic edits in `Workspace.cpp`.

In `Workspace::Impl::update()`:

1. Determine which native filter previews the current panels require.
2. Retrieve those cached filter responses on every refresh, alongside the current native control values.
3. Keep oscillator, LFO, effects, modulation-row and browser sampling on the existing slower cadence.

The patch does not change any filter frequency, gain, amplitude, interpolation, display-domain, or filter-algorithm calculation. The native base response remains 256 dB samples on the logarithmic 13.57–25087 Hz domain, calculated by the existing independent 96 kHz filter-analysis implementation. No change to `PreviewCurves.cpp` or its public API was needed.

### Bounded-cache and thread rationale

`filterResponse()` uses four bounded, thread-local cache entries indexed by scene and filter. Its key contains processor identity, native filter type and subtype, cutoff, resonance and pre-filter gain. For filter 2, the effective key includes the linked/offset filter-1 inputs. Unchanged keys return their cached curve before the native sweep/FFT code.

The workspace performs at most two filter lookups per GUI refresh. An unchanged lookup copies at most 256 sample values and its caption; it does not run another filter sweep. A changed key retains the existing cap of one 8192-sample sweep and two fixed-size FFTs. This work remains in the message-thread update path; nothing was moved into paint or the audio callback. The other expensive preview families retain their previous cadence.

Existing preview tests cover native algorithms, repeated unchanged results, caller-owned curve copies, cutoff/resonance changes, scene isolation, filter-2 linking, unavailable states, and preservation of engine parameters/RNG. The early-return cache structure supplies the bounded-work evidence; no flaky wall-clock performance assertion was added.

## Regression and capture changes

Only `src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp` was directly edited by this assignment.

New case:

`Filter response miniatures follow each control and host edit on the next refresh`

Tags: `[reference-workspaces][reference-previews][reference-filter-refresh]`.

The test exercises the actual Aurora and Live workspaces, their manifest-defined filter-1 and filter-2 plot regions, native slider callbacks, and the real host parameter adapter. For each workspace it alternates four cutoff edits between slider and host origins.

For every edit the case requires:

- Exactly one public refresh synchronizes the slider and rendered response.
- The actual 256-point native response changes by more than 1 dB at some sample; all samples remain finite.
- The marker-free filter-1 miniature changes pixels.
- The independent filter-2 native curve and plot remain unchanged.
- The refresh leaves host parameters, patch dirty state, audible RNG and noise RNG unchanged.
- A subsequent refresh with the same key preserves both exact native samples and plotted pixels.

The test does not advance or reset the audio RNG and does not modify the clock. It tests a base-parameter preview, so no fabricated or substitute audio is needed. It also checks balanced parameter gestures.

The existing ten-workspace capture case now measures the actual filter-1 response before and after the existing real cutoff edit. It checks both finite native sample change (>1 dB) and changed pixels inside the manifest's plot rectangle. The capture audio sequence, parameter requests, headroom checks, host update, reset checks and screenshot timing are otherwise unchanged.

Each capture evidence record gains:

- `filterResponseChangedPixelsAfterControlEdit`
- `filterResponseMaximumDbChangeAfterControlEdit`
- `filterResponsePlotBounds`

The dedicated miniature test prevents the broader plot check from passing solely because a numeric label or cutoff marker moved.

## Verification completed

- `git diff --check`: passed after the production patch and test changes.
- `python3 scripts/reference-workspaces/validate.py --strict-overlap`: all ten packages passed. The existing ClearSignal envelope/LFO plot-control notices and intentional Flow overlay note remain.
- Inspected the full test diff and the bounded production patch.
- Independently reproduced the ten a9 plot-delta measurements above from actual native PNGs.
- Confirmed the final working changes are settled and unstaged; no commit or push was performed by this assignment.

## Remaining gate

The root agent will compile and run the new native case, the complete native behavior suite, and fresh ten-skin captures on the final source commit. This report does not claim that an unexecuted native gate has passed, nor that the reference images are pixel-identical. The previously passing a9 native run predates this fix and cannot validate it.

No further source edits are pending from assignment 59.

## Final native verification — 4dbb414

Verified native source: `4dbb414370c94932670e1b58cf6f5caecbe4c6f4`, [native run 43398309849](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43398309849), native job `131794574573`. Original captures are in `native-artifact-4dbb414`. The root verified artifact ZIP `14024815233` with SHA-256 `07c6824546bd9b645f6b67025ed6a9e7f4d6f7bebcc5f47554a3b8ef714de3ca`.

The previously pending native gate is now complete. This assignment independently read the fresh artifact logs: 42 behavior cases / 10,316,058 assertions passed; the twenty baseline/edited images were produced with 24,100,990 capture assertions passed; both normal and ThreadSanitizer AudioTap logs report PASS.

The prepared verifier checked clean, complete, exact-SHA capture evidence and all twenty 1586 × 992 PNGs. Every independently recomputed filter-plot pixel count exactly matches the native evidence, and each native measured response difference is finite and greater than 1 dB:

| Workspace | A9 plot pixels changed | Final plot pixels changed | Maximum native response change (dB) |
|---|---:|---:|---:|
| 01-obsidian-precision | 153 | 5,523 | 90.938 |
| 02-ivory-atelier | 581 | 6,031 | 90.938 |
| 03-spectral-observatory | 155 | 5,639 | 96.303 |
| 04-bauhaus-modular | 153 | 2,821 | 90.938 |
| 05-space-age-1977 | 152 | 2,844 | 98.448 |
| 06-tokyo-industrial | 156 | 5,158 | 80.068 |
| 07-liquid-aurora | 0 | 3,028 | 66.023 |
| 08-clearsignal-accessible | 310 | 24,419 | 92.888 |
| 09-signal-flow-cartographer | 153 | 8,115 | 100.952 |
| 10-live-performance-deck | 0 | 1,735 | 80.378 |

Aurora and Live now change 3,028 and 1,735 curve-only plot pixels, versus zero in a9. The new one-refresh miniature regression is included in the passing 42-case suite. The defect described earlier in this report is therefore verified fixed in the final native source.

Full final visual findings and residual reference differences are in [w10 a59 final native visual review](w10_a59_final_native_visual_review.md); machine-readable evidence and source-size boards are in `analysis-only/a59_final_visual_4dbb414/`. This liveness result does not establish pixel-identical reference fidelity.

## Subsequent final-source verification — 453df569

Latest verified source: `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, [native run 43400507719](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719), job `131801876413`. Fresh captures: `native-artifact-453df569`. Root independently verified artifact `14024468912`, ZIP 11,019,495 bytes, SHA-256 `4054102ba04157671f0d3464616fdd0575514f657a8c22b089f4eeabd74b7246`.

The complete verifier passed again on this later native source. All twenty PNGs decode at 1586×992; clean/complete source provenance matches exactly. Every independently recomputed filter-plot pixel count and every native reported dB change is identical to the 4dbb414 table above and matches the new evidence. Aurora remains 3,028 changed curve-only pixels and Live 1,735.

The new artifact logs independently show 42 behavior cases / 10,316,058 assertions passed, 24,100,990 capture assertions passed, and both normal and ThreadSanitizer AudioTap PASS results. The filter refresh correction remains verified through the later text/material changes.

Evidence: `analysis-only/a59_final_visual_453df569/comparison-evidence.json`. No further filter source change was made or required.
