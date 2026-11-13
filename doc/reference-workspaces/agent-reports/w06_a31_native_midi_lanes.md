# Assignment 31 — native Live MIDI lanes

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

Status: implementation complete in the recovered checkout; linked native verification pending.
No commit or publication performed by this agent.

## Actual native operation

- New panel kind midi instantiates Slider-derived children with stable component IDs
  workspace.midi.pitch, workspace.midi.mod, workspace.midi.expression and
  workspace.midi.aftertouch. The schema/manifest is owned by assignment 32.
- The measured Live panel is (19,831,152,128), yielding four 38px lanes and glass centres
  x38/76/114/152. Glass tracks are 20px wide. Labels are Pitch, Mod, Exp and Touch.
- Pitch uses native signed MIDI -8192..8191; the other inputs use 0..127.
  Pitch and channel aftertouch spring to zero when this control's gesture ends.
  Modulation (CC1) and expression (CC11) are latched.
- Pointer and arrow-key input, fine pitch steps, explicit reset, integer typed input,
  native accessibility values, and focus/visibility/destruction handling are implemented.
  Invalid, fractional, out-of-range and malformed signed integer text is rejected.
- Native controller events use processor.queueMidiController and the existing GUI MIDI
  queue. No UI thread writes to the engine, patch parameters or modulation-source internals.
- Idle values and accessibility reads come from actual engine atomics. While the audio
  engine has not applied queued input, the idle display does not claim the new value is
  already applied. Active drags/arrow-key gestures show their requested position.
- Merely constructing, refreshing, resizing, changing scenes/skins, or destroying an idle
  lane sends no controller input. Only a locally owned spring gesture is released.
  External controller state and latched inputs survive view changes.

## MIDI delivery and saturation invariants

SurgeSynthProcessor::midiR adds EXPRESSION and AFTERTOUCH cases plus a GUI sequence stamp.
The message-thread helper accepts only the five actual controller types and clamps their
native integer ranges. The stock full-editor Pitch/Mod/Sustain callbacks now use the same
helper; their controller meanings, NOTE events and incoming host-MIDI paths are retained.

Ordinary events use the existing bounded SPSC queue. If a Pitch/Touch zero release cannot
fit, its monotonic sequence is stored in one of two atomic fallback slots. Audio applies
it after draining the queue only when newer than the last applied event for that spring.
Older stamped events are ignored if a later release already took effect. Thus a later
accepted gesture cannot be cancelled by an older fallback reset, and an older queued move
cannot revive a completed spring gesture. A full queue can reject ordinary motion; it
cannot lose a spring release on editor teardown.

SurgeSynthesizer adds expressionCC and channelAftertouchMIDIVal atomics updated at the
existing actual CC11 and global channel-aftertouch native entry points. MPE member-channel
pressure does not overwrite the global lane's displayed pressure. No oscillator, effect,
modulation algorithm, patch fixture, RNG or audio-sample calculation was modified.

## Inspection hooks

Each lane exposes read-only properties:
- reference.midi-controller: pitch/mod/expression/aftertouch
- reference.midi-channel: 1 (native zero-based queue channel 0)
- reference.midi-spring-return: true for Pitch/Touch
- reference.midi-applied: most recent polled native integer value
- reference.midi-last-input-accepted / reference.midi-last-request: last local request result

## Verification performed / remaining

The bounded replacements completed successfully, whitespace checks passed, and the initial
strict validator passed all ten pre-replayed manifests. Combined schema validation follows
assignment 32's replay. The recovered runtime initially has no pinned JUCE headers, so
no C++ syntax, linked build, interaction or new screenshot success is asserted here.

Assignment 34 should exercise actual incoming/outgoing MIDI state, parameter/RNG invariance
during view changes, spring ownership, saturation release delivery, new-motion ordering,
typed/accessible bounds, MPE global pressure semantics, and actual placed child rectangles.
The next native workflow must capture all ten genuine workspaces after the shared change.
