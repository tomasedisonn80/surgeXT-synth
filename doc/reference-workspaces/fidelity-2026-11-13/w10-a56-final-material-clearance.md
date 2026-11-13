# Wave 10 / assignment 56 — Blue paper and Width clearance

## Evidence and scope

Read AGENTS, continuation, engineering log, user guide and assignments 46, 50
and 51. Inspected enlarged original/native blue-paper and Space-global crops.
The existing native evidence is source
`ea674878313006afa0b9d1a2b1d3495b69755627`, gallery
`762d888f7159558db472836a8f9b5f015c1f9427`, run `43497923810`.
The source corrections below have **not** yet received a native capture.

Only two paint regions in `Workspace.cpp` were changed. No manifests, tests,
bindings, component bounds, gesture paths or other material functions changed.
Concurrent route-update work belongs to assignment 54.

## Blue-paper correction

The existing blue-only 512 × 256 immutable hash tile now interpolates a periodic
32 × 16 grid with smooth transitions over 16-pixel cells. The coarse field is
centred by its own mean; a bounded 1.5 gain preserves variation, and independent
fine hash has only 1% weight. Existing white/black opacity limits and the blue
panel's base gradient are unchanged. Red/cream paper and every other skin retain
their existing material path. No reference pixels, engine RNG or signal data
enter this decorative tile.

The reported blank ROI, x1010:1205/y579:584, was remeasured directly:

| Evidence | Median RGB | Channel range | Mean absolute horizontal neighbor difference |
| --- | --- | --- | --- |
| Existing native PNG | 40 / 81 / 111 | 6 / 10 / 12 | 1.924 / 3.252 / 4.206 |
| Original reference | 40 / 81 / 111 | 7 / 9 / 13 | 0.254 / 0.262 / 0.254 |
| New analytic flat-fill approximation | 40 / 81 / 111 | 6 / 10 / 12 | 0.138 / 0.224 / 0.288 |

The approximation applies the new float32 noise and quantized alpha over RGB
40/81/111; it is not a JUCE render. The same approximation reproduces 99.35% of
the old ROI's RGB bytes, with the remaining blue bytes off by one. Its full-tile
median and channel range also remain the same as its old-tile model. These
calculations support the choice but cannot certify the eventual native pixels.

The field is finite and bounded to [-1,1]; its periodic coarse corners join
continuously. Initialization remains a one-time 131,072-pixel operation with at
most 655,872 deterministic hash evaluations and the same 524,288-byte ARGB tile.
No per-frame texture construction or audio-thread work was introduced.

## Space Width correction

Raised only Width's fitted caption rectangle by four logical pixels, from
`[1163,94,42,16]` to `[1163,90,42,16]`. The existing native ink at y99–105 should
therefore move to y95–101, fully inside the unchanged native child
`[1158,93,101,55]`. The unchanged 34-pixel dial remains centred at `(1184,123)`;
its top graduation remains at y103.36–106.36. This provides 2.36 logical pixels
between the expected last ink row and the tick's top coordinate. The next
native image must confirm a visible background row, including antialiasing.
Glide, the actual Width readout, accessibility and interaction code are intact.

## Verification and handoff

`git diff --check` passed. Bounded float32 texture calculations and native-child/
caption/tick geometry calculations completed; no new tests were added for these
paint-only corrections. The coordinator owns the new exact-source native build,
all ten capture pairs and final optical acceptance. No commit or push was made.
Source ownership is released.

## Native follow-up — source `21c2a82`, final amplitude correction

Inspected original 04 and actual baseline 04/05 from source
`21c2a82813c4e57332bff19cf134a78ac9fe013d`, workflow `43502323088`, retained in
`review/native-21c2a82-unaccepted`. The build and capture step passed
(24,101,112 capture assertions). The workflow is **not accepted**: behavior
reported 46/47 cases and 10,395,091/10,395,101 assertions passing, with all ten
failures at the new legal-slider-step comparison. Those test expectations are
assignment 54's separate correction; this review does not waive that gate.

Width's raised lettering is visibly separate from the unchanged top graduation
in the new raster. No further Space change is needed.

The blue material is still wrong in a different spatial band: the 16-pixel
coarse variation produces conspicuous dark patches in Scene and the filters.
The previous five-row strip concealed this imbalance. The table below uses
unobstructed rectangles and reports RGB RMS residual after removing each
rectangle's best-fit RGB plane, so broad gradients do not dominate the result.

| Blank area, x/y half-open | Reference residual RMS | Native `21c2a82` residual RMS |
| --- | --- | --- |
| Scene between columns, 1467:1485 / 175:252 | 0.782 / 0.732 / 0.715 | 1.862 / 3.168 / 3.910 |
| Scene between rows, 1415:1534 / 236:251 | 0.630 / 0.651 / 0.657 | 1.854 / 3.134 / 3.927 |
| Scene below selector, 1420:1534 / 160:175 | 0.846 / 1.011 / 1.154 | 1.710 / 2.743 / 3.323 |
| Filter 2 header gap, 1274:1330 / 373:391 | 0.711 / 0.848 / 0.921 | 1.748 / 3.076 / 4.048 |
| Filter 1 above knobs, 1000:1190 / 500:508 | 1.132 / 1.501 / 1.805 | 1.404 / 2.498 / 3.294 |

Reduced only coarse amplitude from 1.5 to 0.25. The periodic interpolation,
field centering, fine 1% hash, base gradient, clipping and all other paint paths
remain unchanged. This deliberately relinquishes the old texture's excessive
range rather than preserving conspicuous mottling. The updated integer-alpha
flat-fill model exactly reproduces every RGB byte of the captured old strip
at gain 1.5, and retains its median 40/81/111 at gain 0.25. An approximate
inverse-composite calculation predicts blue residual RMS near 0.89/0.90 in the
two larger Scene areas and 0.88 in the Filter 2 gap. These predictions are not
new native capture evidence; full-panel medians can shift slightly as dark
patches are attenuated.

The narrow source diff and `git diff --check` pass. No tests, manifests or
control geometry changed. Source ownership is released again; the fresh
combined native gate and actual-image acceptance remain with the coordinator.
