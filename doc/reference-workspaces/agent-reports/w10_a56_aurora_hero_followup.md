# Assignment 56 — Aurora hero corrective follow-up

> Curated historical assignment report. The source-qualified sequence below includes the original proposal, 4dbb414 review, bounded second correction and settled 453df569 native image review. Software studies remain explicitly non-native. Final native improvements and remaining source-shape/material differences are preserved. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Current result — final source 453df569

The final native source `453df569c977350c3a4da2caf2f0ab7cbd1a0e71` was reviewed from workflow `43400507719`. Aurora's actual luminous coverage now measures 37.005% against the reference's 39.997%; its bright vertical extent is y6–176 against y4–177. Header/controls remain clear. All five edited filter displays remain corrected, with final cropped pixels identical to their previously inspected passing counterparts.

The remaining real-source crest/topology and peripheral-material differences are explicit in the final section. **No pixel-identical or 1:1 verdict is claimed.** Earlier proposal, measurements and pending statuses below are retained as source-qualified history; the last section is the current verification result.


## Purpose and status

The a9d0d0a native Aurora hero still differed substantially from the retained reference. The first acceptance report identified that difference but did not resolve it. Following the user's instruction to continue toward the reference match, this follow-up prepares concrete source and material changes.

Root has applied both isolated patches. Shared source files were **not** edited directly by this assignment. Fresh native verification of source `4dbb414` is completed in the final section below; the historical proposal and validation are retained for traceability.

Both patches passed `git apply --check` together against root's dirty tree containing the Spectral-only renderer and the filter-preview refresh correction. They preserve those changes.

## Actual image evidence

The compared native baseline is `native-artifact-a9d0d0a/07-liquid-aurora.png`; the retained reference is `doc/reference-workspaces/references/07_Liquid_Aurora.png`. The exact hero rectangle is [266,310,969,178].

The source-size pair shows a broad, luminous, crossing cyan/violet/pink composition in the reference, versus a narrow, mostly parallel three-cycle band in a9d0d0a. Assignment 60 independently measured bright/saturated pixels using max(R,G,B) >=150 and max-min >=40:

| Image | Coverage | Mean RGB |
| --- | ---: | --- |
| Retained reference | 39.997% | [71.16,54.18,137.06] |
| a9d0d0a native hero | 8.537% | [28.47,19.96,60.76] |

Those values describe actual retained images. They are not image-fidelity scores and do not describe the new patch.

## Source explanation

The original actual Aurora table advances the dominant harmonic phase as the frame increases. That shifts corresponding waveform features left in phase space. The renderer simultaneously added a positive horizontal frame shear. The two shifts partly cancelled, leaving the visible rows tightly bundled.

The old phase/frame projection was:

- x = X + W × (0.05 + 0.90u + 0.08(f−0.5)).
- y = centerY + H × (0.22(f−0.5) − 0.40a).

Here u is source phase, f is source frame and a is the actual signed copied source sample. The old display also used relatively narrow 7-pixel glow and sparse 12-sample transverse seams. Its material color was primarily phase dependent, so crossing frames at the same phase could not separate into a cyan foreground and violet/magenta background.

## Render-only change

The renderer introduces `Visuals::WavetableView::auroraRibbons`. The Aurora hero alone selects it in the waveform panel branch of `Workspace.cpp`. The existing default views, the Live hero and Aurora's miniature renderer route are preserved; root's new `spectralTerrain` remains independently excluded from ribbon rendering.

The Aurora-only affine projection is:

- x = X + W × (0.025 + 0.95u − 0.04(f−0.5)).
- y = centerY + H × (0.16(f−0.5) − 0.43a).

The horizontal source extent is 0.005…0.995 of the supplied bounds. With the generated table's maximum absolute amplitude 0.92, its signed vertical geometry remains within approximately 0.0244…0.9756 of the supplied height before cosmetic glow. The display clip contains the glow. The renderer does not normalize the samples or take their absolute values.

Material improvements are also isolated to this mode:

