# Wave 10 assignment 59 — final native image verification

> Curated historical assignment report. This report preserves source-size 4dbb414 findings and the settled 453df569 follow-up for the Spectral caps, Bauhaus heading, Space captions and all ten filter responses. Native execution passes while the documented optical/material differences remain. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Result

The native filter-refresh defect is fixed and verified in all ten final screenshot pairs. Spectral's new playable source data, signed camera and material are visible in the genuine native hero. The three scoped cosmetic changes are also visible, with measurable improvements in cap fill, heading alignment and caption width.

The final images still differ from the references. Spectral remains a cleaner wire surface than the reference's dense craggy terrain; Bauhaus's output heading remains lighter; Space's first macro caption remains shorter and lighter. These are remaining visual differences, not evidence of a broken native control. No literal 1:1 pixel-identity claim is supported.

## Exact source and native gates

Verified native source: `4dbb414370c94932670e1b58cf6f5caecbe4c6f4`, [native run 43398309849](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43398309849), native job `131794574573`. Original captures are in `native-artifact-4dbb414`. The root verified artifact ZIP `14024815233` with SHA-256 `07c6824546bd9b645f6b67025ed6a9e7f4d6f7bebcc5f47554a3b8ef714de3ca`.

This assignment independently read the fresh artifact's build provenance and native/audio logs:

- **42 behavior cases / 10,316,058 assertions passed.**
- **All twenty baseline/edited PNGs were produced; 24,100,990 capture assertions passed.**
- Both normal and ThreadSanitizer AudioTap logs report PASS for boundaries, wrap, stalls, stereo ordering, lifecycle and no audio allocations.
- Capture evidence identifies the exact final SHA, a clean configured source, ten expected/completed captures and native JUCE component snapshots attached to the real processor.
- All twenty images independently decode at 1586 × 992.

The build uses an Xvfb/Openbox native desktop and offline processor audio. It is not an OS hardware-driver or DAW certification. Platform-matrix results are outside this image-review assignment.

## Method

The prepared script `analysis-only/a59_verify_final_native.py` produced source-size, unscaled reference/a9/final comparison PNGs. The script first reproduced assignment 55's original Bauhaus and Space raster bounds/counts exactly, then verified the fresh source provenance and independently recomputed every filter-plot delta.

All pixel metrics below use original PNG pixels. The typography masks use the same rule as assignment 55: every RGB channel must be below 85. The Spectral cap measurements use the previously measured value-dependent handle centres (reference y=639; native y=633), taking channel medians over x=77…83 three pixels above/below the centre. The centre difference is not treated as a paint-only defect.

The three cosmetic boards were inspected losslessly at original crop size. Spectral's reference, a9 and final hero were inspected at the same 692 × 236 source-size crop; a high-quality JPEG was used only to transport the final hero view. Its untouched PNG comparison and source hashes are retained in the output directory.

## Spectral hero

Crop: `[455,146,692,236]` in x/y/width/height form.

The final native view visibly contains narrower irregular peaks, changes in frame height, richer violet-to-cyan color and a defined lower sample-range plane. Its actual-position highlight/marker remains visible. The previous broad three-wave sheet is gone.

The waveform is supplied by the exact audible float32 wavetable whose Git blob identity was independently verified before publication. The signed camera and material reveal that data; no image texture or invented plot points are involved. The native runtime/capture gate now closes the earlier source-only recommendation.

The reference still has denser craggy peaks, finer irregular surface texture and a different arrangement of ridges. The final native view has cleaner, more regular wire ribbons and a more explicit rectangular floor. Its marker and labels also differ in placement/content. Therefore this is a substantial, source-honest improvement, not a 1:1 match.

## Spectral fader caps

The previously hollow-looking dark interiors are now filled purple. Their actual handle positions and horizontal stripe treatment remain intact; no new collision or clipping is visible.

| Interior sample | Reference RGB | A9 RGB | Final RGB |
|---|---|---|---|
| 3 px above centre | (92, 83, 181) | (48, 34, 69) | (115, 88, 200) |
| 3 px below centre | (174, 100, 246) | (37, 22, 58) | (150, 94, 227) |

The new fill is substantially closer in brightness/color than the dark inset. The reference still has a stronger light-to-purple contrast and a brighter central core. The comparison is `spectral-fader-caps-comparison.png`.

## Bauhaus output heading

Crop bounds: `[1368,752,1570,782]` (x0/y0/x1/y1).

| Image | Dark-pixel bounds | Width × height | Dark pixels |
|---|---|---:|---:|
| Reference | [1374,759,1549,772] | 175 × 13 | 1,012 |
| A9 | [1370,757,1532,768] | 162 × 11 | 579 |
| Final | [1374,759,1551,771] | 177 × 12 | 682 |

The final heading matches the reference's left and top ink positions and is much closer in width/height. The output-only inset and vertical adjustment are visible. The heading remains lighter than the reference and is two pixels wider and one pixel shorter by this threshold. No title clipping is visible. The comparison is `bauhaus-output-heading-comparison.png`.

## Space macro captions

The first-caption measurements are:

| Image | Dark-pixel bounds | Width × height | Dark pixels |
|---|---|---:|---:|
| Reference | [1049,680,1092,689] | 43 × 9 | 160 |
| A9 | [1048,680,1096,688] | 48 × 8 | 117 |
| Final | [1051,680,1093,688] | 42 × 8 | 109 |

