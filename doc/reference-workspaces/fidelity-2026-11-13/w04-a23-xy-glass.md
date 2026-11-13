# Wave 4 assignment 23 — Live XY glass

## Evidence and scope

Compared `references/10_Live_Performance_Deck.png` with the genuine
`native-gallery/10-live-performance-deck.png` captured from
`a362667839b8e1d742cf3af94bb8710fa3c6d1fd`, both at 1586 × 992.
Only `MacroXY::paint` in `Workspace.cpp` changed.

The declared pad remains `[663, 318, 262, 244]`; its normalized coordinate
rectangle remains `getLocalBounds().toFloat().reduced(3)`. The real fixture
cursor is approximately `(824.7, 470.9)`, whereas the concept cursor is near
`(794, 447)`. That intentional state difference is preserved.

In the upper-left glass region `[675,333,775,432]`, the reference mean RGB is
approximately `(21.0,48.6,39.8)` and the existing native mean is
`(44.4,58.7,47.0)`: the native field is substantially grayer and brighter.
The shared native lattice uses 16 × 8 cells; the concept has approximately
14 × 12, with much weaker contrast and nearly square cells.

## Paint changes

- Added a Live-only recessed dark green glass field, restrained upper/lower
  reflected light, and three broad translucent reflection surfaces. These
  closed, unoutlined decorative surfaces contain no samples, routes, gesture
  history, reference pixels, animation or fixed cursor state.
- Replaced the flat gold outline with opposing silver/brass highlights, a
  dark separating channel and a thin inner reflected lip.
- Added a subdued 14 × 12 lattice without the generic emphasized midline.
- Kept the actual macro coordinate formulas and added pale illuminated axes,
  endpoint highlights, continuous radial cursor bloom and a small white core.

## Validation and remaining gate

Strict manifest validation passed all ten workspaces; existing ClearSignal
overlap warnings and the deliberate Cartographer overlay note remain.
`git diff --check` passed. A focused source comparison verified unchanged
constructor, handlers, gesture lifecycle and accessibility byte for byte;
non-Live drawing remains equivalent after removing unreachable Live branches.
The new Live path performs only the original two parameter reads and no
parameter writes, layout changes, clock calls, random operations or image loads.

No local build, commit or publication was performed. The coordinator owns the
next exact-source native capture and visual review, especially rim visibility,
reflection strength and cursor bloom at normal/scaled sizes. This report does
not claim a new native runtime pass or pixel identity.
