# Wave 10 assignment 59 — supplemental Spectral source-waveform review

> Curated historical assignment report. This supplement preserves the original audible recipe, exact-byte publication proof and native reviews at 4dbb414 and 453df569. Only the temporary study inventory is omitted; later proof sections are retained. The real source geometry remains different from the reference's denser terrain. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Recommendation and integration status

Use an original periodic raised-cosine pulse family for the audible **Spectral Bloom** wavetable, with a single gain across all 256 frames. Keep the renderer's height mapping affine in the signed source sample.

The root agent reports integrating the exact recipe below into the generator and preserving global normalization only for the dunes/Aurora families. Root also reports a Spectral-specific signed camera, horizontal violet-to-cyan material, a clearer lower sample-range plane and stronger actual-position marker contrast. This assignment did not modify repository files during the supplemental review. Native compilation, audio behavior and final visual acceptance remain pending.

## What the actual images and source show

The reviewed reference and actual a9 native hero share rectangle `[455,146,692,236]`. The reference has several narrow peaks and ridges whose height and phase vary across depth. The a9 native view is a broad smooth three-wave sheet.

The generator's former `dunes` family was dominated by harmonic 3, with weaker harmonics 4, 1 and 7. Its actual checked-in 256 × 2048 float32 samples confirm the smooth topology. Changing camera or material alone cannot create the missing narrow ridges without transforming or inventing data.

The native preview is an owned 48 × 256 interpolation of the actual loaded source frames. It is a pre-warp source-table view, not output audio, a frequency spectrum, or a live voice-modulated waveform. The proposed recipe supplies richer playable source topology to that existing path.

## Concrete analytic recipe

Let `x` be phase in [0,1), `m` the frame coordinate in [0,1], and `tau = 2*pi`.

For six components use:

| j | Centre c | Integer power p | Height h | Phase drift d | Depth rate r | Phase phi, radians |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | .075 | 40 | .80 | .035 | .70 | .2 |
| 1 | .225 | 72 | 1.03 | .055 | .94 | 2.3 |
| 2 | .395 | 108 | 1.20 | .032 | 1.13 | .9 |
| 3 | .565 | 56 | 1.00 | .045 | .81 | 3.2 |
| 4 | .740 | 96 | .88 | .030 | 1.27 | 4.4 |
| 5 | .900 | 48 | .65 | .040 | .59 | 1.5 |

Each component is:

```python
amplitude = h * (.28 + .72 * (.5 + .5 * cos(tau * r * m + phi)))
centre = c + d * sin(tau * (.55 + .17 * j) * m + phi)
pulse = (.5 + .5 * cos(tau * (x - centre))) ** p
mean = comb(2 * p, p) / 4 ** p
component = amplitude * (pulse - mean)
```

Sum the six components and add:

```python
.055 * sin(tau * x + .6 * sin(pi * m))
+ .025 * sin(5 * tau * x - 1.1 * pi * m)
```

Subtract residual numerical DC per generated frame, then apply one common gain so the maximum absolute source-table sample is .92. The powers are integers, so the phase signal has finite harmonic degree at most 108. Its periodicity is analytic; it does not depend on matching an arbitrary last stored sample to the first.

The parameter choices are original analytic synthesis choices. No reference-image pixel, luminance, height map, texture, screenshot sample, random generator, or elapsed time enters the signal. The result is ordinary audible native wavetable data. The narrow positive lobes and shallow negative troughs already provide the desired terrain character without rectifying the display.

## Actual float32 study results

The candidate was generated outside the repository as a standard native float32 `.wt` file, then read back from its bytes before measurement. The diagnostic preview follows the exact existing 48-frame/256-phase interpolation and uses signed amplitudes.

| Property | Existing a9 actual table | Proposed actual float32 table |
|---|---:|---:|
| Frames × samples | 256 × 2048 | 256 × 2048 |
| Distinct frames | 256 | 256 |
| Global peak magnitude | .9200000167 | .9200000167 |
| Minimum per-frame peak | .9200000167 | .4664308727 |
| Minimum source sample | −.9200000167 | −.3136177063 |
| Maximum source sample | +.9200000167 | +.9200000167 |
| Largest absolute frame DC | 7.12e−10 | 3.98e−10 |
| Positive local peaks above .1 across frames | 3–4 | 3–6 |
| Per-frame RMS range | .4290–.5104 | .2008–.3016 |

The proposed analytic endpoint maximum error was **9.99e−16**. The common normalization gain was **.9514168717455418**. All 48 interpolated display rows remain distinct. Preview downsampling preserves the proposed source's 3–6 stronger positive-peak range. At the nearest native frame to position .723, three stronger peaks remain, while other ridges appear across depth.

The lower RMS and richer harmonic content are real audible consequences of the new fixture. A bounded .92 source peak does not certify downstream effects or final output headroom; the existing native capture/audio gates must still run. No output gain compensation was introduced by this study.

## Renderer honesty

