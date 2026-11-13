# Wave 6 / assignment 31 — independent native gate

**Result: PASS for Wave 5 source `0708bc76bb194a127064cf18b8849cf77373a7e7`.**
Independently read GitHub run/job status and decoded logs for
[run 43493343847](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43493343847),
then inspected the fetched evidence and twenty PNGs in gallery commit
`facc04210a6c93e8ef846d31cce3e7c65649b3d0`. Both native and publication jobs
completed successfully. No local build, source edit, commit, push or gallery
replacement was performed by this assignment.

## Executed gates

- Strict validation: all ten workspaces; schema self-test passed.
- Native synthesizer executable compiled successfully.
- Behavior: **43 cases / 10,317,666 assertions** passed.
- Capture: **1 case / 24,101,092 assertions** passed.
- Independent normal and ThreadSanitizer AudioTap executions each reported
  PASS, covering boundaries, wrap, stalls, stereo ordering, lifecycle and
  absence of audio allocations.
- Capture evidence names the exact source above, `sourceDirtyAtConfigure=false`,
  `complete=true`, and ten expected/completed records. All **ten baseline/edited
  pairs (20 PNGs)** are 1586 × 992 and match the gallery manifest SHA-256 values.
  Every pair records changed pixels following a real cutoff edit.

## Independent audio evidence review

All forty baseline/edited/host-updated/restored peaks are finite, exceed 0.0001
and remain below 1.0. Range: **0.040551204–0.654747427**. All ten reset peaks
remain below the unchanged **1e-6** threshold; maximum **6.643543049e-9**.

| Skin | Baseline | Edited | Host updated | Restored | Reset |
| --- | ---: | ---: | ---: | ---: | ---: |
| Obsidian | .331432 | .133050 | .170459 | .228408 | 5.318e-9 |
| Ivory | .163720 | .075321 | .086511 | .089716 | 0 |
| Spectral | .401133 | .220147 | .290680 | .319493 | 5.430e-9 |
| Bauhaus | .551698 | .415627 | .143239 | .386116 | 5.324e-9 |
| Space | .654747 | .471947 | .449583 | .548910 | 5.248e-9 |
| Tokyo | .456736 | .288120 | .358170 | .371355 | 5.407e-9 |
| Aurora | .508732 | .372727 | .376830 | .328467 | 6.644e-9 |
| ClearSignal | .086792 | .058002 | .078119 | .054449 | 3.397e-9 |
| Cartographer | .212810 | .149817 | .177027 | .197622 | 0 |
| Live | .124704 | .067688 | .040551 | .061716 | 5.321e-9 |

Each capture records the actual 17-note MIDI schedule. Source review
confirmed finite-sample checks, preserved headroom/reset thresholds and existing
gesture, scene-isolation, state-restore, MIDI and display/RNG coverage. Baseline
and restored nonzero output are asserted; edited/host nonzero output is recorded
and was independently checked here. New fader paint preserves native positioning;
terrain subdivisions use bounded owned snapshots behind the existing clip.

## Source distinction and limits

Earlier run 43490570167 at `411578a` failed compilation because JUCE could not
deduce the braced rectangle type. `2a5a621` supplies `Rectangle<float>` and passed
43491379963. Those results do not certify Wave 5; the exact run above does.
These are Linux native component/offline-processor results, independent of
platform-package, DAW, and visual-fidelity acceptance. Later source changes
require their own native gate.
