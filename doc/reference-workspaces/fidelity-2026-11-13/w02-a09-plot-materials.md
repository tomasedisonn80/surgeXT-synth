# Wave 2 / assignment 9 — plot materials

## Evidence and scope

Read the repository contract, continuation, engineering log, user guide and
assignment 4 native-gap audit. Compared the original and genuine baseline native
Aurora, ClearSignal and Ivory PNGs at their 1586 × 992 canvas. These remain
baseline evidence, not captures of this change.

Changed only `Workspace.cpp::plotBase`, `drawEnvelope`, `drawFilter` and `drawLfo`.
No commit, remote mutation, fixture change or new reference-derived signal.

## Changes

- Dark displays now have restrained vertical/radial lighting, an upper recess
  and a fine lower edge. Ivory receives slightly more green interior depth.
  Bauhaus, Space-Age and pale plot surfaces retain their flat treatment.
- Existing envelope/filter/LFO paths receive narrow translucent bloom and fine
  luminous cores where appropriate. Curve coordinates, samples, axes, parameter
  queries and cutoff interpolation are unchanged.
- Aurora LFOs use the violet accent, transparent fill and a brighter fine core.
  Aurora filter thumbnails receive dark display wells, while its envelope/LFO
  plots retain their integrated panel backgrounds.
- ClearSignal's dual response uses saturated cyan and coral, a stronger
  multistop translucent fill, matching legend colors and existing cutoff
  markers with a white rim, shallow shadow and restrained reflection. Pale
  envelope/LFO grids use faint teal; envelopes gain white-centered nodes.
- Added scoped graphics state and bounds clipping to envelope painting. No
  controls, hit regions, audio code, model calculations or geometry were edited.

## Validation and limits

`git diff --check` passed. Strict manifest validation passed all ten workspaces,
retaining the two existing ClearSignal plot/menu overlap warnings and the
intentional Flow control-overlay note. Source review confirmed that the existing
path construction and all preview data calculations are unchanged.

The local native toolchain is unavailable; no compilation, runtime or fresh
screenshot result is claimed. The coordinator's linked CI build and ten native
baseline/edited captures must verify final lighting, clipping and cross-skin
appearance. These cosmetic changes do not establish pixel identity.
