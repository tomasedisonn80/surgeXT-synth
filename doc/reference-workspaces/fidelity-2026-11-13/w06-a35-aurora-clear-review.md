# Wave 6 / Assignment 35 — Aurora and ClearSignal review

## Evidence and pending boundary

Read-only comparison of the original `references/07_Liquid_Aurora.png` and
`references/08_ClearSignal_Accessible.png` with the corresponding unedited
`native-gallery/07-liquid-aurora.png` and `08-clearsignal-accessible.png`.
All four are 1586 × 992. Verified native provenance: source
`2a5a621241e28e15eda57ff5be3bad6dc3f36ff3`, gallery commit
`48725235d6576e19d05ff24b524bb90b5d91f2ae`, workflow `43491379963`, capture
`2026-11-13T19:25:20.801Z`.

Source inspected at `0708bc76bb194a127064cf18b8849cf77373a7e7`; its workflow
`43493343847` has no reviewed capture in this report. Aurora’s new audible
three-crest wavetable, supported fixture values, MIDI phrase, ClearSignal’s
real +0.64 route, matrix selection/depth finishing and paper-white material
must be assessed on that fresh evidence. No stale waveform, value, selection,
inspector-depth or spectrogram finding below is presented as current failure.

## Three actionable gaps

1. **Aurora miniature dials remain too large and heavy.** OSC 2 Width is centred
   near (629,205) in both images, but the reference rim is approximately 28 px
   across versus the native 38 px face allocation. The three small OSC 1
   controls show the same discrepancy. Envelope dials are approximately 25–27 px
   in the reference versus native `faceDiameter:34`; the large OSC 1 dial is
   roughly 42 px versus 49. In `reference-07-aurora.surge-skin/workspace.json`,
   test face diameters 30, 27 and 43 respectively, keeping existing component
   rectangles and centres. `BoundSlider::paint` already supports this paint-only
   sizing. In `Workspace.cpp::knob`, scale Aurora’s fixed 8.5/4.7 px glow widths
   down with the miniature radius so the reduced dials do not retain oversized
   bloom. Preserve real value arcs, labels, focus and hit targets.

2. **Aurora’s fixed title lip over-brightens compact panels and browser.**
   RGB medians over unchanged, text-free patches are reference/native:
   browser `[175,80,201,102)` = (59,38,81)/(71,50.5,101);
   envelope `[270,582,290,592)` = (48,32,72)/(72,52,100).
   Conversely, the oscillator title patch `[500,80,535,100)` is already close:
   (67,52,96)/(69,49,97). Therefore a global darkening would damage the
   oscillator match. `Workspace.cpp::bezel` applies the same 29 px lip and
   hard-coded violet stops to every Aurora panel. Pass a per-panel lip treatment
   from `drawPanel`: reduce the browser’s saturation/brightness and shorten the
   bright band on 127 px envelope/LFO cards, preserving the oscillator treatment.
   Keep outer rectangles and live plots untouched.

3. **ClearSignal section headings lack their compact identity icons and inset.**
   Dark-pixel measurements place the reference oscillator icon at
   (13,84), 15 × 14 px, with title ink beginning x38; native title ink begins
   x14. Reference/native title starts are x1229/x1206 for MACROS and x464/x441
   for ENVELOPES. Add bounded, noninteractive 14–16 px section pictograms in
   `ReferenceOrnaments.h`, called by the Clear branch of `drawPanel`, and set
   the affected manifest `titleInset` values around 36 instead of 12. Treat
   these as section identity artwork only; do not draw fake filter power,
   routing or state indicators. Recheck title/tab clearance and unchanged
   accessible labels after placement.

No source edits, build, native test or git mutation was performed. These are
provisional measured paint/layout findings, not a fidelity acceptance.
