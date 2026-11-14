# Current continuation: Scoring 1000

The current user goal adds a researched, original 1,000-preset factory bank in ten
categories, using 60 actual subagents in ten waves and five distinct technical QA
gates per preset. Preserve the existing ten reference workspaces described below.
Read `doc/scoring-1000/RESEARCH.md`, `doc/scoring-1000/USER_GUIDE.md`, and the scoring
verification/agent records before changing the bank.

- Edit the original recipes or deterministic compiler, then regenerate the FXPs,
  catalog and package index. Do not hand-edit generated presets or invent QA results.
- Run `scripts/scoring/validate_bank.py --check-generated`; use the compiled native
  `surge-scoring-render` and `qa_bank.py` for sound evidence. Cache reuse requires
  identical preset, fixture, renderer and QA-script hashes.
- Keep all eight performance macros connected and correctly described. Check
  held and released notes, velocity, register, dense polyphony, controls and tempo.
- Preserve separate status for human musical audition. Automated measurements,
  different hashes, and composer interviews do not certify Hollywood adoption,
  perceived uniqueness, acoustic realism or suitability for every cue.
- Full distributions install factory content from `resources/data`. Plugin-only
  packages require companion data; do not claim preset bytes are in the plugin binary.
- Record actual completed agent work, evidence hashes, known limits, and exact
  published commit. Use ordinary fast-forward publication to `main`; fetch and
  reconcile concurrent changes without overwriting unrelated work.

# Working on the ten reference workspaces

The active user goal is ten working native Surge XT skins matching the supplied
reference images, with all implementation and verification committed to `main`.
Read `doc/reference-workspaces-continuation.md`, the existing engineering log,
and the user guide before changing this feature. Preserve unrelated work.

## Implementation contract

- The image files in `doc/reference-workspaces/references/` are visual targets.
  Compare each skin at the 1586 × 992 logical canvas. Measure panel boundaries,
  control centres, typography, materials, state feedback and internal navigation.
- Every apparent control must either operate the real processor/UI, or clearly
  convey an unavailable operation. Use the existing parameter adapter, native
  undo, automation gestures, preset loader and scene isolation.
- Reference values, signal traces, meters, preset names and modulation routes
  must never be baked into the working UI. Decorative artwork is permitted only
  when it cannot misrepresent state; keep its provenance and precise scope clear.
- A view change, skin change or UI animation must not change the sound or consume
  the audible engine's random state. Keep audio-thread work bounded and free of
  UI allocations/locks.
- Keep native keyboard operation, focus, typed value entry and accessibility
  names intact. End active gestures and release held MIDI notes when appropriate.
- Native and Python manifest validation must agree on new fields. Verify actual
  placed child bounds as well as declared rectangles when changing layout.

## Verification and evidence

Run the strict manifest validator and relevant source checks before publishing.
Run native tests and capture all ten actual workspaces after shared renderer
changes. The `Native reference workspaces` workflow provides the linked native
build and a display-capable test environment. Syntax checks alone are not a
build or runtime pass. Never suppress a compiler warning or weaken a behavioral
assertion merely to obtain a green result.

Screenshots must come from the compiled native component attached to the real
processor. Retain the source SHA, fixture, capture conditions and test results.
An uploaded artifact can come from a failed job; inspect its independent results.
Reference images, diff images and native captures must be labelled distinctly.
Do not claim 1:1 identity based on broad composition or static validation; report
the measured remaining differences and the real engine adaptations explicitly.

## Continuation and delivery

Keep a short record of what changed, why, validation performed, exact published
SHA, and the next concrete task. Record actual agent assignments and results;
do not count planned or interrupted reviews as completed work. Respect the
runtime concurrency limit by running the six assignments of a wave in batches.
Integrate and review each batch before starting dependent work. Use ordinary
fast-forward publication to `main`; fetch and reconcile concurrent changes.
Commit screenshots and a browsable comparison gallery with the implementation.
