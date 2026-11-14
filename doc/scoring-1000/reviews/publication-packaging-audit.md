# Publication packaging audit

This independent continuation audit was recorded on 14 November 2026. It is an
additional publication review, not a replacement for, or an extra entry in, the
original 60-assignment ledger. Only this report and its accompanying JSON record
were written by this assignment.

## Observed publication state

At this audit's GitHub API read, remote `main` was
`dfdd2f94adec9c7bee4f3cfd4f757860fb875c3e`. The prepared local feature commit was
`946ed442572d5fa29e6d6467813b4fa6cb150908`. A workflow-runs query filtered to that
feature commit returned **zero runs**. These are historical observations before
publication; they must not be read as the state of a later published revision.

The source/artifact review records for assignments 57 and 58 correctly retained
pending current-source workflow status. Prior successful installer runs preceded
the bank and did not establish present-feature platform success. Several final
verification and ledger records were also uncommitted when this audit began.

## Source integration

The inspected source provides complete factory-bank delivery through existing
recursive resource routes:

- `CMakeLists.txt` includes `cmake/scoring-bank.cmake`.
- `cmake/verify-scoring-bank.cmake` checks 1,000 exact preset paths, all preset
  SHA-256 hashes, 100 files per category, the bound catalog and unexpected FXPs.
- `src/cmake/lib.cmake` makes distribution, staged-assets and packaged-plugin
  targets depend on `check-scoring-bank`.
- `resources/CMakeLists.txt` applies that guard to resource-install targets.
- Configuration and direct CMake installation run the guard; a separate staging
  target copies and verifies the factory layout.

| Distribution | Bank delivery from inspected source |
|---|---|
| Windows x64 Full/Compact setup; Custom with Patches selected | Included in factory data |
| Windows x64 portable installation | Included in complete portable resources |
| macOS full DMG/PKG | Included in required resources package |
| Linux x64/ARM64 full DEB and tarball | Included in installed factory resources |
| Linux x64 RPM and portable content | Included in resource payload |
| Plugin-only archives | Companion preset data required |
| Windows Minimal, JUCE 7, ARM64 and ARM64EC outputs | Companion preset data required |

The presets are factory data installed alongside the program. Their bytes are
not compiled into each VST3, CLAP, AU or application binary. No concrete missing
installer source integration was found in this audit's scope.

## Independently checked payload and source bindings

The archive inspector was invoked read-only against
`doc/scoring-1000/downloads/Scoring-1000.zip`, first in the original checkout and
again in the isolated publication checkout. Both inspections found **1,000
presets, 1,000 matching hashes and zero errors**.

- Archive SHA-256: `b794179791cfad604b2761e3c109aefd88821831c46ac168cb4048a2c69d41c4`
- Manifest SHA-256: `67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`
- Inspector SHA-256: `e3003f6a7419254c3def0cea64419a612f47911671a1d2d83b91d16a58d78126`

All five implementation hashes recorded in `evidence/factory-staging.json`, plus
its manifest and package-index bindings, were independently recomputed and
matched current files. The existing staging evidence reports 1,000 matching
staged files and exactly 1,000 unique native discovery paths, without missing,
unexpected or duplicate bank paths. This audit checked the evidence bindings;
it did not rerun native discovery. CMake was unavailable on this audit process's
PATH, so no fresh CMake execution is claimed.

## CI scope and remaining observations

The dedicated workflow is configured to build the real native DSP renderer,
validate source regeneration and staged discovery for all 1,000 presets, then
run ordinary push smoke coverage for ten selected presets: 230 primary scenarios,
50 technical gates, 80 macro responses, ten semantic save/load cases and twenty
sustain scenarios. Its manual option supports complete-bank processing.

At the observed pre-publication state, no current-feature CI or platform artifact
pass was available. Actual job conclusions must be attached to the published
source SHA, and at least a data-bearing distribution payload should be inspected
against that revision's catalog. A successful artifact upload alone is not a
successful installer test. Ten-preset smoke results are not a second complete-bank
audio run. Unsigned/unnotarized macOS status, installer execution, DAW-host testing
and human musical audition remain separate scopes.

The [machine record](publication-packaging-audit.json) preserves the observed
remote base, unpublished local feature revision, archive results and scope limits.
