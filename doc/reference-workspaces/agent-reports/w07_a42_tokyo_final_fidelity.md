# Wave 7, assignment 42 — Tokyo Industrial final fidelity audit

> Curated historical assignment report. The implementation was subsequently included in passing native source `eccf282`. Its new captures still require visual comparison; the earlier image measurements below describe the stated prior source. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Evidence and scope

Compared the supplied 1586 × 992 reference `doc/reference-workspaces/references/06_Tokyo_Industrial.png` with the actual native JUCE baseline `native-artifact-22b9c29/06-tokyo-industrial.png`, built from source `22b9c299e6aa74093c9e96677cb32df5f2f74e74`. The working source checkpoint was `3ec3998877eab793f8bf2aa4acb367c69de17979`; its subsequent shared renderer refinements are not present in this comparison image.

Read the repository implementation contract, continuation, engineering log and user guide. Inspected both the complete canvas and native-size oscillator/filter crops. Owned and changed only `resources/data/skins/reference-06-tokyo.surge-skin/workspace.json`. No reference image was embedded as UI artwork; no parameter, signal, audio fixture, browser content or runtime value was substituted.

## Implemented change

Raised Tokyo's canvas background from `#090B0D` to `#0E1012`. The panel fill, borders, control inks and accent colors remain unchanged because their measured body samples already match reasonably closely. The original empty gaps were uniformly RGB (9,11,13); this conservative correction produces (14,16,18).

Measured reference medians in clear gap/margin regions:

| Region [x,y,w,h] | Reference median RGB | Previous native median RGB |
| --- | --- | --- |
| Between OSC2/OSC3 [818,200,3,200] | (18.5,20,21) | (9,11,13) |
| Between OSC1/OSC2 [558,200,3,200] | (16,18,19) | (9,11,13) |
| Between LFO/matrix [594,650,5,180] | (15,17.5,18) | (9,11,13) |
| Bottom margin [50,985,1450,5] | (12,14,15) | (9,11,13) |
| Right margin [1581,140,4,840] | (13,15,18) | (9,11,13) |

These limited samples justify the small background correction; they are not a whole-image fidelity score. The reference's brighter shared horizontal recesses and panel-edge gradients cannot all be reproduced by one flat background value.

All 18 panels, 92 controls, 124 declared rectangles, IDs, bindings, labels, menus, source selectors and control geometry are unchanged. An explicit parsed equality check proves the background field is the only changed manifest data.

## Actionable shared renderer findings sent to coordinator

1. **OSC1 mesh color:** The native layered wavetable uses red-to-cyan coloring; the reference mesh is cyan. The layered oscillator call to `Visuals::wavetable` supplied `theme.accent` as its secondary color, whereas the existing non-layer Tokyo waveform already uses `waveformInk` for both colors. The coordinator confirmed a Tokyo-only color correction while preserving real amplitudes, frames and projection.
2. **Numeric readout weight:** Reference bottom numeric values are brighter and bolder. At oscillator 1's first readout approximately [300,363,47,22], the brightest-pixel sample is near RGB (217,217,218), versus native (146,155,157). Filter 1 cutoff at approximately [1390,240,50,25] is near reference (224,227,227), versus native (147,155,157). Recommended Tokyo numeric strings use actual `theme.text` and bold weight, without changing any value or unit.
3. **Oscillator value capsule:** The first oscillator reference readout has a subtle dark rounded backing, with dark-half median about (18,20,22); the native backing is about (25,27,28). Filter readout backgrounds already match closely and should not automatically receive the same capsule.
4. **Knob material:** Reference dials have stronger upper highlights and a more visible perimeter/bevel than the native flat dark caps. No unsupported geometry change was made to compensate for this shared paint difference.
5. **Native semantics remain visible:** Actual algorithm names, units, source waveform shape and browser entries differ from the concept. Filter-header power glyphs are absent where there is no implemented real native power control; no inert replacement was added. The native third modulation envelope remains honestly labelled LFO 3 / Envelope.

The coordinator owns shared renderer changes and their linked native verification.

## Validation

- `python3 scripts/reference-workspaces/validate.py --strict`: pass for all ten workspaces, with only the two previously documented ClearSignal corner-overlay warnings.
- `git diff --check`: pass.
- All 124 declared rectangles remain inside the 1586 × 992 canvas.
- Parsed equality outside `palette.background`: pass.
- Manifest SHA-256 after edit: `e2cd692f95c1928323676da8cb0b48186cc6665693eb01527f00c2e1bc7df977`.
- Changes left unstaged. No commit or push performed by this assignment.
- Fresh native capture after this edit remains pending. This audit does not claim pixel identity.
