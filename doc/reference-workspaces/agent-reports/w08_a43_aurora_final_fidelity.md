# Assignment 43 — Liquid Aurora final native comparison

> Curated historical assignment report. The implementation was subsequently included in passing native source `eccf282`. Its new captures still require visual comparison; the earlier image measurements below describe the stated prior source. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and evidence

Reviewed Liquid Aurora using the retained reference image and the genuine native
22b9c299e6aa74093c9e96677cb32df5f2f74e74 baseline. Inspected the full composition,
filter/mixer controls at native resolution, panel colors in unoccupied areas,
and the updated hero from 3ec3998877eab793f8bf2aa4acb367c69de17979.

Owned and edited only
resources/data/skins/reference-07-aurora.surge-skin/workspace.json.
The coordinator owns shared C++ renderer changes and publication. Changes remain
unstaged; no commit or push was made.

## Implemented

Reduced faceDiameter from 42 to 35 for all four visible filter dials:
F1 Cutoff, F1 Resonance, F2 Cutoff and F2 Resonance. The native ring used an
approximately 19.3 px radius (0.46 × 42) and visibly exceeded the reference's
roughly 15–16 px ring. The revised radius is 16.1 px (0.46 × 35).

The exact controls are c.filter1-cutoff, c.filter1-resonance,
c.filter2-cutoff and c.filter2-resonance. Their component bounds and centres,
labels, values, native bindings and accents are unchanged. All 21 panels,
99 control IDs, selection groups, plot rectangles, palette and adaptation
notes remain unchanged. An explicit parsed-object comparison permits only
these four faceDiameter changes.

A thresholded bright-arc circle fit was used as a diagnostic only. Partial
arcs and value pointers bias some fits, so its centre/radius results are not
presented as exact reference dimensions or a fidelity score. The full-resolution
visual comparison and renderer radius relation justify the bounded diameter edit.

## Shared renderer findings passed to coordinator

1. The selected FX detail title remains “Selected effect” because p.fx-detail
   begins at y724, only 29 px below its containing effects panel y695, while the
   inherited-title lookup requires a 40 px offset. A contained genuine FX editor
   should derive its title from the actual selected algorithm. The coordinator
   has implemented the shared guard correction; the manifest does not hardcode “Chorus”.

2. Aurora's native surfaces are substantially lighter than comparable reference
   empty body regions. RGB medians (reference / native 22b9c29):
   - oscillator blank [425,135,470,141]: [37,27,61] / [63,45,86];
   - amplifier blank [279,579,310,601]: [43,30,66] / [77,56,101];
   - macro gap [400,918,413,964]: [27,24,49] / [68,51,92].
   The shared bezel's fixed Aurora top/bottom gradient, accent radial overlay
   and reflection sheet account for these colors. Palette edits cannot
   independently correct that hardcoded material. The macro panel occupies
   [253,897,996,85] and needs the lower, dark blue-violet treatment.
   The coordinator implemented Aurora top #2b1e43 / bottom #17142b,
   radial accent alpha .04 and white gloss .02, plus a macro coating
   #1d1a35 to #19172e. Those new source changes require a fresh capture.

3. Mixer centres are close, but the native fader rail and rectangular caps are
   visibly wider than the reference's thin purple rails and small rounded caps.
   Value-dependent positions are genuine native levels and must not be moved
   to mimic the pictured levels. Filter algorithms, subtype controls and
   actual level/voice descriptions remain native adaptations.

## Updated hero inspection

The 3ec3998 native hero displays the new thin saturated source ribbons within
the retained viewport. The preview remains visibly simpler than the concept's
many crossing traces and nebular decorative background. It uses real source
frames, so no fabricated curves or frozen image were introduced. No hero
geometry or fixture changes were made by this assignment.

## Validation

- Strict validator passed all ten manifests. Existing ClearSignal corner
  overlay warnings remain unchanged.
- git diff --check passed.
- Exact allowed-field parsed comparison passed.
- All declared panel/control/plot/wavetable/tab rectangles remain in 1586×992.
- A fresh compiled native capture after the four diameter edits remains pending.
  The inspected 3ec3998 hero is not evidence of this later manifest edit.
- Pixel identity is not claimed.

Evidence: w08_a43_overview.jpg, w08_a43_filter_mixer.jpg,
w08_a43_hero_after.jpg, w08_a43_filter_fit.json and w08_a43_validation.json
under /vercel/reports.
