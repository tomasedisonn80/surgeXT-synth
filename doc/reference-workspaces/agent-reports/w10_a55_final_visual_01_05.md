# Assignment 55 — final visual review of skins 01–05

> Curated historical assignment report. This initial visual verdict concerns a9d0d0a. The measured Spectral cap, Bauhaus heading and Space caption corrections were subsequently compiled and reviewed on 453df569 by assignment 59. The later reports preserve the remaining source/material and one-pixel caption differences. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Scope and evidence

Reviewed the genuine native component captures from source `a9d0d0a838632daf178255561def4622cd208258`, native workflow [43393400045](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43393400045), against the five retained reference PNGs. The workflow's native job was independently checked as completed/success. The source compiled and passed 41 behavior cases / 10,299,242 assertions, the ten-pair capture test / 24,095,770 assertions, and the normal/ThreadSanitizer audio-display checks.

All five baseline and edited PNGs decode at 1586 × 992. Capture evidence identifies the exact source, clean source state and a complete ten-skin capture set. The attached validation JSON records the first five PNG hashes and the evidence's resolved-control counts and edited-pixel counts.

Earlier `eccf282` images were used for full-canvas composition comparison while this candidate built. The fresh candidate was then inspected in original-size targeted crops: Obsidian FX detail, Ivory selected Chorus detail, Spectral mixer/filter controls, Bauhaus filter menus/output, and Space oscillator/macro controls. Temporary JPEG crops are analysis aids; the original PNGs remain unmodified.

## Acceptance result

No new clipping, panel overlap or missing-control regression was found in the inspected first-five views. The broad panel composition is close to the retained references, and the recent corrections are visible. This supports the implemented native layouts in the inspected states; it does not establish pixel identity or certify every possible algorithm/state combination.

These images do **not** meet a literal 1:1 pixel-identical claim. The largest remaining difference is Spectral's source surface, whose actual waveform composition is visibly simpler than the reference's dense mountain-like mesh. Several other differences concern native algorithms, units, values, scene/routing semantics and live signals. There are also remaining material and typography differences that are not explained by state alone.

## Per-skin findings

| Skin | Confirmed in candidate | Remaining visible differences |
| --- | --- | --- |
| 01 Obsidian Precision | Selected FX header names the actual Distortion algorithm; the real shaper transfer is orange, inside its plot; controls and captions remain separated. | Reference body power glyph is absent; real FX power is available in the slot tabs. The native algorithm menu and Drive/Frequency/Feedback/Gain controls differ from the concept's Soft Clip and Drive/Tone/Mix/Output arrangement. Source waveforms, filter response and spectrum remain native state. Dial materials and lettering are not pixel-identical. |
| 02 Ivory Atelier | Selected Chorus detail now reads Rate, Depth, Mix and Width. Serif section and macro headings are stronger. The four dials and algorithm menu fit the inset without overlap. | Native Chorus algorithm, units and values differ from the concept's Ensemble example. Detail body power glyph is absent while slot power controls work. Paper decoration, shadows, dial finish and font metrics remain visibly different. |
| 03 Spectral Observatory | Smaller filter rings and aligned columns fit the intended geometry. Noise/Ring captions are above the dials. Mixer rails are thin cyan, with separate purple caps. | Hero waveform structure remains substantially simpler than the reference. Cap interiors are dark/hollow rather than the reference's brighter filled purple/white cores. Native names and actual filter/envelope/spectrum state remain different. |
| 04 Bauhaus Modular | Filter algorithm menus are now dark blue with pale text; red/blue sections and the output yellow coating are closer to the reference. No new overlap was found in the sampled controls. | Output heading is smaller/lighter and slightly left/high relative to the reference. Native full algorithm names differ from abbreviations. Native Scene level replaces the concept's A/B Scene Mix operation. Browser contents, routing and output plots remain real native state. |
| 05 Space-Age 1977 | Small-dial metal reflections are now clear; macro caps are darker after the reduced broad highlight. Delay exposes native Mix/Depth/Width in the revised positions. | Macro captions remain wider, shorter and lighter than the reference; the sculpted bezel is still flatter. Native oscillator parameter names/units and source waveforms differ. Native Delay operations are preserved rather than drawing unsupported switches. |

## Measured cosmetic shortlist delivered to the coordinator

Measurements below use original PNG pixels, not the lossy comparison JPEGs.

### Spectral cap interior

The first reference fader cap is approximately centered at `(80,639)`; the candidate's actual handle is approximately at `(80,633)`. The vertical position is a separate actual-value/range consideration and is not proposed for a paint-only shift.

- Reference interior samples three pixels above the middle stripe: approximately RGB `(85–92,79–87,176–181)`.
- Reference interior samples three pixels below the stripe: approximately RGB `(167–175,93–100,243–247)`.
- Native corresponding interior samples: RGB `(48,34,69)` above and `(37,22,58)` below.

The dark inset is confirmed by both raster pixels and the shared painter's `theme.display.withAlpha(.78f)` fill. A purple-filled inset with the existing white horizontal stripe is a justified paint correction. Preserve the cap footprint, real handle position and the newly corrected pointer mapping.

### Bauhaus output heading

Within `[1368,752,1570,782]`, pixels whose RGB maximum is below 85 produce:

| Image | Dark-pixel bounding box | Dark pixels |
| --- | --- | ---: |
| Reference | `[1374,759,1549,772]` | 1,012 |
| Native | `[1370,757,1532,768]` | 579 |

The native manifest uses `fontSize: 16` and the default title inset of 12. An output-only inset of 16 and a modest title-size increase are supported by this comparison. Additional weight depends on the chosen font treatment. Panel and plot rectangles should remain fixed.

### Space macro caption

For the first caption, the equivalent dark-pixel measurement gives reference `[1049,680,1092,689]` / 160 pixels and native `[1048,680,1096,688]` / 117 pixels. The reference is narrower, taller and heavier. This is a typography treatment; moving the aligned component bounds is not justified by this evidence.

## State and implementation boundaries

Inspected rendering uses actual source frames and native curves, values and meters. The capture fixtures load real wavetable data through the engine; source previews are not raster copies of reference signals. The differences in depicted sound are therefore meaningful and must remain explicit. Working FX power controls in slot tabs are a truthful adaptation; adding inert duplicate body glyphs would not satisfy the control contract.

Actual frame data, algorithm names, parameter units, browser content and unsupported reference scene/routing concepts cannot be treated as interchangeable decorative pixels. Source-level functionality and visual fidelity are separate acceptance questions.

## Handoff

The coordinator accepted three scoped follow-up corrections: a purple-filled Spectral cap inset retaining the white stripe; Bauhaus output title font size 17.5, inset 16 and title y-offset 6; and Space macro caption size 12 with horizontal scale .83. Real text, panel/control bounds and native state remain unchanged. These corrections are pending a new capture at this report's handoff.

This report covers `a9d0d0a`; it does not claim that those subsequent corrections were captured or visually accepted. Fresh images must verify later source changes.

Read-only review only: no source, manifest, workflow, reference PNG or native PNG was modified, staged, committed or pushed. Temporary comparison images, this report and its validation JSON were written under `analysis-only`.
