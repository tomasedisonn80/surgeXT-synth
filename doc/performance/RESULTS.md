# CPU optimization: measurements and validation

**Outcome:** the implementation is published on `main` and passes the recorded
correctness checks. The final experiment provides directional evidence of lower
CPU use in the three effect fixtures, but results across all ten scenarios are
mixed. **It does not establish a broad CPU reduction or a universal minimum.**
The eight-note init fixture used 5.008% more render-thread CPU by the recorded
ratio of medians; that unfavorable result remains part of this delivery.

The final measured implementation is
[`489f0cf7bca2d3e129199b8779c7262a414d81f5`](https://github.com/tomasedisonn80/surgeXT-synth/commit/489f0cf7bca2d3e129199b8779c7262a414d81f5).
It includes the initial optimization commit
[`ae551fa2a4bd95efe21633f1c41a9326c8ef5df0`](https://github.com/tomasedisonn80/surgeXT-synth/commit/ae551fa2a4bd95efe21633f1c41a9326c8ef5df0)
and preserves the intervening verified native-gallery commit
`1b0af2109679256d5624e4464ca97e4310be71b1`.
The original engine baseline is
[`0dcc076937974605095a9567dacd3f28a0b04fd8`](https://github.com/tomasedisonn80/surgeXT-synth/commit/0dcc076937974605095a9567dacd3f28a0b04fd8).
The [agent ledger](AGENT_LEDGER.md) and [work log](WORK_LOG.md) record the actual
60 specialist assignments in ten waves. Earlier measurements and specialist
checkpoints are retained below and in the evidence directory.

## What was compared


The baseline retains the original engine source. Only the offline harness and
its build integration were added to that separate checkout. Both engines used
identical harness source, pinned dependencies and matched compiler/linker
options. Candidate source hashes and executable hashes bind the measured
working-tree build to its implementation; an embedded Git label alone does not
identify uncommitted source changes. The independent audit checked all 86 common
engine object rules and representative expanded compile/link commands.
See [build provenance](evidence/build-provenance.json),
[effective commands](evidence/effective-build-commands.json) and
[the evidence audit](agent-reports/w07-a37.md).

| Setting | Recorded configuration |
| --- | --- |
| System | Linux x86_64; CPU label AMD EPYC 9V74 80-Core Processor |
| Compiler | GNU C++ 14.2.0 |
| Optimization | Release, `-O3 -DNDEBUG`, compile/link `-flto=auto` |
| CPU instructions | Existing `-msse4.1 -mfpmath=sse`; no added AVX, native-CPU or fast-math flags |
| Lua backend | PUC Lua 5.2.4 in both headless builds |
| Sample rate / engine block | 48,000 Hz / 32 frames |
| Determinism | Fresh engines, fixed seed 1701, 128 warmup blocks, plugin floating-point guard |
| Timed work | Engine processing, preallocated PCM copies and transport advancement |
| Outside the timed window | Setup, note events, file I/O, fingerprinting and statistics |

The environment reported nine available logical CPUs. The initial experiment
retained affinity to CPUs 0–8; the longer and final experiments pinned both sides to logical
CPU 0. This was a shared cloud environment without a claim of exclusive CPU
ownership or fixed clock frequency. The hardware label does not mean the test
had 80 dedicated cores. Exact machine descriptions and command arrays are in
the [initial provenance](evidence/initial-48000/provenance.json) and
[longer-run provenance](evidence/long-wall-48000/provenance.json), and
[final provenance](evidence/final-48000/provenance.json).

Release and LTO were already enabled by the repository defaults. Their presence
is not a new performance improvement. The local benchmark uses the common
engine directly: these timings do not measure the JUCE output-pointer change,
a commercial DAW meter, LuaJIT performance, or deadline reliability in a host.
Build commands and the implementation scope are in the
[reproduction guide](README.md).

The final source and binary binding is recorded in
[final-build-provenance.json](evidence/final-build-provenance.json) and
[implementation-publication.json](evidence/implementation-publication.json).
The final build snapshot predates publication and is retained byte-for-byte for
the independent audit. Its candidate executable embeds an earlier Git label;
recorded source and executable hashes, checked against the published source,
identify the measured implementation. Pending language inside immutable
snapshots describes their capture time, not the final result reported here.

## Final render-thread CPU and simultaneous wall-clock experiment

Eight paired rounds ran from **12:05:37 to 12:09:47 UTC on 2026-11-15**: four
baseline-first AB rounds and four candidate-first BA rounds, comprising sixteen
native invocations. Every invocation rendered all ten fixtures with 30 seconds
held and five seconds released at 48 kHz. Both sides used identical harnesses,
frozen binaries and logical CPU 0. No local compilation or heavy validation ran
concurrently with the timed children.

Linux `CLOCK_THREAD_CPUTIME_ID` records CPU charged to the rendering thread over
the same held/released windows as the wall clock, plus clock-read overhead.
It excludes time spent descheduled and CPU used by other threads. Frequency,
cache and shared-machine variation remain. The benchmark includes engine work,
preallocated PCM copying and transport advancement; it is not a DAW CPU meter,
a hardware-cycle measurement, or a real-time deadline test.

Each percentage is `100 × (1 − median(candidate time) / median(baseline time))`.
**Positive means less measured time; negative means more.** This is a ratio of
medians, not the median of the eight paired percentage changes. Released windows
can include both audible releases and idle processing.

| Scenario | CPU held | CPU released | Wall held | Wall released | Held CPU pairs favoring candidate |
| --- | ---: | ---: | ---: | ---: | ---: |
| `idle` | -2.158% | -0.050% | -2.074% | -0.053% | 4/8 |
| `init-1` | -3.020% | -1.085% | -3.050% | -1.671% | 3/8 |
| `init-8` | -5.008% | -1.444% | -5.007% | -1.931% | 3/8 |
| `init-16` | +0.775% | +7.843% | +0.674% | +7.713% | 5/8 |
| `lowcut-steep` | -0.179% | +3.514% | +0.409% | +3.548% | 3/8 |
| `unused-formula` | +5.707% | +5.106% | +5.547% | +5.074% | 4/8 |
| `active-formula` | +1.274% | -3.126% | +0.947% | -3.121% | 3/8 |
| `conditioner` | +2.056% | +1.249% | +2.915% | +1.201% | 6/8 |
| `eq3` | +3.450% | +8.282% | +3.454% | +8.280% | 6/8 |
| `eq11` | +4.890% | +7.338% | +4.899% | +7.218% | 6/8 |

Sources: [thread CPU comparison](evidence/final-48000/thread-comparison.json),
[wall and complete-PCM comparison](evidence/final-48000/comparison.json),
[raw reports and provenance](evidence/final-48000/), and
[independent analysis](evidence/final-cpu-independent-analysis.json).

The three effect fixtures provide the strongest directional support: Conditioner,
three-band EQ and eleven-band EQ each favor the candidate in six of eight held
pairs, and both AB and BA subgroups have positive paired medians. Three-band EQ's
released window favors the candidate in seven of eight pairs. These are observed
fixture results with substantial remaining variation, not a promise for every
preset or host.

The adverse **5.008% increase** for `init-8` is unresolved performance risk.
Its paired median reduction is **-6.539%** in baseline-first AB rounds and
**+0.516%** in candidate-first BA rounds. The baseline and candidate CPU ranges
span approximately 25.7% and 21.4% of their medians. This experiment does not
isolate a particular source change as the cause. All twenty held/released CPU
ranges overlap, and every comparison includes both faster and slower paired
rounds. Overlap does not prove equivalence or rule out a real regression.

The favorable aggregate for `unused-formula` has only four favorable pairs and
opposite AB/BA subgroup signs, so it is not treated as an established win. CPU and
wall held reductions differ by at most approximately 0.86 percentage points;
using the CPU clock does not explain away the unfavorable results as descheduling.
No broad speedup, per-change causal attribution, or CPU minimum is claimed.

Complete first-pair PCM matched for every fixture: **134,400,000 bytes per side**.
All later rounds matched the recorded FNV audio fingerprints. These later-round
fingerprints are not cryptographic full-file comparisons. The independent audit
recomputed the summaries, checked all sixteen wall reports and sixteen CPU
sidecars, verified balanced ordering and checked all five recorded tooling hashes.
See [agent 59's review](agent-reports/w10-a59.md).

## Completed field-placement refinement

The layout investigation also found a measurable structural change in the
initial implementation. A GNU x86_64 type-only probe produced the following
sizes in bytes:

| Type | Original baseline | Initial implementation | Append-only probe |
| --- | ---: | ---: | ---: |
| `SurgeStorage` | 924,144 | 924,160 | 924,144 |
| `SurgeSynthesizer` | 18,994,512 | 18,994,528 | 18,994,512 |

Moving the generation counter after the original storage members fits it into
eight bytes of existing tail padding on that compiler/target. Appending effect
cache fields after the original members also restores the sampled history and
control-counter offsets. Effect objects still contain the added cache state;
their total sizes do not return to the original baseline sizes. These are
measured layout results, not measured speedups, and they do not establish an ABI
layout on other platforms. The probe and exact scope are retained in
[layout-probe.json](evidence/layout-probe.json).

The candidate was rebuilt consistently after the relocation. All final native
validation below uses that rebuild; the final experiment uses its benchmark
binary. The relocation preserves original field placement where measured, but
this combined experiment cannot attribute a speed contribution to that change.

## Final correctness evidence and its scope

| Check | Observed result | Evidence and limit |
| --- | --- | --- |
| Final DSP lifecycle comparison | **63 fixtures; 24,772,608 complete PCM bytes per side** matched | [Final comparison](evidence/dsp-regression-final-layout.json). Covers 44.1/48/96 kHz, parameter changes, disable/re-enable, bypass, initialization, rate/table resets, signed zero, rounding changes and release processing. |
| Final existing engine tests | **4,854,298 assertions in 36 cases** passed | [Final log](evidence/engine-tests-final-layout.log), [validation snapshot](evidence/native-validation-final.json). This is the post-relocation count; historical counts belong to their own runs. |
| Final modulation differential | **59,702 cases; 4,895,564 flag comparisons** passed | [Final log](evidence/modulation-mask-final-layout.log). Independent original-source oracle; a mask-state result, not an audio or concurrency proof. |
| Final paired benchmark audio | **134,400,000 complete PCM bytes per side** matched | [Comparison](evidence/final-48000/comparison.json). Full first pair; subsequent rounds use matching fingerprints. |
| Independent final native audit | All 126 lifecycle files, six executables and recorded source/build hashes verified | [Audit](evidence/final-native-independent-audit.json), [report](agent-reports/w10-a58.md). Preserves the immutable audited snapshot hashes. |
| Python validation contracts | **80 tests** passed | [Final log](evidence/python-tests-final.log). Includes seventeen thread-clock binding/integration tests. |
| Native CLI protection | Original 43 cases and eleven thread-sidecar cases passed | [Original checks](evidence/native-cli.json), [sidecar checks](evidence/thread-native-cli.json). These tooling checks precede the field relocation; tiny renders are not CPU estimates. |
| Ten representative factory presets | Ten passed; **3,840,000 complete PCM bytes per side** matched | [Preset evidence](evidence/preset-equivalence.json). Pre-relocation implementation; two rounds each, eight requested notes, 0.5-second held/released windows. Full first pair, fingerprints thereafter. |
| Factory/skin preservation | All **1,021 checked files** match the baseline | [Integrity record](evidence/factory-integrity.json). Includes 1,000 presets, twenty files in ten skins and the embedded payload. No bank or skin source changes were made later. |

The original pre-relocation 63-fixture comparison, engine test count of 4,867,998,
and local two-gate CTest results remain in [dsp-regression.json](evidence/dsp-regression.json),
[engine-tests.log](evidence/engine-tests.log) and [ctest.log](evidence/ctest.log).
The earlier 4,887,009 count is another historical run. These are not relabelled
as results for the final rebuilt binary.

The lifecycle fixtures change stored effect parameters through the engine's
normal copy path. They do not exhaust real VST/CLAP automation queues or isolate
every modulation-only change with a stationary base parameter. The ten selected
presets represent the ten Scoring 1000 categories; this is not audio validation
of all 1,000 presets, every register/velocity/tempo, long tails, or human musical
suitability. The coefficient-table generation counter invalidates caches; it does
not add synchronization to the existing table mutation lifecycle.

## CI for the final implementation

This exact-commit observation was taken at **2026-11-15 12:17:32 UTC** for
`489f0cf7bca2d3e129199b8779c7262a414d81f5`. Full run/job/step records are retained in
[ci-delivery-checkpoint.json](evidence/ci-delivery-checkpoint.json).

| Gate | Observed status |
| --- | --- |
| [CPU optimization correctness](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43712040433) | **Passed:** report-validation Python stage, native Release/LTO build, modulation differential and deterministic native smoke gates. |
| [Scoring 1000 native bank](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43712040427) | **Passed:** exact regeneration, embedded-content/cache recovery, factory discovery, ten-category native audio/macro checks, semantic round trips, slow cycles and sustained polyphony. |
| [Native reference workspaces](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43712040439) | **Passed:** the real processor tests (including five new callback cases), CLAP preset discovery, all native captures, independent display/TSAN stress and verified gallery publication. |
| [Distribution matrix](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43712040446) | **In progress:** all seven installer jobs running; no failed job or step observed. This includes Windows ARM64/ARM64EC, Windows x64 variants, macOS, Ubuntu ARM and Ubuntu Docker. |

For initial implementation `ae551fa2a4bd95efe21633f1c41a9326c8ef5df0`, the CPU,
Scoring and complete native reference workflows passed. The native workflow
compiled and ran the five new callback cases, CLAP discovery, native captures
and independent display/TSAN stress, then published the preserved gallery commit.
Its distribution workflow was cancelled when superseded by the refinement; one
Ubuntu ARM job had completed successfully, and no failed step was observed.
These earlier passes do not certify later source. See
[agent 56's exact-commit review](agent-reports/w10-a56.md) and the linked CI records.
The final native processor/workspace selection passed **10,690,331 assertions in
52 cases**; its retained [test log](../reference-workspaces/native-gallery/native-tests.log)
and [build provenance](../reference-workspaces/native-gallery/build-provenance.txt)
identify the same refined source. These offline native tests do not constitute
commercial-DAW certification or a deadline guarantee.

The refined implementation's successful native workflow published gallery commit
`5b1e48422897c9b24dc3916cd1d0af3285988dff`, a direct child of the measured implementation.
Only the 25 verified-gallery paths changed. This commit is preserved as the
parent of the documentation publication; no measured production source changed.

## Retained earlier wall-clock experiments

These experiments measured the initial implementation before the final field
relocation. Their harness/window differences are recorded; neither is silently
combined with the final CPU experiment or discarded for unfavorable outcomes.


Each round runs baseline then candidate (AB), or candidate then baseline (BA),
alternating between rounds. Each native invocation records one repetition; the
runner combines raw measurements only after validating comparable metadata and
matching audio fingerprints.

| Experiment | Rounds and order | Held audio | Released audio | Affinity | UTC interval on 2026-11-15 |
| --- | --- | ---: | ---: | --- | --- |
| Initial | 7: four AB, three BA | 3 seconds | 1 second | 0–8 | 11:42:50–11:43:15 |
| Longer follow-up | 8: four AB, four BA | 30 seconds | 5 seconds | 0 | 11:45:52–11:50:08 |

The percentages below are recomputed from the raw samples using:

`100 × (1 − median(candidate time) / median(baseline time))`

**Positive means less measured wall time; negative means more.** This is a ratio
of medians, not the median of per-round percentage changes. The longer released
window is a different workload from the initial one: either fixed post-release
window can include both audible release processing and idle processing. Do not
average the two experiments into one claimed improvement.

| Scenario | Initial held | Initial released | Longer held | Longer released |
| --- | ---: | ---: | ---: | ---: |
| `idle` | -2.195% | +5.907% | -3.245% | +0.670% |
| `init-1` | +3.341% | +3.703% | -6.582% | -8.803% |
| `init-8` | -0.090% | -0.035% | -3.378% | +3.079% |
| `init-16` | -1.702% | -2.034% | -7.001% | -5.733% |
| `lowcut-steep` | -1.566% | +1.289% | -7.277% | +9.006% |
| `unused-formula` | -2.734% | -0.092% | +0.053% | -0.122% |
| `active-formula` | +0.989% | +0.013% | -6.545% | -6.386% |
| `conditioner` | -1.789% | +0.851% | +8.183% | +6.493% |
| `eq3` | -1.214% | +0.525% | +1.116% | -2.181% |
| `eq11` | -0.818% | +2.252% | -6.147% | -3.501% |

Sources: [initial comparison](evidence/initial-48000/comparison.json) and
[longer comparison](evidence/long-wall-48000/comparison.json). Their directories
retain every raw round report, merged report, summary and provenance record.

Both experiments have overlapping baseline/candidate minimum-to-maximum ranges
for all ten held and all ten released measurements. The short experiment's
ordinary held windows took only about 7–118 milliseconds of actual execution,
apart from active formula processing. Its small changes were accompanied by
larger within-build variation and order sensitivity. The longer run reports a
Conditioner held reduction of **8.183%**, while several other scenarios are
slower. Overlapping ranges alone do not prove equivalence or rule out a real
regression, and shared-machine variability is not a reason to discard the
negative measurements. These results support further investigation, not a
general CPU-saving percentage. See the
[independent initial timing review](agent-reports/w07-a41.md).

All non-idle benchmark scenarios produced nonzero output, and the effect
fingerprints differ from `init-8`. Both experiments verified complete PCM bytes
for their first baseline/candidate pair and matching reported FNV fingerprints
for all later rounds. Later-round fingerprints are not cryptographic full-file
comparisons. Scenario note counts are requested MIDI notes; dual scenes and
voice modes can change the actual number of active voices.

## Reproduction and publication

### Downloadable optimized builds

After the timestamped CI observation above, the exact-implementation
[release matrix](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43712040446)
completed successfully at **2026-11-15 12:50:26 UTC**. All seven platform
jobs built and uploaded distributions from source
`489f0cf7bca2d3e129199b8779c7262a414d81f5`. The successful configurations
cover Windows x64, Windows x64 with JUCE 7, Windows ARM64, Windows ARM64EC,
macOS universal, Linux x64 and Linux ARM64. All seven Actions artifacts were
present and unexpired when checked. Their SHA-256 archive digests, sizes,
successful job steps, source binding and expiry are in the
[optimized build record](evidence/optimized-build-artifacts.json).

The [platform download table](../scoring-1000/BUILT_IN_CONTENT.md#build-downloads)
now points to those optimized binaries. The later publication commits only
changed documentation and verified native gallery files; no additional DSP
change separates the downloaded build source from the measured implementation.
The GitHub Release/Nightly publication jobs were skipped under the fork's
upstream-owner rules, so these downloads are expiring Actions artifacts.
Compiled-platform success and offline correctness checks do not imply a
performance improvement for every patch or DAW-host certification.

The [reproduction guide](README.md) contains plugin/headless build commands,
baseline preparation, paired CPU/wall measurement, full PCM comparison and
platform qualifications. The offline tools are opt-in; ordinary plugin builds
retain the existing Lua backend and quality defaults. Release and LTO were
already defaults, so enabling their documented preset is not counted as a new
speedup.

The implementation was published by non-forced, expected-head updates to `main`,
with the remote tree checked against the local staged tree. The final report is
published in a subsequent documentation commit without altering the measured
production source. The ledger distinguishes completed specialist assignments
from measured gains and platform checks that remain in progress.
