# CPU optimization work log

## Scope and baseline

The requested work is a sound-preserving CPU optimization pass, carried out by
60 specialist subagents in ten waves of six. The runtime permits three subagents
alongside the coordinating agent, so each wave is scheduled in two groups.
Only completed assignments count toward the total.

Baseline: `0dcc076937974605095a9567dacd3f28a0b04fd8` on `main`.
An original-engine baseline checkout, with only identical offline tooling and its
build integration added, is retained for differential builds. Source
submodules remain at the repository's pinned revisions. Existing factory presets,
the embedded content payload, reference skins and their visual assets are outside
the change scope.

Acceptance requires actual compiled validation, finite reproducible benchmarks,
and explicit separation of measured improvements from source-level expectations.
Release quality, oversampling, voice limits, effect tails, controller timing and
portable CPU requirements must be preserved. Benchmark timing excludes patch
loading and file I/O, and uses the plugin's floating-point environment.

## Wave 1 — completed: source audit

| Agent | Assignment | Result used in subsequent work |
| --- | --- | --- |
| w01_a01_engine_map | Engine and voice hot paths | Identified modulation-mask simplification, unused formula preparation, macro RTTI, and repeated lowcut frequency conversion. |
| w01_a02_vst_path | VST/CLAP callback and real-time paths | Identified repeated JUCE output-pointer lookup; documented MIDI, bus, aliasing and zero-buffer constraints. |
| w01_a03_build_audit | Build flags and existing measurement tools | Confirmed Release/LTO already enabled; identified native headless build and missing plugin-equivalent denormal mode in existing offline measurement. |
| w01_a04_modulation_audit | Routing and parameter copies | Specified mask invariants; rejected patch-dirty caching of live parameter/modulation data. |
| w01_a05_effects_audit | Effects, coefficients and tails | Recommended a conservative per-block frequency hoist; deferred coefficient caches until lifecycle invalidation is proved. |
| w01_a06_evidence_design | Benchmark fixtures and evidence | Specified fresh seeded engines, preroll, repeated timing, captured PCM, and representative polyphony/formula/lowcut fixtures. |

These audits establish candidates, not measured speedups. In particular, scene
macro sources are shared and their existing smoothing cadence must remain intact;
silent input is not permission to skip stateful filters or effect tails.

## Wave 2 — completed: first implementation and validation tools

| Agent | Assignment | Result |
| --- | --- | --- |
| w02_a07_benchmark | Native CPU benchmark | Added finite seeded scenarios, held/tail timings and optional complete PCM output. |
| w02_a08_engine_reduction | Modulation masks and lowcut | Implemented stateless mask preparation and per-scene frequency conversion reuse. |
| w02_a09_formula_reduction | Formula-input shortcut | Rejected after identifying voice-reuse Lua initialization and MTS tuning side effects; retained production behavior. |
| w02_a10_vst_pointers | Callback output-pointer access | Cached pointers within each callback, preserving zero-buffer and sample ordering behavior. |
| w02_a11_comparator | Evidence comparison | Added strict report/PCM comparison; 25 comparator contract tests passed. |
| w02_a12_mask_tests | Differential modulation test | Added an independent baseline oracle and 59,702 cases; native execution pending at this checkpoint. |

## Continuation

The refined implementation and thread-clock tools are published as
`489f0cf7bca2d3e129199b8779c7262a414d81f5`, preserving native-gallery parent
`1b0af2109679256d5624e4464ca97e4310be71b1`. All 60 actual agents have been
spawned across ten waves. All sixty assignments, including final CPU interpretation
and the closing publication audit, are complete.

## Wave 3 — completed: coefficient caches and host coverage

| Agent | Assignment | Result |
| --- | --- | --- |
| w03_a13_conditioner_cache | Conditioner coefficient inputs | Added allocation-free exact-input cache while retaining filter processing. |
| w03_a14_eq_cache | Graphic/parametric EQ inputs | Added per-band caches with first-run invalidation and existing control cadence. |
| w03_a15_host_tests | Real processor callback tests | Added five tests for buffer segmentation, buses, aliased input, zero-length callbacks and bypass/reset. |
| w03_a16_effect_fixtures | Effect and lowcut audio fixtures | Added three benchmark scenarios and 63 initial lifecycle PCM fixtures. |
| w03_a17_engine_review | Independent DSP review | Identified first-process floating-point and same-rate-reset invalidation needs; coordinator incorporated fixes. |
| w03_a18_build_presets | Reproducible build configuration | Added portable plugin/headless presets and opted-in engine testing without JUCE. |

