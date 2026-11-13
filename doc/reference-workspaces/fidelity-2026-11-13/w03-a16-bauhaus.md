# Wave 3 / assignment 16 — Bauhaus spacing and ink

## Evidence and implementation

Read `AGENTS.md`, the continuation, engineering/user records and current gap
audit. Compared `references/04_Bauhaus_Modular.png` directly with
`native-gallery/04-bauhaus-modular.png`, both 1586 × 992. The gallery is the
previous native baseline, not a capture of this pass. Implementation is confined
to the owned Bauhaus `workspace.json`; no shared renderer changes were made.

- Converted the two actual OSC 1 source parameters from 166 × 40 stacked
  label/rail/value controls to supported 176 × 22 compact rows at y235 and y263.
  Their baselines are y246/y274, matching the reference's compact source-control
  rhythm. Font size is 11 rather than 10; labels still come from the current
  oscillator algorithm. No reference-only Span control was introduced.
- Expanded the inset plot from `[994,139,169,80]` to `[990,135,181,93]`.
  After the existing 5 px plot padding and perspective projection, its maximum
  horizontal source span grows from approximately 151 to 162 px. It remains
  inside the inset, with 7 px clearance before the first compact control.
- Refined horizontal title insets: matrix 16 px, effects 14 px, macros 13 px,
  source inset 14 px. A bounded bright-letter pixel check found the matrix title
  beginning at x24 in the reference versus x20 natively, and macros at x1056
  versus x1055; these adjustments address those small offsets.
- Added explicit `#171A17` ink to the eight macros and three mixer faders.
  Their real readouts consequently use darker graphic ink instead of the muted
  palette. Existing macro face diameters, centers and fader hit areas remain
  unchanged; those proportions are already close.

## Verification

`python3 scripts/reference-workspaces/validate.py --strict-overlap` passes all
ten workspaces, including Bauhaus's 25 panels / 79 controls. Only the existing
two ClearSignal overlap warnings and deliberate Flow overlay note remain.
`git diff --check` passes. Parsed before/after comparison confirms identical
IDs, bindings, selection mappings, labels and all properties outside the listed
geometry/ink fields. The two compact rows retain a 6 px gap.

## Exact renderer follow-ups

`drawPanel`'s title Y inset still places harmonic lettering at y103–115 versus
reference y109–121, and matrix/macro lettering 3 px high. Correct these optical
offsets in paint rather than moving panels. `BoundSlider::paint` currently
excludes Bauhaus macro captions from `emphasizeLabel`; native values also remain
regular weight. The generic fader's roughly 13 px outer rail and 24 × 8 cap
remain wider/thinner than the reference's approximately 7 px rail and 30 × 12
cap; preserve pointer travel when refining that paint.

The coordinator's new native build/capture must check these changes alongside
the shared wordmark/wire refinements. No local build, commit, remote action,
new screenshot or pixel-identity claim was made.
