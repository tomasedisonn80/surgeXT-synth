# Wave 5, agent 25: build integration and packaging

## Result and exact scope

The source tree now verifies the entire Scoring 1000 bank during CMake
configuration, ordinary builds, resource installation and distribution staging.
The verified bank uses the repository's existing recursive factory-data
packaging rules. The integration adds no DSP code to the plugin and requires no
Python installation for ordinary CMake builds.

Actual local verification covered all **1,000 staged FXPs in ten categories of
100**, their SHA-256 values, independent static validation, and discovery by the
compiled native Surge engine. This was a **data-only factory stage**. It was not
execution of a complete Windows installer, macOS package, Linux DEB/RPM or host
plugin. The new GitHub workflow was parsed locally but has not run remotely as
part of this agent's work. See the accompanying
[w05-a25-packaging-evidence.json](w05-a25-packaging-evidence.json) for the exact
manifest and native-renderer hashes used in this bounded check. Later preset
changes require staging verification again.

## Changes

- `cmake/verify-scoring-bank.cmake` checks a compact SHA-256 index, the complete
  set of factory files, exactly ten named categories, 100 files per category,
  distinct paths, and the index's binding to the full manifest. It rejects
  missing or modified files and unexpected extra FXPs.
- `cmake/scoring-bank.cmake` runs the guard at configure time, defines the
  always-run `check-scoring-bank` target, installs the guard for direct
  `cmake --install` invocations, and defines `stage-scoring-bank`.
- The top-level CMake project includes that module. Resource-install targets,
  packaged plugin targets, `surge-staged-assets`, and `surge-xt-distribution`
  depend on the guard. Thus explicitly building the distribution target also
  performs validation even when the default `ALL` target is not built.
- `scripts/scoring/write_package_index.py` creates
  `doc/scoring-1000/package.sha256` from the finished manifest. Its public
  function is `write_package_index(manifest_path: Path) -> Path`. The first
  checksum binds the manifest; the remaining 1,000 checksums bind each FXP.
  The helper verifies the actual files against the manifest and checks category
  counts before writing. The parent integrated its call into the full-bank
  generator; partial-category authoring intentionally cannot publish a complete
  package index.
- `.github/workflows/scoring-native.yml` builds the actual native DSP renderer,
  checks every source definition and exact regeneration, stages all 1,000
  presets, checks the stage, and requires all 1,000 paths to be discovered by
  the native engine. Automatic runs measure one preset in every category
  through 23 fixtures: **230 native scenarios**, clearly a smoke run. A manual
  `full_bank: true` invocation measures **23,000 scenarios across all 1,000**.
  Evidence, including failure output, is uploaded with the source SHA in its
  artifact name. The workflow has read-only repository permissions and does
  not publish releases or send external notifications.

The compact checksum index avoids repeatedly parsing the full descriptive
catalog in CMake. A measured 100-object JSON parsing loop took about 1.8 seconds;
the full checksum guard took about half a second under concurrent local work.
The rich independent Python validator remains responsible for FXP structure,
metadata, native parameter semantics and exact regeneration. A checksum match
alone is not a sound-quality judgment.

## Existing package coverage

These rows describe inspected source behavior, not a claim that each installer
was built on this Linux machine.

