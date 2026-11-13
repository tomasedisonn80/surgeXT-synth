# Wave 8 / assignment 48 — exact-source native acceptance

**Result: PASS for source `ea674878313006afa0b9d1a2b1d3495b69755627`.**
Independently inspected GitHub run/job status and decoded logs for
[native run 43497923810](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43497923810),
attempt 1. Native job `132135796536` and gallery publication job `132138456507`
both completed successfully. Publication logs confirm the actual main push to
[gallery commit `762d888f7159558db472836a8f9b5f015c1f9427`](https://github.com/tomasedisonn80/surgeXT-synth/commit/762d888f7159558db472836a8f9b5f015c1f9427).
After the coordinator fetched that commit, this assignment independently checked
its evidence and files. Earlier `0708bc7` captures were not used for acceptance.

## Executed gates

- Strict manifest validation: all ten workspaces passed; schema self-test passed.
- The real native synthesizer test executable configured, compiled and linked.
- Behavior: **44 cases / 10,345,938 assertions** passed, including the new
  placement/pointer-routing case for four controls at 100%, 125% and 150%.
- Capture: **1 case / 24,101,112 assertions** passed. The twenty additional
  assertions explicitly reject silent edited and host-updated output for all
  ten skins; the existing headroom and reset thresholds are unchanged.
- Normal and ThreadSanitizer AudioTap runs independently reported PASS for
  boundaries, wrap, stalls, stereo ordering, lifecycle and no audio allocations.
  No race or fatal-error result appears in either retained tap log.

## Published evidence integrity

[Capture evidence](../native-gallery/capture-evidence.json),
[build provenance](../native-gallery/build-provenance.txt), and
[gallery manifest](../native-gallery/gallery-manifest.json) agree on the exact
source and run above. Capture time is **2026-11-13T20:07:32.689Z**;
`sourceDirtyAtConfigure=false`, `complete=true`, and expected/completed counts
are both ten. The fixture is the real processor at 48 kHz / 512-sample blocks.

All **twenty baseline/edited native PNGs** decode correctly, measure
**1586 × 992**, and match their recorded SHA-256 hashes. All **thirty files**
listed in the gallery manifest match both byte counts and hashes. The ten
separately labelled reference-image hashes also match. Independently comparing
each decoded baseline/edited pair reproduces its exact recorded changed-pixel
count; every real cutoff edit changes the image (range **3,666–62,853 pixels**).
Retained behavior/capture log summaries agree with the decoded GitHub logs.

## Independent audio evidence review

All **forty** baseline/edited/host-updated/restored main-output peaks are finite,
exceed **0.0001**, and remain below **1.0**. Range:
**0.040551204234362–0.654747426509857**. All ten reset peaks remain below
**1e-6**; maximum **6.643543049023037e-9**.

| Skin | Baseline | Edited | Host updated | Restored | Reset |
| --- | ---: | ---: | ---: | ---: | ---: |
| Obsidian | .331432 | .133050 | .170459 | .228408 | 5.318e-9 |
| Ivory | .163720 | .075321 | .086511 | .089716 | 0 |
| Spectral | .408096 | .194364 | .230787 | .246296 | 5.430e-9 |
| Bauhaus | .528082 | .345038 | .128394 | .362399 | 5.324e-9 |
| Space | .654747 | .471947 | .449583 | .548910 | 5.248e-9 |
| Tokyo | .456736 | .288120 | .358170 | .371355 | 5.407e-9 |
| Aurora | .508732 | .372727 | .376830 | .328467 | 6.644e-9 |
| ClearSignal | .086792 | .058002 | .078119 | .054449 | 3.397e-9 |
| Cartographer | .212810 | .149817 | .177027 | .197622 | 0 |
| Live | .124704 | .067688 | .040551 | .061716 | 5.321e-9 |

Source review confirms independent per-stage maxima, finite-sample checks on
all six channels, main-output peak measurement on the first two channels, and
the preserved host-reset voice/tail checks. Spectral and Bauhaus values above
come from the new source-family assets in this run.

## Scope and handoff

This certifies the exact Linux native component/offline-processor gates above.
Platform workflow `43497923918`, DAW/device use and reference visual fidelity
are separate acceptance evidence. Passing tests and valid PNGs do not establish
pixel identity; Wave 9 should compare these fresh native captures with the
supplied references. This assignment changed only this report; it performed no
local native build, implementation edit, commit, push or gallery replacement.