- Frame-dependent cyan/magenta color separates actual crossing rows while the phase gradient retains the blue/cyan/violet composition.
- Four bounded source-following strokes, with widths 0.30H, 0.18H, 0.085H and 6 pixels, form the haze around the actual copied row paths. Their alphas are 0.008, 0.014, 0.030 and 0.050.
- Adjacent actual frame strips use alpha 0.04+0.035f.
- Transverse connections follow actual frame vertices every five phase samples instead of twelve, with alpha 0.20.
- Visible real row traces use alpha 0.34+0.46f.
- The selected-row highlight continues to follow the actual wavetable position and now receives that row's material color.

No reference bitmap, texture-derived waveform, clock animation, random display signal, fabricated ordinate, extra oscillator or additional audio measurement is introduced. Haze follows the same source paths as the mesh. It is explicitly a material treatment, not new signal content.

Affected renderer files:

- `src/surge-xt/gui/reference/Visuals.h`.
- `src/surge-xt/gui/reference/Visuals.cpp`.
- The Aurora-only hero selector in `src/surge-xt/gui/reference/Workspace.cpp`.

## Audible source change, kept separate

The signal patch changes only the `family == "aurora"` branch of `scripts/reference-workspaces/generate_showcase_wavetables.py`. The actual asset remains:

`resources/data/wavetables/Reference Showcases/Aurora Waves.wt`.

This changes the real audible source and therefore also legitimately changes its oscillator miniature and hero. It is not a display-only substitute. The capture fixture continues to load this ordinary native wavetable.

For phase p=2πx and frame travel t=(m−0.5)π, the new periodic window is:

`e = 0.16 + 0.84 sin²(p/2)`.

After the fixed phase offset `p -= 0.15π`, the unnormalized signal is:

```text
e × [
  0.62 sin(2p + 1.45t)
  + (0.23 + 0.12 cos(t)) sin(3p − 1.2t + 0.25)
  + 0.13 sin(5p + 0.55t)
  + 0.08 sin(p − 0.7t)
]
```

The small harmonic family and opposing phase travel produce broad crossing lobes with quieter ends. The window is exactly periodic; it does not come from traced image coordinates.

Root separately integrated a single bounded global normalization for the dunes and Aurora source families. The submitted Aurora branch uses that existing policy. DC removal is applied to the actual generated rows, followed by the shared source-table peak scale. No row or display is normalized independently by the renderer.

## Completed validation

The isolated candidate generator, using root's global-scale implementation, passed its existing generation/validation checks:

- 48 frames × 512 samples.
- Serialized size 98,316 bytes.
- All finite samples, DC checks and unique frames passed.
- Absolute peak 0.9200000166893005 after float serialization.
- Maximum analytic endpoint error across all 48 frames: 1.5265566588595902e−16.
- Maximum adjacent-frame sample step: 0.08036961662583053.

Both patches passed the read-only apply check. This assignment did not regenerate repository assets, commit or push; root handles exact regeneration, provenance and native verification.

## Diagnostic studies and their limits

Three original periodic families/projections were compared using a small Pillow software geometry study. Its images are clearly marked **SOFTWARE GEOMETRY STUDY / NOT NATIVE CAPTURE**. The chosen broad two-harmonic-dominant family produces visibly wider crossed lobes and a larger luminous area than the narrow a9 baseline.

The separate engineering study has approximately 27.03% bright/saturated coverage and mean RGB [58.42,45.07,96.63]. These numbers are only a geometry/material study: Pillow does not reproduce JUCE path stroking, gradients, antialiasing or the final native patch exactly. The actual new native PNG must be measured and inspected.

The study and patch are a concrete improvement proposal, not a 1:1 claim. Remaining uncertainty includes the final native brightness, exact lobe distribution, material softness and audible peak after the source change.

## Required final native review

Root will provide the new source SHA and native workflow/captures. Assignment 56 will then:

1. Inspect the fresh Aurora hero at source resolution against the retained reference and a9 baseline.
2. Confirm the source-aligned haze, frame color, projection and denser mesh improve the reference match without clipping panel titles or controls.
3. Re-measure actual native coverage/mean color in the exact hero rectangle.
4. Recheck the five edited filter plots after Assignment 59's immediate refresh correction, distinguishing curve changes from markers/labels.
5. State remaining visible differences candidly and retain exact image/source provenance.

