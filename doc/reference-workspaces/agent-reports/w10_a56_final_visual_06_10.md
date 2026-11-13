# Wave 10 / Assignment 56 — Native visual acceptance, skins 06–10

> Curated historical assignment report. The a9 baseline review found the immediate edited-curve lag. The production fix and later Aurora source/material corrections passed final-source native and image reviews at 453df569. The Aurora supplement contains the settled metrics and remaining differences. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

> Current final source: `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, native run `43400507719`. The stale edited-filter issue is resolved in all five skins; Aurora height/material corrections are verified. Final source-specific metrics, actual image inspection and remaining nonidentical source/material details are recorded in [w10 a56 aurora hero followup](w10_a56_aurora_hero_followup.md). The a9 findings below are historical.


## Reviewed source and evidence

- Source: `a9d0d0a838632daf178255561def4622cd208258`.
- Native workflow: https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43393400045
- Independently checked workflow job `131778458976`: completed successfully, including native behavior tests, all ten baseline/edited captures, and independent audio-tap stress.
- Actual capture directory: `native-artifact-a9d0d0a`.
- Retained references: `doc/reference-workspaces/references`.
- All reviewed source PNGs are 1586 × 992. Capture evidence says `sourceDirtyAtConfigure=false`, `complete=true`, ten workspace records and ten corresponding edited images. It identifies native JUCE component snapshots attached to SurgeSynthProcessor, not operating-system desktop or DAW captures.
- Read-only source/image review. Comparison JPEGs are temporary inspection crops; the native PNGs and reference PNGs were not edited.

## Verdict for this source

**No new clipping, label collision, panel overflow or paint blocker was found in the inspected baseline compositions and changed regions of skins 06–10. These screenshots are not pixel-identical to the retained references.**

The final edited-state evidence needs a correction and recapture: the immediate edited screenshot can show an updated cutoff marker/value with the previously cached response curve. Source inspection identifies a preview refresh cadence mismatch in the capture sequence, rather than proving a permanently frozen GUI. Root and Assignment 59 have been notified. This report therefore approves the inspected baseline paint changes but does **not** give unconditional final acceptance to the edited screenshots.

## Inspection method

All five complete fresh baseline compositions were compared against their retained references. The following changed regions were then viewed at source-pixel resolution using paired reference/native crops:

| Skin / region | Original source coordinates |
| --- | --- |
| Tokyo oscillator 1 | x280–565, y117–420 |
| Tokyo filters | x1190–1586, y117–418 |
| Aurora filters | x1250–1586, y70–360 |
| Aurora selected FX detail | x1125–1586, y702–911 |
| Clear filter response and outside captions | x427–1180, y124–410 |
| Flow filter inspector | x1293–1586, y82–570 |
| Live mixer and MIDI performance lanes | x0–450, y601–992 |

The final two inspector crops deliberately begin/end inside surrounding page areas; cropped text at the crop boundary is not native clipping. Pixel differences between baseline and edited native PNGs were also measured inside the actual plot rectangles, independently of whole-workspace screenshot changes.

## 06 — Tokyo Industrial

The cyan-only wavetable mesh is present and stays inside the oscillator display. No secondary red mesh remains in the inspected oscillator. The real waveframe shape differs from the reference illustration.

The oscillator readout row is readable, with dark numeric-field backing. Filter numeric values are legible without introducing dark capsules across the whole panel. The real `WS DRIVE` and `BALANCE` captions fit their allocated control cells, and filter type/subtype menus do not collide with the readouts. Current captions such as Morph, V Skew, Saturate and Formant describe the native oscillator rather than repeating the reference's invented or mismatched parameter names.

The changed filter area x1190–1586/y117–418 has no new ring/value/caption collision. Numeric strings with units, including 2.40 kHz, 72%, 0.0 dB, 560.00 Hz and 46%, fit visibly in the fresh baseline.

Remaining fidelity differences include the native panel's flatter/darker surface and less pronounced metallic bevel, native browser/preset content, real filter nomenclature, and real signal/parameter values. These are visible differences; native functionality does not establish 1:1 appearance.

## 07 — Liquid Aurora

The four smaller filter dials fit the filter cards, with the new dark purple surfaces and colored rings. Cutoff/resonance labels and current values are separated from the graph and subtype controls. The corresponding source crop x1250–1586/y70–360 shows no overlap or clipped control text.

The selected nested effect detail now correctly displays `CHORUS` above its actual algorithm menu, with its real Rate, Depth, Mix, Width and Feedback values. The title fits in x1125–1586/y702–911; it does not inherit an unrelated fixed label. The dark material is visibly calmer than the previous brighter appearance.

The reference's large, many-layered luminous crossing wave composition is still substantially different from the native source-driven ribbon display. The native filter preview has simpler grid/decoration treatment; the FX detail omits the reference's decorative aurora flourish. Native algorithm-specific controls and real LFO/oscillator behavior also differ from the illustrative reference labels. These differences remain material to any literal 1:1 claim.

## 08 — ClearSignal Accessible

The combined response area stays within the retained outer frame. Its external dB labels, spanning −24 through +18, are readable in the left white gutter. Frequency captions below the frame include the rightmost 20k without overrunning the panel. The actual F1/F2 algorithm legend and `FILTER 1 & 2 RESPONSE` caption fit in the dark top band. The two curves and current cutoff points remain inside the black frame.

Source-size review of x427–1180/y124–410 found no dB-label collision, graph-title collision, truncated algorithm legend, right-edge frequency clipping or overlap with the controls below. The full composition also shows the retained two filter rows and four macro controls fitting their panels.

The native curves visibly differ from the illustrative reference response, including the true response magnitude and resonance behavior. Native oscillator-specific parameters, modulation inspector values, FX slots and actual output spectrum remain different from the reference. The reference's heavier caption treatment and some control proportions remain visually distinguishable. This review is not a platform accessibility certification.

## 09 — Signal Flow Cartographer

The adjusted cutoff dial fits the inspector with clear separation between its top caption, ring and 4.20 kHz readout. The stronger mint arc is visible, and the reduced inset does not obscure the pointer or value. The smaller surrounding controls remain spaced and legible.

The true external dB captions fit the existing left margin of the response graph. Frequency captions, including 20k at the right edge, are readable. The inspected region x1293–1586/y82–570 shows no new overlap among response title, dB caption, magnitude ticks, grid and frequency captions.

The actual native graph domain remains −42…+18 dB and is not changed to reproduce inconsistent reference numeric marks. The real routing diagram uses native routing modes, actual sources and genuine controls, so its labels and routes do not duplicate every illustrated node/port. The actual analyzer output and real modulation values remain distinct from the reference artwork.

## 10 — Live Performance Deck

The reduced Amp Gain face fits the mixer without changing the panel bounds. Thin fader rails and smaller caps are visible and remain legible. Mixer labels and current dB readouts fit in x8–448/y601–776. The caps have a visible warm material, and no clipping or text/cap overlap was found in the inspected mixer.

The four performance MIDI lane heights now closely follow the retained reference composition. Capture evidence explicitly records real incoming MIDI before the baseline notes: signed pitch bend +6616 (14-bit value 14808), CC1=104, CC11=56 and channel pressure=85. This is supported by processor assertions in the capture test; the lanes are not painted stand-ins.

The actual lane region x19–171/y831–959 has clear lane outlines, current fill positions and distinct Pitch/Mod/Exp/Touch labels. The adjoining Scene Controls panel uses real scene mode, split-note and voice controls.

Material differences remain in the central real waveform ribbons, XY field and mixer/amp accent treatment. Reference-only three independent key zones and six snapshot states are not implemented as fictional native features; the real two-scene behavior remains. The keyboard/range-strip content, scene level and actual output displays must therefore be described as native adaptations.

## Edited response capture issue — concrete evidence

The capture records all edit `filter1.cutoff` to normalized 0.283. Original baselines differ by skin. Whole-workspace pixel differences alone do not show that the response curve updated.

Independent raw-PNG comparisons inside the actual filter-1 plot rectangles yielded:

| Skin | Plot rectangle [x,y,w,h] | Changed pixels | Changed local bounding box |
| --- | --- | ---: | --- |
| Tokyo | [1138,162,233,107] | 156 | [79,25,169,51] |
| Aurora | [1272,145,181,54] | 0 | None |
| Clear | [480,166,696,212] | 310 | [191,105,438,135] |
| Flow | [1325,432,236,134] | 153 | [65,41,181,59] |
| Live | [478,723,180,44] | 0 | None |

For the larger plots, changed areas are consistent with moved current cutoff points; the small Aurora and Live plots do not change at all in this immediate capture pair.

### Source explanation

- `src/surge-xt/gui/reference/Workspace.cpp`, Impl::update around line 4771: preview sampling, including filterCurves, is inside `if ((ticks++ % 8) == 0)`.
- The actual filter response is refreshed around lines 4804–4806.
- Workspace starts its timer at 20 Hz around line 7603.
- `Workspace::refresh()` around lines 7708–7711 calls update and repaint without resetting preview cadence.
- `src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp` around lines 3390–3400 changes the cutoff, processes four audio blocks, calls refresh once and immediately captures the edited image.

A single capture refresh does not necessarily reach the eighth preview sample. The normal GUI timer can subsequently refresh the curve; therefore this is not evidence of permanent display failure. Assignment 59 is investigating a legitimate refresh/invalidation contract and a focused plot-response regression test. Final edited image acceptance requires that corrected native run and new PNGs.

## Handoff

No source edits, manifest edits, original image modifications, commits or pushes were performed by Assignment 56. Temporary comparison files have prefix `analysis-only/w10_a56_`.

Root was notified immediately of the edited-capture discrepancy and of the source cadence explanation. Baseline paint review found no new blocker in 06–10. Preserve this source-qualified verdict if newer captures supersede a9d0d0a; do not relabel these images as belonging to a newer SHA.


## Subsequent Aurora corrective work

The substantial Aurora hero difference was revisited after this a9d0d0a baseline review. Concrete renderer and actual audible wavetable patches are documented in [w10 a56 aurora hero followup](w10_a56_aurora_hero_followup.md). Root applied them; the next native build/captures are pending. The above source-qualified a9 verdict is preserved and does not describe the newer candidate.


## Final source follow-up completed

Fresh source `4dbb414370c94932670e1b58cf6f5caecbe4c6f4`, workflow `43398309849`, resolves the edited-filter capture issue in all five reviewed skins. Source-size visual and independent raw-PNG checks are recorded in [w10 a56 aurora hero followup](w10_a56_aurora_hero_followup.md) and `analysis-only/w10_a56_verified_4dbb414.json`. The Aurora hero is materially closer but remains visibly different in vertical extent, luminosity and crest placement; no 1:1 claim is made. The earlier a9 observations above are historical.
