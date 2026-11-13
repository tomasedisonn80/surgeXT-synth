# Wave 9 / assignment 50 — Spectral and Bauhaus native review

## Evidence

Read AGENTS, current pass record, continuation/user/engineering records and
assignments 33, 37, 38 and 39. Inspected both original references, all four
1586 × 992 baseline/edited PNGs and hero/material closeups from gallery
`762d888f7159558db472836a8f9b5f015c1f9427`, exact source
`ea674878313006afa0b9d1a2b1d3495b69755627`, run `43497923810`.
All four PNG hashes match the gallery manifest. Its native results record
44 behavior cases / 10,345,938 assertions and 24,101,112 capture assertions.
This review does not certify the superseded `0708bc7` captures.

## Actual native findings

**Spectral:** The new audible source produces visibly sharper subsidiary
peaks and more varied ridges. Violet/cyan separation, complete plot framing,
recessed chassis and control hierarchy work. The field nevertheless appears
suspended above its foreground plane. Within the native PNG, strong colored
terrain ends at y321; the low-intensity floor continues to y380. In
x455:1147/y325:381, excluding the marker at x790:814, a fixed mask finds zero
native pixels versus 15,848 reference pixels. The predicate is maximum RGB
channel >100 and at least one of B−R, B−G or G−R >25. This is local evidence of
the empty foreground, not a similarity score. The earlier analytic source
projection around y164–322 explains the image but is not new PNG evidence.
The native selected marker and peak highlights also remain less luminous.

**Bauhaus:** Actual strands now remain open and the redundant inner hero frame
is gone. Warm-colored PNG bounds are x270–969/y144–323, versus reference
x270–969/y143–323. The real waveform's crest/crossing positions differ from
the concept, so one vertical cutline cannot establish its row count or fidelity.
The remaining paint is markedly faint and desaturated: within
(270,135,700,189), native/reference luminance 99th percentiles are
167.23/231.88. For warm pixels (R>95, G>40, R>1.15B), median maximum-minus-minimum
RGB channel values are 57/120. Open geometry should be retained while improving
that warm core.

**Materials and balance:** The blue paper sample x1010:1205/y579:584 now has
the reference median RGB 40/81/111. Native/reference channel ranges are
6/10/12 versus 7/9/13. Its spatial texture is still too sharp: mean absolute
horizontal neighboring-pixel differences are 1.92/3.25/4.21 versus
0.25/0.26/0.25. Red paper likewise looks stippled rather than softly varied.
Matching ranges alone therefore does not establish a material match.
Bauhaus hero title pixels occupy y109–121 and matrix/macro titles y759–771,
matching reference baselines; control proportions and title balance are sound.
Spectral's hero heading remains approximately three pixels high optically.

## Two consequential follow-ups

1. Reframe Spectral's **whole snapshot and its floor together**, retaining
   signed amplitudes and relative frame levels. The current fixed −1 floor is
   far below this source's troughs. Use bounded global camera framing, with
   explicit finite, single-frame and silent-source safeguards; do not normalize
   individual rows, fabricate foreground samples or merely add more wires.
2. Strengthen Bauhaus's warm wire core luminance/saturation while retaining
   the actual open rows, thin spacing and absence of filled strips. Inspect
   another exact-source capture before accepting the change.

Both edited captures visibly change Filter 1 to 115.14 Hz and its response;
their hero pixels remain identical. Baseline/edited differences are 10,779
Spectral and 4,329 Bauhaus pixels, matching recorded evidence. Native FM2,
Classic, filter identities, actual routes and output traces remain legitimate
engine/state differences. No whole-skin fidelity percentage or literal 1:1
claim follows. Only this report was written; no source changes, commits or push.