The first baseline Release/LTO executable passed a two-repeat deterministic smoke
run of the original seven benchmark scenarios. This is a tool/correctness check,
not a performance result: timing comparisons wait for final source builds.

## Wave 4 — completed: adversarial review and repeatable gates

| Agent | Assignment | Result |
| --- | --- | --- |
| w04_a19_host_review | Independent callback review | Added odd-buffer automatic input-latency coverage and reviewed alias ordering. |
| w04_a20_regression_review | DSP fixture review | Added same-rate reset and outside-callback initialization under directed rounding. |
| w04_a21_benchmark_review | Benchmark contract and paths | Identified sample-rate validation, input/output alias protection and provenance requirements. |
| w04_a22_cache_adversarial | Table rebuild invalidation | Added storage table generation to all three coefficient caches; covers direct rate/tuning resets. |
| w04_a23_paired_runner | Alternating comparison runner | Implemented AB/BA execution, strict merge, raw evidence and provenance; 19 synthetic tests passed. |
| w04_a24_ci_gates | Continuous validation | Added headless Release/LTO native correctness workflow, CTest smoke and artifact retention. |

Native modulation validation passed 59,702 cases and 4,895,564 flag comparisons.
The selected existing engine suite passed 4,887,009 assertions in 36 cases before
the final table-generation and floating-point-environment changes; final source
validation is recorded separately. The comparator and runner together pass 44
synthetic contract tests after consistent 16 kHz fixture normalization.

## Wave 5 — completed: compatibility and evidence hardening

| Agent | Assignment | Result |
| --- | --- | --- |
| w05_a25_fp_cache_review | Floating-point cache dependencies | Found host rounding-mode changes; implemented native FP control key and conservative fallback. Coordinator also included GNU/Clang x87 precision/rounding. |
| w05_a26_platform_review | Compiler/platform compatibility | Confirmed existing C++20/ISA/Lua defaults and documented Windows ARM/macOS universal preset qualifications. |
| w05_a27_factory_integrity | Factory and skin preservation | Verified 1,021 protected files match baseline; bank/embed/skin validators passed; chose ten representative category patches. |
| w05_a28_pcm_comparator | Complete DSP regression comparison | Added strict 63-fixture manifest and full-byte comparator with 19 passing synthetic tests. |
| w05_a29_native_cli | Native argument/output safety | Passed 43 native cases: 41 rejection/sentinel checks and two supported-rate boundary runs. |
| w05_a30_ci_review | Native/plugin CI integration | Confirmed five processor cases are selected by native workflow and identified all four triggered workflows/platforms. |

The 63 lifecycle fixtures now include coefficient processing under upward and
downward rounding with unchanged parameters. Production sources are frozen for
final differential validation. Platform and actual JUCE callback runtime results
remain pending until CI completes.

## Wave 6 — completed: architecture and semantics

| Agent | Assignment | Result |
| --- | --- | --- |
| w06_a31_fp_portability | Native FP control key | Verified ARM64 encoding with primary references and passed 21 native x86 helper assertions. |
| w06_a32_performance_guide | Reproducible build and measurement guide | Documented commands, supported configurations and evidence limits. |
| w06_a33_realtime_cost | Callback boundedness | Found no new allocation/mutex; confirmed cadence, frequency constants and native lock-free generation load. |
| w06_a34_measurement_contract | Timing/provenance audit | Verified timing boundaries and ratio-of-medians; clarified note requests, tail window and binary provenance. |
| w06_a35_automation_semantics | Live parameter and lifecycle behavior | Confirmed post-modulation inputs and unchanged control cadence; documented host-automation coverage limits. |
| w06_a36_oracle_review | Independent modulation oracle | Verified oracle against baseline Git source and reconciled all 59,702 cases. |

