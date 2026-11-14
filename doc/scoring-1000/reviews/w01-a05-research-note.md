# Wave 1 / Agent 5 — factory packaging audit

Read-only audit of `surgeXT-synth`, baseline dfdd2f94adec9c7bee4f3cfd4f757860fb875c3e. Read AGENTS.md. No repository mutations or network writes.

## Conclusion

Place the actual native `.fxp` files in `resources/data/patches_factory/Scoring 1000/<category>/`. Existing native discovery and every full-content installer recursively include this tree; no synth DSP or UI edit is necessary. This is normal factory-resource inclusion, not literal embedding in plugin executable bytes. State that distinction accurately. Existing plugin-only archives intentionally omit factory resources.

## Exact routes

| Distribution | Source mechanism | Installed/staged bank root |
|---|---|---|
| Linux CMake install | `src/cmake/lib.cmake:94`: install(DIRECTORY resources/data/ DESTINATION share/surge-xt), inside surge_juce_package | `<prefix>/share/surge-xt/patches_factory/Scoring 1000` |
| Linux .deb x64 | `scripts/installer_linux/make_deb.sh:78`: recursive copy of resources/data/* | `/usr/share/surge-xt/patches_factory/Scoring 1000` |
| Linux .deb arm64 | same operation in make_deb_aarch64.sh:78 | same |
| Linux .rpm | `scripts/installer_linux/make_rpm.sh`, `%install` cp -r resources/data/* and `%files` data tree wildcard | `%{_datarootdir}/surge-xt/patches_factory/Scoring 1000` (normally /usr/share) |
| macOS full .pkg/.dmg | `scripts/installer_mac/make_installer.sh:131-136` pkgbuild --root resources/data, resources choice fixed selected at 241 | pkg stage `/tmp/SXT/Surge XT`; ResourcesPackageScript/postinstall rsyncs to `/Library/Application Support/Surge XT/patches_factory/Scoring 1000` |
| Windows x64/x86 full/compact/custom installer | surge64.iss:103 / surge32.iss:101 `patches_factory\*`, `Flags: recursesubdirs`, component Patches | `{commonappdata}\Surge XT\patches_factory\Scoring 1000` (normally C:\ProgramData\Surge XT\...) |
| Windows minimal install | Patches not included in minimal type; Data only includes Templates and Tutorials | Bank intentionally absent unless installer component policy changed |
| Windows 64-bit portable install | `src/cmake/lib.cmake:209-219` copies complete resources/data under SurgeXTData | `<portable>/Surge Synth Team/SurgeXTData/patches_factory/Scoring 1000` |
| Linux portable content archive | `src/cmake/lib.cmake:148-164` complete resources/data copy | same relative portable layout |
| All pluginsonly ZIP/tar archives | src/cmake/lib.cmake archive plugin product directory before adding content | Factory bank absent by design; needs full installer or companion content archive |
| Windows ARM64/ARM64EC / JUCE7 archive | lib.cmake explicitly skips full content portable ZIP and installer | plugin-only; document need for companion data download if delivering these |

Developer install-resources-local targets in `resources/CMakeLists.txt` copy data with rsync --delete into `$HOME/.local/share/surge-xt/` on Linux and `$HOME/Library/Application Support/Surge XT/` on macOS. Do not run these against a user's live folder for a test; stage in an isolated directory.

Native discovery: `SurgeStorage.cpp:974` scans patches_factory. `refreshPatchlistAddDir` at 1157 supplies case-insensitive .fxp filter to recursive scanner. Linux/Windows portable discovery searches ancestor paths for SurgeXTData; SURGE_DATA_HOME overrides resource root at ~2015. Thus exact installed-copy engine test can set SURGE_DATA_HOME to the isolated staged root and verify count/load paths through native storage.

## Minimal integration changes recommended

1. Commit 1000 real FXP assets, deterministic authoring source and a manifest of relative path, category, SHA256, unique ID and musical role. Count exactly 100 in each of 10 folders. Keep licenses and preset comments appropriate.
2. Add a small `cmake/ValidateScoringBank.cmake` or equivalent dependency of `surge-xt-distribution` / `surge-staged-assets` that fails on missing manifest entries, count mismatch or hash mismatch. Pure CMake avoids making Python mandatory for normal native compilation. `file(SHA256)` can validate manifest rows. Path strings must be quoted because the bank/category names contain spaces.
3. A separate `.github/workflows/scoring-presets.yml` with read-only contents permission should run on pushes/PRs affecting the bank, its generator/specs, validator, native renderer or core loading/DSP sources; support workflow_dispatch. Run deterministic rebuild/no-diff, static checks, native renderer five-pass reports, then stage factory data and recheck exact path set and hashes. Upload JSON/CSV reports and sample WAVs with `if: always()`; job should still fail when a validation fails. Avoid pretending artifact upload means success.
4. Add a quick validator step to `build-release.yml` immediately after each checkout, before distribution compilation, or establish a prerequisite validation job. Existing workflow already triggers on bank changes and automatically produces Nightly full installers from main; bank path is not ignored.
5. On Linux CI inspect the *actual packaged .deb*: `dpkg-deb -x` to a temporary path, then run count/hash validator with that data root. If Windows/macOS CI cannot fully install, validate extracted package payload where feasible and report that difference honestly. Checking only source copy rules is not an install test.
6. For user-accessible standalone bank, a supplemental ZIP containing `Scoring 1000/<categories>/*.fxp` plus README/license is useful and small; use current native factory source as input. It is supplemental, not a substitute for full installer verification.

## Scope cautions

- User asks "bake into build": normal full-install factory resources satisfy installation integration. Avoid changing every plugin-only archive to carry hundreds of MB of unrelated factory data.
- If guaranteed availability in Windows *minimal* install is required, add a specific bank `Source ... Scoring 1000\*; Components: Data; Flags: recursesubdirs` entry and exclude bank from the general Patches entry to avoid duplicated content; otherwise document current Patches component requirement. Root can choose scope.
- No native UI layout change needed, so the expensive ten-skin screenshot suite is unrelated to this preset-only change. Preserve prior UI work and do relevant schema/source checks.
- Current build-release.yml creates unsigned macOS fork builds when signing secrets absent; do not claim signed/notarized output without observed job evidence.
