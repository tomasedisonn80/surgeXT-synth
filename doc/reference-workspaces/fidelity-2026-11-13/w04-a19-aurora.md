# Wave 4 / Assignment 19 — Aurora filament material

Compared `07_Liquid_Aurora.png` with the genuine native gallery at source
`a362667` (gallery commit `418151a`), including the full-resolution hero and
surrounding controls. Read the repository instructions and continuation,
engineering and user records. Changed only Aurora branches in
`Visuals::wavetable`; its manifest remains byte-identical.

## Implemented

- Each row now fades in its own phase domain, using its projected first/last
  sample x coordinates. The old canvas-wide fade left sheared row endpoints
  inside the visible gradient.
- Four compound strokes replace 192 separately composited source-row halos.
  Each compound contains the same 48 real row paths. The narrower halos are
  composited once per width, avoiding amplification of low-alpha quantization
  where many near-silent endpoints coincide.
- Removed Aurora's filled inter-frame sheets. Fainter transverse wires omit the
  first/last eight phase points, reducing the closed-looking edge fans.
- Original frame wires are 0.60 logical pixels wide. Forty-seven additional
  0.45-pixel filaments use the existing linear interpolation between adjacent
  copied rows; no independent waveform is generated. The selected morph row
  keeps its genuine position, with narrower bloom/core and matching frame color.

Read-only geometry studies used the decoded audible table, never reference
pixels. Increased negative shear merged its crest families; positive shear
simplified the crossings into a bow. The existing fixed camera was retained.

## Verification

Strict validation passed all ten manifests, with the two existing ClearSignal
overlay warnings. Scoped `git diff --check` passed. A focused source comparison
confirmed all changes are confined to `wavetable`, with sample copying,
interpolation, projection and all surrounding functions unchanged. Aurora's
manifest and audible WT are byte-identical to HEAD; IDs, bindings, controls,
layout and audio/state contracts are unchanged.

The real 12,288 source vertices and 12,032 midpoint vertices are finite and
within the hero: normalized x 0.005–0.995, y 0.03515–0.98143. Each midpoint lies
between its two existing projected samples. No build, commit or push was run.

## Remaining gap

The existing source still has broader, more regular crests than the reference's
irregular crossing ribbons. Material changes cannot establish its precise
topology. Fresh native CI/captures must confirm endpoint dissolution, haze
brightness, fine-line antialiasing and surrounding-control clearance. The
software geometry study is not a native render or a pixel-fidelity claim.