At the wave-6 checkpoint, local validation passed all 63 lifecycle byte
comparisons (24,772,608 bytes per side), 4,867,998 existing engine assertions in
36 cases, both CPU CTest gates and all 63 Python contract tests. These are
historical results; the final rebuilt candidate and expanded Python suite are
recorded in the final validation section below.

## Wave 7 — completed: independent evidence and initial publication

| Agent | Assignment | Result |
| --- | --- | --- |
| w07_a37_evidence_integrity | Source/binary/PCM bindings | Rehashed all 126 lifecycle files and verified unchanged original baseline engine plus recorded build inputs. |
| w07_a38_callback_review | VST callback semantics | Confirmed pointer ordering and guarded lifetimes; documented actual bypass path and untested host cases. |
| w07_a39_ci_monitor | Exact-commit workflow startup | Confirmed all four expected workflows started for the published checkpoint; no pass claimed while pending. |
| w07_a40_effective_builds | Effective compiler/linker options | Verified all 86 common object rule options and representative expanded commands match. |
| w07_a41_timing_review | First paired experiment | Found short-window results inconclusive and recommended one longer balanced-order experiment. |
| w07_a42_preset_equivalence | Ten representative factory categories | Passed all ten preset comparisons, two rounds each, with 3,840,000 bytes compared per side. |

The initial seven-round, three-second held timing experiment is retained in
`evidence/initial-48000/`. Its small mixed changes and overlapping timing ranges
do not establish a broad CPU improvement. A predeclared follow-up uses eight
rounds (four AB and four BA), 30-second held and five-second released windows,
with both processes pinned to the same allowed logical CPU. All scenarios and
unfavorable results will be retained. Timing remains a shared-cloud measurement.

## Wave 8 — completed: honest interpretation and CPU-clock instrumentation

| Agent | Assignment | Result |
| --- | --- | --- |
| w08_a43_cache_diagnosis | Steady-state cache behavior | Found no source-level continual miss; identified retained DSP costs and unmeasured layout effects. |
| w08_a44_ci_gates | Exact-commit CI progress | Recorded passing configuration/Python stages and pending native compilation without claiming completion. |
| w08_a45_document_claims | Guide accuracy | Clarified timing/automation limits and corrected explicit embedded-content test invocation. |
| w08_a46_continuation | Maintainer handoff | Added CPU-efficiency continuation while preserving all prior project instructions. |
| w08_a47_thread_cpu_clock | Native render-thread measurement | Added optional Linux thread CPU sidecar outside the existing wall-clock timing boundaries. |
| w08_a48_thread_evidence | Strict CPU evidence comparison | Added metadata/audio-bound CPU validation and 17 contracts; all 80 Python tests passed. |

The longer wall-clock run remained mixed and is retained in
`evidence/long-wall-48000/`. No broad CPU reduction was asserted. The added
thread-clock measurement separates CPU charged to the render thread from time
spent descheduled; CPU frequency/cache variation and clock overhead remain.
Both timer builds and a two-round end-to-end native sidecar/PCM smoke passed.

## Wave 9 — completed: layout and final evidence preparation

| Agent | Assignment | Result |
| --- | --- | --- |
| w09_a49_thread_native_safety | Native sidecar protection | Passed 11 native alias/argument/valid-clock cases with preserved sentinel files. |
| w09_a50_layout_review | Preserve original DSP data layout | Measured eight bytes of storage tail padding; moved bookkeeping after original fields, restoring sampled GNU x86_64 offsets and object sizes. |
| w09_a51_native_ci_results | Native/Scoring/platform CI | Confirmed CPU and Scoring workflows plus actual processor/CLAP/display/TSAN steps passed for implementation checkpoint. |
| w09_a52_results_draft | Complete measurement report | Preserved and checked all 40 historical percentages, exact validation scopes and pending final measurements. |
| w09_a53_layout_lifecycle | Independent relocation review | Confirmed member initialization, visibility, serialization and cache lifetime invariants. |
| w09_a54_agent_ledger | Actual-agent accounting | Reconciled exact task names and evidence; all 54 assignments through wave 9 completed. |

