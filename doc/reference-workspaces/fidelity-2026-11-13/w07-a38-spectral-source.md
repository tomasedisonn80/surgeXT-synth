# Wave 7 / assignment 38 — Spectral audible ridge source

## Evidence and scope

Read AGENTS, continuation, engineering log, user guide and assignment 33. Compared
the original Spectral reference with the genuine `0708bc7` baseline retained by
gallery `facc042` (passing run `43493343847`). The expanded camera is working;
the remaining source repeats six large, smooth dune groups instead of the
reference's sharper subordinate ridges.

Changed only `DUNES_PEAKS`, `DUNES_MEANS`, the `signal("dunes")` branch and
`Spectral Bloom.wt`. Assignment 39 independently owns the Bauhaus branch and
its global-scale membership in the same script. The camera, 142 interpolated
render rows, snapshot API and native loader remain unchanged.

## Original audible signal

Fourteen independently moving periodic pulses replace the six simple pulses.
Each combines a 60% sharp raised-cosine core with a 40% broader shoulder; eight
lower subsidiary pulses provide additional ridges. Three bounded depth rates
vary each amplitude, and three independent phase-travel rates break up the
long regular folds. Small harmonics 1, 5, 17 and 31 connect the lower shoulders.
All variation is encoded in the playable asset; no reference pixels, display
noise, rectification, per-frame normalization or random state are involved.

Every pulse power is an integer. The highest analytic harmonic is 880, below
the 2048-sample source Nyquist of 1024. Amplitude factors stay within
`[0.15,1]` times their declared height; the finite sum is bounded before the
existing DC removal and single global 0.92 scaling. The ordinary float32 `.wt`
header remains `vawt`, 2048 samples, 256 frames, flags 0, and the existing
capture fixture still selects the table through its normal name/loader path.

## Numerical verification

The snapshot comparison reproduces `sampleWavetable`'s float32 frame and phase
interpolation from the actual old/new asset bytes into 48 × 256 values. Peak
counts use local prominence of at least 0.04 in signed source amplitude;
widths are measured at half that peak's prominence. These are topology checks,
not image-similarity scores.

| Check | Previous source | New source |
| --- | --- | --- |
| Significant peaks per snapshot row, min / median / max | 5 / 6 / 6 | 10 / 12 / 15 |
| Strong peaks at prominence ≥0.20, min / median / max | 4 / 5 / 6 | 5 / 7 / 8 |
| Median significant crest width, snapshot columns | 15.3323 | 6.4534 |
| Crest width 10th–90th percentile, snapshot columns | 12.0367–19.0798 | 3.5199–12.9987 |
| Energy in harmonics 17 and above | 0.08443% | 9.61736% |
| Energy above snapshot harmonic 127, fraction | 5.73e-16 | 5.65e-16 |
| Projected source y range with unchanged camera | 162.1165–322.6046 | 164.2357–321.7929 |

The sharper phase detail therefore survives the 256-column snapshot instead of
depending on unresolved upper partials. A separately labelled, non-native
analytic projection was inspected to reject repeated corrugation. The source
contains 256 unique finite frames / 524,288 float32 samples. Encoded range is
−0.3281260133 to +0.9200000167 (float32 rounding of the 0.92 global peak);
maximum absolute frame DC is `3.448406e-10`. Analytic endpoint disagreement
over all 256 frames is at most `1.568190e-15`. All projected source vertices
remain inside `(455,146,692,236)`; the plane/camera were not altered.

Targeted generation/validation wrote only Spectral Bloom. After assignment 39
settled, the complete generator `--check` passed for all ten actual assets,
including the independently regenerated Bauhaus table. `git diff --check`
passes. No build, native capture, commit or push was performed here; the next
exact-source native run must verify final appearance and audible fixture gates.

Asset: `resources/data/wavetables/Reference Showcases/Spectral Bloom.wt`
(2,097,164 bytes), SHA-256:
`2a6b9be1590a279d800cf916ab61c1fe8fe0a58a356c8c5c8c14240aa59b8020`.
