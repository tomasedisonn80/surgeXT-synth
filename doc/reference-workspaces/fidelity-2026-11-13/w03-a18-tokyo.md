# Wave 3 assignment 18 — Tokyo mechanical wheel surfaces

## Evidence and changes

Read the repository contract, continuation, engineering log, user guide and
current native-gap audit. Inspected the complete original
`references/06_Tokyo_Industrial.png` and the genuine baseline
`native-gallery/06-tokyo-industrial.png`, followed by enlarged footer crops.
These baseline captures predate this pass and are not evidence of the new paint.

The original footer has dark ribbed rollers with narrow red side indicators;
the native footer previously used thin fader rails and wide horizontal caps.
Added a narrow `BoundSlider::paint` branch limited to Tokyo faders bound to
`scene.pitch` or `scene.fmdepth`. It paints a recessed dark housing, raised
roller shading, fine fixed ribs and a red side strip. The bright position line
uses `getPositionOfValue(getValue())`; its filled tail ends at the real
zero-value position. Decorative ribs and highlights never depend on the value.

The housing derives from the existing `standardTrackBounds`. Control bounds,
native travel, drag mapping, keyboard focus, typed entry, accessibility,
undo/automation gestures and parameter semantics are preserved. `PITCH` and
`FM` captions and their actual numerical values remain visible. FM is not
relabeled as a MIDI modulation wheel. Other Tokyo faders and all other styles
continue through their existing rendering paths.

The Tokyo manifest changes only one adaptation note to describe the wheel
surface and its true scene pitch/FM bindings. Its 18 panels, 92 controls,
positions, dimensions, fonts, bindings and labels are unchanged.

## Validation and next gate

- `python3 scripts/reference-workspaces/validate.py --strict-overlap`: all ten
  manifests pass; only the existing ClearSignal overlap warnings and intentional
  Cartographer overlay note remain.
- `git diff --check`: pass.
- Parsed manifest comparison: identical outside one adaptation entry.
- Source comparison: `CompactSliderLookAndFeel`, slider construction, sync,
  pointer handlers, keyboard handling and gesture lifecycle remain unchanged.

No local build, commit or remote operation was performed. The coordinator's
exact-source CI build and native capture must verify the new material at normal
and scaled sizes, particularly legibility of the narrow red marker at both
travel endpoints. The reference's taller wheels omit numeric readouts; this
implementation deliberately retains the working controls' value bands. No
pixel-identity or runtime-pass claim is made.
