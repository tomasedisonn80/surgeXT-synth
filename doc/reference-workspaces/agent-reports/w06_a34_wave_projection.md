# Assignment 34 — Actual-source wavetable projection

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and status

Implemented a bounded drawing refinement in `Visuals.cpp` and updated the
projection description in `Visuals.h`. No Workspace call-site, manifest, source
sampling, oscillator, patch fixture, generator, DSP or RNG code was changed.
The changes remain unstaged; the coordinator owns publication.

The original workspace became unavailable before this assignment edited files.
Work resumed in the recovered Vercel Sandbox. Only the checks explicitly listed
below ran there; no C++ syntax, linked build or post-edit native render pass is
claimed.

## Evidence inspected

Compared the real native `22b9c299e6aa74093c9e96677cb32df5f2f74e74` captures
with the retained Spectral, Aurora and Live reference images at the manifest
plot rectangles. The labelled crop contact is
`analysis-only/w06_a34_hero_before.jpg`; it is pre-edit evidence.

The native luminous stacks had aligned phase coordinates and broad pale
surfaces. Spectral's near and far frames had similar amplitude scale, making the
surface read as a shallow folded sheet. RGB interpolation also desaturated the
cyan/violet and mint/amber transitions.

For a limited colour diagnostic, pixels with max(R,G,B) > 120 and
max(R,G,B) - min(R,G,B) > 25 were measured inside each plot; Spectral excluded
the top 35 pixels of captions. Median relative chroma is (max-min)/max:

| Plot | Reference | Native before edit |
| --- | ---: | ---: |
| Spectral | 0.925 | 0.605 |
| Aurora | 0.691 | 0.589 |
| Live | 0.500 | 0.342 |

These are thresholded RGB appearance diagnostics, not fidelity scores, physical
colour measurements or evidence of a post-edit improvement.

## Changes

- Large luminous plots use a fixed slanted phase/frame plane. With phase u,
  frame f and signed sample a, x/W = 0.05 + 0.90u + 0.08(f-0.5), and
  y/H = 0.5 + 0.22(f-0.5) - 0.40a. Every source frame stays within 1–99% of
  viewport width; frame separation rises from 13% to 22% of viewport height.
- Large perspective plots retain their horizontal camera and use
  y/H = 0.17 + 0.43f - a(0.13 + 0.27f), increasing near/far height perspective.
  Unit-range signed samples remain within y/H = 0.04…1.00. The existing
  viewport clip still bounds larger sample excursions.
- Reduced adjacent-strip opacity, removed large ribbon fills to the zero plane,
  and added thin seams connecting actual adjacent frames at fixed sample phases.
  Perspective seams are finer and denser. Glow remains bounded to the viewport.
- Preserved hue while strengthening large-view saturation and keeping a
  saturated gradient midpoint. Large ribbon gradients no longer add a pale
  white stop; actual selected-frame highlights remain intact.
- Miniature branches and Bauhaus harmonic ribbons preserve their prior
  projection, palette, strokes and fills. Current-frame interpolation, source
  arrays, sample-mode behavior and native unavailable-state logic are untouched.
  Zero-valued samples remain a flat plane; no normalization invents amplitude.

## Validation and next gate

- Reviewed the exact two-file diff; changes are confined to drawing and its
  documentation.
- `git diff --check`: passed.
- `python3 scripts/reference-workspaces/validate.py --strict`: all ten passed.
  Existing ClearSignal corner-overlay warnings remained unchanged.
- The coordinator must compile and capture all ten native workspaces after this
  shared renderer change, then inspect the new large plots and the unchanged
  thumbnail/Bauhaus branches. Pixel identity is not asserted.
