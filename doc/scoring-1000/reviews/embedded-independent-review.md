# Embedded factory content: independent integration review

Reviewed 14 November 2026 against the recovered `13fdca858` bank and the subsequent
embedded-content working tree. This is an engineering review, not musical
audition, platform-installation certification, or a new approval of the sounds.

## Findings corrected during review

### CLAP host preset discovery needed the additional factory root

The internal patch browser received the new cache path, but the CLAP preset
discovery provider advertised only the installed `datapath/patches_factory`.
A plugin-only installation would therefore expose the bank internally while
omitting it from the host's preset browser.

`SurgeCLAPPresetDiscovery.cpp` now uses `declarePresetLocations` to declare the
cached factory bank. When a different installed root is present, it declares
the remaining installed categories and root-level FXPs individually, excluding
the superseded Scoring 1000 directory with a case-insensitive comparison. This
avoids declaring a parent location that would reintroduce the old bank. Third
party and user locations retain their original content flags. A matching full
installation still has one factory-root declaration. Paths are passed as
complete UTF-8 strings instead of truncated fixed-size buffers; metadata loading
uses the repository's UTF-8-to-filesystem conversion.

`XTTestEmbeddedPresetDiscovery.cpp` exercises the real location-declaration
function through a recording CLAP indexer. Its four cases cover plugin-only
content, a mixed installation with a stale bank and unrelated content, reuse of
a matching installation, and rejection by the host. The file is added to the
native XT test target and is conditional on CLAP support. Native compilation and
execution belong to the final validation record; this review does not invent
a successful test run from the source alone.

### Search indexing needed the same factory boundary

`PatchDB.cpp` previously walked parent directories until reaching only the user,
installed factory, or installed third-party root. Cached presets would add the
cache identifier and application/profile ancestors to their search terms.
The embedded factory root is now another stopping boundary. Preset descriptions,
tags, categories, and file contents are unchanged.

### Duplicate suppression was kept narrow

The first skin-discovery change suppressed every repeated factory package name.
The runtime author narrowed that rule to the embedded reference packages. Other
factory packages and user skins retain their previous handling. Both installed
Scoring 1000 exclusion and embedded skin-name matching use case-insensitive
comparisons, covering Windows/macOS filesystem naming behavior.

## Integrity guard verification actually performed

The CMake guard was exercised against the current source data and isolated,
temporary metadata fixtures. No source preset, source skin, or checked-in
payload was modified by these tests.

| Fixture | Observed result |
|---|---|
| Original index, identifier, and archive | Pass: 1,000 presets, ten skins, 190,454 compressed bytes |
| One changed archive byte | Rejected by compressed-payload hash check |
| Changed content identifier | Rejected by identifier binding |
| Stale source-file hash in the index | Rejected by source-content comparison |
| Removed index entry without count adjustment | Rejected by count consistency |

`git diff --check` also passed for the changes made in this review. Temporary
fixtures were removed automatically. No additional repository clone, sample
download, or audio render was needed.

The CMake guard binds the stored archive bytes and the exact source/staged file
set to the generated index. The native materializer's comparison with the actual
source files is the additional check that proves the decompressed archive holds
those same bytes. These are complementary checks; index agreement alone is not
a native extraction test.

## Runtime and platform review boundaries

The format parser bounds the decompressed container to 64 MiB, rejects unsafe
and case-colliding relative paths, requires exactly 1,000 presets in ten groups
of 100, and requires ten complete reference-skin pairs. It checks a single zstd
frame with a known expansion size. Installed matching files are read and reused;
fallback writes target only the private content-keyed cache. Staging names use
the clock and an atomic counter, not the synthesizer's random state. The work is
performed during initialization, outside the DSP processing path.

The actual longest embedded relative path is 92 characters. A typical Windows
profile example produces a 230-character final cache path, below the legacy
260-character limit. Long or redirected profile roots remain a Windows runtime
edge case; no Windows host execution is claimed by this review. Windows uses
`MoveFileExW` for replacement, while Unix uses filesystem rename. Native tests
and final platform artifacts must record their actual environments and results.

The source payload contains only the existing original preset bytes and the
ten `skin.xml`/`workspace.json` packages. The reference workspaces' rendering
code and built-in artwork remain in their existing native binary resources.
No new acoustic-realism, perceptual-uniqueness, or composer-endorsement claim
follows from embedding these files.
