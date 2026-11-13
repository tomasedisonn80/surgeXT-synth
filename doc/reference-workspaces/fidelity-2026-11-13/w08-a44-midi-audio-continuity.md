# Wave 8 / assignment 44 — MIDI and audio continuity

**Source change ready for the next native gate.** Added only
`CHECK(editedAudioPeak > 0.0001f)` and
`CHECK(hostUpdatedAudioPeak > 0.0001f)` in the capture-stage subsection of
[`XTTestReferenceWorkspaces.cpp`](../../../src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp).
These close assignment 31's concrete coverage gap: both peaks were already
measured independently, checked below 1.0 and recorded, but silence could pass
those stages. Baseline/restored checks, all forty active-stage evidence fields,
the 1.0 headroom limit and the 1e-6 reset limit are unchanged. The shared block
helper still requires every sample on all six channels to be finite and measures
the peak only on the two main-output channels. No renderer or MIDI implementation
was changed.

## Capture phrase and lifetime audit

The source schedule matches all ten retained capture records exactly. It sends
17 integer-velocity note-ons and 14 explicit zero-velocity note-offs on channel 1,
at sample offset 0. Each block emits all due note-offs before its note-ons.
The present schedule has at most three gated notes; release/FX tails may outlive
their gates. Block numbers are zero-based, at 512 samples / 48 kHz.

| On block | Off block | MIDI pitch @ 7-bit velocity |
| ---: | ---: | --- |
| 0 | 72 | 48 @ 48 |
| 96 | 168 | 60 @ 58; 67 @ 53 |
| 192 | 264 | 72 @ 44 |
| 288 | 360 | 48 @ 64; 55 @ 57 |
| 384 | 456 | 55 @ 51; 62 @ 47; 69 @ 43 |
| 480 | 552 | 60 @ 60; 64 @ 55; 67 @ 50 |
| 576 | 624 | 48 @ 49 |
| 640 | 672 | 72 @ 42 |
| 704 | Host reset | 48 @ 70; 60 @ 76; 67 @ 73 |

Baseline measures 768 blocks (8.192 seconds); the final chord starts at
7.509333 seconds. Edit, host update and restored output each measure four new
blocks with their own zero-initialized maximum. No new MIDI or previous maximum
can substitute for their active output. The host stage sets the existing cutoff
adapter to 0.579; restore returns the original cutoff after 1.25× and 1.5× layout
snapshots. Capture notes enter the host MIDI buffer, while workspace cancellation
releases only keys owned by its keyboard component. Existing lifecycle coverage
also checks that hide/rebuild/destruction preserve an external note.

After restore, `processor.reset()` calls `stopSound()`: both scene voice lists
must be empty, and four fresh blocks must remain below 1e-6. This is the host
panic contract, including clearing effect tails, rather than ordinary note-off
envelope decay. The final chord is deliberately held until that reset.

## Live controller and gesture preservation

Live receives pitch wheel 14808 (signed +6616), CC1 104, CC11 56 and channel
pressure 85 before the first phrase note. Capture code checks all four applied
engine values after baseline and retains those inputs in evidence.
[`XTTestReferenceMidi.cpp`](../../../src/surge-xt/xt-tests/XTTestReferenceMidi.cpp)
already covers native target delivery/bounds, idle host values across skins,
parameter/dirty/RNG preservation, pointer/keyboard/typed input, owned spring
release during skin changes and destruction, saturation, and rapid release/new
gesture/host-input ordering. Pitch and Touch release only an owned GUI spring;
Mod and Expression remain latched. Readback uses applied processor atomics and
does not enqueue MIDI while refreshing or painting.

Source review of `MidiController`, `queueMidiController()` and
`processBlockMidiFromGUI()` confirms the bounded native MIDI route and ordered
spring acknowledgements. MIDI lanes do not use parameter automation gestures.
The cutoff/XY controls retain `ParameterBridge`'s native begin/change/end route,
stable target adapter and scene-change cancellation, with existing gesture
balance regressions. No implementation defect requiring a new test or fix was
identified by this audit.

## Verified evidence versus pending execution

Re-read the committed [capture evidence](../native-gallery/capture-evidence.json)
and [native behavior log](../native-gallery/native-tests.log), alongside
[assignment 31](w06-a31-native-gate.md). Those results certify source
`0708bc76bb194a127064cf18b8849cf77373a7e7`, gallery
`facc04210a6c93e8ef846d31cce3e7c65649b3d0`, native run 43493343847:
43 behavior cases / 10,317,666 assertions and one capture case / 24,101,092
assertions. Evidence reports a clean configured source and ten complete records.

A fresh independent read of all forty recorded active peaks found every value
finite and in (0.0001, 1.0): minimum **0.040551204234362**, maximum
**0.654747426509857**. All ten reset peaks remain below 1e-6; maximum
**6.643543049023037e-9**. Live's baseline/edit/host/restore values are respectively
0.124704226851463, 0.067687660455704, 0.040551204234362 and 0.061716269701719;
its reset peak is 5.321059592944266e-9.

This assignment performed source/evidence review, schedule consistency checks
and `git diff --check`; it did not run a new native build. The two added checks
require the coordinator's next exact-source native gate. Retained Wave 5 logs
do not certify the later Wave 7/8 working tree, DAW/device behavior or visual
identity. No commit or push was performed.