The four-header refinement changes declaration placement only. On GNU x86_64,
`SurgeStorage` returns to 924,144 bytes and `SurgeSynthesizer` to 18,994,512 bytes,
the original sizes; original sampled field offsets are restored. This structural
result is not a speedup claim. This wave ended before the final rebuild and measurement; both subsequently
completed as recorded below.

## Final native validation after field relocation

A consistent rebuilt candidate passed all 63 complete lifecycle byte comparisons
against the original engine: 24,772,608 bytes per side. The rebuilt existing engine
suite passed **4,854,298 assertions in 36 cases**; the rebuilt modulation test passed
59,702 cases / 4,895,564 flags. Agent 58 independently verified every PCM byte and
all recorded source/executable hashes. Earlier assertion counts remain tied to
their historical binaries and runs. The immutable build snapshot is mapped to the
subsequently published implementation by `evidence/implementation-publication.json`.

The final experiment uses eight balanced AB/BA rounds at 48 kHz, 30-second held
and five-second released windows, logical CPU 0, and both Linux render-thread CPU
and wall-clock measurements. No local build or heavy validation runs concurrently.

## Wave 10 — completed: final source, evidence and publication review

| Agent | Assignment | Result |
| --- | --- | --- |
| w10_a55_final_source_scope | Final production scope | Confirmed preserved quality, assets, controller cadence and portable flags. |
| w10_a56_platform_status | Exact-commit CI and ancestry | Recorded successful initial CPU/Scoring/native workflows, preserved gallery ancestry and separate refined-commit status. |
| w10_a57_final_timer_review | Final measurement contract | Confirmed clock boundaries, balanced ordering, strict report bindings and failure handling. |
| w10_a58_final_native_evidence | Independent final native evidence | Rehashed all 126 PCM files, six binaries and recorded source/build inputs; confirmed final engine and mask passes. |
| w10_a59_final_cpu_analysis | Independent final experiment analysis | Recomputed all CPU/wall results and ordering diagnostics; reported mixed overall results and directional effect gains without discarding regressions. |
| w10_a60_publication_audit | Closing source and documentation audit | Verified all eighty displayed timing percentages, all 55 final raw evidence files, source/gallery ancestry, immutable snapshots and all sixty ledger assignments; preflight passed. |

All sixty agents were spawned in ten waves, and all sixty assignments completed.
Follow-ups do not add to the count. The
[ledger](AGENT_LEDGER.md) provides every canonical task name and evidence link.

## Final experiment and delivery record

The final eight-pair experiment completed successfully, with four AB and four BA
rounds, sixteen native invocations and both thread CPU and wall measurements.
All first-pair PCM matched: **134,400,000 bytes per side** across ten fixtures.
Later rounds retained matching audio fingerprints. The raw reports, logs,
summaries and provenance are preserved in `evidence/final-48000/`.

Held render-thread CPU reductions were 2.056% for Conditioner, 3.450% for EQ3 and
4.890% for EQ11; each favored the candidate in six of eight pairs and in both
execution-order subgroups. The eight-note init fixture used 5.008% more CPU by
the ratio of medians. All twenty CPU ranges overlap, and no scenario/window wins
every paired round. Unused-formula's favorable aggregate is order-sensitive.
The final result is mixed, with no established broad CPU reduction, universal
minimum or per-change speed attribution. [RESULTS.md](RESULTS.md) retains the
complete table, historical experiments and limitations.

At 2026-11-15 12:17:32 UTC, the refined implementation's CPU correctness,
Scoring 1000 and complete native reference workflows had passed. The real
processor cases, CLAP discovery, native captures and display/TSAN stress all
passed. Seven distribution jobs remained in progress, with no failed job or
step observed. Exact records are in `evidence/ci-delivery-checkpoint.json`;
`ci-final-checkpoint.json` preserves the earlier 12:14:39 status.

The native workflow published verified-gallery commit `5b1e48422897c9b24dc3916cd1d0af3285988dff`,
whose sole parent is the measured implementation. The coordinator verified all
25 changed files were in `doc/reference-workspaces/native-gallery/` and advanced
the local checkout without overwriting work. Final documentation publication
preserves this gallery and all measured production source.
