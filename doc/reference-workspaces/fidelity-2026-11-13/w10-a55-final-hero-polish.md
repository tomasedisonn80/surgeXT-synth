# Wave 10 / assignment 55 — final source-driven hero polish

## Scope and evidence

Read AGENTS, the continuation, engineering log, user guide, current-pass README,
assignments 38/39/46/47, and final native reviews 50/52. Inspected the original
Spectral, Bauhaus and Aurora references beside their actual baseline PNGs from
gallery `762d888f7159558db472836a8f9b5f015c1f9427`, compiled source
`ea674878313006afa0b9d1a2b1d3495b69755627`, run `43497923810`.

Changed only hero rendering in `Visuals.cpp` and one narrowly scoped native
raster regression in `XTTestReferenceWorkspaces.cpp`. Assignment 54 owns the
separate inspector refresh regression; insertion locations were coordinated.
No audible table, generator, processor state, snapshot acquisition, asset,
manifest, commit or push was changed by this assignment.

## Spectral: one camera for the real surface and floor

The old fixed `-1` floor put the source's strong terrain above a nearly empty
foreground. The verified PNG ended its strong-color field at y321, while the
reference extends through y380. The actual Spectral Bloom snapshot has 48 × 256
samples, with range `-0.3274968565…+0.9095133543`; the prior camera projected
its source vertices to y164.236–321.793.

The full Spectral view now takes one minimum across its entire finite snapshot
and translates every sample by that same floor. It uses a fixed height of
0.50 plot heights per sample, reduced only when the whole snapshot would exceed
the top margin. It does not normalize individual rows, raise the gain of a quiet
table, rectify amplitudes, or generate foreground vertices. A global DC shift
leaves the surface shape unchanged; signed differences and relative row levels
retain one common scale.

For normalized frame position `t`, the floor is at `0.59 + 0.405t`. The shared
height scale is the minimum of 1 and every nonzero sample elevation's available
headroom divided by its fixed projected height. The final top margin is 0.06;
the entire surface and floor therefore lie in normalized y0.06–0.995. Both
coordinates are affine between copied rows, so the same fit contains the
interpolated seams and selected trace. A final coordinate clamp handles only
roundoff at these limits. Large finite amplitudes are fitted in double precision
instead of flattened at the former ±1.35 sample limit. Nonfinite input becomes
zero, as before. A one-frame table projects its row and selected trace at the
same midpoint, regardless of morph setting.

### Final analytic projection, actual Spectral Bloom bytes

| Quantity | Result |
| --- | --- |
| Explicit caller rectangle | x455–1147, y146–382 |
| Snapshot floor | −0.32749685645103455 |
| Shared reduction factor | 0.9640826161925862 |
| Effective height per sample | 0.4820413080962931 plot heights |
| Original and 142 interpolated row extrema | x455–1147, y160.160–376.509 |
| Floor plane extrema | y285.240–380.820 |
| Original snapshot vertices at y≥325 | 2,595 of 12,288 |
| Vertices including existing interpolation at y≥325 | 7,554 of 36,352 |
| Highest crest in metadata x467–675 | y186.191 |

These are analytic coordinates, **not replacement native PNG measurements**.
The limiting upper crest is around x749.71, outside the left metadata text.
A trial top margin of 0.04 reached y155.44, but brought a left crest to y182.51
inside the existing metadata glyph area. The coordinator selected 0.06 to keep
clearance while moving the principal crest nearer the reference's y155 extent.
No individual peak was moved or altered.

The fit adds two bounded passes over at most 12,288 copied samples, with no new
allocation. Existing maximums remain 48 owned source rows, 142 displayed rows
and 128 cross-frame seams. Floor wires and the selected-position foot use the
same global floor and camera. Other view cameras and miniature plots retain
their previous behavior.

## Bauhaus: brighter, saturated open cores

Retained the 32 original open rows, source coordinates, absence of filled
strips, and separately rendered actual selected trace. The full harmonic core
is now 1.00 px at full alpha, previously 0.55 px at 0.90 alpha. This roughly
doubles the thin core's covered paint area without adding rows or broad halos.
The selected trace retains its 1.05 px stroke and existing highlight treatment.

The warm tint now mixes 24% white into the yellow endpoint, rather than 50%,
and raises both warm palette endpoints to full brightness. The resulting
yellow-to-red cores retain more chroma through antialiasing. Endpoint opacity
is 96% of the core, previously 90%. Miniature paint is unchanged.

The previous actual PNG's luma q99 of 167.23 versus reference 231.88, and warm
channel-span medians 57 versus 120, remain the comparison baseline. New native
values must come from the final build; this source edit is not itself evidence
that those reference statistics have been reached. Geometry remains the prior
x270–970/y144.331–323.559 analytic field within its x270–970/y135–324 clip.

## Aurora: smooth light falloff along the same paths

Replaced four widely separated compound halo widths with 16 closely spaced
compound strokes approximating a Gaussian falloff. Each stage adds only the
opacity missing from its wider predecessors. The outermost alpha falls from
0.055 to approximately 0.00444; consecutive widths are at most 2.75 logical
pixels apart, or 1.375 px in radius. The total inner opacity is approximately
0.393, compared with the old four-stage composite's 0.379. This softens the
outer transition while keeping similar light near the real wires.