| Build or installer | Scoring 1000 inclusion | Relevant existing source |
| --- | --- | --- |
| Linux full DEB, RPM and full tar payload | Included by recursive `resources/data` copy into `share/surge-xt/patches_factory/Scoring 1000` | `scripts/installer_linux/make_deb.sh`, `make_deb_aarch64.sh`, `make_rpm.sh` |
| Linux normal CMake install | Included in the `share/surge-xt` resource directory for normal synth builds | `src/cmake/lib.cmake`, `surge_juce_package` |
| macOS full package | Included in the resources package; postinstall places it below `/Library/Application Support/Surge XT` | `scripts/installer_mac/make_installer.sh`, `ResourcesPackageScript/postinstall` |
| Windows full/compact installer | Included when the Patches component is selected; destination is `{commonappdata}/Surge XT/patches_factory` | `scripts/installer_win/surge64.iss`, `surge32.iss` |
| Windows custom installer | Included only if the user selects Patches | Same Inno Setup component rules |
| Windows 64-bit minimal installer | Excluded by the existing minimal component selection | `surge64.iss` lists Patches for full, compact and custom, not minimal |
| Windows standard 64-bit portable install | Included below `Surge Synth Team/SurgeXTData` | `src/cmake/lib.cmake`, portable resource copy |
| Linux portable-content archive | Included below `Surge Synth Team/SurgeXTData` when that target is produced | `src/cmake/lib.cmake` |
| Explicit plugin-only archives | Excluded by their existing purpose; use a full installer or companion resource content | `src/cmake/lib.cmake`, plugin-only archive commands |
| Windows ARM/ARM64EC and special compatibility plugin-only builds | Require companion resource content because those branches do not build the full portable resource archive/installer | `src/cmake/lib.cmake`, architecture conditions |

The factory bank is bundled as ordinary synthesizer resource files; it is not a
literal array of preset bytes embedded in each plugin binary. Retaining that
established architecture preserves the factory browser and installer behavior.
All newly authored sounds are self-contained synthesis, so the compact
installer's omission of optional wavetable collections does not create an
external wavetable dependency for this bank.

## Observed verification

1. Regenerated the compact index from the current complete manifest and ran the
   CMake guard successfully.
2. Reconfigured the existing headless build with its cached options. The new
   configure-time guard passed. This did not rebuild the renderer executable.
3. Built `stage-scoring-bank`, which copied all 1,000 presets into
   `build-scoring/scoring-bank-stage/share/surge-xt/patches_factory/Scoring 1000`
   and verified their destination hashes and exact path set.
4. Ran `validate_bank.py --data-root` against that stage: 1,000 files,
   10 × 100 categories, 1,000 distinct active definitions, no static errors.
5. Ran the already compiled `surge-scoring-render --data ... --discover ...` on
   the stage. Its discovered Scoring 1000 paths exactly matched all 1,000
   manifest paths; no paths were missing or additional.
6. In an isolated copied fixture, verified that the guard passes the complete
   bank, rejects a modified FXP, rejects a removed FXP, rejects an unexpected
   extra FXP, rejects a manifest whose bytes changed without index regeneration,
   and passes after restoration. The repository's sound files were not mutated
   by these negative cases.
7. Parsed the workflow as YAML. Remote job execution, cross-platform installer
   execution and DAW behavior remain outside this result.

Raw local evidence was retained at:

- `/workspace/scratch/9118c5c72ad8/w05-a25-staged-static.json`
- `/workspace/scratch/9118c5c72ad8/w05-a25-native-discovery.json`
- `/workspace/scratch/9118c5c72ad8/w05-a25-staging-evidence.json`
- `/workspace/scratch/9118c5c72ad8/w05-a25-package-guard-tests.json`

The compact evidence beside this report preserves the important outcomes and
hashes inside the repository; these raw scratch files are supplementary.

Useful local commands after an ordinary configure are:

```sh
cmake --build build-scoring --target check-scoring-bank stage-scoring-bank
python3 scripts/scoring/validate_bank.py   --data-root build-scoring/scoring-bank-stage/share/surge-xt
build-scoring/src/scoring-tools/surge-scoring-render   --data "$PWD/build-scoring/scoring-bank-stage/share/surge-xt"   --discover staged-factory-discovery.json
```

The stage target deliberately does not erase an existing destination. An extra
stale FXP causes verification to fail rather than silently deleting user data.
For a deliberately renamed bank, remove the dedicated build staging directory
and stage again. None of these checks changes `musical_audition: pending` or
asserts that a Hollywood composer has used or endorsed the sounds.
