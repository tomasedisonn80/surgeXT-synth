# CPU optimization and reproducible validation

This pass reduces repeated work in Surge XT's engine and plugin callback while
preserving DSP quality and existing processing behavior. The implementation and
validation tools are separate: ordinary plugin builds do not include the offline
benchmark executables.

The [CPU-optimized platform downloads](../scoring-1000/BUILT_IN_CONTENT.md#build-downloads)
were built successfully from the measured implementation on all seven release
matrix configurations. The download record identifies the source SHA, artifact
checksums, package selection and retention period.

**Measured outcome:** the final experiment shows directional improvements in
three effect fixtures and mixed results overall, including an unfavorable
8-note init result. [RESULTS.md](RESULTS.md) retains every scenario, both clocks,
all earlier measurements, exact source bindings and validation limits. It does
not establish a broad CPU reduction or universal minimum. The
[agent ledger](AGENT_LEDGER.md), [work log](WORK_LOG.md) and
[specialist reports](agent-reports/) record all sixty assignments in ten waves;
historical reports retain the status observed at their own checkpoints.

## What changes

| Area | Repeated work removed | Behavior retained |
| --- | --- | --- |
| Modulation masks | Prepare active scenes with a direct reset and the required source checks. | Step-sequence triggers, formula controller dependencies, global/scene/voice routes, muted routes and inactive-scene flags. |
| Scene lowcut | Convert cutoff to angular frequency once per scene and block. | Every cascade stage still updates its coefficients and processes audio. |
| Conditioner and EQ | Reuse coefficient targets when their exact inputs and relevant environment are unchanged. | Existing update cadence, coefficient smoothing, enabled-band processing and effect tails. |
| Plugin output callback | Obtain output channel pointers once per nonempty callback. | Sample-accurate MIDI order, main and scene outputs, aliased input handling and zero-length callbacks. |

The coefficient caches include parameter values, sample-rate information, storage
table generation and the relevant floating-point controls. Reset paths invalidate
the cache. Unsupported floating-point-control architectures use the original
recalculation behavior. Signed zero and changed bit patterns remain distinguishable;
NaNs do not become cache hits. These are coefficient-target caches, so a quiet
input does not justify skipping a stateful filter or its release tail.

Release mode and link-time optimization were already enabled by the repository's
defaults. The presets below make the intended configuration explicit; enabling
LTO is not itself a newly discovered CPU saving. This pass does not introduce
fast-math or a machine-specific instruction-set requirement, reduce oversampling,
lower voice limits, change factory presets, or remove Lua functionality.

Implementation: [engine](../../src/common/SurgeSynthesizer.cpp),
[effect cache environment](../../src/common/dsp/effects/CoefficientCache.h),
[Conditioner](../../src/common/dsp/effects/ConditionerEffect.cpp),
[graphic EQ](../../src/common/dsp/effects/GraphicEQ11BandEffect.cpp),
[parametric EQ](../../src/common/dsp/effects/ParametricEQ3BandEffect.cpp), and
[plugin callback](../../src/surge-xt/SurgeSynthProcessor.cpp).

## Build prerequisites

Use CMake **3.22 or newer**, Ninja, a supported C++20 compiler, and recursively
initialized submodules. The performance tools require **Python 3.10 or newer**.
Full plugins additionally need the platform dependencies in the root
[build instructions](../../README.md#setting-up-for-your-os). Headless validation
does not require JUCE or desktop development libraries.

Run commands from the repository root unless a command changes directories.
Examples use a POSIX shell; on Windows use the corresponding executable paths
with `.exe` and an environment initialized for the intended compiler architecture.

```sh
git submodule update --init --recursive
cmake --list-presets=all
```

The [presets](../../CMakePresets.json) use Ninja, Release, LTO, two build workers
and `SURGE_COPY_AFTER_BUILD=false`. Two workers bound build memory use; they do
not set the synthesizer's audio-thread count.

## Build plugins

```sh
cmake --preset cpu-release
cmake --build --preset cpu-release
```

`cpu-release` enables Surge XT, Surge FX and CLAP with the normal plugin targets.
It leaves the existing default Lua selection intact and disables offline
performance, scoring and engine-test tools. It uses the architecture selected by
the compiler environment rather than automatically building the entire release
matrix.

For Windows ARM64 or ARM64EC, select the proper compiler environment/toolchain
and request the PUC backend, as the existing release workflow does:

```sh
cmake --preset cpu-release -DSURGE_LUA_IMPL=puc
cmake --build --preset cpu-release
```

For a universal macOS build, explicitly request both architectures:

```sh
cmake --preset cpu-release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build --preset cpu-release
```

The [Lua backend guide](../../libs/lua/README.md) explains the Windows ARM
constraint. The release preset does not switch other plugin builds to PUC.

## Build and run headless correctness checks

```sh
cmake --preset cpu-headless
cmake --build --preset cpu-headless
python3 -m unittest discover -s scripts/performance -p 'test_*.py'
ctest --preset cpu-headless -R '^cpu-'
```

This preset explicitly selects **PUC Lua**, builds the benchmark, DSP fixture
renderer, modulation differential test, existing engine tests, scoring renderer
and embedded-content test. The `cpu-modulation-mask` test compares the optimized
mask with an independent implementation of the baseline behavior. `cpu-smoke`
checks ten benchmark scenarios with two repetitions and all 63 finite DSP
lifecycle fixtures. It uses fresh temporary PCM directories and retains JSON,
logs and SHA-256 summaries under
`build/cpu-headless/src/performance/cpu-smoke-evidence/`.

Run the broader registered engine tests with:

```sh
ctest --preset cpu-headless
```

The embedded-content executable is built by this preset but is not registered
with CTest. Run it explicitly:

```sh
build/cpu-headless/src/scoring-tools/surge-embedded-content-test resources/data
```

A successful single-build smoke test establishes fixture integrity and
repeatability. A comparison against the baseline is a separate check. Native
JUCE callback tests in [XTTestCpuProcessing.cpp](../../src/surge-xt/xt-tests/XTTestCpuProcessing.cpp)
run through the existing **Native reference workspaces** workflow; the headless
targets do not instantiate a plugin or DAW.

## Prepare a matching baseline

The original engine baseline is commit
`0dcc076937974605095a9567dacd3f28a0b04fd8`. Compare it with the candidate using
the **same current benchmark source and build wiring**, the same pinned
dependencies, compiler, flags and Lua backend. The baseline predates the tools,
so add only their harness and CMake integration to its separate checkout.

The following setup uses a fresh sibling worktree. It copies no optimized DSP or
plugin processor source:

```sh
git worktree add --detach ../surge-cpu-baseline 0dcc076937974605095a9567dacd3f28a0b04fd8
git -C ../surge-cpu-baseline submodule update --init --recursive
cp -R src/performance ../surge-cpu-baseline/src/performance
cp -R scripts/performance ../surge-cpu-baseline/scripts/performance
cp CMakeLists.txt CMakePresets.json ../surge-cpu-baseline/
cp src/CMakeLists.txt ../surge-cpu-baseline/src/CMakeLists.txt
cp src/surge-testrunner/CMakeLists.txt ../surge-cpu-baseline/src/surge-testrunner/CMakeLists.txt
```

Build the matching baseline tools in that worktree:

```sh
cd ../surge-cpu-baseline
cmake --preset cpu-headless
cmake --build build/cpu-headless --target surge-cpu-bench surge-dsp-regression --parallel 2
```

Return to the candidate checkout for the comparison commands below. Before
measuring, retain the baseline and candidate source identities, effective CMake
cache/build commands, submodule revisions and binary hashes. A Git label embedded
in a binary does not independently attest an uncommitted working tree. Review
the baseline diff to confirm it contains only the explicitly copied tooling and
build integration.

## Measure alternating baseline/candidate pairs

Stop builds and other heavy jobs first. Keep sample rate, fixtures, user defaults,
external tuning services, machine power policy and compiler configuration
consistent. The output directory must be absent or empty.

```sh
python3 scripts/performance/run_comparison.py \
  --baseline ../surge-cpu-baseline/build/cpu-headless/src/performance/surge-cpu-bench \
  --candidate build/cpu-headless/src/performance/surge-cpu-bench \
  --output-dir ../surge-cpu-evidence/48000 \
  --rounds 8 --seconds 30 --tail-seconds 5 \
  --sample-rate 48000 --scenario all
```

Eight rounds balance four AB and four BA runs. The synthetic fixture command
omits `--data`, matching the measured initialization path. Keep any ambient user
defaults and tuning services controlled; this does not create a sandbox for them.
For Linux render-thread CPU evidence, add `--thread-cpu` to the same command.
This writes separate `thread-comparison.json`, `thread-summary.txt` and per-round
clock sidecars alongside the wall reports. The native option is
`--thread-output FILE`. Other platforms reject this optional clock explicitly;
ordinary wall/PCM measurement remains available without it.

Use separate output directories for other relevant rates, for example
44100 and 96000 Hz; those are additional experiments, not implied by the 48 kHz
timing report. The accepted rate range is 12000–192000 Hz; held and tail
durations must each be greater than zero and at most 30 seconds. Durations round
up to complete engine blocks. `--patch /absolute/path/to/preset.fxp` selects a
single preset scenario with eight requested MIDI notes; use the same patch and
data directory for both builds. Requested notes are not a measured active-voice
count: scene configuration and the preset's voice mode can change that count.
For preset fixtures, provide the same explicit `--data` directory to both builds.

The [paired runner](../../scripts/performance/run_comparison.py) alternates
baseline/candidate and candidate/baseline order. Each process renders one
repetition; the runner merges raw timings, checks metadata and audio fingerprints,
and compares the first pair's complete PCM. It retains per-round JSON/logs,
merged reports, `comparison.json`, `summary.txt` and `provenance.json`, including
commands and executable/patch SHA-256 hashes. Subsequent rounds retain audio
fingerprints rather than their full PCM. One round is useful for a smoke check
but cannot establish timing repeatability.

Positive `median_reduction_percent` means less measured wall time:

```text
100 × (baseline median − candidate median) / baseline median
```

This is a ratio of the two medians, not the median of per-pair percentage changes.
Read held-note and released-tail results separately, with their raw times and
ranges. The tail measurement covers the entire requested window after note
release, including any idle processing after the sound has decayed.
`median_realtime_percent` divides rendering wall time by audio duration;
it is not a DAW's CPU meter. The timed region includes engine processing,
preallocated PCM copies and transport advancement. It excludes engine setup,
patch loading, note events, file I/O, hashing and statistical analysis. The
benchmark uses fresh seeded engines, preroll and the synthesizer callback's
floating-point guard. Shared-machine contention and clock variation can still
affect timing. PUC measurements do not establish LuaJIT performance or a gain
for every preset, host, buffer size or processor architecture.

The Linux clock charges CPU to the rendering thread and excludes time spent
descheduled and CPU consumed by other threads. Its window includes the two
wall-clock reads and small instrumentation overhead. Both clock summaries use
the same ratio-of-medians formula. CPU frequency, cache state and shared-core
activity can still affect CPU time; it is neither a hardware cycle count nor
a real-time deadline guarantee. The final experiment pins both children to one
allowed logical CPU (CPU 0 on the recorded machine); retain actual affinity in
provenance and choose an allowed CPU on another machine.

## Compare complete DSP lifecycle audio

The separate fixture renderer covers Conditioner, graphic EQ, parametric EQ and
the four lowcut slopes across 44.1, 48 and 96 kHz. Fixtures include controlled
parameter changes, deactivation/reactivation, bypass, suspend/init, changed and
unchanged sample rates, direct storage/tuning table rebuilds, signed-zero inputs, directed rounding
and note release. EQ cases vary the control-phase offset.
Parameter changes enter through stored effect values and the engine's normal
copy path. These fixtures do not drive real VST/CLAP host automation queues or
specifically isolate modulation changes while a stored base parameter stays fixed.

Use fresh output directories:

```sh
../surge-cpu-baseline/build/cpu-headless/src/performance/surge-dsp-regression \
  --output-dir ../surge-cpu-evidence/regression-baseline
build/cpu-headless/src/performance/surge-dsp-regression \
  --output-dir ../surge-cpu-evidence/regression-candidate
python3 scripts/performance/compare_regression.py \
  ../surge-cpu-evidence/regression-baseline \
  ../surge-cpu-evidence/regression-candidate \
  --output ../surge-cpu-evidence/regression-comparison.json
```

The [strict comparator](../../scripts/performance/compare_regression.py) requires
the full 63-fixture set and matching metadata, verifies finite samples, reported
fingerprints, peaks and file lengths, then compares every PCM byte. It records
per-fixture SHA-256 hashes. A pass applies to these rendered fixtures and the
recorded binaries; it does not prove universal audio equivalence or host deadline
reliability.

## Continuous validation

The [CPU correctness workflow](../../.github/workflows/cpu-performance.yml)
builds headless Release/LTO tools, runs Python contract tests and both `cpu-`
gates, and retains diagnostic evidence. It deliberately has no timing threshold
on shared CI hardware. Existing native-reference, scoring-bank and platform
release workflows provide complementary checks. Inspect each job's result;
an uploaded artifact alone is not evidence of a successful job.
