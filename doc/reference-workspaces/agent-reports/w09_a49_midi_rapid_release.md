# Assignment 49 — Rapid MIDI spring gestures

> Curated historical assignment report. The rapid spring-ordering change was subsequently included in a9d0d0a and passed its native behavior/capture gates. This does not certify later filter/hero changes or manual host behavior. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Defect and scope

The previous lane keyboard fix correctly used applied input after audio had consumed a spring release. It still started a new key gesture from the old applied value when release and the next press both arrived before an audio block. For example, applied Pitch 256, release, then Shift+Up would queue 272 rather than 16. This was identified from the native event path; no failed native run of the new regression is claimed here.

The fix covers actual Pitch bend and channel aftertouch inputs. Existing Mod/Expression behavior, MIDI units, input ranges, native queue ordering, host parameter behavior, audio algorithms and reference materials are preserved.

## Implementation

- `SurgeSynthProcessor.h/.cpp`: `queueMidiController` optionally returns the accepted packet sequence. Existing callers retain their boolean API through the default null output.
- Two atomic acknowledgements publish the most recent GUI Pitch/Touch sequence only after the real native controller input has been applied. Their release/acquire ordering also orders observation of the native applied-value atomics. A newer applied packet acknowledges an older superseded release.
- Emergency zero releases retain the existing bounded fallback path. Stale packets remain rejected by the existing audio-only sequence checks. Acknowledgements publish for both ordinary and emergency deliveries.
- `MidiController` remembers its latest accepted owned spring sequence and value. A fresh key gesture uses that pending value until audio acknowledges it; subsequent input then starts from actual applied state, including incoming host MIDI. Repeated keys accumulate the active gesture target.
- The Workspace edit was prepared as a class-bounded patch and applied only after assignment 47 released its separate linear-control changes. Those changes and the coordinator's Clear/Flow paint edits were preserved.

There are no new audio-thread allocations, locks, timers, sleeps or queue-depth guesses. The additional audio work is one bounded atomic store for each applied GUI spring packet or fallback.

## Focused native regression

Added one test case to `XTTestReferenceMidi.cpp`, exercising both Pitch and Touch:

1. Release and two fine key presses before any intervening audio block or UI refresh.
2. GUI release followed by incoming host MIDI in the same audio block; the next key must start from the host value without requiring a timer to observe an intermediate zero.
3. A newly accepted absolute accessibility value followed immediately by a key press.
4. Saturated GUI queue, guaranteed spring-zero fallback, one newly free queue position, then a newer accepted gesture that must win.

All prior tests and assertions are retained without relaxed thresholds.

## Validation and publication status

- Strict validator: all ten manifests pass, with the two previously known intentional Clear corner-selector warnings.
- Schema self-test: passes, including 28 accepted / 140 rejected shared schema cases.
- `git diff --check`: passes.
- The bounded patch script parses successfully; targeted post-application source invariants pass.
- C++ compilation and linked native runtime verification remain pending the coordinator's workflow. No source-only check is represented as a runtime pass.
- Changes remain unstaged. No commit or push was made by this assignment.

Files: `src/surge-xt/SurgeSynthProcessor.h`, `src/surge-xt/SurgeSynthProcessor.cpp`, the `MidiController` class in `src/surge-xt/gui/reference/Workspace.cpp`, and `src/surge-xt/xt-tests/XTTestReferenceMidi.cpp`.

Detailed source hashes and checks: `analysis-only/w09_a49_validation.json`.
