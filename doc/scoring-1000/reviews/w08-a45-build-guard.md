# Wave 8 / Agent 45 — build guard and CI integration

## Outcome and scope

The factory-bank guard now uses the Surge project root when included from an
outer CMake project, recognizes unexpected `.FXP` files consistently across
platforms, and refreshes only its own staging subtree. The checksum-bound catalog
and index use canonical UTF-8/LF bytes on Windows as well as Linux. All bounded
positive and negative checks listed below passed.

This review changed the two scoring CMake modules, the dedicated scoring workflow,
the release workflow's path exclusions, `.gitattributes`, and the package-index
writer. It did not rebuild or modify the measured native renderer, edit FXPs, or
change the audio QA protocol. Root independently changed the manifest writer to
binary UTF-8 output as part of the same newline correction.

## Concrete fixes

1. **Project-root selection.** The original scoring module used `CMAKE_SOURCE_DIR`,
   which identifies an unrelated parent project under `add_subdirectory`. It now
   uses the repository's `SURGE_SOURCE_DIR`, with a module-location fallback, and
   puts its stage under `CMAKE_CURRENT_BINARY_DIR`. An isolated outer/child project
   configured and staged all 1,000 files successfully. This verifies the new module;
   it does not certify every existing upstream packaging helper for embedded use.

2. **Complete extension matching.** An extra uppercase `.FXP` demonstrably passed
   the original Linux glob. The guard now enumerates the bank and compares
   extensions in lowercase before checking the exact path set. The same isolated
   extra file is now rejected. Existing duplicate-path, missing-file, changed-byte,
   category and manifest-binding failures remain enforced.

3. **Clean, bounded staging.** `stage-scoring-bank` replaces only its generated
   `patches_factory/Scoring 1000` directory before copying. This prevents obsolete
   filenames from surviving a legitimate source rename. An unrelated staged bank
   was preserved in the test. Direct verification still rejects extra staged FXPs.
   Source validation is a dependency and runs before the staged subtree is replaced.

4. **Canonical integrity bytes.** Scoped `text eol=lf` attributes protect
   `doc/scoring-1000/manifest.json` and `package.sha256`. The index helper now writes
   encoded bytes, preventing Python text-mode newline conversion outside Git too.
   An isolated Git index/checkout with `core.autocrlf=true` restored both files with
   exactly their original hashes. Regeneration is byte-identical, contains no CR
   bytes, and succeeds with text-mode writes deliberately disabled in the helper.

5. **Explicit CI failure propagation.** Every shell step in the dedicated workflow
   now starts with `set -euo pipefail`, including the commands piped through `tee`.
   A failing left side of a matching pipeline returned nonzero in the local check.
   YAML parsing and the actual selection shell were exercised without invoking a
   new audio run: false/empty `FULL_BANK` selects exactly ten category IDs; true
   passes no ID filter and therefore selects all 1,000 catalog entries. With the
   current 23-fixture protocol these are 230 and 23,000 scenarios respectively.

## Build and installation coverage

The existing integration runs the hash/count guard at configure time and through
an `ALL` target. Resource-install, packaged-plugin, staged-asset and distribution
aggregate targets depend on that target. A direct `cmake --install` runs the guard
again because installation does not build dependencies. Both default build and
direct install rejected an altered fixture file after configuration; both passed
again after restoration.

The bank remains factory data installed through existing recursive resource
rules. The dedicated data-only stage proves the relative factory layout and exact
file set; it is not a complete platform installer or a DAW-host test. Full data
installers and plugin-only distributions retain their documented distinction.

The release workflow now ignores supplemental scoring documents, offline authoring
and QA helpers, the optional scoring renderer, `README.md`, and `AGENTS.md` when
those are the only changed paths. These files do not change ordinary plugin builds.
Actual factory FXPs, CMake integration, and common DSP changes remain release
triggers. The dedicated scoring workflow explicitly watches its tools, manifest,
checksum index, `.gitattributes`, and native sources. This lets evidence-only
commits avoid canceling a full installer build already in progress while retaining
the relevant targeted checks. No remote GitHub run is claimed by this review.

## Observed verification

| Isolated check | Observed result |
|---|---|
| Complete and restored 1,000-file source snapshot | Passed |
| Altered or missing FXP | Rejected |
| Extra lowercase or uppercase FXP | Rejected |
| Duplicate index path or invalid category | Rejected |
| Stale manifest binding or unsafe index path | Rejected |
| Outer/child CMake configure and stage | Passed |
| Default build and direct install after source corruption | Rejected |
| Restored default build and direct install | Passed |
| Dirty stage checked directly | Rejected |
| Stage refresh with an unrelated bank present | Exact bank restored; unrelated data preserved |
| Workflow YAML, strict shell and full/smoke selector | Passed locally |
| Forced-autocrlf checkout and binary index regeneration | Byte-identical |
| Final source-bank guard | Passed for all 1,000 hashes and ten categories |

The isolated structural fixtures used catalog
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
The final source verification and newline checks use catalog
`8396df6694b92e644f027e90014be629e3c3f542c9d9403c69cb72a0b5b6bee1`, with index
`93432d2ce636edda3c0609dd7556a3bbf5afb134d9210ff505b96080c2824a78`.
The renderer remains byte-identical at
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.

Machine-readable logs, fixture copies, the outer/child project, and the forced-LF
checkout remain in `/workspace/scratch/9118c5c72ad8/a45-build-guard/`:
`negative-tests.json`, `integration-tests.json`, `workflow-tests.json`, and
`line-ending-tests.json`. Final staging/native discovery should be repeated against
the final catalog by the release-verification assignment, as planned.

The relevant implementations are [the scoring CMake integration](../../../cmake/scoring-bank.cmake),
[the exact-data guard](../../../cmake/verify-scoring-bank.cmake),
[the package-index writer](../../../scripts/scoring/write_package_index.py),
[the dedicated workflow](../../../.github/workflows/scoring-native.yml), and
[the release workflow](../../../.github/workflows/build-release.yml).
