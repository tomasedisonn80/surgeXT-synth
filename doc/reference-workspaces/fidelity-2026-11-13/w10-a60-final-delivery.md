# Wave 10 / assignment 60 — final native and delivery verification

**Result: PASS for the exact-source Linux native gate and published gallery.**
The accepted source is
[`6028da1b15b3263e459bbd7742d2fd0572adcc89`](https://github.com/tomasedisonn80/surgeXT-synth/commit/6028da1b15b3263e459bbd7742d2fd0572adcc89),
tree `3e485f76c82fdab623f0078233eb6655e0ca2fb0`, tested by
[native run 43504150637](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43504150637),
attempt 1. Independently read the workflow metadata, job status and decoded
logs. Native job `132157292601` and gallery job `132159586465` both completed
successfully. Publication logs show the ordinary push from `6028da1` to
[`e2f26e4dc711e1127e66b9aae92b1e94afd15345`](https://github.com/tomasedisonn80/surgeXT-synth/commit/e2f26e4dc711e1127e66b9aae92b1e94afd15345).
The GitHub main branch and the coordinator's fetched checkout both identified
that gallery commit during verification.

This report completes the sixtieth scoped assignment. Final README/ledger
reconciliation was pending at the initial native-evidence handoff; the final
documentation check below confirms its subsequent completion. The coordinator's
documentation publication remains the final step, separate from this native pass.

## Executed native gates

| Gate | Independently verified result |
| --- | --- |
| Strict manifest validation | All ten workspaces passed; schema self-test passed |
| Actual native target | CMake configuration, compilation and linking of `surge-xt-tests` succeeded |
| Behavior | **47 cases / 10,395,101 assertions passed** |
| Native capture | **1 case / 24,101,112 assertions passed**, covering all ten baseline/edited pairs |
| Normal AudioTap | PASS: boundaries, wrap, stalls, stereo ordering, lifecycle and no audio allocations |
| ThreadSanitizer AudioTap | Separate PASS for the same stress coverage; no sanitizer race or fatal result |
| Comparison and publication | Same-source comparison passed; validated gallery was pushed to main |

The committed source's native test declarations retain all **44** behavior
cases from the preceding accepted `ea674878` checkpoint. The three additional
cases are:

- **Native modulation ranges follow base edits and state restore on the first
  refresh:** all-ten live formatter, native edit, host update and saved-state
  restore coverage; ClearSignal inspector raster restoration; scene, RNG,
  dirty-state and host-value isolation; zero-depth drag continuity.
- **Busy native route snapshots preserve selection without leaking across
  scenes:** actual routing-mutex contention, same-scene selection/row retention,
  active-edit amount retention, public/direct scene switching and genuinely
  empty snapshot clearing.
- **Spectral terrain frames one source floor without magnifying silence:**
  silent/tiny/nonfinite input, extreme finite values and clipping, one-frame
  selection, common DC translation and the actual native raster path.

The exact case count in the passing log matches that independently derived
source inventory. The retained behavior/capture logs also match the decoded
GitHub job summaries. No assertion threshold, failure handling or test filter
was relaxed to obtain this result.

## Failed checkpoint retained explicitly

Source `21c2a82813c4e57332bff19cf134a78ac9fe013d`,
[run 43502323088](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43502323088),
compiled and passed its capture and both AudioTap stages, but behavior passed
only **46/47 cases**, with **10 failures among 10,395,101 assertions**. All ten
failures were the same stage-2 restored-slider comparison at the new inspector
regression: displayed `0.618` versus full-precision processor
`0.6175675392`. Its gallery publication job was skipped. That run is not an
accepted final checkpoint.

Independent source review confirmed `BoundSlider`'s existing normalized
`0.001` interval. The correction compares the displayed value with its actual
`NormalisableRange::snapToLegalValue` result using **exact equality**, without
enlarging a tolerance. All other assertions remain. The same replacement
source also reduces only Bauhaus blue-paper coarse amplitude from `1.5` to
`0.25`, following the broader native-image finding in assignment 56.
The entire native gate was then rerun at `6028da1`; neither the failed
`21c2a82` captures nor the earlier `ea674878` images certify the final source.

## Provenance and artifact integrity

[Capture evidence](../native-gallery/capture-evidence.json),
[build provenance](../native-gallery/build-provenance.txt),
[gallery manifest](../native-gallery/gallery-manifest.json), and both gallery
pages agree on source `6028da1` and run `43504150637`. Capture time is
**2026-11-13T20:47:19.186Z**. The source was clean at configuration,
`complete=true`, and expected/completed capture counts are both ten. The
fixture uses the real processor at **48 kHz / 512-sample blocks**.

Independently checked:

- All **30 files** listed in the gallery manifest match their byte counts and
  SHA-256 hashes.
- All **20 native baseline/edited PNGs** decode completely, measure
  **1586 × 992**, and match both their file records and per-capture hashes.
  The identifiers cover each numbered workspace exactly once.
- All **10 separately labelled reference PNGs** decode at the same dimensions
  and match their recorded hashes and exact-source repository bytes.
- Decoded comparison of every baseline/edited pair reproduces its **exact
  recorded changed-pixel count**, including the separately recorded filter
  response rectangle. Whole-image changes range from **3,666 to 63,866 pixels**.
- Every capture's manifest and referenced playable-source bytes match the
  accepted source. Real before/after parameter values differ, and all captures
  retain the recorded 100%, 125% and 150% layout coverage.
- The publication job downloaded native artifact `14075652001` from this same
  run with expected ZIP digest
  `9cffa3baa8a6e394d49efede67f082b153e60b07a96a7b572faa5b9f2ee7c49b`
  and required digest checking. Its logs confirm validation and copying of
  twenty unmodified native PNGs before the main push.

Fresh ClearSignal inspection independently confirms the original stale-display
defect is corrected. Its physical inspector changes from
`11.62 < 1420.00 > 50175.42 Hz` to `0.94 < 115.14 > 14071.87 Hz` after the
real cutoff edit while normalized Amount remains **+0.64**. The inspector
rectangle `(1197,548)–(1576,759)` contains **1,013 changed pixels** in this
final pair. This is a scoped state/display check, not a whole-image fidelity
measurement.

## Independent audio evidence

All **40** baseline, edited, host-updated and restored main-output peaks are
finite and strictly within `(0.0001, 1.0)`. Their full-precision range is
**0.040551204234362–0.654747426509857**. All **10** reset peaks are finite and
below the unchanged **1e-6** limit; maximum
**6.643543049023037e-9**. Values below are rounded for readability; the linked
capture evidence retains their original precision.

| Skin | Baseline | Edited | Host updated | Restored | Reset | Changed pixels |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Obsidian | .331432 | .133050 | .170459 | .228408 | 5.318e-9 | 10,993 |
| Ivory | .163720 | .075321 | .086511 | .089716 | 0 | 14,169 |
| Spectral | .408096 | .194364 | .230787 | .246296 | 5.430e-9 | 10,779 |
| Bauhaus | .528082 | .345038 | .128394 | .362399 | 5.324e-9 | 4,329 |
| Space | .654747 | .471947 | .449583 | .548910 | 5.248e-9 | 3,666 |
| Tokyo | .456736 | .288120 | .358170 | .371355 | 5.407e-9 | 5,970 |
| Aurora | .508732 | .372727 | .376830 | .328467 | 6.644e-9 | 4,205 |
| ClearSignal | .086792 | .058002 | .078119 | .054449 | 3.397e-9 | 63,866 |
| Cartographer | .212810 | .149817 | .177027 | .197622 | 0 | 28,960 |
| Live | .124704 | .067688 | .040551 | .061716 | 5.321e-9 | 25,248 |

Source inspection confirms independent per-stage maxima, finite-sample
assertions on all six rendered channels, peak measurement on the first two
main-output channels, and preserved host-reset voice/tail assertions. Valid
images alone are not the basis of this audio result.

## Assignment count, links and implementation consistency

Before this report was written, the renewed-pass directory contained exactly
one report for each assignment **1–59**, with correct six-per-wave numbering
and no duplicates. This report supplies **60**; historical assignment records
outside this directory are excluded. The existing ledger contains the sixty
numbered scopes and is awaiting the coordinator/A58 final status reconciliation
using this completed handoff. Completion counts refer to actual scoped work,
not sixty simultaneous workers or sixty native test cases.

Checked the user guide, current-pass record, ledger and native gallery's local
links; the gallery HTML resolves its original, baseline, edited and evidence
files. Also reviewed the coordinator's five-line root README addition linking
the gallery and current record. The guide and corrected ClearSignal/Live FX
headings agree with the accepted source and its real eight-slot scope.

The accepted source is an ancestor of fetched HEAD and origin/main. All
subsequent committed differences are gallery documentation. Remaining local
changes at this handoff are the reviewed root README, renewed-pass
documentation and this report; no implementation, resource, script or build
change is unfinished. Final documentation-only descendants do not constitute
a different tested implementation. The coordinator will publish those
documents after the final count/link reconciliation.

## Separate platform status and visual scope

At **2026-11-13 20:49:09 UTC**, the exact-source
[platform workflow 43504150486](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43504150486)
had completed version setup successfully. Ubuntu 20 Docker, Ubuntu ARM,
Windows 64-bit, Windows ARM64, Windows ARM64EC and Windows MSVC JUCE 7 builds
were in progress; macOS was queued. No completed final platform-package
matrix or installer publication is claimed here. Later documentation-triggered
platform runs are distinct from this source's run. These optional jobs do not
block the completed Linux native evidence gate.

This acceptance covers the native JUCE component attached to the actual
offline processor, mandatory native behavior/capture/tap gates, exact source
provenance and published artifact integrity. DAW/device operation and complete
visual equivalence are separate questions. The coordinator's final
Spectral/Bauhaus/Aurora and edited ClearSignal review accepted the intended
corrections. [Assignment 56](w10-a56-final-material-clearance.md)
also independently accepts the final blue material and Width clearance on this
same source, retaining the measured texture/palette residuals in its report.
Source-driven topology, native capability adaptations,
remaining font/material differences and the absence of a literal pixel-identity
claim remain explicit in the guide and scoped visual reports.

This assignment wrote only this report in the repository. It made no
implementation edit, local native-build claim, commit, push or gallery
replacement. Final documentation reconciliation/publication is the next
coordinator handoff.

## Final documentation reconciliation check

After assignment 58 released the final records, independently verified exactly
**60 actual numbered reports**, six per wave, and **60 unique completed ledger
rows**, each linking to its matching report. All **182 actual local Markdown
links** across the renewed-pass documents, root README, continuation, user guide
and native-gallery README resolve; the ledger's
`README.md#exact-native-checkpoints` anchor also matches the current heading.

The final README, ledger and assignment 58 report agree on source `6028da1`,
run `43504150637`, gallery `e2f26e4`, and the full behavior/capture counts.
Failed checkpoints, report-specific visual evidence boundaries, remaining
texture/native-capability differences and the separate timestamped platform
status remain explicit. No stale pending completion field remains in the
current README or ledger, and no literal pixel-identity claim was introduced.

All seven pending files are the expected root README or `doc/` records. The
accepted implementation and gallery remain unchanged, HEAD and origin/main
still identify `e2f26e4`, and `git diff --check` passes. This check completes the
documentation reconciliation that was pending in the earlier handoff sections;
the coordinator owns the ordinary final documentation-only publication.