The same compound path contains at most 48 × 256 vertices. Halo work is
explicitly capped at 16 strokes no wider than 44 logical pixels: at most
196,608 input vertex visits per paint for these strokes, rather than 49,152.
At 20 paints per second that is an upper bound of approximately 3.93 million
input vertex visits per second for this part of the renderer, not a measured
frame-time claim. One existing compound path is reused; no bitmap buffer,
blur worker, per-row halo accumulation, audio-thread work or random source was
introduced. Actual raster cost remains a native runtime observation.

Source vertices, cyan-centre/magenta-side material, open row and intermediate
filaments, phase-domain endpoint fading and selected trace remain unchanged.

## Verification and final native gate

- Strict manifest validation passes all ten workspaces; existing ClearSignal
  advisory overlaps remain unchanged. `git diff --check` passes.
- Float32 resampling of all ten actual showcase tables passed the new camera's
  finite/containment/reduction-only analytic checks. 288 additional cases
  covered 1/2/3/48 frames, zero, tiny noise, ordinary and very large finite
  amplitudes, constant tables, and NaN/infinity sanitization. Those broad checks
  used the more permissive 0.04 trial margin; the final 0.06 margin tightens the
  same shared reduction constraint. The actual Spectral table was measured
  again at the final margin for the table above.
- Added native case **“Spectral terrain frames one source floor without
  magnifying silence”**. It requires the real silent near row/selected trace
  to meet the floor, identical silent/tiny/nonfinite rendering, clipped painting
  under ±maximum-float input, a single selected row for a one-frame table, and
  shape preservation under a common DC translation. It uses the native
  `Visuals::wavetable` raster path; it is **pending compilation and execution**.
- Existing source-snapshot ownership, nonblocking loader, state/RNG isolation,
  audio and all ten baseline/edited native capture gates remain unchanged.

The coordinator's final exact-source build must establish new native bounds,
Bauhaus luminance/chroma, Aurora outer-falloff appearance, metadata clearance
and practical paint behavior. No local linked build or final screenshot result
is claimed here. The earlier run remains valid evidence for its exact source,
not for these new paint changes.

## Actual native follow-up — source `21c2a828`, workflow not accepted

Inspected the unmodified baseline and edited captures for Spectral, Bauhaus and
Aurora from source `21c2a82813c4e57332bff19cf134a78ac9fe013d`,
[run 43502323088](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43502323088),
against the original references and the preceding accepted `ea674878` gallery.
The coordinator downloaded and hash-verified the captures into a separate review
directory; they have **not** replaced the accepted gallery.

This workflow compiled successfully and its capture case passed all twenty
images / 24,101,112 assertions. Behavior passed 46 of 47 cases, including the
new Spectral raster regression, with 10,395,091 of 10,395,101 assertions passing.
The ten failed assertions all belong to assignment 54's restored-slider test:
its actual legal displayed step `0.618` was compared with raw normalized
`0.6175675392`. Normal and ThreadSanitizer audio-tap logs both pass. The overall
workflow remains failed; the test-only correction requires a fresh full gate.

### Observed results

| Hero check | Previous native | New native | Reference |
| --- | --- | --- | --- |
| Spectral strong-color foreground pixels, x455:1147/y325:381, selected marker excluded | 0 | 10,293 | 15,848 |
| Spectral strong-color maximum y, selected marker excluded | 321 | 376 | 380 |
| Bauhaus luma q99 within x270:970/y135:324 | 167.23 | 199.27 | 231.88 |
| Bauhaus median warm-pixel RGB channel span | 57 | 130 | 120 |
| Bauhaus warm-pixel bounds | x270–969/y144–323 | x270–969/y144–323 | x270–969/y143–323 |

The Spectral predicate remains maximum RGB channel >100 and at least one of
B−R, B−G or G−R >25, excluding marker x790:814. Bauhaus warm pixels use R>95,
G>40 and R>1.15B. These local measurements are not whole-skin similarity scores.

**Spectral:** The real field visibly reaches the foreground instead of floating
above an empty floor. Its metadata remains legible, the highlighted source row
and marker remain visible, and the field stays inside the plot. The selected
marker's shorter foot follows the same source-minimum floor; its position is
not replaced with the concept marker's longer illustrative stem.

**Bauhaus:** The warm core is clearly brighter and more saturated. Source rows
remain open through their crossings, with unchanged field bounds and no restored
filled sheets. Luma still falls below the reference's brightest cream cores;
the new warm channel span is slightly above the reference median. These are
remaining material differences, not a new blocking regression.

**Aurora:** In the actual x650:1010/y305:489 crop, the four wide hard-edged halo
bands have been replaced by a smooth continuous outer falloff. The cyan centre,
magenta side, real source geometry, visible strands and fading endpoints remain.
The changed light does not reproduce the reference's different waveform shape,
which remains a deliberate actual-source adaptation.

All three new baseline/edited hero crops have zero changed pixels, as expected
for the filter-cutoff-only capture edit. **No concrete blocking new visual
regression was found in these three heroes.** This follow-up changes only this
report; it authorizes no gallery promotion and makes no successful-workflow or
literal pixel-identity claim. No further hero source change is proposed before
the corrected full native gate.
