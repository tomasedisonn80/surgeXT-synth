# Wave 1 / Assignment 5 — native waveform rendering audit

Reviewed the native gallery at tested implementation `453df569` against original references 03, 07 and 10, and read `Visuals.cpp`, `Visuals.h`, the showcase generator, AGENTS.md and continuation documents. This is an audit; no renderer or waveform asset was changed.

## Highest-value correction: Aurora endpoints

The native Aurora hero has a conspicuous block-shaped blue glow at approximately x273–335 / y385–443, and an abrupt magenta ending at its right edge. The reference's ribbons dissolve into the surrounding dark purple material. The renderer draws 48 real rows with four broad haze strokes (up to 0.30 of plot height), each with default butt caps, while its horizontal color gradient retains full opacity at both endpoints. Overlapping end caps produce the hard slab.

Narrow next fix in `Visuals::wavetable`:

- In the `aurora` gradient branch, set the left and right endpoint colors to alpha zero.
- Add bounded intermediate alpha stops around horizontal fractions 0.035 / 0.085 and 0.935 / 0.98. Preserve the existing central hue stops and frame-dependent cyan/magenta material.
- Give only Aurora's four broad haze strokes curved joins and rounded end caps.
- Keep sample coordinates, source interpolation, selected row, waveform data and amplitudes unchanged. Apply the gradient to strips, seams and selected-row glow too, through the existing shared gradient helper.

This is cosmetic opacity treatment, not source normalization or a waveform replacement. New native captures are required before claiming visual improvement.

## Remaining specific discrepancies

**Spectral:** current 48×256 copied-source mesh forms six broad smooth ridges above a highly exposed flat grid; the original is a denser irregular mountain field filling nearly the entire hero. Raising wire density alone will not change its actual topology. Follow-up should separate camera/floor opacity adjustments from changes to the audible `Spectral Bloom` asset. Keep signed amplitudes affine and retain a single table-wide source gain.

**Live:** original has stronger amber/mint separation, more angular fine-wire crossings and a taller irregular mesh; current copied frames are smooth parallel lobes. A renderer-only improvement can sharpen the two material colors, reduce opaque-looking strip accumulation and use more subtle transverse wires. Changing peak shapes requires an explicit audible showcase-source change, followed by existing headroom/reset validation.

No native runtime test or new screenshot was produced by this assignment.
