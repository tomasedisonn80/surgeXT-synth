# Wave 7 / Agent 38 — Pads native evidence review

## Scope and decision

All **100 Pads presets** in the current unity-master generation have valid,
stable input bindings and pass their recorded **2,300 native scenarios and 500
technical gates**. This review identifies useful corrections before release:
responsive Motion behavior for slow Noise-LFO families, two subtle Contour
responses, and improved saved gain for Slow Orbit. It is an audit of measured
native output and source behavior, not a human listening pass.

This snapshot precedes the parent's planned corrections. Its catalog SHA-256 is
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
The [evidence file](w07-a38-pads-evidence.json) records every preset/report hash,
renderer and protocol hashes, distributions, control findings, and supplemental
probe results. The catalog and all 100 reports remained unchanged throughout the
review. Later corrected files require fresh evidence; these results must not be
silently relabeled as testing their new hashes.

## Verification and operating range

I checked actual FXP hashes against the catalog, each report's preset-definition,
fixture, renderer, QA-script and configuration binding, and every native row's
name, notes, duration, frame count, velocity, tempo, controllers and PCM format.
I checked recorded thresholds for all scenarios and required successful native
rows and all five gate results. The source is the actual Surge engine; the
executable's embedded base Git identity is `dfdd2f94a`, while its precise binary
and newly added renderer-source hashes are recorded separately.

| Check | Observed result |
|---|---:|
| Passing presets / scenarios / gates | 100 / 2,300 / 500 |
| Byte-identical default repeats | 100 / 100 |
| Distinct full default PCM hashes | 100 |
| Resolved macro responses | 783 / 800 |
| Maximum sample peak, all scenarios | 0.721878 |
| Maximum absolute full/active channel DC | 0.00005623 |
| Minimum default mono-energy ratio | 0.601285 |
| Minimum mono-energy ratio, all scenarios | 0.207300 |
| Largest final default-tail RMS | 0.00000004436 |

The maximum peak belongs to **SC04-049 Winter Fold Drift**, under sixteen-note
stress. The lowest mono ratio occurs for **SC04-076 Feather Bridge Close** with
aggressive macros: about **6.83 dB** of energy reduction in the mono sum. It remains
above the protocol's 10 dB warning boundary, but broad stereo settings deserve
mix-context audition. Passing that threshold does not mean no audible change
when summed to mono.

Default peaks span **0.00456–0.09455**, with median **0.05347**. Active RMS spans
**0.00163–0.03601**, median **0.01542**. Spectral centroids span **214–3,229 Hz**,
median **414 Hz**. Those are descriptive measurements at each family's stated
fixture note, not equal-pitch timbral rankings or loudness scores.

All declared low/high endpoints render above the silence floor. Slow Orbit Open
is the quietest endpoint example: low-note peak **0.00214**, high-note peak
**0.00649**. All Attack responses resolve; adaptive maximum-Attack holds range
from **3 to 21.893 seconds**, and the smallest Attack-max peak is **0.00394**.
These tests demonstrate output and control response within their fixture windows;
they do not convert an analog ADSR setting into an exact acoustic arrival time.

## Motion and Contour findings

There are **17 unresolved Motion responses**, all within four Noise-LFO families:
Hollow Velvet, Winter Fold, Feather Bridge, and Slow Orbit. Twelve of these are
exactly zero under the short fixture; the remaining five are below the documented
response threshold. Hollow Velvet Drift and Slow Orbit Drift also meet the
parent's small-response screening rule, despite clearing the technical threshold.

The [native Noise-LFO attack code](../../../src/common/dsp/modulators/LFOModulationSource.cpp)
initializes interpolation history multiplied by start phase; phase zero starts
that history at zero. This supports agent 37's startup diagnosis. It is different
from disconnected routing, and a longer held note can reveal later random motion.
The planned sine-shaped, zero-default companion is a reasonable targeted way to
make Motion responsive while retaining authored random motion in the main LFO.
All affected new FXPs still require native re-testing.

Two additional Contour controls merit attention:

| Preset | Relative waveform change | Feature distance |
|---|---:|---:|
| SC04-012 Glacier Loom Open | 0.007889 | 0.000119 |
| SC04-082 Spectral Silk Open | 0.004965 | 0.000139 |

Both have relatively open low-pass designs. They pass the numerical response
test, but satisfy the requested small-change screen. That screen is an engineering
triage rule, not a validated perceptual threshold. An additional bounded body-decay
route is consistent with the parent's existing Contour correction strategy.

## Slow Orbit: measured saved-gain recommendation

The five Slow Orbit defaults sit well below the category median, while their
largest measured stress peaks leave considerable room. The band-pass design and
soft source blend support a subdued role, but preserving an unusually low saved
level is not necessary to preserve that timbre.

I recommend testing a saved-VCA increase toward a **0.03 default sample peak**.
This remains below the category's first-quartile default peak and does not claim
perceptually equal loudness. The proposed adjustment was made only to scratch
copies; the bank and recipe were not edited.

| Variant / ID | Original default peak | Added VCA gain | Measured boosted dense peak |
|---|---:|---:|---:|
| Close / SC04-096 | 0.011014 | +8.703765 dB | 0.283292 |
| Open / SC04-097 | 0.004558 | +16.367462 dB | 0.252351 |
| Bloom / SC04-098 | 0.005173 | +15.267439 dB | 0.323547 |
| Drift / SC04-099 | 0.005288 | +15.076545 dB | 0.295742 |
| Veil / SC04-100 | 0.009754 | +9.758333 dB | 0.315067 |

All **10 supplemental native renders**—default and sixteen-note stress for each
copy—passed, with no normalization or output repair. Default peaks landed between
**0.0299998 and 0.0300001**. The largest dense peak leaves about **9.8 dB** below
sample full scale in that fixture.

This is consistent with the dB-to-linear voice gain calculation in
[SurgeVoice](../../../src/common/dsp/SurgeVoice.cpp) and its placement after voice
filtering in the [Wide filter chain](../../../src/common/dsp/QuadFilterChain.cpp).
The original full QA identifies dense polyphony as the worst measured fixture
for all five. Nevertheless, effects can be nonlinear, and the planned Motion
change alters another control endpoint: re-run all 23 scenarios on every final
changed preset. The proposed boosts exceed the compiler's current +6 dB trim
allowance; any revised bounded allowance should be explicit and tied to these
measurements, rather than bypassed silently.

## Tail and diversity limits

Every saved-default tail settles strongly: the least attenuated final half-second
is **115 dB below active RMS**. Maximum-Release fixtures observe only a capped
six-second tail. **Slow Orbit Veil** still measures **−6.75 dB relative to active
RMS** at that endpoint, with tail RMS **0.00193**. That is compatible with a long
extended release; the present evidence does not establish complete settlement at
every Release setting.

All 100 default PCM hashes differ. Closest same-family feature pairs include
Winter Fold Drift/Veil (**0.09334**) and Feather Bridge Drift/Veil (**0.10825**)
under matched active fixtures. Their recipes change envelope, filter, blend and
motion settings. These measurements organize focused A/B listening; they do not
certify perceived uniqueness or professional adoption.

Original gain-probe FXPs, raw PCM, fixtures and native JSONL are retained in
`/workspace/scratch/9118c5c72ad8/a38-pads/`. Only this report and its compact
companion evidence were written to the repository. No source, presets, recipes,
QA implementation or commits were changed. Musical audition remains pending.
