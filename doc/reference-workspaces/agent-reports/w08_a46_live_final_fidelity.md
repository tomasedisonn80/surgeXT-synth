# Wave 8 / assignment 46 — Live Performance Deck fidelity review

> Curated historical assignment report. At this archive snapshot, these completed manifest edits follow the latest verified native checkpoint and require a fresh exact-source build and capture. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and inspected evidence

Compared the supplied 1586 × 992 reference,
`doc/reference-workspaces/references/10_Live_Performance_Deck.png`, with the actual
native component screenshot
`native-artifact-3ec3998/10-live-performance-deck.png`.

The inspected native source is
`3ec3998877eab793f8bf2aa4acb367c69de17979`, workflow run `43389991532`.
The coordinator verified that run's 39 native behavior cases and ten baseline /
edited capture pairs passed. Those results precede this assignment's manifest edit.

Owned and changed only
`resources/data/skins/reference-10-live.surge-skin/workspace.json`.
No C++ renderer, capture fixture, engine source or test source was edited.
No commit or push was made by this assignment.

## Implemented correction

Added `faceDiameter: 56` to the existing `c.mixer-amp` native Amp Gain knob.
Its previous face filled the available 70-pixel control width. A color-mask
circle fit over the bright reference annulus gave center approximately
(405.66, 700.31), radius 25.72 pixels; the native amber annulus gave
(404.15, 697.37), radius 31.90 pixels. The renderer's 0.46 × diameter arc radius
predicts 25.76 pixels for the requested diameter of 56.

The source parameter remains `amp.level`; it is not relabelled as the concept's
unsupported Scene A/B Mix operation. The control's position, hit rectangle,
caption, value, gesture behavior and accessible identity remain unchanged.
No other manifest field changed.

## Shared renderer findings delivered to coordinator

- Live's mixer currently fills broad mint rails. The reference has thin mint
  rails. The coordinator confirmed a three-pixel rail treatment while retaining
  actual native handle positions.
- Thresholded amber cap pixels span approximately 19–20 pixels in the reference
  and 22 pixels in the native image; the native painter uses 24 pixels before
  its outline. A 20-pixel painted cap was suggested. This is a paint change,
  not a change to the input hit region or value mapping.
- Master and mixer knob surfaces remain flatter than the reference's dark
  inset caps and strong beveled annuli. The master control's geometry was not
  moved speculatively.

## Genuine MIDI fixture suggestion, not implemented

The new MIDI lanes are real processor inputs and currently show the applied
zero input values in the inspected capture. The reference's brightest indicator
rows are Pitch y843, Mod y852, Exp y891 and Touch y867. With the existing native
track spanning y833–937, those rows correspond approximately to:

| Input | Native value |
| --- | --- |
| Signed pitch bend | +6616 (MIDI fourteen-bit value 14808) |
| Modulation CC1 | 104 |
| Expression CC11 | 56 |
| Channel pressure | 85 |

These are optional real incoming MIDI inputs for a future offline capture
fixture. The exact integer values are inferred from indicator positions, not
read from labelled reference values. They should be documented in provenance
and applied through actual MIDI processing, with the existing headroom and
reset gates retained. No input, telemetry value, painted state or fixture was
changed by this assignment.

## Remaining differences and honest native adaptations

The full composition is close at the panel level, but this is not a pixel
identity result. The source waveform and analyzer remain real native signals.
The concept's decorative crossing waveform traces are not copied into the view.
Current waveform, spectrum, native parameter values and algorithm names remain
different from the illustrative reference where the actual processor differs.

Scene level remains a genuine scene gain control rather than a fictional
timbre morph. The playable keyboard shows actual two-scene Single/Split/Dual/
Channel Split allocations; the concept's three independent zones and patch
snapshot buttons are not fabricated. The modulation overview remains an actual
selected native LFO. Limiter/Soft Clip controls are not mapped onto unrelated
parameters.

## Validation

- Strict manifest validator: all ten workspaces pass.
- `git diff --check`: pass.
- Parsed equality outside the single new faceDiameter field: pass.
- 22 panels, 64 controls and 107 declared rectangles retained.
- All 107 declared rectangles are within the 1586 × 992 canvas.
- Control IDs, bindings, selections, labels, palette and adaptation notes unchanged.
- Final manifest SHA-256:
  `2d121f1822bd86167f16a6d471c91f085e5a4c8bd3768f652652c1709934765f`.
- A new compiled native capture is required to approve the changed face size.