The final horizontal typography treatment brings the width close to the reference. The visible captions are still thinner and one raster pixel shorter, and the first caption's ink begins two pixels to the right of the reference. The new size/scale alone did not achieve the reference's heavier appearance. There is no new clipping or overlap. The comparison is `space-macro-captions-comparison.png`.

## All ten filter responses

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

Every final PNG plot count above was independently recomputed from the baseline/edited image pair and exactly equals the corresponding native capture-evidence field. The plot bounds match the expected actual manifest rectangles. Every actual native cutoff value changed, and every reported maximum response difference is finite and greater than 1 dB.

The dB column is the maximum absolute difference between actual native response samples before display clipping, as measured by the compiled capture test. It is not an output-level or perceived-loudness measurement and was not inferred from image pixels.

Aurora and Live are the decisive miniature checks: their plot regions contain no moving cutoff marker or numeric label. They now change **3,028** and **1,735** pixels respectively, after previously changing **zero**. The new dedicated regression also passed consecutive real slider/host edits after one refresh, independent filter-2 preservation and unchanged-refresh/sample/RNG invariants.

## Review scope and limits

Temporary source-size comparison boards and machine-readable measurements supported this review; they are not delivered native screenshots. Original references and native PNGs remained unchanged. No repository edits, commits or pushes were made during the review. The native gate passed for its named source, while the stated visual differences remained.

## Latest final-source follow-up — 453df569

Latest verified source: `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, [native run 43400507719](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719), job `131801876413`. Fresh captures: `native-artifact-453df569`. Root independently verified artifact `14024468912`, ZIP 11,019,495 bytes, SHA-256 `4054102ba04157671f0d3464616fdd0575514f657a8c22b089f4eeabd74b7246`.

The previous sections document the 4dbb414 checkpoint. This follow-up records the subsequently compiled glyph-outline and cap-gradient corrections.

### Native gates and provenance

The fresh verifier passed clean, complete, exact-SHA capture evidence, all twenty 1586 × 992 PNGs, and all ten independently recomputed filter-plot counts. This assignment independently read the new artifact logs: **42 cases / 10,316,058 assertions passed**, **24,100,990 capture assertions passed**, and both normal and ThreadSanitizer AudioTap logs report PASS.

All ten filter pixel counts and native dB deltas are identical to the 4dbb414 table above and exactly match their newly generated capture-evidence records. Aurora remains 3,028 changed curve-only pixels and Live 1,735; the regression remains fixed.

### Spectral cap gradient — final measured colors

The new gradient uses the actual inset coordinates. Original PNG median measurements at the existing handle centres are:

| Interior sample | Reference RGB | 4dbb414 RGB | 453df569 RGB |
|---|---|---|---|
| 3 px above centre | (92,83,181) | (115,88,200) | **(89,83,180)** |
| 3 px below centre | (174,100,246) | (150,94,227) | **(173,99,245)** |

The final colors are within three RGB levels per channel above the stripe and within one below. Source-size inspection confirms the filled cap treatment and clean caption/rail separation. The value-dependent handle position remains native y=633 versus the reference example y=639; no artificial paint-position shift was introduced.

### Bauhaus output heading — final measured ink

| Image | Dark-pixel bounds | Width × height | Dark pixels |
|---|---|---:|---:|
| Reference | [1374,759,1549,772] | 175 × 13 | 1,012 |
| 4dbb414 | [1374,759,1551,771] | 177 × 12 | 682 |
| 453df569 | **[1374,759,1549,772]** | **175 × 13** | **1,005** |

The final ink bounding box exactly matches the reference under the established threshold. Ink count differs by seven pixels. The actual glyph outlines now have the intended heavier appearance, and the heading remains clear of its panel edges. Matching bounds/counts is a measured local result, not proof that every individual glyph pixel is identical.

### Space macro captions — final measured ink

| Image | First-caption bounds | Width × height | Dark pixels |
|---|---|---:|---:|
| Reference | [1049,680,1092,689] | 43 × 9 | 160 |
| 4dbb414 | [1051,680,1093,688] | 42 × 8 | 109 |
| 453df569 | **[1049,680,1093,689]** | **44 × 9** | **182** |

The final first caption matches the reference's left, top and bottom ink positions. It is one pixel wider and somewhat heavier at the same threshold. The prior short/light appearance is corrected; the remaining one-pixel width and 22-pixel ink-count excess are retained as measured differences.

All four actual Space macro-caption controls were inspected losslessly at source size. Their text remains within its existing control region with clear separation between captions; no clipping, overlap or missing label was found.

### Spectral hero and remaining fidelity boundary

The entire 692 × 236 Spectral hero crop is **pixel-identical between 4dbb414 and 453df569**. Therefore the prior native confirmation of real source ridges, signed height, material and marker remains applicable to this final source. Its previously recorded difference from the reference's denser/craggier terrain remains; no 1:1 whole-skin identity claim is made.

### Final comparison evidence

The final source-size boards and machine-readable results retained exact crop bounds, source hashes, cosmetic measurements and all-ten filter deltas. They are inspection intermediates, not replacements for the unmodified gallery PNGs. The 4dbb414 and 453df569 measurements remain distinguished in the tables above. No new functional or spacing defect required another source edit.
