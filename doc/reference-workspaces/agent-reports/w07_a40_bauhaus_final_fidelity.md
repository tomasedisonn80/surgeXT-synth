# Assignment 40 — Bauhaus final fidelity

> Curated historical assignment report. The implementation was subsequently included in passing native source `eccf282`. Its new captures still require visual comparison; the earlier image measurements below describe the stated prior source. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Evidence and scope

- Capture source: `22b9c299e6aa74093c9e96677cb32df5f2f74e74`, passing native run `43382049170`.
- Actual native baseline: `native-artifact-22b9c29/04-bauhaus-modular.png`.
- Reference: `doc/reference-workspaces/references/04_Bauhaus_Modular.png`.
- Inspected the complete 1586 × 992 canvas, then enlarged lower controls, filters, effect detail and output area.
- Sole modified repository file: `resources/data/skins/reference-04-bauhaus.surge-skin/workspace.json`.
- All 25 panels, 79 controls, rectangles, IDs, bindings, selections, accessibility labels and source values remain unchanged.
- Source edits remain unstaged. This assignment made no commit or push.

## Implemented color calibration

| Surface | Prior manifest | New manifest | Native/reference evidence |
| --- | --- | --- | --- |
| Red sections and accents | #D83C28 | #C93F2A | Oscillator body native median about [212,59,40], reference about [198,63,43]; reduces excess red brightness |
| Filter 1 and Filter 2 panel bodies | blue (#2E6080) | #265171 | Native lower body [46,95,126], reference [39–40,80–81,111] |
| Matrix and macro blue heading bars | blue (#2E6080) | #245476 | Native top/bottom approximately [52,100,131]/[47,95,126], reference approximately [40,86,119] |
| Yellow LFO 2 body | yellow (#FFD851) | #FBCC3A | Native top/bottom [253,215,86]/[250,211,80], reference [251,206,66]/[243,193,50] |

The four custom blue panel colors explicitly retain white title ink. The shared renderer otherwise recognizes the symbolic blue/red names for white headings, so omitting explicit ink would have darkened these titles.

Scene and LFO 1 blues remain unchanged because their native tones were already close. Global background remains #FCF0D9: independent empty left/mid gaps are reference [252,240,218] versus native [252,240,217]. The reference right gap is lighter, [253,243,227], so a single global lightening would worsen the already matching left area.

These are measurements from the prior native capture and conservative palette corrections. The corrected native pixels must be checked after the next build; no after-edit screenshot pass is claimed.

## Geometry verification

Color-support bounding boxes were extracted inside each expected panel region. They are useful corroboration of alignment, not a subpixel edge metric: borders and antialiasing affect threshold support.

| Region | Reference colored support x,y,w,h | Native colored support x,y,w,h |
| --- | --- | --- |
| Oscillator 1 | 254,341,239,246 | 254,341,239,247 |
| Oscillator 2 | 498,341,238,246 | 498,341,239,247 |
| Oscillator 3 | 741,341,239,246 | 742,341,239,247 |
| Filter 1 | 986,341,232,246 | 988,342,228,245 |
| Filter 2 | 1221,341,233,246 | 1224,342,229,245 |
| LFO 1 | 583,608,327,119 | 584,609,325,118 |
| LFO 2 | 915,607,329,120 | 916,608,328,120 |
| LFO 3 | 1250,608,331,119 | 1250,608,331,120 |
| Matrix heading | 6,747,523,40 | 7,748,522,36 |
| Macro heading | 1041,748,309,39 | 1042,748,307,36 |

Existing control centers and face diameters follow the measured layout. No new geometric movement was justified by the comparison.

## Shared-renderer findings sent to coordinator

1. Output material needs a stronger yellow upper band and warmer/lighter lower field. Exact panel is `output-panel`, kind `analyzer`, bounds [1357,747,224,239], symbolic yellow, no variant, actual plot [1369,789,204,125]. Empty vertical side strips show reference/native at y770 about [249,201,50]/[254,216,87], y850 [252,214,87]/[252,214,84], y900 [253,220,103]/[251,213,82], y940 [254,222,100]/[250,212,80], and y970 [253,219,92]/[250,211,79]. A material-only branch can change this without altering spectrum or slider state.
2. Filter algorithm menus are cream paper in native but dark blue in the reference. The two native controls are `filter1-type` [1105,375,99,23] and `filter2-type` [1341,375,99,23]. A Bauhaus filter-type-only dark inset treatment can preserve the native menu implementation.
3. The shared bevel gives some blue regions a thin light border/inset, while the reference colored support reaches slightly farther toward their edges. Do not shift controls to compensate for paint.

No shared renderer file was edited by this assignment.

## Genuine native differences retained

- Main ribbons and small waveforms derive from actual current oscillator sources.
- Three real OSC preview tabs replace the concept's unavailable MIX display.
- Native envelope labels describe the real two scene envelopes and LFO envelopes.
- Filters, routing, menu choices, frequency labels, knob values and the actual selected effect remain processor-driven.
- FX transfer uses the real selected Distortion shaper and accurately marks its preview scope.
- Current browser categories/patch metadata, modulation assignments, output spectrum and scene level are actual data.
- Missing hypothetical controls are not replaced with inert power glyphs or fabricated values.

The result is not claimed to be pixel-identical to the concept.

## Checks and files

- Strict validator: all ten workspaces pass; only the two already-documented ClearSignal corner overlays warn.
- `git diff --check`: pass.
- Before/after parsed controls equality: pass.
- Every panel property except the listed color/ink fields: unchanged.
- All top-level data except palette and those panel fields: unchanged.
- Edited manifest SHA-256: `762fbf396e7f8c3b3238245ea874a74b391a945d59c47bcda88f2e1180c42064`.
- Full comparison: `analysis-only/w07_a40_compare.jpg`.
- Labeled output crop: `analysis-only/w07_a40_output_compare.jpg`.
- Raw side-strip/background samples: `analysis-only/w07_a40_color_samples.json`.
- Geometry samples: `analysis-only/w07_a40_geometry.json`.
- Command evidence: `analysis-only/w07_a40_validation.json`.

Native post-change capture remains the integration gate.
