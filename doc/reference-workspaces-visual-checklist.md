# Native workspace visual evidence

This checklist separates a working synthesizer interface from visual resemblance
to the ten supplied concept images. No screenshot or metric alone establishes
both. A successful package validator is not a visual approval.

## Capture and identify the build

1. Build the actual modified synthesizer and run the complete
   `surge-xt-tests '[reference-workspaces]'` suite on a display-capable runner.
2. Set `SURGE_REFERENCE_SCREENSHOTS` to the artifact directory. Retain its
   `capture-evidence.json`, build provenance, native test log, and all ten
   baseline PNGs plus their `-edited.png` counterparts.
3. Check the workflow conclusion independently. A comparison script cannot infer
   a passing test suite merely because screenshots or metadata exist.
4. Match the complete `sourceRevisionAtConfigure` to the intended build commit.
   Inspect `sourceDirtyAtConfigure`: when true, the commit is not a complete
   statement of the source tree that was compiled. Preserve the corresponding
   build log and identify the extra files/changes.
5. Captures are native JUCE component snapshots attached to the real processor.
   They are not OS desktop screenshots, DAW screenshots, or an assertion of
   successful installation, accessibility certification, or host compatibility.
6. Keep the reported patch, scene, audio fixture, sample rate, and capture scale.
   A waveform or meter must be generated from its documented real source, not
   copied from concept art to make a comparison look closer.

## Generate comparison evidence

From the repository root, with Pillow installed (NumPy is optional):

```bash
python3 scripts/reference-workspaces/compare_renders.py \
  --references /absolute/path/to/surge-xt-ui-reference-pack \
  --captures /absolute/path/to/native-artifact \
  --output /absolute/path/to/new-comparison-directory \
  --expect-commit FULL_40_CHARACTER_BUILD_COMMIT_SHA
```

The output directory must not already exist or overlap either input directory.
The script validates all ten fixed design-to-capture mappings, exact 1586 × 992
dimensions, opaque single-frame PNGs, capture-method/source metadata, and the
recorded versus recomputed changed-pixel counts for the control-edit images.
It rejects incomplete evidence, mismatched IDs, traversal filenames, or input
symlinks escaping their declared input directory. Input files are not modified.

Outputs:

- `native-contact-sheet.png`: only actual native captures, labeled by design.
- `comparison-contact-sheet.png`: concept references on the left and native
  captures on the right. Only contact-sheet thumbnails are resized.
- Ten `*-comparison.png` files: full-resolution side-by-side images with explicit
  **REFERENCE CONCEPT** and **NATIVE JUCE RENDER** labels.
- Ten `*-difference.png` files: unamplified absolute RGB differences. These are
  diagnostic visualizations, not screenshots of an implemented interface.
- `comparison-report.json`: full build SHA, dirty flag, original input hashes,
  capture settings, metrics, warnings, and exact artifact filenames.
- `README.txt`: interpretation and provenance limitations.

Run `python3 scripts/reference-workspaces/compare_renders.py --self-test` to test
the tool using synthetic fixtures. That result does **not** constitute a real
comparison or evidence that the native UI has rendered successfully.

## Interpret differences honestly

The script does not crop, align, stretch, recolor, sharpen, substitute, or overlay
the reference onto a native capture. Its MAE and RMSE use the same-coordinate
8-bit RGB values encoded in the PNG, not linear-light color or a perceptual model.
Changed-pixel fraction is especially sensitive to antialiasing. None is a
percentage of fidelity, a pass threshold, or a measure of functional correctness.

Review these confounders before interpreting a metric:

- **State and audio:** the concept may depict a different preset, oscillator
  algorithm, cutoff, envelope, routing, modulation assignment, or playing note.
  A truthful live display is allowed to differ from illustrated fictional data.
- **Typography:** default platform fonts, font availability, font metrics,
  glyph hinting, kerning, and fractional scaling affect many edge pixels. Check
  actual label legibility, hierarchy, baselines, truncation, and units rather
  than normalizing away differences before comparison.
- **Materials and canvas:** illustrated chrome, drop shadows, metal texture,
  glass, glow, empty background, and the application window frame can dominate
  average error without determining whether the controls work.
- **Scale:** compare the baseline logical canvas at 100%; independently inspect
  actual 125% and 150% renders for clipped text and hit targets. A resized contact
  sheet is not proof that a high-DPI native render was correct.
- **Engine adaptations:** review each manifest's `adaptations`. Do not call an
  unsupported feature complete because its label or diagram is present. Native
  fallbacks and previews should be clearly identified.
- **Provenance:** file hashes make later alteration detectable only relative to
  the retained report. Locally supplied JSON is not a cryptographic attestation
  that the pixels came from the claimed commit. Verify the workflow/artifact
  separately. Pixel identity alone also cannot prove independent native origin.

## Per-design inspection

For every design, record the inspected build SHA, screenshot filename, concrete
differences, severity, and the fix or intentional adaptation. Do not substitute a
single numerical grade for this record.

| Area | What must be checked in the actual native view |
|---|---|
| Structure | Header, columns, panel edges, padding, spacing, separators, and reference-specific hierarchy |
| Controls | Knob/fader size, indicator direction, labels above/beside, actual values and units, menu widths |
| Text | No clipping or collisions; readable contrast; selected, disabled, focused, and hover states |
| Curves | Genuine source, correct selected algorithm, scale/caption, no invented modulation or activity |
| Input | Mouse drag, fine adjustment, typed entry, keyboard focus, scene changes, preset loading |
| Fallback | Full native editor reachable; advanced functions remain available; return preserves state |
| Scaling | Native 100%, 125%, and 150% views retain layout, text, and target access |
| Lifecycle | Theme/reload/editor close leaves no stuck gesture, held note, modal dialog, or changed sound |

Conclude separately: **native behavior verified**, **visual differences reviewed**,
and **remaining mismatches**. A 1:1 claim requires evidence for that claim, not
only a green build or a visually similar contact sheet.
