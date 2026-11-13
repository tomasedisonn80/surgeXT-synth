# Wave 9 / Assignment 52 — final Aurora and ClearSignal review

## Evidence

Reviewed gallery `762d888f7159558db472836a8f9b5f015c1f9427`, compiled source
`ea674878313006afa0b9d1a2b1d3495b69755627`, run `43497923810`, captured
`2026-11-13T20:07:32.689Z`. Read AGENTS, current gallery/continuation READMEs,
assignments 35/41, and the coordinator’s cyan-centre/compound-haze note.
The reported gate passed 44 behavior cases, all twenty captures, and normal/TSAN
audio-display checks; this assignment performed image/source review only.

Viewed the full original `07_Liquid_Aurora.png` and
`08_ClearSignal_Accessible.png`, all four corresponding baseline/edited native
PNGs, the `review/w7-07-liquid-aurora.png` and
`review/w7-08-clearsignal-accessible.png` comparisons, and lossless in-memory
crops of the original PNGs for dials, hero glow, headings and inspector.

## Accepted corrections

**Aurora:** The smaller oscillator/envelope faces and proportional halos now
leave the intended breathing room around captions. The large Morph face and
three miniature OSC 1 faces remain clearly distinct. Browser and compact-card
darkening is local: reference/current median RGB is `(59,38,81)/(58,38.5,81)`
at browser `[175,80,201,102)`, and `(48,32,72)/(48,32,72)` at envelope
`[270,582,290,592)`. The oscillator title patch stays essentially unchanged,
`(67,52,96)/(69,49,97)` at `[500,80,535,100)`. The hero now has a cyan centre
and magenta right-side strands. Its real three-crest wavetable geometry is an
engine adaptation, not an error to replace with reference artwork.

**ClearSignal:** All eleven section identities are visible and suitably small.
Oscillator, envelope, LFO and analyzer headings clear their tabs; the envelope/LFO
strip `[427,586,1178,629)` shows no title collision or clipped icon. The selected
matrix route and inspector both display the real `+0.64` depth. Native algorithm
names, oscillator labels, physical units and spectrogram contents legitimately
differ from the concept. In both skins, the edited filter cutoff and response
move together; ClearSignal changes from `1.42 kHz` to `115.14 Hz`.

## Two remaining bounded improvements

1. **Soften Aurora’s outer hero haze.** In native
   `[650,305,1010,489)`, several broad, sharply terminated halo bands trace the
   cyan crest and magenta trough. They read as stacked outlines, unlike the
   reference’s continuous luminous falloff. This remains conspicuous at full
   canvas size despite the corrected colour. Smooth the decorative falloff or
   use more, weaker halo stages around the existing source paths; preserve
   sampled vertices, source state, open strands and endpoint fading.

2. **Refresh ClearSignal’s selected-route physical readout with its target.**
   The inspector `[1197,548,1576,759)` is identical in baseline and edited PNGs:
   below `+0.64` it still reads `11.62 < 1420.00 > 50175.42 Hz` while the cutoff
   control reads `115.14 Hz`. `Workspace.cpp::update()` refreshes filter curves
   every call but obtains `modulationRows()` only every eighth 20 Hz refresh;
   the capture test snapshots after one post-edit refresh. This supports a
   transient refresh-lag finding, not an incorrect normalized depth or native
   range formula. Refresh the inspected route’s physical string when its target
   changes, preserving the existing route selection and slower unrelated work.

The previous dial, title-material and section-identity findings are resolved in
these captures. No source edits, tests, commits or pushes were performed.
