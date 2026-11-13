# Wave 7 / assignment 40 — Space and Tokyo hardware

## Evidence and scope

Inspected the original references 05/06 against the fresh unmodified native
gallery from source `0708bc76bb194a127064cf18b8849cf77373a7e7`, successful run
`43493343847`. Enlarged global-strip, brass and dial crops confirmed the three
priorities in assignment 34. Its older capture disclaimer does not apply to this
fresh comparison. Changes are confined to Space/Tokyo paint in `Workspace.cpp`
and three Space manifest rectangles; concurrent Ivory/Precision/Bauhaus,
Aurora/Clear and Live/Flow work is retained.

## Changes

- Space's global strip now paints five separate plates at x ranges 255–405,
  409–665, 670–1085, 1088–1261 and 1266–1578, all y=85–153. The actual scene
  buttons retain their existing bounds over a recessed well. An output well at
  `[1418,96,131,48]` calls the existing `headerMeters` helper, reading the current
  stereo output tap. Its −60, −40, −20 and 0 dB marks match that helper's actual
  logarithmic mapping. No reference activity or meter state is painted in.
- The coordinator expanded the presentation scope after checking the reference:
  the old Width placement would cross the fourth/fifth plate boundary. Glide,
  Pitch and the honest Width adaptation now have explicit native-slider paint
  layouts. Their processor bindings, value strings, typed entry, callbacks,
  gestures and focus handling are retained. Glide/Width use the existing dark
  recessed Space cap material and keep their 34 px requested face diameters.

| Control | Manifest bounds | Painted position on the logical canvas |
| --- | --- | --- |
| Glide / `scene.portamento` | `[952,93,130,55]` | Dial centre `(1014,123)`; live value to its right |
| Pitch / `scene.pitch` | `[1093,93,62,55]` | Field `[1095,115,58,30]`, centre `(1124,130)`; caption above |
| Width / `scene.width` | `[1158,93,101,55]` | Dial centre `(1184,123)`; native Width/% caption/value |

- Space's brass panel base is warmer and darker. Two localized horizontal
  reflections provide broad variation while the existing deterministic fine
  machining grain is unchanged. This addresses the measured empty macro sample:
  reference mean RGB 171.81/143.87/109.71, red standard deviation 17.20; previous
  native mean 184.26/159.87/126.02, red deviation 7.04. Small selected chips and
  dark instrument wells do not receive the new brass-base offsets.
- Tokyo's thin top reflected lip and lower enclosure shadow are stronger without
  raising the graphite body. Its dials have a mechanical housing, inset dark cap
  and thicker value-driven red collar, approximately 3.75 px for the existing
  48 px macro faces. The pointer is proportionately wider and stays on the same
  native value angle. Shadows sit behind the collar so they cannot erase its
  lower-range arc. Disabled accent handling, focus outline, centres and control
  hit bounds remain unchanged.

## Validation and remaining gate

`python3 scripts/reference-workspaces/validate.py --strict-overlap` passes all
ten packages, including the three relocated Space controls. Only the existing
ClearSignal plot/control warnings and intentional Cartographer routing overlay
note remain. `git diff --check` passes. No paint-mirroring tests were added.

The new rectangle arguments use typed `Rect` values. No audio, parameter bridge,
shared meter transfer, manifest schema or processor state code was changed.
Actual native child placement/hit routing for the three relocated Space controls
is handed to assignment 43, including 100%, 125% and 150% layouts. The local
executor still lacks CMake/Ninja, Xvfb and Openbox; this assignment does not claim
a new linked build or native capture. Fresh exact-source native screenshots must
confirm the new brass distribution, Tokyo edge contrast/collar and small caption
clearance. No pixel-identity claim, commit or push is made here.
