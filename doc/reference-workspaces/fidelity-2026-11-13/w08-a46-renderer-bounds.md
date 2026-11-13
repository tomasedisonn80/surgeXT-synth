# Wave 8 / assignment 46 — Renderer bounds and paint-path review

## Reviewed source and result

Reviewed the combined Wave 7 renderer at
`ea674878313006afa0b9d1a2b1d3495b69755627`, including `Workspace.cpp`,
`Visuals.cpp`, `ReferenceOrnaments.h` and the affected manifests. Read AGENTS,
continuation, engineering log, user guide, current pass record and assignments
37–42. This is source/geometry review, not a linked build or native render pass.
Assignment 43 owns the new actual-child placement and pointer regression.

One declaration mismatch was found and corrected by the coordinator before
publication: the first Space subplate begins at x255, but `space-global`
previously began at x256. The published parent is `[255,85,1323,68]`, preserving
right edge x1578 and containing all five plate faces. No further component or
plot containment defect, new child interception, processor-state mutation or
evident C++ overload/type defect was identified in the reviewed changes.

## Geometry findings

| Area | Reviewed result |
| --- | --- |
| Space global plates and wells | Plate faces span x255–1578/y85–153. Scene well `[303,94,99,52]`, meter well `[1418,96,131,48]`, L/R labels and dB ticks fit their plates. The shared pre-existing bezel deliberately adds an exterior hardware shadow; its combined envelope is x254–1579/y87–157, clear of the adjacent browser and lower panels. It is not a plot clip or a new child. |
| Ivory master | Component `[1312,10,206,72]` contains the 66 px face `[1397,11,66,66]`, caption and actual value rectangle `[1471,43,47,18]`. There are 8 px between the requested face and value rectangle, 2 px to the previous header action, and 18 px before the meters. Native input routing remains assignment 43's gate. |
| Aurora dials and title glaze | All 24 changed face allocations retain their component rectangles. Conservative full-circle halo radii are 16.407/14.766/23.516 px for the 30/27/43 px faces, within their available paint areas. The browser glaze ends at y111 before its child begins at y116. The six compact-card glazes are clipped to the inset rounded panel and end at y600; later plot painting retains its own bounds. |
| ClearSignal identity glyphs | All eleven vectors use a scoped 16 px clip and restore the graphics transform. Glyph-to-heading gaps are 8 px normally, 9 px for FX and 2 px for Analyzer. Source/analyzer tabs retain 8 px gaps; oscillator/envelope/LFO/analyzer heading widths are 137/97/60/90 px. The lower Clear-only threshold permits the 60 px LFO heading without moving any tab or control. Actual font rasterization remains a capture check. |
| Flow response and macros | Response well `[1322,430,244,136]` supplies data rectangle `[1323,431,242,134]`. Axis painting uses the containing panel clip: numerical dB labels occupy x1291–1318; frequency/unit captions occupy y568–581 inside the panel ending at y586. Trace/glow/marker painting then narrows to the well clip. All eight badges fit the macro panel; their right edge is 5 px before the inset caption start. Separators remain inside that panel. |
| Live meter scale and halo | Scale labels occupy x1545–1574 within the master panel ending at x1578. Tick positions are y805/818.7/832.4/859.8/887.2/942, matching the active core's inset −60…0 dB mapping; text stays within y798.5–948.5. The new glass fill uses only the measured active rectangle, and the extra large-dial halo explicitly clips to the supplied face. The strengthened rim remains inside the control allocation. |
| Bauhaus full harmonic field | The caller supplies `[270,135,700,189]`, inside the dark panel, to both the wavetable and real-waveform fallback. Their existing scoped clip bounds strokes and any arbitrary-source excursions. The full view retains 32 of 48 original rows plus the actual selected trace; it adds no row interpolation or strip fills. |
| Spectral terrain | The caller retains `[455,146,692,236]`. Its scoped clip also contains floor, mesh, selected marker and glow. Available snapshots are limited to 48 × 256 samples; full terrain has at most 142 rows and 128 seams of at most 142 points. The new playable source does not enlarge that renderer budget. |

### Remaining optical clearance item

The Space Width caption allocation is `[1163,94,42,16]`. Its top major dial
graduation is at x1184, y103.36–106.36 with a 1.2 px stroke, so the reserved
caption rectangle overlaps that tick. This is a concrete paint-allocation
intersection, not proof of a glyph collision: the caption uses fitted native
text within that rectangle. Both remain inside the slider. The coordinator was
notified; inspect Width's actual glyph/tick clearance in the exact-source native
capture before optical acceptance.

## Bounded work and state

The new ornaments, subplates, badges and meter labels are parent/control paint,
not components or mouse interceptors. The changed paint branches read existing
value strings, immutable snapshots or current output-tap peaks; they introduce
no parameter writes, gesture changes, subscriptions, audio processing or engine
RNG consumption. Existing unrelated keyboard metadata updates are outside this
change's scope.

New Bauhaus grain is a single cached 512 × 256 cosmetic hash tile, initialized
on the UI paint path. Hero vertex storage remains bounded at 12,288 points;
temporary JUCE paths/gradients and the bounded Flow manifest scan stay on the
message thread. Aurora changes alpha and colour without adding compound haze
strokes. No shader, background worker or audio-thread allocation was added.

## Verification and handoff

Read-only numeric inspection checked the published rectangles, heading/glyph
gaps, meter transfer positions and row/vertex limits above. The coordinator
reported strict manifest validation passing after the Space-parent correction.
`git diff --check` passes for this report. No implementation, tests or assets
were changed by this assignment; no commit or push was performed.

The existing `0708bc7` native gallery predates this source. Compilation, native
behavior, actual 100%/125%/150% rendering and optical acceptance remain the
coordinator's pending exact-source gate for `ea674878`; this review does not
replace it.
