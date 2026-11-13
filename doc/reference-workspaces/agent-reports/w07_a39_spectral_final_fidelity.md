# Assignment 39 / wave 7 — Spectral final manifest fidelity

> Curated historical assignment report. The implementation was subsequently included in passing native source `eccf282`. Its new captures still require visual comparison; the earlier image measurements below describe the stated prior source. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Evidence and scope

Compared the original 1586 × 992 reference
`doc/reference-workspaces/references/03_Spectral_Observatory.png` with the genuine native component capture
`native-artifact-22b9c29/03-spectral-observatory.png`, captured from
`22b9c299e6aa74093c9e96677cb32df5f2f74e74`.
Read AGENTS, the user guide, continuation and engineering log, and assignment 34's pending projection report.
Current checkout base at audit completion: `3ec3998877eab793f8bf2aa4acb367c69de17979`.

Only `resources/data/skins/reference-03-spectral.surge-skin/workspace.json` was edited.
No shared renderer, DSP, parameters, routes, fixtures, tests or image assets were changed.
The observatory hero and oscillator source plots remain untouched because their shared projection/material changes need their own native capture.

## Changes

Eight of 69 controls have narrowly scoped presentation changes. All 18 panels, source-selection blocks, IDs, bindings, palette and adaptations are unchanged.

| Controls | Change | Measured reason |
| --- | --- | --- |
| Noise / Ring 1×2 mixer knobs | `labelPosition: top`; y 622→614, h 80→66 | Reference puts the caption above the dial. The native pre-edit Noise caption occupies y674–681; reference y618–626. Native value occupies y690–697; reference y668–676. New layout preserves the face x center and changes face y center only from 646 to 647, while moving both text bands into the reference region. |
| Filter 1 knob column | x 713→718 for cutoff, resonance and envelope amount | Reference cutoff ring center is approximately (744.43, 545.49); pre-edit native (739.85, 545.51). Keeping the three controls on one column preserves their local hierarchy. |
| Filter 2 knob column | x 1010→1007 for the same three controls | Reference cutoff ring center is approximately (1034.05, 545.74); pre-edit native (1036.98, 545.56). |
| Both cutoff dial faces | faceDiameter 42→35 | Reference ring radii are approximately 15.88 / 16.09 px; native 19.29 / 19.38 px. The inspected native drawing code uses radius = faceDiameter × 0.46, so 35 predicts 16.10 px. Cutoff y bounds remain unchanged. |

Circle measurements fit x²+y² = 2cx·x + 2cy·y + k to saturated bright ring pixels in isolated cutoff windows. They are local geometry diagnostics, not global similarity scores. Small resonance/amount arcs had too little reliable colored circumference for a comparable fit, so their y and diameters were left unchanged.

## Geometry verified without changes

- Oscillator cards: (25,108,420,135), (25,250,420,132), (25,390,420,134).
- Oscillator knob x centers: 244, 293, 351, 411 in each row. Menus and live wavetable selectors remain in the established title bands.
- Mixer fader x centers: 80, 165, 249. Their native values and interaction areas remain intact.
- Hero: (454,108,694,347), source plot (455,146,692,236). Seven source-following parameter centers remain 555, 644, 733, 822, 911, 1003, 1095.
- Filter plot bounds remain (495,523,212,135) and (799,523,208,135), including the renderer's internal caption margins. Both keytracking fields remain separate and bounded.
- Macro strip (1157,108,405,142), envelope panel (1157,257,405,198), LFO shell (1157,463,405,247), and inner LFO card (1162,467,395,168) retain the reference's three-tier right-hand hierarchy.
- Bottom matrix / effects / output panels remain (25,724,658,225), (692,724,507,225), and (1208,724,354,225).

## Verification

- `python3 scripts/reference-workspaces/validate.py --strict-overlap`: PASS, all ten workspaces. Spectral retains 18 panels and 69 controls, with no Spectral warnings. Existing ClearSignal plot/menu overlap warnings are unrelated and unchanged.
- Exact JSON invariant comparison: only the listed fields on the eight controls differ. All 69 native bindings and IDs are identical; all non-control sections are byte-equivalent after JSON parsing.
- All control bounds remain within the 1586 × 992 canvas.
- `git diff --check -- resources/data/skins/reference-03-spectral.surge-skin/workspace.json`: PASS.
- Final manifest SHA-256: `6ad29e0d0372cb59f2b9745ea906f0839044431e68f2dd505bb61a9adaf6ed89`.
- No staging or commits performed.

## Remaining differences and limits

The inspected screenshot predates these manifest corrections and assignment 34's hero projection. **A compiled native capture after this edit is pending.** No 1:1 or runtime acceptance claim is made here.

The pre-edit native mixer paints solid purple fader tracks and plain caps; the reference uses much thinner cyan rails and inset luminous caps. This is a shared control-paint issue, with already aligned fader centers, so no manifest compensation was applied.

Reference/native waveform shape, oscillator-family titles, filter response, envelope/LFO phase, parameter units, route rows and output spectrum differ with actual engine state. The manifest's existing adaptations correctly retain native source families, real parameter meanings, actual audio data and honest algorithm-family FX pictograms. These should not be replaced with reference values or frozen traces.
