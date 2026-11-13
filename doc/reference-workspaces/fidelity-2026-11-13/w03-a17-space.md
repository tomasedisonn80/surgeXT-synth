# Wave 3 / assignment 17 — Space-Age refinement

## Evidence

Compared `references/05_Space_Age_1977.png` with
`native-gallery/05-space-age-1977.png`, both 1586 × 992. The native evidence
predates the current shared material pass; this is not a post-change capture.
Read the continuation, engineering log, user guide and current native gap audit.

The panel grid, oscillator rotary centres and macro centres are already close.
The native oscillator/FX values are visibly smaller and duller than the cream
reference readouts. The reference filter graph starts nearer its left recess
edge and fills slightly more of the available well.

## Changes

Only `reference-05-space.surge-skin/workspace.json` was changed:

- Raised the eighteen oscillator and eight selected-FX rotary caption/readout
  font sizes from 10.5 to 11.5. Their existing 16-pixel lower text bands retain
  space for the larger glyphs.
- Applied the existing cream text ink `#F7E8BF` to those controls. The painter
  keeps the caption cream and derives live value ink at 85% alpha, a restrained
  lift from the previous muted tan readouts.
- Expanded both filter plots from `[1210, y, 230, 74]` to
  `[1206, y, 236, 76]`, retaining their original vertical positions. Their right
  edges remain eleven pixels clear of the value controls.

All ninety control rectangles, face diameters, IDs, bindings, slot maps,
dynamic parameter names, labels and values remain unchanged. No decorative
state or unsupported controls were added.

## Validation and remaining renderer work

`validate.py --strict-overlap` passes all ten workspaces, with only the two
pre-existing ClearSignal plot/control warnings. `git diff --check` passes.
A structural before/after check confirms only 52 intended control presentation
fields and two plot rectangles changed.

The brass heading baseline still needs shared-renderer attention. In the browser
heading crop `(30,90)–(185,120)`, dark reference glyphs occupy rows 101–111;
native glyphs occupy 95–105. The fixed Space `drawPanel` title offset is three
pixels. Changing panel positions to solve this six-pixel text offset would
misalign their correctly placed controls. Fader cap proportions likewise
belong in the painter; interaction rectangles were not reduced to style them.

Fresh compiled captures must check larger dynamic labels, value legibility and
the combined shared-material changes. No runtime pass or pixel identity is
claimed. No shared C++, commits or remote operations were performed.
