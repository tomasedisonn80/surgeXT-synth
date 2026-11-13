# Wave 8 / assignment 43 — Native placement and pointer routing

## Scope and evidence

Read AGENTS, continuation, engineering log, user guide, the current pass record,
and assignments 37/40. Coordinated the final Space rectangles and painted input
centres directly with assignment 40. The latest completed native evidence is
still source `0708bc76bb194a127064cf18b8849cf77373a7e7`, run `43493343847`;
it predates these layout changes and this regression.

Added one `[reference-placement]` behavior case in
`src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp`. No renderer, manifest,
binding, or existing assertion was changed by this assignment. Assignment 44's
independent audio assertions in the same source file are retained.

| Native control | Reviewed logical bounds | Visible input centre |
| --- | --- | --- |
| Ivory master / `master.volume` | `[1312,10,206,72]` | `(1430,44)` |
| Space Glide / `scene.portamento` | `[952,93,130,55]` | `(1014,123)` |
| Space Pitch / `scene.pitch` | `[1093,93,62,55]` | `(1124,130)` |
| Space Width / `scene.width` | `[1158,93,101,55]` | `(1184,123)` |

## Coverage

For all four controls at 100%, 125%, and 150%, the new case checks declared
geometry, actual child bounds, parent containment, and projected top-left and
bottom-right coordinates. Expected projection comes from the requested scale
and viewport, independently of the child's transform. It includes the 0.25 px
horizontal inset from rounding the 125% viewport width outward to 1983 pixels.
The test uses `std::ceil` for its viewport, so JUCE's half-to-even integer
rounding cannot inadvertently shrink the requested 1.25 canvas scale (see the
[JUCE rounding contract](https://github.com/juce-framework/JUCE/blob/master/modules/juce_core/maths/juce_MathsFunctions.h)).

The actual workspace hit test must return the intended native slider at its
reviewed visible centre and four inset component corners. Mouse-down, upward
drag, and mouse-up are delivered to that hit result using JUCE coordinate
conversion. The intended processor/host/slider values must agree and increase;
every other host parameter, including nearby controls and the other scene,
must remain unchanged. Each drag must produce one balanced host gesture, a
value notification, and no lingering bridge or adapter edit state. Resizing
and refreshing must preserve all host values. Focus eligibility, accessible
name, and binding identity remain explicit checks.

Existing compact-value tests already exercise real typed-entry callouts and
native units, and existing native undo/scene tests remain intact. This layout
regression uses pointer edits to cover the missing placement/routing boundary;
it does not duplicate unrelated parser or undo fixtures or assert paint pixels.

## Validation and remaining gate

`validate.py --strict-overlap` passes all ten workspaces with only the existing
ClearSignal overlap warnings and intentional Cartographer overlay note.
`validate.py --self-test` and `git diff --check` pass. Source review uses JUCE
component, point, mouse-event and parameter APIs already present in the native
codebase; no version-specific JUCE 8 API was introduced.

No fresh linked executable is available in this executor. The new case is
selected by the existing native workflow's
`[reference-workspaces]~[reference-render]` behavior gate and still requires
exact-source compilation and runtime execution. Synthetic JUCE events exercise
native component hit routing and callbacks; they do not constitute an OS
pointer/DAW test or a pixel-fidelity result. Fresh native captures must confirm
the reviewed painted centres and caption clearance. No commit or push was made.
