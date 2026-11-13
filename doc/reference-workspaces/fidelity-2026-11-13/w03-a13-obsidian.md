# Wave 3 / assignment 13 — Obsidian oscillator colors

## Evidence

Read `AGENTS.md`, continuation, user/engineering records and the current pass
reports. Directly inspected `references/01_Obsidian_Precision.png` beside
`native-gallery/01-obsidian-precision.png` with `view_image`, both 1586 × 992.
The gallery predates this pass; it is baseline evidence, not a new capture.

The OSC 1 display frame is `[16,161,221,124]`. Its reference has cyan strands
on the left and amber strands at the right; the native gallery reverses them.
A bounded pixel check corroborates the direction: in x22–81/y171–277 the
reference has 857 cyan / 0 amber pixels versus native 0 cyan / 980 amber;
in x179–231/y171–277 the reference has 311 cyan / 963 amber versus native
740 cyan / 0 amber. Counts include only HSV saturation >0.25, value >0.3,
cyan hue 160–215° and amber hue 20–55°; they are color evidence, not a fidelity
score. The reference FM right edge is cyan, while the native develops amber.

## Implementation

Changed only three color arguments in `Workspace.cpp`'s oscillator branch:

- Precision wavetable primary/secondary are exchanged, accounting for the
  miniature renderer's secondary-at-top-left to primary-at-bottom-right ramp.
- Precision non-wavetable traces use cyan for both endpoints, avoiding the
  unintended amber right edge, including when OSC 1 changes algorithm.

All other style color results, preview samples, projection, parameter labels,
bindings and algorithms remain unchanged. No renderer-function rewrite,
reference-derived trace or palette replacement was introduced.

The owned manifest is unchanged. Its compact oscillator centers and FX detail
centers already align closely with the reference; the native real parameter
names and units legitimately differ. The evidence did not warrant moving or
shrinking those controls in advance of the shared-material native checkpoint.

## Validation and remaining gate

`python3 scripts/reference-workspaces/validate.py --strict-overlap` passed all
ten manifests (Obsidian: 14 panels / 83 controls), retaining only the existing
ClearSignal warnings and intentional Flow overlay note. `git diff --check`
passed. The reviewed source diff is confined to the assigned color arguments.

No local build, new native capture, commit or remote action was performed.
The coordinator's exact-source native workflow must verify the new hue direction
and monochrome cyan traces. Remaining reference differences include source
waveform shape, genuine control values and the existing native capability
adaptations; no pixel-identity claim is made.
