# Assignment 44 — ClearSignal final fidelity review

> Curated historical assignment report. At this archive snapshot, these completed manifest edits follow the latest verified native checkpoint and require a fresh exact-source build and capture. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and evidence

Reviewed only the ClearSignal manifest, with actual native screenshots from source
`22b9c299e6aa74093c9e96677cb32df5f2f74e74`, successful native run
`43382049170`. The comparison reference is
`doc/reference-workspaces/references/08_ClearSignal_Accessible.png`; the native
image is `native-artifact-22b9c29/08-clearsignal-accessible.png`.
Both are 1586 × 992. The source review used the public repository at
`3ec3998877eab793f8bf2aa4acb367c69de17979`.

The complete composition, filter plot, and filter control strip were visually
inspected in separately labelled reference/native composites. The existing
envelope-model and LFO-shape menus remain intentional, usable plot-corner
overlays; they were not removed or moved merely to silence overlap warnings.

## Manifest change

Changed only `filter2.cutoff`:

| Field | Before | After |
| --- | ---: | ---: |
| x | 822 | 824 |
| y | 460 | 459 |

A least-squares circle fit to saturated large cutoff-ring pixels measured:

| Dial | Reference centre | Native centre | Reference radius | Native radius |
| --- | --- | --- | ---: | ---: |
| Filter 1 cutoff | (491.14, 514.26) | (491.19, 514.22) | 33.24 | 32.88 |
| Filter 2 cutoff | (876.32, 513.48) | (873.92, 514.40) | 33.25 | 33.13 |

The unchanged 72 px face diameter already matches the reference radius. Moving
only Filter 2 by (+2, −1) predicts a centre near (875.92, 513.40), reducing the
measured alignment error without changing its control size or neighboring
layout. Filter 1 was retained. These are pixel-fit measurements of the
**pre-edit** native capture, not a claim that a newly compiled image has passed.

The fit's mean radial residual is approximately 1.1–1.6 px, reflecting stroke
thickness and antialiasing. Small partial-arc fits in the raw measurement file
were not sufficiently constrained and were not used to change small dials.

All 77 control IDs, labels, bindings, selections and component sizes are unchanged.
All 27 panels, plot bounds, palette, adaptation notes and native actions are
unchanged. No state or audio fixture was altered.

## Shared renderer findings sent to the coordinator

### Combined filter framing

The manifest plot `[480,166,696,212]` already matches the reference's outer dark
frame. Moving it would misalign that frame.

The reference has a plot-title/legend band around y171–193, then a grid starting
near `[480,200]` and extending to y378. Its dB labels occupy the existing white
gutter to the left of x480; frequency captions are below the dark frame around
y383–402.

Current `drawFilter` adds an internal 30 px Clear gutter plus the generic 7 px
inset, giving an inner graph `[517,181,652,182]`; all captions are placed inside
the dark frame. A Clear-specific external-axis treatment can use the already
reserved white gutter without moving any manifest rectangles. The reference
also uses +18 to −24 dB in 6 dB steps. A corresponding displayed range can map
the actual native response and clip values outside that display range; it must
not replace the native response with the illustrated signal. Any new legend
must display the real current filter algorithms.

The parent filter panel is [427,124,753,286], ending at x1180/y410.
Neither proposed external-caption band intersects a manifest control. Clamp the
rightmost frequency label to the parent/frame right edge: the unconstrained
[466..1190,383..401] band otherwise extends 10 px past the parent.

This work is renderer-owned and was not implemented in the manifest.

### Caption weight and control contrast

Reference filter headings, small knob captions and value fields appear bolder
than the native counterparts. Native Clear knob caps and inactive arcs have
less gray contrast than the reference. These are shared `BoundSlider`/`knob`
paint issues, not a reason to alter already aligned control rectangles.

Real filter power icons remain absent because the renderer does not expose a
native filter bypass operation. No decorative switch was inserted in their place.

### Native semantic differences retained

Actual filter algorithm names, parameter units, oscillator content, live route
identity/depth, output spectrogram and meters differ from the reference's
illustrated state. The manifest's adaptation list describes these native
constraints. The review does not equate matching composition or passing
validation with pixel identity.

## Validation

- Strict validator: all ten workspaces pass. The two existing Clear plot-corner
  warnings remain intentional and were visually inspected.
- `git diff --check`: pass.
- Explicit bounds check: 128 panel/control/plot/navigation rectangles remain
  within 1586 × 992.
- Parsed before/after invariants: every field except the two specified cutoff
  coordinates is identical.
- Changes remain unstaged; no commit or push performed.
- Compiled native capture after this manifest change: pending integration.

Final manifest SHA-256:
`b48e1fa668c08eda3bf733567cd10ead99e758928a91ebbf4d0762ec7289631c`.
