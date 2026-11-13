# Assignment 53 — Platform matrix and portability audit

> Curated historical assignment report. The all-seven-platform success below belongs to 22b9c29. A later source review is not a later platform pass. Final source, run IDs and seven-job results belong in the current ledger. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

Completed: 2026-11-13 UTC. This audit made no repository source changes.

## Verified checkpoint

I independently fetched the workflow metadata, all seven platform job logs,
job outcomes, artifact metadata and exact source configuration for
`22b9c299e6aa74093c9e96677cb32df5f2f74e74`.

[Build run 43382049249](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249)
completed successfully. It started at 06:55:20 UTC and was updated complete at
07:37:07 UTC, approximately 42 minutes later. The version-setup job and all seven
platform build jobs succeeded. Both public release jobs were skipped.

| Build job | Configured architecture / toolchain | Native formats observed in build output | Distribution output observed |
| --- | --- | --- | --- |
| [macOS](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791229) | Universal arm64 + x86_64; AppleClang 21.0.0 | VST3, AU, CLAP, Standalone and CLI | DMG and plugins-only ZIP; explicitly unsigned and unnotarized |
| [Ubuntu 20 Docker](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791250) | Linux x86_64; actual image ubuntu22_gcc12, GNU 12.3.0 | VST3, LV2, CLAP, Standalone and CLI | Debian package, plugins-only archive and additional distribution contents |
| [Windows ARM64EC](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791297) | ARM64EC; MSVC 19.44.35229.0 | VST3, CLAP, Standalone and CLI | ARM64EC plugins-only ZIP |
| [Windows ARM64](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791325) | ARM64; MSVC 19.44.35229.0 | VST3, CLAP, Standalone and CLI | ARM64 plugins-only ZIP |
| [Ubuntu ARM](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791357) | Ubuntu 24.04 ARM64; GNU 13.3.0 | VST3, CLAP, Standalone and CLI | ARM64 Debian package and plugins-only archive |
| [Windows MSVC JUCE 7](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791411) | x86_64; MSVC 19.44.35229.0; JUCE surge-7.0.12 | VST3, CLAP, Standalone and CLI | JUCE 7 plugins-only ZIP |
| [Windows 64-bit](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249/job/131742791450) | x86_64; MSVC 19.44.35229.0 | VST3, CLAP, Standalone and CLI | Setup EXE, portable-install ZIP and plugins-only ZIP |

Every log contains an actual compilation of `gui/reference/Workspace.cpp`,
followed by native linker and packaging output. The wrapper lists are also
confirmed by CMake's configuration messages. CLAP is enabled separately from
the JUCE wrapper list.

The Docker job's display name says “Ubuntu 20”; its workflow image and compiler
log identify Ubuntu 22 / GCC 12. It should be described by that actual
configuration when reporting compatibility.

## What these jobs establish

The workflow builds the explicit `surge-xt-distribution` target. It compiles,
links and packages the real synthesizer and effects formats. It does not build
or execute `surge-xt-tests` as part of that target: all seven logs contain zero
reference-test compilation lines or native behavioral-test completion records.
The CMake “Testing juceaide” messages are build-tool checks, not synthesizer tests.

Behavioral, UI-capture, schema and audio-display evidence comes from the
separate `Native reference workspaces` workflow. The seven platform successes
must not be described as seven independent DAW/runtime validations or as
seven executions of the reference-workspace test suite.

The release workflow itself contains no compiler-cache restore step. The
Windows cache hits in these logs are for Inno Setup 6.7.1, not a cached
synthesizer build. The approximately 42-minute duration is relevant when
scheduling final verification.

The matrix uses `fail-fast: false`, so one platform failure does not cancel the
others. New main builds can cancel superseded non-tag release runs through the
workflow concurrency policy. Final results therefore need to be tied to the
actual final source revision.

Existing CMake configuration omits MSVC warnings-as-errors for ARM64 and
ARM64EC and enables it for the other MSVC configurations. That policy predates
this candidate and is unchanged. No warning policy was weakened by this audit.