## Fresh native verification — 4dbb414 (completed)

### Exact evidence

- Reviewed source: `4dbb414370c94932670e1b58cf6f5caecbe4c6f4`.
- Native workflow: https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43398309849
- Native job `131794574573`: independently checked completed/success, including native controls, twenty captures and audio-display stress.
- Gallery job `131796480386`: independently checked completed/success.
- Actual native capture directory: `native-artifact-4dbb414`.
- Capture evidence independently checked: exact source revision, source clean, complete, native JUCE component snapshot method.
- Logs independently checked: 42 behavior test cases / 10,316,058 assertions, and the twenty-image capture case / 24,100,990 assertions. Build/test success is not used as proof of reference fidelity.
- Independent measurements: `analysis-only/w10_a56_verified_4dbb414.json`.

### Aurora result

The source-size reference/a9/new-native comparison was actually viewed:
the temporary diagnostic image.

The new native hero has visibly broader crossing lobes, stronger blue/cyan/magenta light, more visible inter-frame mesh and a more substantial source-following haze. It corrects the old a9 appearance of a thin, nearly parallel band. No reference bitmap or separate display signal is involved.

| Metric in exact [266,310,969,178] hero | Retained reference | Old native a9d0d0a | Fresh native 4dbb414 |
| --- | ---: | ---: | ---: |
| Bright/saturated pixel coverage | 39.997% | 8.537% | 25.674% |
| Mean red | 71.157 | 28.470 | 43.915 |
| Mean green | 54.182 | 19.957 | 31.203 |
| Mean blue | 137.065 | 60.755 | 95.043 |
| Bright horizontal extent, crop-local | 9–957 | 29–949 | 13–952 |
| Bright vertical extent, crop-local | 4–177 | 17–166 | 19–150 |

Coverage uses the identical simple criterion max(R,G,B) >=150 and max-min >=40 on original PNGs. It measures luminous area, not pixel similarity.

**The result is a substantive improvement, but the hero remains visibly different from the reference.** The current colored envelope is less tall, less luminous and distributed differently. The retained reference's large cyan and magenta crests are not at the same phase positions as the new actual source family. Its soft peripheral haze is also richer. These are appearance differences, not a reason to substitute frozen artwork.

Concrete remaining visual work, if another matching iteration is undertaken, is to expand the safe vertical source projection and tune source-following material brightness/softness; the real periodic family would need further phase/harmonic tuning for the reference's precise crest distribution. Those changes were **not** applied or claimed in this final source review. No new control overlap, title collision or functional blocker was found in the inspected fresh hero.

The changed audible Aurora fixture remains below full scale in the actual native audio evidence: baseline peak 0.584684967994690, edited peak 0.296577066183090, post-reset peak 3.875818332943481e−9.

### Five edited filter plots — issue resolved

All five baseline/edited crop pairs were actually viewed at source size. The miniature Aurora and Live plots, previously unchanged, now visibly move with the real cutoff. Tokyo, Clear and Flow curves move coherently with their markers and numeric values to 115.14 Hz. Filter-2 curves remain visually stable in the inspected paired controls. Captions, axis labels, legends and values remain readable without new collisions.

Independent raw-PNG pixel counts exactly match the new capture evidence:

| Skin | Old a9 plot changes | Fresh 4dbb414 plot changes | Native response maximum change |
| --- | ---: | ---: | ---: |
| Tokyo | 156 pixels | 5,158 pixels | 80.068 dB |
| Aurora | 0 pixels | 3,028 pixels | 66.023 dB |
| Clear | 310 pixels | 24,419 pixels | 92.888 dB |
| Flow | 153 pixels | 8,115 pixels | 100.952 dB |
| Live | 0 pixels | 1,735 pixels | 80.378 dB |

The dB changes are differences between native response samples, including the deep stopband, not a statement about audible output level. The new plot changes cover the curves/fills; they are no longer limited to moved cutoff points.

