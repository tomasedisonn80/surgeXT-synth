# Wave 5 / assignment 29 — Native matrix finishing

Read `AGENTS.md`, the continuation, engineering log and user guide. Compared
the original ClearSignal, Aurora, Spectral and Bauhaus matrix crops with genuine
1586 × 992 native captures from source `a362667`. Those images predate this
patch and the new Clear fixture.

## Changes

Only the matrix paint branch in `Workspace::Impl::drawPanel`, selection setup
in `syncRouteControls`, and this report were edited.

- Route refresh now retains an existing valid `inspectedRoute`, chooses the
  first real route when the key is absent/stale, and clears it for empty routes.
  The inspector, painted row and native row selection therefore agree at load
  and after deletion. Active depth drags still return before any refresh change;
  pointer and accessibility focus callbacks retain their existing selection path.
- ClearSignal paints its selected material beneath every label, including the
  row number. The reference row's sampled background median is RGB 169/241/235.
  The new fill anticipates the native row's existing 6.5% accent overlay, giving
  approximately 166–174/241–243/235–237 before antialiasing. Alternating rows gain
  modest separation.
- Narrow cells in these four skins show canonical `Macro N` identities and
  abbreviate an exact native `Voice LFO N` label to `LFO N`. Complete source and
  destination names, user macro names, native amounts and actions remain in the
  untouched accessible row/slider metadata and tooltips.
- Bauhaus cell faces leave 5 px between adjacent rows, and its rails use a cream
  recess. Spectral rails become darker with a violet edge; signed readouts gain
  weight. Aurora source/destination cells gain contrast and its linear curves
  lose the extra boxes. Brighter rounded curve strokes still use each route's
  actual signed slope. Muted Clear/Bauhaus routes retain muted amount ink.

## Verification and limits

Strict manifest validation passes all ten workspaces; the two existing Clear
overlap warnings remain. `git diff --check` passes. Source comparisons verify
unchanged `MatrixLayout`, route placements, row/slider handlers, route metadata
adapter, signed-depth formatting, native endpoint calculation and drag guard.
The first Clear route's new +0.64 fixture is not edited or cosmetically inflated.

No build, fixture change, commit or push was run. No synthetic selection test was
added; the native modulation interaction regression remains unchanged. The
coordinator's exact-source CI and captures must verify initial selection,
preserved explicit selection, deletion/empty refresh, and final material balance.
This is a source-verified refinement, not a new native-render or identity claim.
