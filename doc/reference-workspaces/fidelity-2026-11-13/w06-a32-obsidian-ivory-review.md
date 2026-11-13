# Wave 6 / assignment 32 — Obsidian and Ivory native review

## Evidence and status

Read `AGENTS.md`, continuation, engineering log, user guide and current-pass
reports. Directly inspected original references 01/02 and their unmodified
baseline/edited native PNGs at 1586 × 992. Gallery `4872523` records source
`2a5a621241e28e15eda57ff5be3bad6dc3f36ff3`, successful native workflow
`43491379963`, and capture time `2026-11-13T19:25:20.801Z`. These are JUCE
component snapshots attached to the processor, not desktop photographs.

**Provisional:** Wave 5 source `0708bc76bb194a127064cf18b8849cf77373a7e7` is
published; its new native captures are pending. Current source was reviewed to
avoid reporting corrections already made. The coordinator accepted these three
remaining issues for Wave 7; fresh-capture verification remains required.

## Three material findings

1. **Ivory green surfaces are washed into sage.** The preset well occupies
   `[548,14,434,46]`. Median RGB over x600–909/y20–27 is native `89/111/93`
   versus reference `26/57/35`; its lower strip x600–909/y45–52 is approximately
   `59/84/63` versus `24/52/32`. Selected Scene A has the same defect:
   x415–450/y31–35 gives `108/131/110` versus `41/69/50`. Selected FX1 and
   Spectrum tabs are also affected. In `src/surge-xt/gui/reference/Workspace.cpp`,
   `bezel()` applies cream interpolation whenever Ivory fill differs from
   `theme.display`, including green fills. Restrict cream treatment to cream
   surfaces and give green wells/selected controls a dark green material path.
   Retain their actual selection state.

2. **Ivory header typography and master placement remain displaced.** With
   maximum RGB below 95, reference brand ink spans `[104,22,168,24]`; native
   spans `[103,14,180,25]`: eight pixels high and twelve pixels wider. Adjust
   `paintHeader()`'s Ivory `brand({101,13,200,39},36)` geometry/font treatment;
   the tagline is also two pixels high. The master value appears beneath its
   left caption at x1350–1384/y47–55, while the reference places it right of the
   dial at x1472–1506/y45–54. Refine `ivory.master.volume` in
   `resources/data/skins/reference-02-ivory.surge-skin/workspace.json` and its
   `BoundSlider::paint` side-label path, preserving the live value. The dial
   centre also needs approximately seven pixels rightward.

3. **Obsidian lacks the reference's brighter panel lips.** Blank OSC1 title
   pixels x315–467/y125–148 have median native `18/22/26` versus `24/29/33`;
   the FX lip x600–999/y676–682 is `18/23/27` versus `26/31/36`. OSC1 lower
   body x250–494/y286–290 is `12/15/18` versus `17/21/24`. Refine the
   precision-only panel material in `Workspace.cpp::bezel()` with a bounded
   code-space lift and local title lip; preserve dark display wells and control
   geometry.

## Limits

Obsidian's cyan-left/amber-right waveform direction and Ivory's major title
baselines are improved. Both edited captures visibly update the cutoff readout,
dial and response. Wave 5 changes real fixture values and audio history; their
earlier differences are not layout defects. Native source shapes, capability
adaptations and actual values must remain truthful. No code edits, builds,
repeated validators or pixel-identity acceptance were performed.
