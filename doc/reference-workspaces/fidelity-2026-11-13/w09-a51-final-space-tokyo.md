# Wave 9 / assignment 51 — Final Space and Tokyo native review

## Evidence

Read AGENTS, the current fidelity record and assignments 34, 40 and 46. Inspected
both complete 1586 × 992 references, fresh native baselines and edited states,
the supplied side-by-side comparisons, and enlarged hardware crops. Evidence is
gallery `762d888f7159558db472836a8f9b5f015c1f9427`, source
`ea674878313006afa0b9d1a2b1d3495b69755627`, run **43497923810**. The retained
provenance reports 44 passing behavior cases, twenty captures and both audio-tap
checks; this assignment did not rerun that gate.

## One final correction

**Space Width has a real glyph/graduation collision.** In both fresh PNGs, WIDTH
ink occupies approximately y99–105. The major graduation at x1184,
y103.36–106.36 enters the **D** counter and joins its lower stroke. This is
visible in the enlarged unmodified raster, beyond the allocation intersection
identified by assignment 46. At x1183–1184, the counter darkens at y103–104 and
merges with the dark glyph at y105; it is not merely adjacent bounding boxes.
The entire fourth plate is pixel-identical between baseline and edited state.

Raise only the Width caption ink about four logical pixels, retaining its ink
inside the existing child, or reserve an equivalent shorter caption band above
the graduation. Keep the dial centre `(1184,123)`, value, native slider bounds and
binding unchanged. Confirm a visible background row between lettering and tick
in the next actual capture. No second consequential correction is identified.

## Accepted changes and measured residuals

| Area | Fresh native observation |
| --- | --- |
| Space global strip | Five individually edged plates occupy x255–405, 409–665, 670–1085, 1088–1261 and 1266–1578 at y85–153. The narrow gutters remain readable; scene buttons and output hardware stay within their plates. |
| Space actual controls | Glide is centred at `(1014,123)`; Pitch has its field at `[1095,115,58,30]`; Width is centred at `(1184,123)`. The output meter well at `[1418,96,131,48]` is present with readable live bars and its own scale. Apart from the Width caption, these allocations are optically clear. |
| Space brass | Empty macro ROI x1070–1280/y840–950 averages RGB **170.64/142.13/107.10**, versus reference **171.81/143.87/109.71**. Red-channel deviation is **12.92 versus 17.20**. The former pale cast is corrected; reflection variation remains somewhat smoother than the concept. Dark wells and recessed caps retain useful separation. |
| Tokyo panel lips | Across x400–500, the mixer upper reflection averages RGB **183/188/191** at native y438, versus **182.77/183/182.09** at reference y437. The localized bright lip and lower shadow now read as enclosure depth without washing out the graphite face. |
| Tokyo dials | Macro/oscillator crops show separate housing, inset dark cap and substantial red value collar. The approximately 3.75 px macro collar is visibly thicker; the pointer remains distinct, including the low-value dials. Centres and surrounding labels stay clear. |

Both edited states visibly update Filter 1 cutoff and response to **115.14 Hz**;
Space changes 3,273 pixels inside its first filter panel and Tokyo 5,788. The
remaining live-meter changes are expected. Native oscillator families, browser
contents, modulation routes, envelope assignments, value formatting and honest
Width adaptation differ from the concepts; these are not classified as renderer
bugs or invitations to paint fictional state.

Only this report was written. No implementation, commit or push was performed.
