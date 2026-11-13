# Wave 2 / Assignment 11 — waveform material finish

Reviewed `AGENTS.md`, continuation/user/engineering records, the wave-1 rendering
audit and gap audit, and original/native pairs 03, 04, 07 and 10. Implementation
is confined to `Visuals.cpp::wavetable`; this report accompanies that change.

## Changes

- Aurora's shared material gradient now reaches zero alpha at both ends, with
  intermediate stops at 0.035/0.085 and 0.935/0.98. The existing central hue
  stops remain intact. Strips, transverse seams, rows and selected-row glow all
  inherit the fade. Its four source-following haze strokes now use curved joins
  and rounded caps, with lower opacity to reduce accumulation.
- Full-size Bauhaus, Aurora and Live filled strips have lower opacity. Their
  main row cores are thinner (0.8/0.72/0.72 logical pixels), retaining the same
  real samples. Bauhaus retains its unlit graphic material.
- Live has a dedicated mint/amber gradient with short transitions and bounded
  frame-dependent color offsets, separate from Spectral's violet/cyan material.
  Its narrower, fainter bloom surrounds slightly brighter wire cores. Transverse
  connections use every eighth existing phase sample instead of every twelfth,
  with lower opacity; the source snapshot and frame count are unchanged.
- Spectral receives lighter surfaces, bloom and floor material, plus a thinner
  row core, so its existing sample mesh has less opaque band accumulation.

## Scope and verification

No sample generation, source asset, signed amplitude, camera coordinate,
snapshot bound, selection position, audio path or public API changed. No image
traces or fabricated rows were introduced. Miniature paint branches retain
their previous weights. All work remains bounded on the UI thread.

`git diff --check -- src/surge-xt/gui/reference/Visuals.cpp` passed. The diff was
reviewed to confirm that sampling and projection code are untouched. No local
native build or new capture was run; the coordinator owns native CI and the
all-skin capture gate. The existing smooth source topology still differs from
the original artworks, and this paint pass does not claim to correct it.

Next: compare current native captures against both prior captures and originals,
especially Aurora's edge dissolution and Live's color separation.
