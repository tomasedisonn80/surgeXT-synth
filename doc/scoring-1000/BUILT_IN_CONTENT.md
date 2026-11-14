# Scoring 1000 and ten skins inside every engine build

The current fork links the complete Scoring 1000 bank and all ten native
reference skin packages into `surge-common`, the engine shared by the platform
plugin/application targets. This closes the earlier companion-data requirement
for plugin-only, Windows Minimal, JUCE7, ARM64 and ARM64EC distributions.
It does not depend on a network download, optional addon, or external samples.

The sound definitions and skin designs are unchanged by this integration.
The original bank's [25,000-scenario evidence](VERIFICATION.md) remains evidence
for those exact presets; new delivery tests cover the compiled payload, cache,
discovery, and installer routes separately. Automated technical checks do not
constitute human musical audition or composer endorsement.

## Compact payload

| Item | Exact value |
|---|---:|
| Presets | 1,000 |
| Categories | 10, with 100 presets each |
| Skin packages | 10 |
| Included ordinary files | 1,020 |
| Compressed payload | 190,454 bytes |
| Expanded container | 35,239,208 bytes |
| File data on disk, before filesystem allocation | 35,154,272 bytes |

The payload is approximately 186 KiB. Compressing the shared native preset
structure as one bounded zstd frame saves about 99.46% of the expanded container.
This is lossless compression: every FXP, `skin.xml`, and `workspace.json` is
preserved byte for byte. No LFS or sample streaming is necessary for this bank.
The payload is linked into each applicable binary; the 190 KB figure is the
resource itself, not the size of the whole synthesizer or installer.

Payload SHA-256:
`58267b4a52708a0136435eb2967a8667e951a609a30e9a1783edbf7dd6b2124a`.

Expanded-container identity:
`ab0c68a7ec408b85c1146c2497904332a5115d5e7e51517ec1104a85c8cd8381`.

The [entry index](../../resources/surge-shared/scoring-factory.json) binds all
included source paths, sizes, and SHA-256 hashes to the frozen scoring catalog.
The ten layouts use the fork's native renderer and already compiled visual
assets. Their latest [README screenshots](../../README.md#latest-native-uiux-redesign-screenshots)
remain available alongside the source.

## Startup and updates

During ordinary initialization, the native engine decompresses the bounded
payload and compares its file bytes with the installed data. A complete matching
installation is used directly. If that data is absent, older, incomplete, or
changed, the engine creates a content-versioned directory under its per-user
application-data `FactoryContent` directory and discovers the included presets
and skins there. This runs during initialization, never in the audio callback.

The internal browser sees exactly one factory Scoring 1000 bank. When a cached
copy is active, its factory bank and reference skins take precedence over stale
installed copies. Other installed, third-party, and user presets remain available.
CLAP preset discovery also declares the active content to compatible host browsers.

Cache writes use separate staging files and atomic replacement. Incomplete or
damaged expected files are checked and repaired; symbolic links inside the owned
cache are rejected. Multiple processes may initialize the same content safely.
An error is reported if the cache cannot be prepared, and existing installed
resources remain available. Normal writable application data is required for a
plugin-only first launch. The expanded payload is temporary memory; cached file
data takes roughly 35 MB, plus filesystem allocation overhead. Matching full
installations need no second copy.

The content identifier changes when included bytes change. Previous version
directories are retained so running older plugin versions are not invalidated.
The application does not delete unrelated files or user patches. Personal edits
belong in the ordinary user patch collection, rather than the private cache.

Explicit `SURGE_DATA_HOME`, pipeline data roots, supplied native data paths, and
no-scan headless fixtures retain their deliberate behavior. They do not silently
fall back to embedded data; this keeps installer verification capable of finding
a genuinely incomplete installation.

## Distribution routes

| Output | Included delivery |
|---|---|
| Windows x64 full/compact/custom/minimal setup | Embedded content plus required Data-component bank and skins |
| Windows x64, ARM64, ARM64EC, and JUCE7 portable ZIPs | Embedded content plus complete `SurgeXTData` tree |
| macOS universal application, AU, VST3, and CLAP outputs | Embedded content; full installer also carries its resource package |
| Linux x64 and ARM64 applications/plugins | Embedded content; normal full packages also install factory resources |
| Plugin-only archives | Included binaries supply the bank and ten skins automatically |

This describes the source/build contract. Actual platform compilation,
installer execution, signing and host testing are separately reported in the
[verification record](VERIFICATION.md); they are not inferred from source rules.
The bank does not add a platform that the synthesizer itself does not support.

## Reproduction and maintenance

After editing original recipes or included skin files, regenerate the normal
FXPs/catalog first, then run:

```bash
python scripts/scoring/embed_factory.py
python scripts/scoring/embed_factory.py --check
cmake -P cmake/verify-embedded-factory.cmake
python scripts/scoring/test_distribution_contract.py
cmake --build build-scoring --target surge-embedded-content-test stage-scoring-bank
build-scoring/src/scoring-tools/surge-embedded-content-test resources/data
```

Regeneration requires the zstd CLI. Ordinary configure/build/install checks
only need the existing CMake toolchain; the committed compressed resource is
used directly. The Python check uses the standard library. Source paths are
normalized to LF on checkout, so Windows line-ending conversion does not make
the skin payload stale.

The native test retrieves the actual compiled CMakeRC resources and compares
them with all 1,020 source files. It exercises installed reuse, first-run cache
creation, repair, invalid-input rejection, concurrent callers, and native patch
discovery. The distribution contract test executes the real CMake platform
conditions in small temporary projects and checks Inno Setup file selection.
These complement the bank's independent synthesis and audio checks.
