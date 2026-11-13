# Assignment 41 — Space-Age final fidelity review

> Curated historical assignment report. The implementation was subsequently included in passing native source `eccf282`. Its new captures still require visual comparison; the earlier image measurements below describe the stated prior source. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Evidence inspected

- Native source: `22b9c299e6aa74093c9e96677cb32df5f2f74e74`.
- Native image: `native-artifact-22b9c29/05-space-age-1977.png`.
- Reference: `doc/reference-workspaces/references/05_Space_Age_1977.png`.
- Both images are 1586 × 992. This review examined a labelled full composition and full-resolution oscillator, macro, LFO and selected-FX crops.
- Crop evidence: `analysis-only/w07_a41_overview.jpg`, `w07_a41_knobs.jpg`, `w07_a41_fx.jpg`. Each comparison labels the reference and native separately.
- Read the repository implementation contract, continuation, engineering log and user guide.

## Implemented manifest correction

Only `resources/data/skins/reference-05-space.surge-skin/workspace.json` changed.

The selected Delay panel previously exposed native parameters 0–7 in simple numerical order. The reference places Mix in the fourth large position and modulation amount / Width in the two lower positions. The native image instead showed Crossfeed, Rate and Depth in those positions. The three controls now resolve to the real native functions:

| Existing control ID | Previous FX4 binding | New FX4 binding | Actual native function |
| --- | --- | --- | --- |
| space-fx-detail-param3 | fx4.param3 | fx4.param10 | Mix |
| space-fx-detail-param6 | fx4.param6 | fx4.param7 | Modulation Depth |
| space-fx-detail-param7 | fx4.param7 | fx4.param11 | Width |

Both each control's default binding and its `bindings.fx4` map entry changed together. The other seven slot maps are unchanged. The eight visible Delay parameters are now Left, Right, Feedback, Mix, Low Cut, High Cut, Depth and Width. Labels, units, edit gestures and values remain supplied by the real selected algorithm. Omitted Delay controls remain available in Full FX editor.

The mapping was verified against the repository's pinned `sst-effects` revision, not inferred from displayed values:
[Delay.h at 5f2bd1a8fdea77f9f2c8acad3ed71e66fd7fa41d](https://github.com/surge-synthesizer/sst-effects/blob/5f2bd1a8fdea77f9f2c8acad3ed71e66fd7fa41d/include/sst/effects/Delay.h).
`DelayEffect.cpp::init_ctrltypes` separately confirms native Mix, modulation Depth and Width types. Native Delay Width uses the synth's actual units; the artwork's percentage is not substituted.

The adaptation note explains this priority and the continued native differences. No decorative Sync or Ping-Pong button was added.

## Geometry review

All 21 panels, 90 control IDs, 90 control rectangles, colors and typography settings remain unchanged. Existing main panel boundaries, oscillator knob centres and macro centres are already close to the supplied composition; a geometry shift was not justified by this capture.

Specific inspected native rectangles:

- Three oscillator panels: [258,158,307,256], [572,158,307,256], [884,158,304,256].
- Selected FX inset: [656,679,365,266].
- Six large FX control centres: x700, 757, 813, 870, 925, 978 (outer control bounds y727–822).
- Two lower FX control centres: x869 and950 (outer bounds y839–914).
- Four macro controls: x1039/1109/1179/1249, y674, size66×137, face diameter60.
- Four macro centres: x1072/1142/1212/1282.

## Shared renderer findings sent to coordinator

The most visible remaining material difference is in the shared Space knob painter, not the manifest rectangles. The native cream caps have a comparatively flat olive fill; the reference uses a dark perimeter and brighter cream/specular sectors. Native macro caps are smooth dark disks with a thin gold border; the reference uses a deep black inner cap, substantial bronze bevel and dark radial ticks. The reference macro headings are also more compact and bold.

The coordinator inspected the labelled crop and took ownership of these shared paint corrections. This assignment did not edit shared C++ files or claim that those corrections were already present in the inspected native image.

## Validation

- Strict validator: all ten workspaces pass; the two pre-existing intentional ClearSignal corner overlays remain documented warnings.
- `git diff --check`: pass.
- All125 declared panel/control/plot/wavetable/selection rectangles are within the1586×992 canvas.
- Before/after panel equality and control geometry/ID equality: pass.
- Other seven FX slot maps unchanged: pass.
- Manifest SHA-256: `1a65c9e66acfa83857f20e353fea41a59b6b3fdf087522d4e2b5a4c56e114774`.
- Machine-readable evidence: `analysis-only/w07_a41_validation.json`.
- Fresh compiled native capture of the modified manifest is pending the coordinator's integration workflow. This is source and visual review, not a post-change runtime claim.

## Remaining differences

Native oscillator shapes, parameter names and units, installed preset catalog, route contents, macro values, meters and filter responses remain derived from the real processor. The third envelope bank is the real LFO3 envelope; UNI is real polarity. These cannot be replaced with the reference's illustrative data or unsupported controls merely to reduce image differences.

Brass grain, inset shading, type weight, header branding and knob material still require the shared-renderer comparison. This review does not establish pixel identity.

Changes are unstaged. No commit or push was made by this assignment.
