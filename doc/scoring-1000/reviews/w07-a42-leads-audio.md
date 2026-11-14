# Wave 7 / Agent 42: Leads native evidence review

## Result and scope

All **100 Leads presets, representing 20 families and five variants each**, have
current, internally consistent evidence for **2,300 native scenarios and 500
passing technical gates**. There are no recorded warnings or independently
identified integrity failures. All **800 macro responses resolve above the
runner's measurement thresholds**. Two Contour responses nevertheless meet the
parent's stricter small-response screening rule and should receive targeted
attention before the final bank is frozen.

This review covers catalog SHA-256
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
It precedes any subsequent control corrections. The
[compact evidence](w07-a42-leads-evidence.json) records every preset and QA-report
hash, source bindings, distributions and exceptions. The renderer hash is
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`;
the QA-script hash is
`203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681`.

Only this report and its evidence JSON were added to the repository. No presets,
recipes, source or measurement protocol were changed. Existing native evidence
resolved the bounded questions, so no additional audio was rendered. This is a
technical review, not a listening pass or composer endorsement.

## Independent checks

The review compared each FXP's bytes with the catalog and report binding, then
checked the renderer, QA script, configuration, catalog-entry and fixture hashes.
It matched every native scenario's note list, duration, hold, sample rate,
velocity, tempo, macros and MIDI expression values to its declared fixture.
Frame counts, channel count, float32 format and native loaded names also agree.

Native peak/RMS values agree with the separate PCM-analysis metrics. Independent
comparisons against the documented peak, silence, active/full DC and default-tail
criteria found no violations. All five gates have empty failure lists; all 23
fixtures are present for every preset. Input files remained stable during the
review. The audit script remains in the task scratch directory
`/workspace/scratch/9118c5c72ad8/a42-leads/audit.py` for reproduction.

The reports record **100 byte-identical default repeats** and **100 different
default PCM hashes**. These support repeatability and rule out exact default
audio collisions within this fixture set. They do not establish perceived
uniqueness or musical value.

## Levels, register and stereo behavior

| Measurement | Observed result |
|---|---:|
| Default sample peak, minimum / median / maximum | 0.024125 / 0.065620 / 0.140683 |
| Default active RMS, minimum / median / maximum | 0.003167 / 0.017387 / 0.076818 |
| Highest peak across all 2,300 scenarios | 0.718165 |
| Maximum absolute active/full channel DC | 0.000039524 |
| Lowest mono-energy ratio across all scenarios | 0.454737 |
| Low-register endpoint peak range | 0.022496–0.134143 |
| High-register endpoint peak range | 0.025139–0.143762 |
| Saved-default tail relative to active RMS | −191.03 to −134.79 dB |

The highest peak is **SC08-033 Glass Cantabile Clear** under the 16-note stress
fixture. The weakest mono sum is **SC08-081 Gold Arc Near** at maximum Width,
retaining approximately 45.5% of measured stereo energy. Neither crosses the
documented warning limit. No default peak falls below 0.01, and no scenario peak
exceeds 0.8; this category does not require the gain corrections proposed for
some other categories. The gain reference is the saved unity master, with no
post-render normalization.

The declared endpoints render finite, nontrivial signals throughout the tested
ranges. This establishes technical coverage at those notes, not comfortable
listening level, acoustic realism or performance on every playback system.

All saved-default tails settle deeply in their complete default fixtures.
Maximum-Release scenarios use a shorter adaptive observation window, capped at
six seconds. For example, **SC08-067 Satin Harmonics Warm** still measures
−24.19 dB relative to active RMS at the end of its one-second release observation.
Do not describe this as complete settlement at every Release setting.

## Expression and mono scope

Hard versus soft velocity produces an active-RMS increase of **7.48–14.27 dB**,
with a median of **10.88 dB**. The smallest difference is Satin Harmonics Clear;
the largest is Copper Call Warm. The
[compiler](../../../scripts/scoring/build_bank.py) maps velocity to voice gain
and filter brightness. These measurements confirm dynamic differences in the
specified notes, not a calibrated perceptual loudness scale.

Wheel-maximum relative waveform differences range from **0.0191 to 0.6894**.
Pressure-maximum differences range from **0.4447 to 1.3033**. The FXPs preserve a
zero-default 5.2 Hz vibrato LFO, pressure-to-magnitude routing and its pitch route;
the [native renderer](../../../src/scoring-tools/ScoringRender.cpp) sends actual
channel-aftertouch and controller messages. Waveform differences are sensitive
to phase and cannot themselves quantify perceived expression or validate the
exact vibrato trajectory. These fixtures set expression before note-on; they do
not test continuous controller gestures during a phrase.

There are **75 mono presets and 25 poly presets**. Native mono mode 1 retriggers.
The mono fixtures contain one held note, including a hard-velocity case; they
do not exercise overlapping note priority, portamento transitions, legato
envelopes or stuck-note recovery. Polyphonic presets receive the separate
16-note cluster stress. These are different performance scopes and must remain
distinct in the final claims.

## Two small Contour responses

| Preset | Relative waveform difference | Feature distance |
|---|---:|---:|
| SC08-058 Prism Voice Clear | 0.004491 | 0.00007085 |
| SC08-088 Porcelain Line Clear | 0.005184 | 0.00006720 |

Both pass the current technical response threshold. Both fall below 0.01 on
both measures, the parent's prioritization screen rather than a perceptual
standard. Their expanded low-pass cutoffs are already **7,905 Hz and 7,440 Hz**,
respectively, which is consistent with a small additional filter-envelope effect
on these harmonic digital voices. A targeted body/decay mapping is a reasonable
candidate, but any change needs fresh evidence for the new FXP hash. No other
Leads macro meets this small-response screen. All Lead motion shapes are Sine,
so the separate Noise-startup correction does not target this category.

## Brightness and related variants

**SC08-028 Hollow Compass Clear** is the brightest default, with a 3.73 kHz
spectral centroid, rising to 7.79 kHz at its upper endpoint, MIDI 86. Its band-pass
architecture, 0.85 keytracking, pulse/FM blend and light noise agree with the
authored focused, hollow wind-like role. Its upper endpoint peak remains 0.03016.
This is an appropriate high-register audition priority, not evidence of an
automatic brightness defect or proof of an acoustic wind instrument.

The closest within-family feature pair is **Narrow Horizon Warm / Float**, at
distance 0.02412 under matching default notes, hold and duration. Their nominal
attack settings differ from 12.4 to 22.4 ms, release from 165.2 to 301 ms, cutoff
from 2,108 to 3,060 Hz, and FM amount from 0.077 to 0.11. The differences extend
beyond metadata, effects or constant gain, while the close feature profiles
justify an eventual musical A/B. No perceptual distinction is certified here.
