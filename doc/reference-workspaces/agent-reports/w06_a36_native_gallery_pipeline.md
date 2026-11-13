# Wave 6 / assignment 36: publish verified native gallery

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Result

Implemented a same-workflow publication path for ten baseline/edited native screenshot pairs.
Changes are unstaged; the parent owns the repository commit and push.
No real repository commit, branch mutation, or external publication was performed by this assignment.

## Owned files

- scripts/reference-workspaces/publish_native_gallery.py — new standard-library generator and artifact validator.
- .github/workflows/reference-workspaces.yml — new publish-native-gallery job.
- doc/reference-workspaces-user-guide.md — committed-gallery documentation.

The generator emits 31 files: 20 untouched native PNGs, eight original evidence/log files,
a comparison HTML page, a GitHub-renderable README, and gallery-manifest.json.

## Publication controls

- Publication needs the complete native-screenshots job to succeed.
- Only trusted main runs from tomasedisonn80/surgeXT-synth can publish.
- Workflow/build defaults remain contents: read.
- Only the publication job has contents: write.
- A dedicated actions/checkout@v6 checks out the exact successfully tested SHA.
- The source must still equal origin/main before download and immediately before commit.
- actions/download-artifact@v8 selects the exact native-reference-workspaces-SHA artifact from the same run.
- Digest mismatches fail the download.
- The generator requires complete evidence, exactly ten known unique IDs, a clean configured source matching the expected SHA, the matching workflow URL, and passing native/audio/manifest/schema logs.
- Native image names and source manifest paths are fixed allowlists.
- Both images per skin must exist and differ; each PNG signature, dimensions, chunk checksums, and termination are validated.
- Reference concept bytes are loaded from the capture source commit and must match the checked-out original files.
- Only the gallery directory is staged.
- Publication uses an ordinary fast-forward push, never force.
- A competing main update skips obsolete publication. A push failure while main remains unchanged fails the job.
- Gallery-only changes are outside workflow path filters, preventing a publication loop.
- No new secret, token handling, signed URL, PR, comment, or messaging integration was added.

## Gallery presentation

Every skin has separately labelled Reference concept, Native baseline, and Native edited state images.
The HTML compares the supplied reference and native baseline, with an expandable edited image.
The README provides all ten comparisons as links and renders all ten native baseline PNGs on GitHub.
The HTML is portable with its sibling references directory; it is not a GitHub Pages deployment.
Every gallery records the source commit, native workflow URL, capture time, canvas dimensions, and SHA-256/size metadata.
Stored PNG bytes are copied unchanged. CSS only controls their displayed size.
The native component scope and lack of a pixel-identity or DAW/OS desktop certification are explicit.
No fidelity percentage, fake native frame, or embedded image data URI is generated.

## Positive validation on the real successful native artifact

Input: /vercel/native-22b9c29
Output: analysis-only/gallery-test
Source: 22b9c299e6aa74093c9e96677cb32df5f2f74e74
Run: https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049170

- 20 native PNGs, all 1586 × 992, successfully decoded.
- 31 total output files; total size 11,295,308 bytes.
- All 20 image hashes match the input artifacts byte for byte.
- All 30 other-file entries in gallery-manifest.json have correct hashes and byte sizes.
- Exactly ten source/native/edited comparison groups and 30 HTML image elements.
- Every local image, document, and anchor link resolves in the intended repository layout.
- Two generation passes produce identical file hashes.
- Evidence retains 33 behavior cases / 9,988,099 assertions and 24,095,778 capture assertions.

## Negative validation

31 malformed-input cases were rejected with exit status 2 before changing a sentinel output directory.
Coverage includes incomplete/dirty/wrong source evidence, missing/count/duplicate/unknown IDs,
unsafe filenames and manifest paths, missing baseline/edited PNGs, invalid PNG signatures,
wrong image dimensions, corrupt checksums, identical edit images, missing edit evidence,
wrong provenance source/run, failing or absent behavior/capture/audio/schema/manifest pass logs,
duplicate JSON fields, invalid CLI source/run values, and linked native images.
Detailed outcomes: analysis-only/w06_a36_gallery_negative_checks.json

## Workflow validation

The workflow parses successfully; all 12 shell steps pass bash -n.
Structural assertions confirm the native dependency, main/repository restriction, exact named same-run artifact,
digest enforcement, permission split, and exclusion of gallery paths from trigger filters.
A temporary local Git fixture verified successful fast-forward publication, gallery-only staging,
both stale-source skip points, and failure propagation when a rejected push leaves main unchanged.
No remote GitHub repository was written by the fixture.
Detailed outcomes: analysis-only/w06_a36_gallery_workflow_checks.json
Python syntax and git diff --check passed.

## Version/API reference

The primary actions/download-artifact README was checked:
https://github.com/actions/download-artifact
It documents @v8, same-run name/path downloads without a separate GitHub token,
and digest-mismatch: error.

## Remaining integration

The parent must commit and push the three owned source/documentation files.
The next successful native workflow for a still-current main source will commit its own fresh gallery.
The historical dry run proves the generator against real passing native captures; it does not certify
future renderer changes or assert visual identity with the supplied concept art.