Viewed comparison boards:


**The stale edited-response capture issue identified by Assignment 56 is resolved in this exact source.** This conclusion is supported by independent image-region measurements, actual response sample evidence, direct visual inspection and the focused native regression; it does not depend only on whole-workspace changed-pixel totals.

### Final assignment verdict

The new source passes this assignment's functional display follow-up and inspected paint/legibility checks. Aurora's corrected source/projection/material is visibly closer than a9. The report does **not** give a 1:1 or pixel-identical visual verdict for Aurora or the other five-skin adaptations. The specific remaining appearance differences above are retained as open fidelity work.

No source edits were made after root integrated the isolated patches, and no source edits, commits or pushes were performed during this fresh-capture review.

## Bounded second Aurora correction — native gate pending

After the actual 4dbb414 comparison, root authorized one further renderer-only correction to the measured height/luminosity gap. The prepared patch is:

- `analysis-only/w10_a56_aurora_height_light.patch`.
- 6,734 bytes.
- SHA-256 `f6c8b119caa981c64e105b2c77a920e3cd6674355b1120d0534c200bd5202cbd`.
- Based on the reviewed 4dbb414 source, with no source asset or fixture changes.
- `git apply --check` passed; this assignment did not apply it to the shared repository.

### Actual source geometry, measured before patching

The source-table values were decoded from the real serialized `Aurora Waves.wt` and sampled exactly as the native preview does: 48 real rows, 256 points, linear interpolation over the original 512 samples. These are projected actual sample coordinates, not image-traced coordinates.

| Camera | Source vertex y extent in original hero | Allowed plot y |
| --- | ---: | ---: |
| 4dbb414: inset8, height.43, depth.16, translation0 | 19.1708…149.2325 | 8…170 |
| Proposed: inset1, height.52, depth.16, translation+.04H | 7.1872…173.7320 | 1…177 |

The measured current geometry closely matches the observed native bright extent 19…150, supporting the specific correction. The new actual source vertices remain inside the plot. The low-alpha source-following halo can fill the remaining border toward the reference's 4…177 luminous extent; it remains clipped to the true hero plot, clear of controls and headers.

### Isolated renderer changes

The Aurora hero receives a one-pixel drawing inset instead of the common eight-pixel inset. Other waveform styles keep eight pixels. Its existing horizontal camera is retained. Its signed vertical projection becomes:

`centerY + H × (0.04 + 0.16(f−0.5) − 0.52a)`.

The source sample a is unchanged. This is a fixed affine projection, not per-frame normalization or display-specific amplitude synthesis.

The same source paths receive a moderate material correction:

- Aurora-only input saturation multiplier 1.0 instead of the shared ribbon multiplier 1.15.
- The crossing-row middle color retains natural RGB interpolation instead of forcing its saturation back up, with 12% white mixed into the material.
- Small white mixes soften blue/violet phase stops and increase the pink lightness.
- Four halo alphas change from [0.008,0.014,0.030,0.050] to [0.012,0.021,0.045,0.070].
- Adjacent-strip alpha becomes 0.065+0.050f.
- Transverse seam alpha becomes 0.24.
- Actual row-trace alpha becomes 0.40+0.45f.

The source family, samples, selected morph position, phase positions, audio engine, fixture, filters and all nine other visual modes remain unchanged by this patch. Aurora's miniature renderer also retains its previous route. The change therefore addresses height/light, while the already documented difference in precise crest phase distribution remains an actual source-family difference.

The next native captures must establish the observed coverage, color means and edge behavior. The source-coordinate check is not a substitute for that image review, and no new native or 1:1 verdict is claimed here.

## Final native-source review — 453df569 (completed)

### Verified evidence

