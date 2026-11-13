# Wave 2 / assignment 12 — native button finishing

## Evidence and scope

Inspected original references 01 Obsidian Precision, 07 Liquid Aurora and 10 Live
Performance Deck beside their existing native-gallery captures, plus the current
pass README and assignment-4 native gap audit. The existing captures predate this
patch; this is a source refinement, not fresh native rendering evidence.

Changed only the assigned `Workspace.cpp` paint sites: `themedButton`,
`HeaderButton::paintButton` and `ViewTabButton::paintButton`.

## Changes

- Aurora's selected native chips and tabs now use a darker violet surface with
  white captions, a soft upper reflection and a thin luminous inner rim. This
  replaces the previous pale, uniformly filled selected surfaces.
- Live retains mint selected navigation chips while adding a fine pale/mint
  upper edge and a restrained warm lower reflection from its secondary palette.
  Selected effect slots now have a dark green center and mint captions/outline,
  matching the reference's recessed effect treatment more closely.
- Obsidian's small selected controls use dark amber-tinted centers and a defined
  amber outline. Its existing bright header scene selection remains intact;
  selected header navigation receives a stronger fine border.
- Unselected controls in these three styles receive a quieter short reflection
  and inner edge so their surfaces remain distinct without implying selection.

No bounds, captions, callbacks, accessibility definitions, keyboard behavior,
toggle state handling or engine bindings changed. No icons or apparent new
operations were added. Other style branches, including Ivory and Bauhaus, retain
their original painting.

## Verification and next step

`git diff --check` passed after the edit. Source review confirms all added drawing
is inside the existing button rectangles and style-gated. No local toolchain,
native build or interaction test was run for this assignment. The coordinator's
integrated native workflow must build this source and compare fresh captures,
including hover, selection, focus and effect-power legibility. No commit was made
by this assignment.
