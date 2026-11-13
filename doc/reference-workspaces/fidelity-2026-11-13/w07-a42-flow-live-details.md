# Wave 7 / assignment 42 — Flow and Live details

## Evidence and scope

Reviewed the original Flow/Live concepts, assignment 36, the current native
gallery, and the coordinator's `review/w5-10-live-performance-deck.png` pair.
The comparison baseline is source
`0708bc76bb194a127064cf18b8849cf77373a7e7`, native workflow `43493343847`.
The new changes below require the coordinator's combined native gate; that
baseline does not certify the changed renderer.

## Implemented corrections

- **Flow response:** the manifest well is now `[1322,430,244,136]`. A Flow-only
  1 px inner border yields the requested data/grid rectangle
  `[1323,431,242,134]`, replacing `[1332,447,222,104]`. Frequency captions sit
  below the well; numerical dB captions stay in the left gutter and the dB unit
  sits in the footer. The native logarithmic 13.57–25,087 Hz response domain
  with 20 Hz–20 kHz labelled ticks, native −42…+18 dB range, cutoff marker and
  unavailable-state handling remain.
- **Flow macros:** quiet cell separators now lie at x106/199/292 and y883, with
  a header rule at y787. The eight 18 px badges derive their numbers from the
  existing `macro1`…`macro8` identities. A Flow-only 18 px inset in top-caption
  painting clears the badges while retaining full real names, values, dial
  centres and all native component bounds.
- **Live meters:** 9 px illuminated cores share the existing channel centres,
  with a saturated green vertical gradient and narrow glass reflections.
  Their heights still use the unchanged actual stereo peak mapping; clipping
  still paints from `peak >= 1`. The right gutter now labels 0 dB, −6, −12,
  −24, −36 and −60, aligned to the same inset height. There is no held or
  invented peak state.
- **Live knob material:** the native pair confirms weak macro rings. Stronger
  mint/brass rim reflections and an additional clipped halo on the existing
  value arc improve the large Live dials. The true arc endpoint, pointer angle,
  face dimensions, component bounds and interaction paths are unchanged.
  Aurora's independently owned halo scaling is preserved.

Changes are confined to `Workspace.cpp`, the Flow response plot rectangle and
this report. No processor, scene, MIDI, parameter, event, accessibility or
audio-source operations changed. All 51 Flow and 64 Live control definitions
compare equal to `HEAD`; the Live manifest is unchanged.

## Validation and next gate

- Strict manifest validation passes all ten workspaces. The existing two
  ClearSignal plot/control advisory warnings and Flow routing-overlay note
  remain; neither concerns these changes.
- Schema self-tests pass: 42 schema rejections, 24 malformed JSON rejections,
  9 path checks, and 28 accepted / 140 rejected shared cases.
- `git diff --check` passes. Numerical geometry checks confirm the Flow grid,
  separator positions, meter gutter fit and matching meter-scale positions:
  y805 / 818.7 / 832.4 / 859.8 / 887.2 / 942 respectively.
- No linked build or fresh native capture is claimed here. Inspect these exact
  changes in the next combined native run, especially Flow caption clearance
  and the Live material/core optical balance. No commit or push performed.
