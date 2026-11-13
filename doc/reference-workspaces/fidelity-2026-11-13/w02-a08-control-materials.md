# Wave 2 assignment 08 — knob and fader materials

## Changes

Inspected the full-resolution Live Performance Deck native/reference pair and
the Liquid Aurora original, together with assignment 04's all-skin audit.
Changes are confined to `Workspace.cpp::knob` and the fader-cap portion of
`BoundSlider::paint` (the actual slider class; there is no `LiveSlider` class).

- Live, Aurora and Spectral dials now have a thin fixed material rim separate
  from the real value arc. Live uses a restrained brass/neutral finish; the
  violet skins use a cooler pearl finish. Opposing bright/dark rim segments
  create the reference's concentric hardware appearance.
- The same three styles have a narrower, stronger halo, a 2.7-pixel live arc and
  a one-pixel pale core. The arc angle and indicator still derive from the
  actual normalized parameter value. Disabled cores remain subdued instead of
  becoming bright when their color is interpolated toward white.
- Darker lower cap shading and fine upper/lower edge reflections replace the
  previous broad gray face highlight. A scoped graphics state confines this
  cosmetic drawing to the dial call.
- Hardware/glass fader handles gain narrow polished leading edges and dark
  undersides. Live/Space caps have a fine lower ridge; Aurora caps use a softer
  pill radius. Only the existing enabled glass handles receive restrained glow.

ClearSignal, Bauhaus and Ivory retain their distinct knob treatment. Existing
control rectangles, cap travel, hit geometry, value calculations, MIDI, event
handlers, accessibility names and parameter/undo code were not changed. No
reference-image pixels, baked values, clocks or random generators are involved.

## Checks and remaining gate

`python3 scripts/reference-workspaces/validate.py --strict-overlap` passed for
all ten manifests, retaining only the previously documented ClearSignal overlap
warnings and deliberate Cartographer routing overlay note. `git diff --check`
passed. Reviewed the diff to confirm changes remain in the assigned functions;
other agents' concurrent edits were preserved.

These are source/layout checks, not a native build or visual acceptance. The
coordinator must inspect the next exact-source native gallery, especially the
small Aurora controls and large Live macro/master dials, for overbright rims or
crowding at normal and scaled sizes. No pixel-identity claim is made.
