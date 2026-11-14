# Standalone preset-bank package

Current builds of this fork already embed all 1,000 presets and ten reference
skins. **No separate preset archive is needed for those builds**, including
plugin-only outputs. See [built-in content](BUILT_IN_CONTENT.md). This page
documents the optional compatibility package for an existing installation.
For the synth binaries themselves, use the
[platform build downloads](BUILT_IN_CONTENT.md#build-downloads).

## Ready-to-import archive

[Scoring-1000.zip](downloads/Scoring-1000.zip) contains all **1,000 presets** in
ten category folders, with the complete catalog and installation/licensing notes.
The archive is **4,485,424 bytes** (about 4.49 MB).

[ZIP checksum](downloads/Scoring-1000.zip.sha256) ·
[Package verification metadata](downloads/Scoring-1000.zip.metadata.json)

SHA-256: `72692ee39f464c66a475af4ffffb92607d92924f8d15181f7da74a5bd5723192`.

The repository's full factory-data installers also include **Scoring 1000** in
the normal resource tree. The standalone package helper provides a user-bank ZIP
for an older or upstream Surge XT installation, independently of the embedded
content in current fork binaries.

## Generate the package

From the repository root, run:

```bash
python scripts/scoring/package_bank.py --output /path/to/Scoring-1000.zip
```

The helper requires the complete bank, exact catalog hashes, ten categories of
100, self-contained native patch definitions, and reproducible regeneration. It
then verifies the bytes and CRC of every archived entry. It writes:

- `Scoring-1000.zip` — 1,000 FXPs, complete catalog, installation notes, CC0 legal
  text and provenance notice, archive-path mapping, and an internal checksum index.
- `Scoring-1000.zip.sha256` — checksum of the complete ZIP.
- `Scoring-1000.zip.metadata.json` — archive size, input hashes, counts and the
  precise scope of packaging verification.

Entries are sorted and have fixed timestamps and regular-file permissions. The
same input bytes and compression implementation produce the same ZIP bytes.
Compression libraries can differ between environments, so cross-environment
reproducibility should be checked rather than assumed. Packaging does not change
the preset files, normalize audio, or add an artistic approval claim.

No finished ZIP is asserted by this document alone. Use the artifact and checksum
published for the release you select, or generate them from the exact source
revision you intend to use. Any later preset or catalog change requires a fresh
package and checksum; the package metadata binds its exact inputs.

## Install the standalone bank

1. Extract the ZIP to a temporary folder.
2. In Surge XT's patch-selector context menu, select **Open User Patches Folder...**.
3. Copy the extracted **Scoring 1000** folder into that location, preserving its
   ten numbered category folders. Keep unrelated patches and personal edits in
   their existing folders.
4. Refresh the patch browser or restart the synth, then open the user collection.

The native user-patch location is the `Patches` folder beneath the configured user
data directory. The menu respects custom and portable locations. This installation
route adds the bank to user patches. It does not modify an existing plugin
binary. Current fork binaries separately carry their own embedded factory copy.

The ZIP's complete `manifest.json` retains repository source paths; `PACKAGE.json`
maps each stable preset ID to its archive path. `CONTENTS.sha256` hashes all payload
entries except itself. The external `.sha256` file covers the whole ZIP.

## Licensing and review scope

The original preset data and descriptive catalog are offered under **CC0-1.0**,
matching their native metadata. The container is initialized from Surge's CC0
Init Saw. The package includes the complete CC0 legal text and a notice explaining
that Surge XT, its dependencies, and the project's packaging/compiler programs
retain their own repository licenses; no synth binary or program source is in
the standalone data ZIP.

See the [player guide](USER_GUIDE.md) for macros, range, gain and genre adaptation,
and the [research](RESEARCH.md) for the primary-source basis and its limitations.
Package verification establishes complete, reproducible preset delivery. It does
not substitute for the separately published native audio evidence or human
listening in a real cue.
