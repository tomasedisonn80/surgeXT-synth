# Wave 7 / Assignment 37 — Obsidian final fidelity inspection

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and source evidence

- Owned source: `resources/data/skins/reference-01-obsidian.surge-skin/workspace.json`.
- Reference: `doc/reference-workspaces/references/01_Obsidian_Precision.png` (1586 × 992).
- Actual native capture inspected: `native-artifact-22b9c29/01-obsidian-precision.png` (1586 × 992), from main commit `22b9c299e6aa74093c9e96677cb32df5f2f74e74`.
- The capture contains the wave-5 material, signal-plot and FX power-button work. Subsequent shared renderer work must be checked in the next native capture.
- This inspection used the actual native image, not a static approximation or manifest-only audit.
- No commit, staging or push performed by this assignment.

## Implemented manifest correction

Only the stale FX explanatory adaptation was changed. It previously claimed the FX inset was always a category pictogram. The native capture and renderer now show a real Distortion waveshaper transfer when available. The note now accurately describes that live transfer, its `Shaper only` label, and the excluded EQ, feedback and output stages. Other algorithms retain labelled category pictograms.

All other parsed fields are identical before and after: 14 panels, 83 controls, selectors, IDs, bindings, rectangles, knob diameters, typography settings, palette, canvas and plot bounds. No geometry change was justified by the full-size evidence, and no shared source was edited.

## Concrete observed geometry and rendering differences

### Filter plots

- Panel rectangles: Filter 1 `[521,118,319,264]`; Filter 2 `[844,118,318,264]`.
- Plot frames: Filter 1 `[552,163,271,114]`; Filter 2 `[875,163,270,114]`.
- `drawFilter()` currently reduces each non-miniature plot frame by `(7,15)`. Thus the exact native inner grid rectangles are `[559,178,257,84]` and `[882,178,256,84]`.
- The reference first grid begins approximately at `(560,168)` and reaches approximately `(825,267)`. Its left dB readouts occupy roughly x533–550. The native grid's horizontal start is already close; its top is approximately 10px lower and its height approximately 16px shorter.
- The native image has no left dB captions. The reference has them. This is controlled by `drawFilter()` in `src/surge-xt/gui/reference/Workspace.cpp`, around lines 5418–5424 and 5479–5487 at inspection time.
- Simply adding Precision to the existing `dbReadouts` branch would trim another 21px from the plot's left edge, shifting its current x559 grid start to x580. It would also trigger the frequency-caption density condition around lines 5457–5458, hiding six captions in these compact cards. These side effects should be handled explicitly if the shared renderer is corrected.
- The real native preview domain is −42…18 dB. The reference's painted ±24 dB captions must not be substituted for this actual domain.
- Filter type menu text differs because the native control reports its actual filter algorithm. The reference's repeated `Multimode` label and inside-plot LP24/BP12 labels are not evidence to rename the real algorithm.
- The reference filter headers include power glyphs around `(540,135)` and `(863,135)`; the inspected native filter headers do not. A static glyph would not supply a real bypass action and was not added.
- Filter control centers and overall density are already close. Remaining rim/material differences do not justify speculative manifest diameter changes.

### FX detail

- Native and reference transfer plot outer frames align closely at `[557,811,151,102]`; the four detail knob face centers are also within roughly 2px by the full-size comparison.
- The actual native transfer is cyan, whereas the reference curve is orange. In the inspected shared renderer, the color is chosen around lines 6681–6682: clear has a special choice, compact uses accent, and the remaining styles use secondary. A Precision-specific accent selection would fix the evidenced color discrepancy without changing any curve data.
- The native transfer correctly reflects the current shaper and is explicitly labelled `Soft transfer` / `Shaper only`. The reference's unlabelled soft-clip curve is not a reason to replace the native response or remove its semantic qualification.
- The reference detail section has a power/title row, with power near `(566,785)` and the title beginning near `(589,785)`. The native image leaves this area blank. Any shared-renderer addition should use the selected slot's actual algorithm and bypass state; a fixed `DISTORTION` title would become false when the slot changes.
- Confirmed free dynamic-title rectangle: `[589,773,585,24]`, or relative to the FX panel `(x+47,y+103,w−61,24)`. It intersects zero manifest controls. The slot tabs end at y754; the rectangle ends at y797; nearest knob components begin at y803; the algorithm menu begins at y824. The full FX editor button occupies the separate y673–695 panel header. A 15.5px bold, actual selected-algorithm name fits this region.
- The native FX cards show each real slot's index, algorithm and working power state. Their equal-width layout differs from the reference's wider first selected card and denser single-line labels.
- Actual native algorithms differ in the screenshot (`Phaser`, `Reverb 2`, `Conditioner`, EQ, etc.). These are live patch contents and were not overwritten to imitate the reference's named slots.
- Four detail bindings expose real Drive, pre-EQ frequency, Feedback and Output in the showcased Distortion. Reference labels Tone/Mix cannot be imposed on different native parameters.

### Oscillator panel and controls

- OSC 1 outer panel `[9,119,502,176]`, waveform frame `[16,161,221,124]`, wavetable selector `[247,168,121,26]`.
- Full-size comparison confirms the compact knob centers and table-selector placement are already close, approximately 1–2px apart in the inspected image.
- The waveform uses actual wavetable frames and its shape naturally differs from the concept artwork. It must remain a real preview.
- Reference OCT/SEM/FINE controls are represented by actual native OCT/TUNE. A separate nonexistent fine/semitone parameter was not fabricated.
- Native algorithm parameter names and units differ from concept labels, including Morph, Formant, Detune and H Skew. These are truthful, working controls. Label substitution would misrepresent their behavior.
- Power glyphs in the oscillator headers already match the intended placement and drive real mute parameters.

## Validation

- `python3 scripts/reference-workspaces/validate.py --strict`: PASS, all ten workspaces. Obsidian: 14 panels / 83 controls, no Obsidian warnings.
- Existing ClearSignal plot/control overlap warnings remain in the global validator output; no related source was changed by this assignment.
- `git diff --check`: PASS across the current worktree.
- Explicit rectangle/canvas check: 115 manifest rectangles fit within the 1586 × 992 canvas.
- Semantic before/after check: deleting only `adaptations` from both parsed documents produces exactly equal objects. All native IDs/bindings and all geometry therefore remain unchanged.
- Diff: one insertion and one deletion in the owned manifest.
- Validation output saved to `analysis-only/w07_a37_validation.json`.
- This documentation-only edit does not change captured pixels. The root's next real native capture should assess shared paint corrections independently.

## Assessment

The inspected skin is not pixel-identical to the reference. Its outer composition and many control centers are close, while the concrete remaining shared paint differences include missing filter dB labels, filter plot vertical inset/density, cyan rather than orange Distortion transfer, and the blank selected-FX title row. Native algorithm names, values, waveform shapes, filter responses and modulation routes remain intentionally driven by the actual processor. This assignment corrected the inaccurate manifest explanation and delivered exact evidence to the root instead of introducing unmeasured geometry or fake native controls.
