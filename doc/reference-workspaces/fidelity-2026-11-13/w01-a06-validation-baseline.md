# Wave 1 / assignment 06 — native validation baseline

## Scope and source

This assignment audited the existing native build, capture and gallery pipeline
for the new visual-fidelity continuation. It made no renderer, manifest, workflow,
test or remote repository changes. The checkout was
`a0d016fd59ef9aee1c96a720c6a2c5a0b9a56f35` when inspected.

Read: `AGENTS.md`, the continuation record, engineering log, user guide, native
workflow, capture test, gallery publisher and comparison tool. The previous
native evidence was independently checked through GitHub's workflow-run and
workflow-job endpoints, not inferred solely from checked-in documentation.

## Checks executed in this assignment

Both commands returned exit status zero:

```sh
python3 scripts/reference-workspaces/validate.py --strict-overlap
python3 scripts/reference-workspaces/validate.py --self-test
```

The ten validated manifests contain 194 declared panels and 779 declared controls.
These counts exclude controls generated internally by the native components.

| Skin | Declared panels | Declared controls |
| --- | ---: | ---: |
| 01 Obsidian Precision | 14 | 83 |
| 02 Ivory Atelier | 17 | 75 |
| 03 Spectral Observatory | 18 | 69 |
| 04 Bauhaus Modular | 25 | 79 |
| 05 Space-Age 1977 | 21 | 90 |
| 06 Tokyo Industrial | 18 | 92 |
| 07 Liquid Aurora | 21 | 99 |
| 08 ClearSignal Accessible | 27 | 77 |
| 09 Signal-Flow Cartographer | 11 | 51 |
| 10 Live Performance Deck | 22 | 64 |

The validator retained the existing two ClearSignal plot/control overlap
warnings: `panel.envelope.plot / envelope.mode` (79 × 24) and
`panel.lfo.plot / lfo1.shape` (60 × 21). It also reported the existing deliberate
19-control routing-plot overlay in Cartographer. These are not new failures;
new captures must still be inspected for clipping or obstructed controls.

The self-test passed valid/Unicode/duplicate-binding fixtures, 42 schema rejection
cases, 24 malformed-JSON rejections, 9 confined-path checks, and the shared 28
accepted / 140 rejected selection/widget cases. It also exercised declared
controls, individual tabs, gaps/unused cells, wavetable and MIDI-lane overlaps.
This is schema/layout validation, not native execution or visual approval.

## Independently verified previous native baseline

[Native run 43400507719](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719)
is completed with conclusion `success`, attempt 1, for source
`453df569c977350c3a4da2caf2f0ab7cbd1a0e71`. GitHub reports success for both jobs:

- **Linux native UI and parameter tests:** native configuration, compilation,
  behavior, captures, normal/ThreadSanitizer audio tap, reference comparison and
  artifact uploads all succeeded.
- **Commit verified native screenshot gallery:** source-current checks,
  same-run artifact retrieval, evidence validation and gallery commit succeeded.

The committed logs and gallery manifest agree on 42 behavior cases with
10,316,058 assertions, plus one ten-skin capture case with 24,100,990 assertions.
The twenty PNGs are 1586 × 992 native baseline/edited pairs. These results apply
to the previous source, not to any new modifications made in this continuation.

## Current native test contract

The workflow builds the real `surge-xt-tests` executable in Release mode, with
warnings still treated as errors, using pinned recursive submodules. It runs:

```sh
bash scripts/reference-workspaces/run-native-ui.sh \
  build/src/surge-xt/xt-tests/surge-xt-tests \
  '[reference-workspaces]~[reference-render]'
bash scripts/reference-workspaces/run-native-ui.sh \
  build/src/surge-xt/xt-tests/surge-xt-tests '[reference-render]'
```

The behavior group covers parameter/scene isolation, gestures, units and typed
entry, native undo and restore, editor switching, accessibility and focus,
browser/navigation actions, rendered slider positions, routing and FX power,
MIDI lifecycle and bounded queues, wavetable selection, preview/model isolation,
audio RNG preservation, analyzer timing/stereo behavior, schema parity and reset.