## Packaging and publication scope

The observed platform files were uploaded as GitHub Actions artifacts.
They were not published by either release job:

- `Publish Surge XT release` was skipped.
- `Publish Surge XT nightly` was skipped.

Both jobs require the upstream repository owner `surge-synthesizer`; the
release job additionally requires the release-tag form. A file version beginning
`NIGHTLY-` does not establish a published nightly release.

The macOS fork build followed the explicit unsigned branch. The logs show
`Building UNSIGNED PKG`, an `UNSIGNED-BUILD.txt` label and the artifact name
`build-macOS-unsigned`. No Apple Developer ID signature or notarization is claimed.

The ARM64, ARM64EC and JUCE 7 Windows configurations intentionally emit
plugins-only archives. Compatible factory data, including the reference skin
packages, must accompany those binaries. The regular Windows setup/portable
paths and Linux/macOS full distribution paths include their corresponding data
packaging. See the packaging audit for the individual resource-path checks.

## Candidate portability review

I reviewed candidate
`a9d0d0a838632daf178255561def4622cd208258` through the approved public GitHub
source connector, including its processor header/implementation, workspace,
native-test CMake list and both workflows.

The release workflow is byte-identical to the verified 22b9c29 workflow. The
candidate's recent changes add no platform-specific dependency or language
requirement; the repository already builds as C++20.

The MIDI acknowledgement API uses an optional `std::uint64_t*` argument,
preserving two-argument callers. Its declarations include the required integer
header. The two acknowledgement counters use standard atomics, and the audio
path performs bounded stores after native MIDI application. No platform API,
mutex, timer wait or allocation was added to that acknowledgement path.

For the linear-control correction, I directly compared the relevant
`Slider::Pimpl::resized(LookAndFeel&)` implementation in JUCE
`surge-7.0.12` and the pinned JUCE revision
`a12f0de5a3d1e3a43cca626889ac2ee1c0cc0dbe`. The implementation blocks are identical:
both use the supplied `SliderLayout::sliderBounds` for the pointer region.
The new custom layout and `getPositionOfValue` use therefore have a consistent
API/coordinate contract across those two JUCE versions. This source review
does not replace compiling the new candidate with both toolchains.

The earlier Windows RPC `small` identifier collision remains absent from
the candidate workspace variable declarations.

No concrete portability defect was found in this review. The complete
platform matrix for the candidate still needs to run after main promotion;
the successful 22b9c29 jobs are historical evidence, not proof for later code.

## Pending exact-source gate

At audit time, candidate native run
[43393400045](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43393400045)
was in progress. Main remained at the earlier verified checkpoint. No final
candidate matrix or final gallery publication success is claimed here.

After final-source promotion, inspect all seven job conclusions and their exact
head SHA. If a job fails, inspect that job's compiler/linker output and resolve
the concrete defect without suppressing warnings or weakening tests.

## Primary source references

- [Verified build workflow](https://github.com/tomasedisonn80/surgeXT-synth/blob/22b9c299e6aa74093c9e96677cb32df5f2f74e74/.github/workflows/build-release.yml)
- [Candidate processor API](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/surge-xt/SurgeSynthProcessor.h)
- [Candidate processor implementation](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/surge-xt/SurgeSynthProcessor.cpp)
- [Candidate workspace](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/surge-xt/gui/reference/Workspace.cpp)
- [Format configuration](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/CMakeLists.txt)
- [Distribution target](https://github.com/tomasedisonn80/surgeXT-synth/blob/22b9c299e6aa74093c9e96677cb32df5f2f74e74/src/cmake/lib.cmake)
- [JUCE 7 Slider implementation](https://github.com/surge-synthesizer/JUCE/blob/surge-7.0.12/modules/juce_gui_basics/widgets/juce_Slider.cpp)
- [Pinned JUCE Slider implementation](https://github.com/surge-synthesizer/JUCE/blob/a12f0de5a3d1e3a43cca626889ac2ee1c0cc0dbe/modules/juce_gui_basics/widgets/juce_Slider.cpp)
