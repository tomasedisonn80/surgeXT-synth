# Assignment 50 — Native packaging and installation audit

> Curated historical assignment report. This packaging audit is scoped to its named source and existing distribution evidence. The identified source-frame wording and factory-data guidance were subsequently addressed in the user guide. Final-source platform results remain a separate gate. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Outcome and scope

No blocking packaging or loader defect was found. All ten reference packages are complete and participate in the fork's existing skin discovery and factory-data distribution paths.

This was a read-only source and distribution audit plus the existing deterministic packager self-test. No implementation, user guide, installer script or workflow was changed. The inspected published source is `eccf282722f719b10a918424663244af0b5688fa`; the shared working tree also contains later integration edits. This report does not certify those later edits, installation on every OS, or pixel identity.

## Package completeness

Each of these folders under `resources/data/skins/` contains exactly `skin.xml` and `workspace.json`:

| Number | Package |
| --- | --- |
| 01 | reference-01-obsidian.surge-skin |
| 02 | reference-02-ivory.surge-skin |
| 03 | reference-03-spectral.surge-skin |
| 04 | reference-04-bauhaus.surge-skin |
| 05 | reference-05-space.surge-skin |
| 06 | reference-06-tokyo.surge-skin |
| 07 | reference-07-aurora.surge-skin |
| 08 | reference-08-clear.surge-skin |
| 09 | reference-09-flow.surge-skin |
| 10 | reference-10-live.surge-skin |

All XML files use skin schema 2, unique numbered display names, category `Reference Workspaces`, and the exact adjacent `<reference-workspace layout="workspace.json"/>` declaration. Every JSON manifest has the 1586 × 992 logical dimensions and a distinct ID/style. The supplied folder suffixes and filenames use the exact case expected by discovery and loading.

The C++ workspace, bridge, manifest, data, visuals and preview-curve sources are explicitly included in `src/surge-xt/CMakeLists.txt`. These files are not a legacy skin-only implementation: a compatible binary built from this modified fork is required. The existing user guide states this prominently and correctly.

## Installation and distribution paths

| Path | Source evidence and result |
| --- | --- |
| Linux CMake install | `src/cmake/lib.cmake` installs the entire `resources/data/` tree into `share/surge-xt`, so all twenty skin files are included. |
| Linux DEB/RPM | `scripts/installer_linux/make_deb.sh` and `make_rpm.sh` copy all factory data recursively. |
| macOS resource installer | `scripts/installer_mac/make_installer.sh` packages the entire factory-data root. Its `ResourcesPackageScript/postinstall` places it under `/Library/Application Support/Surge XT`. |
| Standard Windows installer | `scripts/installer_win/surge64.iss` includes `resources\\data\\skins\\*` recursively in the Data component under common application data. |
| Resource helper targets | `resources/CMakeLists.txt` recursively synchronizes the complete data tree for local/global macOS and UNIX resource installs. |
| Regular portable content | `src/cmake/lib.cmake` copies the complete factory-data tree into `SurgeXTData` for the regular portable distribution. |
| Skin-only package | `scripts/reference-workspaces/package.py` captures and validates the twenty data files, guide, license, adaptation notes, hashes and provenance. It deliberately excludes native binaries and reference/screenshot imagery. |

**Distribution distinction:** files named `pluginsonly` intentionally omit factory data. Windows ARM64, ARM64EC and JUCE 7 variants intentionally skip the full resource/installer path in `src/cmake/lib.cmake`. Those binaries need compatible factory data or the skin data package installed separately. A successful platform build must not be described as proof that every artifact embeds these data files.

## Discovery, navigation, fallback and paths

`src/surge-xt/gui/SkinSupport.cpp` recursively discovers factory and user skin directories, recognizes the exact `.surge-skin` suffix, and reads display names/categories from XML. `SurgeGUIEditorMenuStructures.cpp` groups entries by that category, selects the actual native skin, rescans both roots, and makes **Install a New Skin...** open the actual user skin directory. This agrees with the guide's Settings → Skins → Reference Workspaces route and avoids guessing OS-dependent user paths.

`SurgeSynthEditor::updateReferenceWorkspaceFromSkin` reads the adjacent manifest declaration and loads the real Workspace. In-memory/conventional skins and missing/invalid/unconfined metadata retain the original editor. Failed manifests are cached rather than repeatedly interrupting the user. The native **Full editor** and **Return to reference workspace** paths preserve access to the complete synthesis interface.

The new manifest loader constructs Windows filesystem paths from JUCE wide characters and POSIX paths from UTF-8 bytes, then checks the canonical package/file relationship. It rejects absolute, nested, traversing and external-entity metadata paths. The supplied package filenames are ASCII with consistent case. This source inspection is not a claim that a Unicode user-directory installation was manually exercised on Windows or macOS during this assignment.

## Scaling contract

`Workspace::resized` keeps its internal canvas at 1586 × 992, computes a uniform minimum scale from the available window, centers the result, and applies the same transform to native child controls. It does not stretch the two axes independently. The native capture test exercises 1.0, 1.25 and 1.5 scales. The editor's visible workspace size is remembered within the editor instance; the guide correctly avoids promising persistence after destruction.

## Verification

Ran the existing packager self-test against the shared working tree:

```text
PASS: 10 validated skins; 25 archive entries; byte-identical repeated ZIPs;
SHA-256/CRC/metadata verification; overwrite/path guards
```

The twenty package data files were independently parsed to check exact XML declaration/category/version, unique names and IDs, and the logical canvas. No new test mirroring the existing packager was added.

Independently checked GitHub job states:

- [Native run 43391258392](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43391258392): Linux native UI/parameter job succeeded for `eccf282`; gallery publication was skipped on the verification branch.
- [Platform run 43382049249](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249): all seven platform build jobs succeeded for the earlier published `22b9c29` checkpoint. The **Publish Surge XT release** and **Publish Surge XT nightly** jobs were skipped. This does not attest later source on all seven platforms or claim a published installer/nightly.

## Concrete documentation correction sent to the coordinator

The user guide's oscillator-display bullet still says a terrain view merely stacks the current waveform and is not a sweep of source frames. The current wavetable renderer can project copied real wavetable frames. Update that sentence to describe actual source-frame previews when available, while retaining the distinction from a recording of currently sounding voices. The existing distinction between actual output-tap analyzers and private oscillator previews remains useful.

No packaging source change was necessary. Final source, fresh native images and any final platform gates remain the coordinator's integration work.