A fixed camera that maps signed sample amplitude linearly to height is an honest source-waveform presentation. Camera perspective, depth scaling, hue, glow, translucency and line weight can change the presentation while preserving the sample geometry.

The root-reported Spectral camera uses:

```text
heightFraction = .31 + .29*frame - amplitude*(.21 + .13*frame)
```

This remains affine in the signed amplitude for each frame. Its floor is the same projection evaluated at amplitude −1. The floor is therefore a defined lower sample-range plane, not a spectrum or an invented output measurement.

Applying `abs(amplitude)` would fold negative samples above the plane. Applying `max(0, amplitude)` would discard negative samples. Either is a legitimate *magnitude* visualization only if named and presented that way; neither should silently replace the signed source waveform. Neither is needed for this recipe.

Likewise, per-frame display normalization would erase the real frame-height differences that the global source-table gain now preserves. The selected row and marker should continue following the actual native wavetable position.

## Relevant native coverage

There was no direct `WavetableView` enum rendering loop in the existing native tests. The relevant case is:

`Wavetable terrain snapshots own distinct real source frames without editing the synth`

It already validates actual samples, frame distinctness, repeatability, position, immutable snapshots, mutex contention and native table replacement. Root can extend this case with all renderer views, including the two new dedicated views, checking changed selected-row imagery and preservation of source samples, parameters and RNG. The all-ten native capture case then exercises their real workspace integration.

## Study scope

The optional engineering projections used actual table samples and were labelled as non-native diagnostics, not fidelity approvals. The candidate wavetable was not loaded into a running native engine by this supplemental study. Temporary study source, assets and image inventories are omitted. The later exact-byte publication proof and native confirmations below are retained.

## Exact-byte publication proof

The final Spectral Bloom file was independently regenerated in functions JavaScript from the exact repository recipe, using a compensated DC sum matching Python 3.14, the same global gain, and little-endian float32 serialization. The generated file is exactly 2,097,164 bytes (2,796,220 base64 characters). GitHub returned blob SHA dd8c02a17005117347cdac9a7c11c9edce385027, exactly equal to the root agent's expected blob SHA. No commit or ref was created by this assignment. Root independently verified tree 3251fbdd3365bc7473160f9fbbb0cc5f73f53843 and published final source 4dbb414370c94932670e1b58cf6f5caecbe4c6f4 to main. Native run 43398309849 and platform run 43398310063 were pending at this record; this byte-identity result does not substitute for their runtime gates.

## Final native image confirmation — 4dbb414

Verified native source: `4dbb414370c94932670e1b58cf6f5caecbe4c6f4`, [native run 43398309849](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43398309849), native job `131794574573`. Original captures are in `native-artifact-4dbb414`. The root verified artifact ZIP `14024815233` with SHA-256 `07c6824546bd9b645f6b67025ed6a9e7f4d6f7bebcc5f47554a3b8ef714de3ca`.

Fresh native behavior and capture logs passed: 42 cases / 10,316,058 assertions and all twenty images / 24,100,990 capture assertions, with passing normal/ThreadSanitizer AudioTap checks. The final native hero was inspected at the original 692 × 236 reference crop.

The implemented playable source family is visible as narrower irregular peaks, real frame-height variation, richer violet-to-cyan material and a defined lower sample-range plane. The actual position marker remains visible. The prior broad three-wave sheet has been replaced without rectifying signed amplitudes or loading image data.

The reference still has denser, craggier terrain and finer irregular surface texture; the native view remains a cleaner wire-ribbon surface with a more explicit floor. This is a verified native improvement, not a 1:1 image-identity result.

Detailed source-size image findings, exact remaining typography/cap differences and all-ten filter liveness evidence are recorded in [w10 a59 final native visual review](w10_a59_final_native_visual_review.md). The untouched comparison PNG is `analysis-only/a59_final_visual_4dbb414/spectral-hero-comparison.png`.

## Later native source confirmation — 453df569

Latest verified source: `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, [native run 43400507719](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719), job `131801876413`. Fresh captures: `native-artifact-453df569`. Root independently verified artifact `14024468912`, ZIP 11,019,495 bytes, SHA-256 `4054102ba04157671f0d3464616fdd0575514f657a8c22b089f4eeabd74b7246`.

The 692×236 Spectral hero crop has zero changed pixels compared with the verified 4dbb414 native hero. The analytic source-waveform findings and remaining reference-terrain differences therefore remain applicable. The later source's native gates passed 42 cases / 10,316,058 assertions, 24,100,990 capture assertions, and both normal/ThreadSanitizer AudioTap checks.

The separately corrected Spectral cap gradient now measures upper RGB(89,83,180) and lower RGB(173,99,245), versus reference (92,83,181)/(174,100,246). Exact final image findings are appended to [w10 a59 final native visual review](w10_a59_final_native_visual_review.md); evidence and source-size boards are in `analysis-only/a59_final_visual_453df569/`.
