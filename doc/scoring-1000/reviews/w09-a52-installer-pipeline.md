# Wave 9 / Agent 52 — platform packaging and artifact inspection

## Outcome and scope

The platform packaging sources consistently place **Scoring 1000** in their
existing factory-data payloads. The new count/hash guard is a dependency of the
distribution, staged-assets, packaged-plugin, and resource-install targets. The
bank is verified at configure time and again before a direct CMake install.
No additional platform-specific list of 1,000 files is required because the
existing installers copy the factory tree recursively.

This review independently inspected those source paths and the prior successful
GitHub Actions run. **A platform installer containing the new bank has not yet
been claimed as built or inspected by this initial report.** The baseline run
predates the bank. The exact future run, commit, and downloaded artifact payload
must be recorded separately before reporting platform completion.

The accompanying [machine evidence](w09-a52-installer-pipeline-evidence.json)
records the reviewed source hashes, observed baseline jobs/artifacts, and archive
inspector checks. No workflow, bank, compiler, renderer, or synthesis source was
changed by this assignment.

## Which packages carry the bank

The principal routing is in [the packaging CMake module](../../../src/cmake/lib.cmake).

| Build/package | Bank delivery | Exact source behavior or limitation |
|---|---|---|
| Windows x64 setup EXE | Yes, with **Patches** selected | The [64-bit Inno definition](../../../scripts/installer_win/surge64.iss) recursively includes `resources/data/patches_factory/*` under `{commonappdata}/Surge XT/patches_factory`. Full and Compact select Patches; Custom can select it. Minimal omits Patches. Its required Data component contains Templates/Tutorials, not this bank. |
| Windows x64 portable-install ZIP | Yes | Copies all `resources/data` into `Surge Synth Team/SurgeXTData` alongside the plugins and applications. Retain the portable directory layout. |
| Windows x64 pluginsonly ZIP | No | The ZIP is created from staged binaries before the factory data is copied into the separate portable archive. |
| Windows MSVC JUCE 7 build | No data package | A nonempty `SURGE_EXTRA_ZIP_NAME` deliberately disables both the installer and portable archive. The build emits only its plugin ZIP; use companion data. |
| Windows ARM64 and ARM64EC builds | No data package | Their architecture branches deliberately disable the installer and portable archive. These builds emit plugin ZIPs; use companion data. |
| macOS full DMG/PKG | Yes | The [installer script](../../../scripts/installer_mac/make_installer.sh) runs `pkgbuild --root resources/data` for `Surge_XT_Resources.pkg`. The product distribution selects this resource component and disables deselection. Its [postinstall script](../../../scripts/installer_mac/ResourcesPackageScript/postinstall) moves the data into `/Library/Application Support/Surge XT`. |
| macOS pluginsonly ZIP | No | It archives the staged binary product directory separately from the resources package. |
| Linux x64 DEB and full tarball | Yes | [The DEB builder](../../../scripts/installer_linux/make_deb.sh) recursively copies the resource tree to `usr/share/surge-xt`; it also archives the resulting `usr` contents into the full tarball. |
| Linux ARM64 DEB and full tarball | Yes | [The ARM64 builder](../../../scripts/installer_linux/make_deb_aarch64.sh) uses the same resource layout. Linux ARM64 is not limited to plugin-only delivery. |
| Linux x64 RPM | Yes | [The RPM builder](../../../scripts/installer_linux/make_rpm.sh) copies the resources under `%{_datarootdir}/surge-xt` and includes that tree in `%files`. |
| Linux x64 portable-content tarball | Yes | Copies the complete resource tree into `Surge Synth Team/SurgeXTData`. Pair it with the appropriate plugin-only binaries. The ARM branch does not separately generate this portable-content archive. |
| Linux pluginsonly tarballs | No | They archive `surge_xt_products`, which contains binaries, not the factory data tree. |
| Ordinary Linux CMake install | Yes | The native packaging function installs `resources/data/` into `share/surge-xt`. The standalone resource-install targets likewise copy the whole data tree. |