The reviewed final source is `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, native workflow https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719, job `131801876413`. The native job was independently checked as successful. Root supplied the successful artifact in `native-artifact-453df569`.

The prepared read-only verifier checked the actual capture evidence against this exact source: source clean, complete and the stated native JUCE component snapshot method. The run reports 42 behavior cases / 10,316,058 assertions and the twenty-image capture case / 24,100,990 assertions, with normal and TSAN audio-tap gates passing.

Final measurement evidence:

- `analysis-only/w10_a56_verified_453df56.json`.
- `analysis-only/w10_a56_final_filter_plot_identity.json`.

Actual source-size images viewed during this final review:


These are labeled comparison crops of actual PNGs. No software study or generated illustration was substituted for the native review.

### Height and material correction achieved

| Exact hero-region metric | Retained reference | Previous 4dbb414 native | Final 453df569 native |
| --- | ---: | ---: | ---: |
| Bright/saturated coverage | 39.997% | 25.674% | 37.005% |
| Mean red | 71.157 | 43.915 | 67.773 |
| Mean green | 54.182 | 31.203 | 54.549 |
| Mean blue | 137.065 | 95.043 | 117.357 |
| Bright horizontal extent, crop-local | 9–957 | 13–952 | 6–960 |
| Bright vertical extent, crop-local | 4–177 | 19–150 | 6–176 |

The actual final image confirms the predicted larger vertical envelope. Source geometry remains at y7.1872…173.7320 within the permitted y1…177 drawing bounds. The observed bright envelope y6…176 includes the source-aligned halo and closely approaches the retained reference's y4…177.

The pale cyan/violet/magenta material is visibly brighter and less harshly saturated than 4dbb414. The same actual rows, adjacent strips and transverse connections create that light; the source table and its sample amplitudes were not changed in this correction.

### Edge and surrounding-control inspection

The final context crop shows the Waveform Playground heading, OSC1/OSC2/OSC3 tabs, Wavetable selector, Morph control/value, V Skew, Saturate, Formant and H Skew controls clear of the waveform. No new title/control collision or source-core clipping was found.

An independent raw-pixel comparison within the surrounding panel rectangle [253,267,1249,558], excluding the true hero rectangle [266,310,1235,488], found **zero changed pixels** between 4dbb414 and 453df569. The correction stays inside its intended hero plot.

The stronger low-alpha halo terminates within that plot. Its open-path ends still show some stepped layering at the left edge; this was already present in the preceding source and is a remaining material difference from the reference's softer peripheral wisps. It is not a covered control or clipped source crest.

### Edited filter verification repeated on this exact source

All five final baseline and edited filter plots were measured again directly from the new PNGs. Every one of the ten cropped plots is pixel-for-pixel identical to the corresponding 4dbb414 plot already inspected at source size. Raw pixel hashes and identity results are saved in `analysis-only/w10_a56_final_filter_plot_identity.json`.

The exact final-run baseline-to-edited changes remain:

| Skin | Changed native plot pixels | Maximum native response change |
| --- | ---: | ---: |
| Tokyo | 5,158 | 80.068 dB |
| Aurora | 3,028 | 66.023 dB |
| Clear | 24,419 | 92.888 dB |
| Flow | 8,115 | 100.952 dB |
| Live | 1,735 | 80.378 dB |

Thus the previously resolved response-refresh issue stays resolved, and all five plots retain their inspected legible captions/axes/legends and coherent cutoff movement. This conclusion uses exact final-source images and sample evidence, not merely an earlier test pass.

### Final visual verdict and remaining differences

**The final source passes this assignment's inspected paint/legibility checks and functional display follow-up.** The bounded correction successfully addresses the concrete height and material deficit observed in 4dbb414. The original a9 hero's thin parallel-band appearance has been replaced with a much broader, luminous, source-driven crossing mesh.

**The final hero is still not a pixel-identical copy.** Its real periodic source family has different crest phase positions and crossing topology from the reference illustration. The reference also has finer cross-weave variation, more diffuse peripheral wisps and a higher mean blue level. Native controls and displays elsewhere in skins 06–10 retain the documented real-engine adaptations. These remaining differences are recorded explicitly; the metrics above are not presented as a reference-fidelity percentage.

No new functional or paint regression was found that calls for another source correction within this review. No additional source edits, asset edits, commits or pushes were made during final verification.
