# Wave 9 / assignment 49 — final Obsidian and Ivory visual review

## Evidence and verdict

Read `AGENTS.md`, the current-pass README, continuation, user guide, engineering
log and assignments 32/37. Inspected both original references, all four latest
baseline/edited native PNGs at 1586 × 992, and pixel-preserving header, material
and filter closeups. Gallery `762d888f7159558db472836a8f9b5f015c1f9427`
records clean source `ea674878313006afa0b9d1a2b1d3495b69755627`, successful
[run 43497923810](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43497923810),
and capture time `2026-11-13T20:07:32.689Z`. All four PNG dimensions and SHA-256
hashes independently match the gallery manifest. The earlier `0708bc7` images
were used only as historical comparisons.

**Accept the scoped material/header corrections.** The fresh native pixels
verify their intended effect. No consequential new clipping, overlap or
material regression was observed; no further source correction is recommended
for these two skins in this pass.

## Measured appearance

Regions use half-open `[x0,y0,x1,y1)` coordinates; numbers are median RGB from
the original PNGs, without rescaling or colour adjustment.

| Region | Reference | Latest native |
| --- | --- | --- |
| Obsidian OSC1 title `[315,125,468,149)` | 24/29/33 | 24/29/33 |
| Obsidian FX lip `[600,676,1000,683)` | 26/31/36 | 26/31/36 |
| Obsidian OSC1 body `[250,286,495,291)` | 17/21/24 | 17/21/24 |
| Ivory preset top `[600,20,910,28)` | 26/57/35 | 25/54/34 |
| Ivory preset bottom `[600,45,910,53)` | 23.5/52/32 | 24/52/32 |
| Ivory selected Scene A `[415,31,451,36)` | 41/69/50 | 42/72/50 |

Obsidian's former title/body medians of 18/22/26 and 12/15/18 are corrected
without washing out the separate display wells. The local title lips now read
clearly across the full layout. Equal sampled medians do not imply identical
texture or whole-image identity.

Ivory's washed-out sage preset and selection surfaces now read as dark green
enamel. FX1 and Spectrum selection are also visibly dark green. Scene B remains
a distinct cream unselected chip; its sampled 242/233/221 is still lighter than
the reference's 238/225/211.

With maximum RGB below 95 in the brand region, native ink bounds are
`[104,22,167,24]` in x/y/width/height, against reference `[104,22,168,24]`.
The previous eight-pixel vertical displacement is gone. The master green arc
bounds are `[1399,13,62,53]`, against `[1399,13,62,55]`, using
G−R ≥ 10, G−B ≥ 10, G < 140 and R < 115. The dial's horizontal extent now
matches the reference and its actual −6.0 dB readout sits clearly to its right.
The reference's 0.0 dB is not substituted.

## Actual edit and remaining limits

Both native pairs visibly update the cutoff pointer, readout and response.
Obsidian changes from 1.42 kHz to 115.14 Hz; Ivory from 2.40 kHz to 115.14 Hz.
Independent pixel counts reproduce 10,993 and 14,169 changed pixels; their
filter plots change by 6,026 and 7,388 pixels respectively. These agree with
the capture evidence's real `filter1.cutoff` edits.

Ivory cream caps retain flatter shading than the reference, and small chip,
font and texture differences remain. Native oscillator terrain, filter curves,
modulation routes and output spectra reflect actual sources/state and documented
engine adaptations. This is a visual acceptance of the measured corrections,
not a literal pixel-identity claim. Only this report was written; no source,
capture, fixture, commit or publication was modified.