These routes install factory files alongside the program. They do not compile
the preset bytes into VST3, CLAP, AU, or application binaries. The standalone
[bank ZIP](../DOWNLOADS.md) also provides a user-patch installation route for
existing synth installations.

## Workflow behavior and observed baseline

The [release workflow](../../../.github/workflows/build-release.yml) builds six
matrix jobs plus the Docker Linux job. `fail-fast: false` lets independent
platform builds finish if another platform fails. Main pushes that change the
actual factory files or CMake integration trigger these builds. Scoring review
documents and offline authoring tools alone are excluded; the dedicated scoring
workflow separately checks its tools, manifest, index, and engine inputs.

The current checkout's baseline was observed through the GitHub API:

- Run [43514380041](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43514380041),
  commit `dfdd2f94adec9c7bee4f3cfd4f757860fb875c3e`.
- Overall conclusion **success**; created 13 November 2026 at 21:51:34 UTC,
  updated at 22:37:40 UTC.
- Version setup and all seven build jobs succeeded: Windows x64, Windows JUCE 7,
  Windows ARM64, Windows ARM64EC, macOS, Ubuntu ARM, and Docker Linux.
- Seven nonexpired artifacts were listed. The macOS artifact was explicitly
  named `build-macOS-unsigned`; its baseline package was unsigned and unnotarized.
- Both publish jobs were **skipped**, as intended by the repository-owner guard.
  This fork uploads Actions artifacts; the workflow does not automatically
  publish an upstream release or send the upstream Discord/site notifications.

No baseline archive was downloaded merely to recheck its old bank contents. A
successful old run establishes prior pipeline operation, not success for the
new feature. Future fork macOS builds without complete signing credentials are
explicitly labelled unsigned/unnotarized by the workflow; check the actual run's
signing mode and artifact name.

## Exact payload inspector

[inspect_archive.py](../../../scripts/scoring/inspect_archive.py) verifies the
1,000 expected FXP paths and SHA-256 hashes in a ZIP or tar archive. Repeated
`--member` arguments select exact nested archive paths inside an Actions ZIP.
The program reads entries without extracting or executing their paths. It rejects
duplicate bank paths, missing/extra files, altered bytes, and nonregular bank
entries. It records the outer and selected inner archive hashes, catalog hash,
and helper hash. Presence is required unless `--expect absent` is explicitly used
to verify a plugin-only package.

```bash
python scripts/scoring/inspect_archive.py /path/to/actions-artifact.zip \
  --member exact-portable-install-name.zip \
  --output /path/to/portable-bank-verification.json

python scripts/scoring/inspect_archive.py /path/to/actions-artifact.zip \
  --member exact-pluginsonly-name.zip --expect absent \
  --output /path/to/plugin-only-verification.json
```

The helper supports ZIP and Python-supported tar compression formats. It does not
silently treat an EXE, RPM, DEB, DMG, or PKG as a verified archive. Use a supported
full/portable ZIP or tar payload, or separately inspect the platform-specific
container with the appropriate tools. The JSON result is an integrity check,
not an installer execution or plugin-host compatibility pass.

Nine focused fixture checks passed: direct complete ZIP, nested complete ZIP,
nested complete tar, explicit absence, and rejection of a modified preset,
missing preset, duplicated path, present bank when absence was requested, and
empty archive when presence was required. The positive cases each matched all
1,000 hashes. These fixtures use a self-contained copy of the previously generated
standalone ZIP and its own embedded catalog; their exact hashes are in the
machine evidence. Raw fixtures and results remain in `a52-archive-checks` beside
the repository. Syntax and whitespace checks passed.

## Remaining acceptance step

After publication, identify the release workflow by its exact `head_sha`, verify
every build job's final conclusion, and record the artifact IDs and names. Inspect
at least a data-bearing package against the bank catalog from the same source
revision, and report unsupported or unexamined platform containers accurately.
A successful upload step by itself does not establish that all jobs succeeded or
that an archive contains the correct preset bytes. Human musical audition and
DAW-host operation are outside this packaging review.
