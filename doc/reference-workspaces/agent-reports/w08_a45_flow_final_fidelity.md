# Assignment 45 — Signal-Flow Cartographer final fidelity review

> Curated historical assignment report. At this archive snapshot, these completed manifest edits follow the latest verified native checkpoint and require a fresh exact-source build and capture. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and inspected evidence

Reviewed `doc/reference-workspaces/references/09_Signal_Flow_Cartographer.png`
against the genuine compiled native capture
`native-artifact-3ec3998/09-signal-flow-cartographer.png`.
Both original images are 1586 × 992. The native source is
`3ec3998877eab793f8bf2aa4acb367c69de17979`, workflow run
`43389991532`. The coordinator verified that this source passed the native
behavior and capture gate. This review used that baseline, not a painted mockup
or an image copied into the live UI.

The full composition, native-size inspector, and bottom macros/matrix regions
were inspected. AGENTS.md, the continuation, user guide, engineering work log,
manifest and current public drawFilter/knob code were read.

## Implemented manifest correction

Changed only
`resources/data/skins/reference-09-flow.surge-skin/workspace.json`.

The actual saturated cutoff arc was fitted to a circle in a bounded crop around
the inspector's main Filter 1 cutoff control. The mask excludes near-white
captions and most interior cap pixels; fit residuals are recorded to avoid
presenting antialiased pixels as exact vector measurements.

| Image | Fitted center | Fitted radius | Selected pixels | Mean radial residual |
| --- | --- | --- | --- | --- |
| Reference | (1430.33, 239.96) | 37.10 px | 707 | 1.52 px |
| Native baseline | (1431.48, 238.04) | 38.25 px | 335 | 0.62 px |

For `inspector.filter1.cutoff`, changed x 1384 → 1383, y 178 → 180,
and faceDiameter 84 → 81. Its component width/height, accessible label, binding,
font and interaction behavior are unchanged. The new center is predicted near
(1430.48, 240.04); fresh compiled capture must verify that prediction.

All 11 panels, 51 controls, palette values, selections, plot rectangles,
flowNodes, and 19 original route-node controls remain otherwise identical.
Only three JSON lines changed. Original ASCII escaping and formatting remain
preserved.

## Shared renderer findings sent to the coordinator

- The reference's cutoff arc is visibly thicker and more separated from its
  recessed cap. Flow currently follows the generic nonglass cap inset and
  2.2px active arc. A Flow-specific proportional cap inset and approximately
  3.5–4px large-dial arc can improve material fidelity without changing the
  native parameter value or the measured outer center/radius. Small dials need
  a proportionate treatment, rather than a fixed 6px inset.
- The reference Filter 1 response uses dB captions; Flow is excluded from
  drawFilter's `dbReadouts` condition. The existing declared plot is
  [1325,432,236,134], and its current inner grid is [1332,447,222,104].
  The response panel starts at x1291, leaving a 34px left margin.
  Any truthful dB treatment should use the available margin and retain the
  actual native response domain. A generic new internal gutter would move the
  existing grid and hide or crowd its frequency captions.
- Background colors vary spatially in the concept. Outer margins, the right
  gap and bottom gaps already agree with the native base within a few code
  values, whereas the narrow browser/flow gap is brighter in the reference.
  A global palette lift would worsen the matching outer regions, so the
  background was preserved.
- Blank inspector, macro and output surfaces already match closely:
  inspector medians reference/native (10,30,49)/(9,29,49), macro
  (8,27,44)/(8,28,47), output (6,23,40)/(6,24,42). No broad recolor was justified.

## Validation

- Strict manifest validation: all ten packages pass; the two known ClearSignal
  plot-corner warnings and intentional Flow route-node overlay note remain.
- `git diff --check`: pass.
- Parsed before/after equality outside the three allowed cutoff fields: pass.
- All 82 declared panel/control/plot/tab/node rectangles stay within the
  1586 × 992 canvas.
- Manifest SHA-256:
  `f83ccc77b71833e5184df57795ca94799f09f498145344bef996e9a68d6d0ab1`.
- Changes remain unstaged. This assignment made no commit or push.
- Compiled capture after this exact change: pending integration.

## Honest remaining differences

The native diagram represents actual Surge filter routing, oscillator and FX
algorithms, modulation connections, source availability and current parameters.
The reference's arbitrary modular wiring, third dedicated envelope, independent
per-filter velocity amount, preset listing and invented/static state are not
substituted into the working UI. Waveforms, response shapes, meters, spectra,
macro names, route labels and numeric values continue to come from the native
processor. Passing this audit and validation does not establish pixel identity.
