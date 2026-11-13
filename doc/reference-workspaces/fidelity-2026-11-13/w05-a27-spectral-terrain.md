# Wave 5 / assignment 27 — Spectral terrain

## Evidence and scope

Read AGENTS, continuation, engineering log and user guide. Compared the original
`03_Spectral_Observatory.png` with the genuine `a362667` native gallery at
1586 × 992. Source changes start from `411578a` and are confined to
`Visuals::wavetable` terrain paint/projection plus this report.

The current native landscape is shallower and more regular than the reference;
its lower range plane ends around y361 instead of y380. The source asset cannot
provide all of the concept's craggy detail, and interpolation does not create it.

## Changes

- The fixed Spectral camera widens the rear mesh and increases rear sample
  height and frame depth. Its signed height is
  `.30 + .345*frame - amplitude*(.29 + .06*frame)`. The same camera still projects
  the lower plane at sample −1; its near edge reaches `.995` of the supplied
  height. Renderer bounds and clipping are unchanged.
- Full-size terrain adds two linearly interpolated wires between each copied
  row: at most 142 rows from the existing 48 × 256 snapshot. Transverse seams
  use every second existing phase point and the same interpolated frame
  coordinates. No source detail, amplitude rectification, normalization, phase
  warp, randomness or new signal is introduced.
- Thinner row/seam strokes, narrower/fainter row halos and lighter filled strips
  retain separation at the increased wire density. The lower range plane has
  finer, thinner grid lines. Existing violet/cyan material is retained.

The coordinator separately owns removing the Spectral hero's caller-side
8-pixel inset in `Workspace.cpp`; Aurora's 1-pixel and other styles' 8-pixel
insets remain their existing values. This camera targets the resulting
`(455,146,692,236)` rectangle.

## Verification and limits

Float32 numeric reproduction checked 878,592 projected vertices across preview
frame counts 1–48: finite coordinates, bounded source indices and convex
interpolation weights passed. Checked actual-source vertices remain inside the
full hero rectangle. Predicted source y extent changes from 185.29–308.08 to
162.12–322.60; the plane's near edge is y380.82. These are geometry checks,
not screenshots.

`git diff --check` passes. Sampling APIs, renderer clipping, Aurora/Live camera
and gradient blocks, Aurora haze/intermediate wires, and selected-row/marker
code compare byte-identical with HEAD. Real morph position and source metadata
remain unchanged. Spectral Bloom is byte-identical to HEAD, still 256 × 2048;
SHA-256: `ce5ce7edcdf8a7a240731c5218ea559fd6cebf35c70205a2192354f127f6260b`.

No native build, capture, test-file changes, commit or push was performed.
Coordinator-owned exact-source native capture must assess wire separation,
full-plot framing and remaining source-topology differences.