The capture case creates an actual `SurgeSynthProcessor` and native `Workspace`
for each manifest. It loads a documented showcase fixture, sends MIDI notes into
the real engine, feeds actual audio to the independent display tap, and captures
the component using `JUCE Component::createComponentSnapshot`.

For every skin it verifies:

- All manifest bindings resolve; values are finite; placed components exist,
  are visible and have positive dimensions. Value/top-label controls retain
  real sliders, accessible names and appropriate keyboard focus behavior.
- Baseline and edited snapshots use the exact 1586 × 992 canvas. A real cutoff
  control edit changes both rendered pixels and the independently computed
  native filter response by more than 1 dB.
- A host-notified cutoff edit is reflected by the native control. Native
  snapshots also render at 1.25× and 1.5× scaling without changing the patch.
- The baseline and restored patches produce finite nonzero audio with peaks
  above 0.0001 and below 1.0. Edited and host-updated stages are checked for
  finite audio and peaks below 1.0; their independent measured peaks are retained.
- Host reset removes voices and leaves a post-reset peak below the unchanged
  0.000001 threshold.

The Live fixture additionally checks applied real pitch/mod/expression/touch
values. Capture evidence is checkpointed after each skin and records source SHA,
configure-time dirty state, fixture, sample/block settings, image filenames,
changed-pixel count, filter-response change, audio-stage peaks and scale checks.
A complete evidence file is still not a passing test result: inspect the logs
and independent workflow steps.

## Publication and screenshot-review route

No workflow change is required for renderer, manifest or fixture-test edits.
The native workflow already watches these paths and triggers for `main` and
`work/reference-*`. A verification branch runs all native gates but skips the
gallery-writing job. A current `main` run publishes a normal child commit only
after the native job succeeds.

The gallery publisher requires a clean configured source with an exact matching
40-character revision; all ten baseline/edited PNG pairs; correct dimensions and
CRC-valid PNG structure; source-versioned reference/manifest agreement; complete
native evidence; unique passing behavior/capture summaries; passing normal and
ThreadSanitizer tap logs; and passing manifest/schema logs. It copies the native
PNG bytes without visual edits and records SHA-256 checksums.

The workflow checks `main` before and after generation and performs a normal
non-forced push. A competing main update therefore retains artifacts without
overwriting a newer gallery. Do not advance main with an unrelated documentation
commit while the desired main capture/gallery job is still running. Fetch the
gallery child commit before publishing subsequent documentation.

The comparison command is already available:

```sh
python3 scripts/reference-workspaces/compare_renders.py \
  --references doc/reference-workspaces/references \
  --captures PATH_TO_NATIVE_ARTIFACT \
  --output FRESH_COMPARISON_DIRECTORY \
  --expect-commit FULL_TESTED_SOURCE_SHA
```

It produces original/native pairs, raw difference images and contact sheets,
checks the native edited-image pixel count, and leaves inputs unchanged. Its raw
RGB MAE/RMSE and differing-pixel metrics are not fidelity percentages. Use the
unmodified individual native PNGs for delivery and separately labelled
comparison sheets for review.

## Minimal follow-up recommendations

1. Keep the existing workflow and assertions for this visual pass. Changes to
   shared paint code require a fresh complete behavior/capture/tap run, followed
   by inspection of all ten native baselines and edited images.
2. If source `.wt` assets alone are changed, note that
   `resources/data/wavetables/Reference Showcases/**` is not currently a native
   workflow path trigger. Editing their generator under
   `scripts/reference-workspaces/**` already triggers the job. Add the specific
   asset path only if a new asset-only commit route is needed; do not rely on
   an unrelated change to establish validation.
3. Add new targeted tests only for a concrete behavior or layout contract that
   changes, such as new native actions or a different pointer/hit region. Purely
   cosmetic paint refinements can use the existing behavior suite and fresh
   native captures without implementation-mirroring tests.
4. Preserve the exact tested revision in all screenshots and final reporting.
   Do not relabel the previous gallery as evidence for newly edited source.

## Local execution constraint

Read-only environment inspection found `g++` and `xprop`, but no `cmake`,
`ninja`, `Xvfb`, `xvfb-run` or `openbox`. ALSA, X11 and Freetype development
headers are also absent. No local native build was attempted or claimed.
The existing GitHub Actions environment already supplies these dependencies
and has a proven successful native capture route.
