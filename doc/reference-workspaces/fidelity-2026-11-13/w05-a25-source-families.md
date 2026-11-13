# Wave 5 / Assignment 25 — Audible source families

Reviewed references 07/10 against the genuine `a362667` native gallery and the
current fixed projection. Changed only the `aurora` and `horizon` branches in
`generate_showcase_wavetables.py`, their two generated WT files, and this report.
No reference-image samples enter either recipe.

## Formula and rationale

For phase `x`, morph `m`, `theta = 2*pi*x`, and `t = pi*(m-.5)`, both families
remain finite sums of integer harmonics with smoothly varying amplitudes and
phases. All coefficients are explicit in the generator.

Aurora is `W(x) * sum(a[k](m) * sin(k*(theta-.08*pi) + p[k](m)))`, with
`W(x) = .60 + .40*sin(pi*x)^2` and harmonics `{1,2,3,4,5,7}`. Its dominant third
harmonic has amplitude `.60 + .06*cos(1.3*pi*m+.2)` and phase travel
`1.48*t + .28*sin(2*pi*.72*m+.3) + .10*sin(2*pi*1.75*m+1)`. Counter-travelling
second/fourth harmonics and quieter fifth/seventh partials make unequal crossing
crests and shoulders. The wider periodic window preserves more activity away
from the centre. Multiplication adds at most one harmonic: the highest partial
is eight, before float32 quantization.

Horizon is an unwindowed sum of harmonics one through seven. The third harmonic
has amplitude `.47 + .065*cos(2*pi*.65*m+.3)` and phase `1.08*t+d(m)`, where
`d(m)=.31*sin(2*pi*.8*m+.2)+.10*sin(2*pi*1.6*m+1.1)`. Neighboring harmonics use
opposing travel and a separate `.26*sin(2*pi*1.1*m+1.4)` phase variation. Fifth,
sixth and seventh amplitudes `.17`, `.06`, `.065` create smaller folds between
the broad crests. These are audible source changes, consumed by the normal
wavetable loader and copied by the real preview.

## Verification

`generate_showcase_wavetables.py --check` passed all ten tables. Both changed
files retain 48 distinct frames × 512 samples, 98,316 bytes, finite samples,
analytic periodic endpoints, DC removal and the unchanged 0.92 peak limit.
Maximum absolute DC is `9.27e-10` / `8.16e-10`; Aurora retains table-wide scaling
with frame peaks `0.526898–0.920000017`, while Horizon retains per-frame scaling.

AST comparison confirms every other recipe, dimension, scaling rule and
validator is unchanged. Byte comparison confirms exactly two WT changes; all
other eight assets, including Spectral Bloom, remain identical to HEAD. Scoped
`git diff --check` passed.

SHA-256:

- Aurora Waves.wt: `bb71e207fa6d4b8745ac2dd3e3ffe7b30c9cad8af5844e842a8d5179bb4393b8`
- GlassWavetable 03.wt: `1c8db741e5aeaa1df1f28e3aee9d4b978854a25b1937ff9a0fc9d161c71d1262`

Internal decoded-sample projection studies were labelled studies, not native
screenshots. Their vertices remain within both current hero bounds. Fresh
native audio/headroom/reset tests and captures are still required. The source
change may need capture-brightness adjustment after integration with the
independently refined filament materials. No build, commit or push was run.
