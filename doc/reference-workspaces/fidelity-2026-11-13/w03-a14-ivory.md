# Wave 3 / assignment 14 — Ivory typography and geometry

## Evidence and scope

Inspected the original `references/02_Ivory_Atelier.png` and actual baseline
`native-gallery/02-ivory-atelier.png` at 1586 × 992, and read the required
repository guidance and wave-1 gap audit. The gallery represents source
`453df569c977350c3a4da2caf2f0ab7cbd1a0e71`; it does not show this pass or the
recent shared materials work. Owned edits are this report and Ivory's
`workspace.json` only.

## Implemented refinements

- Increased actual oscillator parameter captions/readouts from 11.5 to 12.5,
  FX parameters from 11.5 to 13, ADSR stages from 12 to 13, and mixer controls
  from 13 to 14. Baseline oscillator/FX readout ink was approximately eight
  pixels tall versus ten in the reference. Scoped dark ink strengthens those
  live values without changing the cream palette or global serif sizing.
- Widened oscillator caption cells by one pixel while retaining their declared
  centers. Widened FX cells from 66 to 72 pixels and aligned their centers to
  756, 834, 913 and 993. Captions remain actual algorithm-dependent names;
  no reference-only parameter labels were introduced.
- Moved mixer centers horizontally by 1–2 pixels. Their cells now begin two
  pixels higher and are two pixels taller, aligning the measured reference
  label row while retaining the lower value band. Aligned ADSR centers to
  1177, 1260, 1342 and 1424; retained face diameters and the value-band bottom.
- Corrected green plot extents: OSC1 starts at y=188 instead of 190 and ends
  at 305 instead of 309; OSC2 moves up one pixel; OSC2/3 gain one pixel of
  width. Filter 1 expands two pixels left; Filter 2 moves two pixels right.
  The envelope becomes `[1139,357,321,137]`, closer to the measured reference
  green area `[1139,357,1460,493]` in edge coordinates. Wavetable selectors
  move up one pixel where appropriate. Analyzer geometry was already close.
- Increased FX card text from 11.5 to 12.5 while preserving its live names.

## Renderer handoff and validation

Reported section-title ink six pixels too high: original OSCILLATORS y=107–118,
baseline native y=101–112. The coordinator integrated the targeted `titleY=9`
correction for large Ivory headings; smaller ring/effect subgroup headings and
oscillator/filter headings remain on their existing path. Major manifest
heading sizes already matched the reference's 12–13-pixel visible height.

Strict validation passed all ten skins. Ivory has no interactive or plot/control
overlap warnings. Explicit invariants preserved all 17 panel IDs, 75 control
IDs, bindings, selection choices, face diameters and dynamic-label behavior.
`git diff --check` passed. No native build, fresh capture or pixel-identity result
is claimed; the coordinator's combined capture must verify actual typography
and child placement. No commits, remotes or toolchain changes were performed.
