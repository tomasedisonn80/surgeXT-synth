# Assignment 59 — isolated native glyph-weight follow-up

> Curated historical assignment report. The real-glyph proposal and API review were subsequently compiled and measured on native source 453df569. The final addendum records exact Bauhaus ink bounds, the remaining one-pixel Space width difference and lossless caption-spacing inspection. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Deliverable and ownership

Patch file: `analysis-only/w10_a59_optical_text_weight.patch` (3,810 bytes).

This assignment created the patch outside the repository. It did not modify `Workspace.cpp` or another repository file. `git apply --check --whitespace=error` passed against the current workspace source.

The patch changes only two painted text cases:

- Bauhaus `output-panel` heading.
- Space controls whose native binding starts with `macro`.

No manifest rectangle, component bounds, native binding, text string, focus/gesture path or accessibility metadata changes.

## Evidence and targets

The final 4dbb414 native images had the following dark-ink measurements, using all RGB channels below 85:

| Case | Reference | Current native |
|---|---|---|
| Bauhaus heading bounds | [1374,759,1549,772] | [1374,759,1551,771] |
| Bauhaus dark pixels | 1,012 | 682 |
| Space first caption bounds | [1049,680,1092,689] | [1051,680,1093,688] |
| Space first caption dark pixels | 160 | 109 |

The current embedded fonts are selected by `Theme::type()` from the existing font manager. This was inspected directly; the patch makes no assumption about a substitute system font or family. Space retains its existing bold font at `fs+1` and horizontal scale .83. Bauhaus retains its current heading font and size.

## Method

A small `weightedGlyphText()` helper asks JUCE to lay out the actual native text with the existing font, then obtains those glyph outlines. It applies a slight affine transform around the actual ink centre, fills the outlines, and adds a bounded same-ink outline stroke. Painting is clipped to the original label rectangle and preserves the caller's graphics state.

Only the two callers above invoke the helper; all other text retains the existing painter.

| Case | Outline x scale | Outline y scale | Stroke width | Ink-centre offset |
|---|---:|---:|---:|---|
| Bauhaus output | .9856 | 1.0375 | .55 px | (−1, +.5) px |
| Space macro caption | 1.015 | 1.0775 | .38 px | (−1.5, +.5) px |

These small geometry changes target the measured 175×13 and 43×9 ink extents, while the outline stroke increases the visibly deficient weight. The offsets compensate the measured centre differences; they move only painted ink inside the existing bounds.

The labels remain real glyphs from the current font. No bitmap label, reference-image pixels, alternate fixed string or synthetic control is introduced.

## API verification

Verified against the official [JUCE 7.0.12 GlyphArrangement header](https://github.com/juce-framework/JUCE/blob/7.0.12/modules/juce_graphics/fonts/juce_GlyphArrangement.h):

- `addFittedText(const Font&, const String&, float x, float y, float width, float height, Justification, int maximumLinesToUse, float minimumHorizontalScale)`.
- `createPath(Path&) const`, which appends the actual glyph outlines to the path.

The helper uses ordinary JUCE path fill/stroke and a six-value affine matrix. Work is limited to one heading or a short macro caption in these specific paint paths; no audio or engine state is involved.

## Verification boundary

Patch application and API/source review passed. Native compilation and fresh PNG measurements are the next gate. Outline rasterization can differ from hinted glyph rendering, so the target geometry and dark-pixel density are not claimed as achieved before that capture. The earlier 4dbb414 measurements remain valid historical evidence.

## Final native raster verification — 453df569

Latest verified source: `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, [native run 43400507719](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719), job `131801876413`. Fresh captures: `native-artifact-453df569`. Root independently verified artifact `14024468912`, ZIP 11,019,495 bytes, SHA-256 `4054102ba04157671f0d3464616fdd0575514f657a8c22b089f4eeabd74b7246`.

The isolated patch was compiled and captured successfully with the real embedded glyph outlines. Native logs passed 42 cases / 10,316,058 assertions and 24,100,990 capture assertions; normal and ThreadSanitizer AudioTap checks passed.

At the established all-channels-below-85 ink threshold:

- **Bauhaus:** bounds exactly match the reference, [1374,759,1549,772] (175×13); 1,005 dark pixels versus reference 1,012 and previous 682. This is a successful measured geometry/weight correction.
- **Space:** bounds are [1049,680,1093,689] (44×9), versus reference [1049,680,1092,689] (43×9); 182 dark pixels versus reference 160 and previous 109. Left/top/bottom alignment and height now match, while one extra width pixel and somewhat greater weight remain.

All four actual Space macro captions and the Bauhaus heading were inspected losslessly at source size. No label clipping, overlap or missing content was found. Existing bounds, real strings and controls remain intact.

Exact reference/4dbb414/453df569 tables and the final cap-color correction are appended to [w10 a59 final native visual review](w10_a59_final_native_visual_review.md). Machine-readable evidence and unscaled images are in `analysis-only/a59_final_visual_453df569/`. No further source edit is proposed from this review.
