# Wave 6 / assignment 33 — Spectral and Bauhaus provisional review

## Evidence and status

Compared both original references with genuine 1586 × 992 native baselines from
`2a5a621241e28e15eda57ff5be3bad6dc3f36ff3`, retained by gallery commit
`48725235d6576e19d05ff24b524bb90b5d91f2ae` (run `43491379963`). Read AGENTS,
continuation, engineering log, user guide, current pass record and intervening
changes. Wave 5 source is now `0708bc76bb194a127064cf18b8849cf77373a7e7`;
its native captures were unavailable when the coordinator requested this
provisional handoff. These are remaining source-supported priorities, not
acceptance of that uncaptured revision.

## Three priorities for Wave 7

1. **Spectral source topology.** The reference's central mountain field
   `(455–1147,155–380)` contains sharp subsidiary ridges; the inspected native
   field repeats six smooth dune groups. Wave 5 already expands the caller to
   the declared `(455,146,692,236)` plot and changes depth, height and wire
   density; do not repeat the old framing fix. Its 142 rows interpolate the
   same 48 × 256 snapshot and cannot create missing detail. In
   `scripts/reference-workspaces/generate_showcase_wavetables.py`, refine
   `DUNES_PEAKS`/`signal("dunes")` with bounded periodic subsidiary harmonics and
   independently varying ridge amplitudes; regenerate the actual audible
   `Spectral Bloom.wt`. Preserve global scaling and signed amplitudes.
   `Visuals.cpp::sampleWavetable` and `Visuals::wavetable` must continue displaying
   that real source without invented roughness.

2. **Bauhaus open hero and framing.** Native strands around
   `(275–964,157–295)` merge into regular colored ribbons; reference strands
   remain open across approximately `(270–969,144–324)`. At x730, a fixed bright
   color threshold finds one connected native band versus 28 reference runs;
   this is local evidence, not a similarity score. `Visuals::wavetable` still
   fills harmonic strips at .010 alpha and draws 48 rows at .8 px/.84 alpha.
   Remove or attenuate those fills, refine thin strokes, and evolve the audible
   `signal("bauhaus")` 5/4/1-harmonic recipe and per-row normalization toward
   varied amplitudes. In `Workspace.cpp::drawPanel`, the waveform branch's
   `plotBase` adds the extra `(268–973,135–316)` inner outline and 8 px inset;
   make the harmonic hero's surround continuous and tune its fixed camera,
   retaining real source geometry and selection feedback.

3. **Bauhaus matte material.** The blue blank patch
   `(1010–1204,579–583)` is uniformly RGB `(37,79,110)` natively; reference median
   is `(40,81,111)` with channel ranges `(6,9,13)`. Red lower-panel blanks are
   likewise constant natively. `Workspace.cpp::bezel` explicitly excludes
   Bauhaus from `materialGrain`. Add restrained deterministic grain and local
   color variation to its panel branch, preserving the flat graphic hierarchy.

## Pending and limits

Fresh captures must verify Spectral camera/density, Bauhaus fader caps, title
weight and updated real profile values before those changes are accepted.
Native LP24 versus conceptual Peak24, FM2/Classic versus illustrated source
families, and actual modulation/output traces are engine-state differences;
do not repaint them into reference state. No source edits, builds, commits or
broad validation were performed.
