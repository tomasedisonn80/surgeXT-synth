# Agent 59: committed-tree and delivery integrity

## Outcome and exact scope

The independent repository audit **passed** for local feature commit
`946ed442572d5fa29e6d6467813b4fa6cb150908`, tree
`2c39f27aeed1348497cffb1b3dc3f843fc823b76`, against baseline
`dfdd2f94adec9c7bee4f3cfd4f757860fb875c3e`. The audited bank catalog is
`67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`.

This review identifies the local Git tree. At the time of this audit, publication
through the authenticated connector was in progress; a remote commit may have a
different author/date and SHA while retaining this exact tree. Remote tree
identity and actual platform builds are separate checks, not assumed here.

Two isolated sparse checkouts used the committed objects. One enabled
`core.autocrlf=true` and `core.eol=crlf`; the other kept LF source bytes. The root
index, native renderer, presets, recipes, compiler and workflows were not changed.

## Results

| Check | Result |
|---|---:|
| Changed paths within the intended feature scope | 1,144; no unexpected paths |
| Existing files modified | 8 expected integration/documentation files |
| Scoring documentation/artifact files identical to Git blobs in CRLF checkout | 104 / 104 |
| Preset files identical to Git blobs in CRLF checkout | 1,000 / 1,000 |
| Published evidence checksum entries verified | 18 / 18 |
| Published audio/metadata checksum entries verified | 12 / 12 |
| Source-provenance hashes matched to committed blobs | 21 / 21 |
| Independent static validation and exact preset regeneration | 1,000 passed |
| Categories / unique active definitions | 10 × 100 / 1,000 |
| CMake package-integrity guard | Passed |
| Readable catalog regeneration | Byte-identical |
| Python source syntax | 13 / 13 files passed |
| Modified/new workflow YAML | Both parsed |
| Native-workflow shell blocks | 9 / 9 passed `bash -n`; all enable strict error handling |
| Isolated checkout status after checks | Both clean |

The detailed results and raw-evidence hashes are in
[w10-a59-repository-integrity.json](w10-a59-repository-integrity.json).

## Existing native workspaces were preserved

The complete Git trees for `src/common`, `src/surge-xt`, `resources/data/skins`,
`doc/reference-workspaces`, and `libs` are byte-identical to the baseline. This
preserves the existing common DSP, native UI implementation, skins, reference
assets and dependency revisions. The legacy workspace sections of both README
and AGENTS are also unchanged after their existing section markers.

The eight modified existing paths are `.gitattributes`, the release workflow,
README, AGENTS, the root CMake file, `resources/CMakeLists.txt`,
`src/CMakeLists.txt`, and `src/cmake/lib.cmake`. Other changes are confined to the
new bank, its tools, documentation, evidence, staging checks and native workflow.
No existing factory preset was changed by this feature commit.

## Exact bytes survive the CRLF checkout

The checkout genuinely exercised Git's conversion policy: ordinary Python source
files contained CRLF line endings. The protected catalog and checksum index
remained LF. The original primary and sustained CSV summaries each retained
**1,001 CRLF line endings**, matching their committed bytes and recorded hashes.
All 104 scoring documentation/artifact files and all 1,000 FXPs matched their Git
objects exactly. This resolves the concrete risk that Git normalization could
invalidate a published CSV checksum while leaving apparently identical text.

Source provenance was compared with the committed Git blobs. Ordinary source
files can have different checkout line endings without changing the preserved
report bytes; their checkout hashes must not be mistaken for the recorded LF
source hashes.

## Package reproducibility

The delivered standalone ZIP remains unchanged through the CRLF checkout:

- Size: **4,485,374 bytes**.
- SHA-256: `b794179791cfad604b2761e3c109aefd88821831c46ac168cb4048a2c69d41c4`.

Regenerating the package in the LF checkout produced a **byte-identical ZIP**.
The same Python/zlib runtime was used, so this establishes reproducibility for
identical inputs and compression implementation.

Regenerating from the CRLF source checkout also passed all static, regeneration
and archive-byte checks. Its 1,004 content entries were identical, including all
1,000 FXPs, the manifest, installation notes and licensing text. Only
`PACKAGE.json` and its entry in `CONTENTS.sha256` differed: the package truthfully
recorded the changed byte hashes of its CRLF generator and validator sources.
That is a source-provenance difference, not a changed preset payload or a damaged
download. Cross-runtime or differently encoded source inputs are not asserted to
produce the same outer ZIP checksum.

## Reproduce the central checks

In an isolated checkout of the audited commit, run:

```bash
python scripts/scoring/validate_bank.py --check-generated --output static.json
cmake -P cmake/verify-scoring-bank.cmake
python scripts/scoring/write_catalog.py --output CATALOG-regenerated.md
python scripts/scoring/package_bank.py --output Scoring-1000-regenerated.zip
```

Compare the regenerated catalog and package with the committed artifacts. For
line-ending verification, configure `core.autocrlf=true` before checkout, then
compare committed blobs and file hashes for the bank and `doc/scoring-1000`.
Use an LF checkout for the identical-source ZIP comparison.

This was Git line-ending emulation on Linux, not a Windows filesystem or plugin
execution test. No native DSP rerender, GUI/DAW-host test, installer execution,
human listening pass or composer endorsement is claimed by this audit.
