# Wave 7 / Assignment 41 — Aurora and ClearSignal details

## Evidence and implementation

Compared the 1586 × 992 references with the fresh native gallery from source
`0708bc76bb194a127064cf18b8849cf77373a7e7`, passing workflow
`43493343847`, generated `2026-11-13T19:37:53.210Z`. These are the inputs to
this correction, not captures of the changed source.

- Aurora's 24 affected paint allocations now use 30 px for ten miniature
  oscillator dials, 27 px for twelve envelope dials, and 43 px for the two large
  OSC 1/3 dials. `BoundSlider` still centres the painted face inside the same
  component. Aurora halo widths scale with `min(1, radius / 22.5)`; value arcs,
  labels, focus, bindings and event paths remain native.
- A bounded `drawPanel` glaze darkens only the Aurora browser title and the
  six 127 px envelope/LFO cards. It fades out by 36/40 px respectively, leaving
  oscillator title material intact. Aurora's browser child starts at y116;
  the y75–111 parent treatment therefore reaches the measured y80–102 title
  patch. `bezel()` and `Visuals.cpp` were not edited by this assignment.
- ClearSignal has eleven 16 px section identity pictograms implemented as
  clipped, fixed vectors in `ReferenceOrnaments.h`. They receive no signal,
  selection, routing or power state. Existing native power controls are not
  replaced. Main heading insets increase from 12 to 36 px; the filter-chain
  heading uses 26 px and the analyzer 30 px to fit their measured layouts.
  Existing FX inset 37 px is retained. ClearSignal's title/tab threshold now
  accepts its 60 px LFO heading space without suppressing the heading.

## Validation and remaining gate

`validate.py --strict-overlap` passes all ten manifests. Its existing ClearSignal
envelope/LFO plot-menu overlap notices remain unchanged. `git diff --check`
passes. A comparison against the original manifests confirms that only
`faceDiameter` or `titleInset` fields changed: all component/panel bounds,
bindings, IDs, labels and navigation data are identical. Every new icon fits
before its heading; fixed source/analyzer tabs retain an 8 px gap, with heading
widths 137 px (oscillators), 97 px (envelopes), 60 px (LFOs) and 90 px (analyzer).

New-source native build, actual placed-child verification, screenshots and
optical acceptance remain the coordinated Wave 7 gate. No commit or push was
performed by this assignment.
