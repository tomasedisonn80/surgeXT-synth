# Wave 3 / assignment 15 — Spectral placement refinement

## Evidence and scope

Compared original `references/03_Spectral_Observatory.png` and genuine baseline
`native-gallery/03-spectral-observatory.png`, both 1586 × 992. The gallery was
captured from `453df569c977350c3a4da2caf2f0ab7cbd1a0e71`; it predates the current
shared material work. Read AGENTS, continuation, engineering log, user guide and
the current gap audit. Manifest work started from `a362667`.

Only the Spectral manifest and this report changed. No shared renderer, source
tables, algorithms, bindings, control identities, menus or navigation changed.

## Changes

- Expanded the first filter plot leftward from `(495,523,212,135)` to
  `(472,523,243,135)` and the second from `(799,523,208,135)` to
  `(788,523,219,135)`. The renderer reserves 28 pixels at left and 7 at right;
  actual data columns therefore move from x523/x827 to x500/x816, matching the
  original's approximately x500/x816 starts. Right data edges are x708/x1000,
  close to the reference's x707/x998. Keytracking fields follow the same widths.
- Set filter title insets to 41/36 pixels, placing the real headings at
  x495/x821, matching their reference positions while retaining native names.
- Both cutoff controls move from y524/h76 to y527/h65. The native layout formula
  preserves their dial centers within 0.2 pixels while lifting the caption about
  6 pixels and the value about 9 pixels toward the reference baselines.
- Both resonance controls move from y601/h51 to y594/h58. The dial centers rise
  about 5 pixels, with the readout bottom unchanged. Both columns retain equal
  dimensions and separated interactive bounds.
- The twelve oscillator knobs retain their positions, diameters and native
  dynamic labels, with heights reduced 78→75. Their label/value bands rise
  approximately 3 pixels and faces approximately 1.3 pixels, matching the
  reference's more compact caption rows.

## Hero and shared follow-up

The hero plot already occupies the reference extent `(455,146,692,236)`;
changing that rectangle would mask a renderer issue. In the inspected native
capture its projected floor ends near y360, versus approximately y380 in the
original. The remaining depth, mountain topology and mesh-density work belongs
to the planned source/projection pass. Filter axes also retain a 15-pixel
internal top inset; exact vertical graph alignment needs shared renderer work,
not a plot rectangle overlapping real algorithm menus.

## Validation and limits

Strict manifest validation passes all ten skins with no Spectral warnings.
Existing ClearSignal warnings and the deliberate Flow overlay are unchanged.
`git diff --check` passes. Exact JSON comparison restricts changes to presentation
fields on two panels and eighteen controls; all other data is identical.

No local build, commits or remote operations. New compiled native capture and
placed-child inspection remain pending; this is not a runtime or identity claim.
