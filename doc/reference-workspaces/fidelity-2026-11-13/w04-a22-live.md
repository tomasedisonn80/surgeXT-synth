# Wave 4 / assignment 22 — Live Performance Deck

## Compared evidence

Compared `references/10_Live_Performance_Deck.png` with the genuine
`native-gallery/10-live-performance-deck.png` from source
`a362667839b8e1d742cf3af94bb8710fa3c6d1fd`, at 1586 × 992. This assignment's changes
are uncommitted and require the coordinator's new native build/capture.

## Measurements and changes

The main panel boundaries already agree closely. A clear hero-background patch
at (670,154), 30 × 10 pixels, has median RGB 31/33/32 in the native image versus
7/9/10 in the concept. Added one dark, paint-only `p.wave-glass` panel behind the
existing waveform and widened its plot. This surface ends above MacroXY.

The reference Oscillator 1 heading's thresholded ink spans x54–121, y212–222;
the native heading spans x54–129, y211–221. Reduced the three oscillator heading
sizes from 15.5 to 14.5. Aligned mixer, filter-chain, matrix, effects, LFO,
performance and master heading insets. Expanded miniature plot/selector wells
slightly, moved the oscillator control/readout columns left by 1–3 pixels, and
moved Amp Gain down 3 pixels to approach its measured reference centre.

In the Live-only luminous rendering branches, removed filled sheets and repeated
broad row halos. Ordinary source wires are 0.52 pixels, phase seams 0.42 pixels,
and the selected real row 0.9 pixels, with smaller selection halos. Mint and
amber remain distinct along the real rows. Opposite frame shear separates the
mesh; the signed sample-height factor stays 0.40. No source generator, fixture,
sample acquisition, oscillator state or audio processing changed.

## Verification and remaining work

- Strict manifest validation passes all ten skins; Live has 23 panels and the
  original 64 controls. The 93 declared/generated interactive rectangles retain
  their IDs, stay on canvas and do not overlap.
- Existing bindings, navigation selections, palette and adaptations are retained.
  The XY panel and all eight macro-control definitions are unchanged.
- Aurora projection, endpoint gradient, compound haze and intermediate-filament
  blocks remain byte-identical to the fresh pre-edit file.
- Projection analysis of the actual copied GlassWavetable samples gives vertex
  bounds (466,161.834)–(1117,277.158), within the plot. This is source-coordinate
  analysis, not a native screenshot. Zero samples remain flat frame baselines.
- `git diff --check` passes. No build, commit or push was performed.

The audible source family still determines the regular waveform topology and
is reserved for the next wave. Macro-caption and scene-button font differences
require their shared painter rather than distorting control bounds. Fresh native
captures must confirm the dark surface, finer wires and small geometry changes;
no pixel-identity claim is made.
