# Assignment 33 — native FX power regressions

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Scope and recovery

Wave 6, assignment 33 of the current 60-assignment continuation. Only
`src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp` was edited.
The exact 291-line addition was recovered into `the recovered checkout` at base
`22b9c299e6aa74093c9e96677cb32df5f2f74e74`, preserving the MIDI implementation
and schema work owned by agents 31/32. No staging, commit or publication.

## Added coverage

Two native cases and a scoped native-listener probe were added:

1. **FX power switches edit only their native slot and expose accessible state**
   iterates the seven FX racks, requires 48 independent power children across
   six skins, and requires no power children on Spectral's eight compact tabs.
   It checks child containment, hit targets, focus eligibility, accessible
   names/descriptions/tooltips, checked state and native accessibility actions.
   Two synchronous JUCE accessible toggles per child exercise the actual click
   callback. Assertions cover the exact native disable bit, untouched other
   bits including higher undisplayed slots, suspend mask, dirty/editor flags,
   exact SCT_FX_DEACT notification, unchanged host values and no invented
   automation gesture. External bitmask refresh and separate view-only card
   selection are also checked.
2. **FX power state follows Off assignment and remains separate from chain bypass**
   uses real ParameterBridge algorithm changes and deferred engine operations.
   It covers enabled/checked to Off, rejected commands on an unassigned slot,
   retained disabled bits across reassignment, external enabling, all four
   native global bypass modes, and Off to reassignment again.

The tests do not claim that the stock hidden bitmask has dedicated undo or host
automation. No existing fixture, threshold, source behavior or assertion was
weakened.

## Defect found

EffectPowerButton::sync originally called setEnabled(false) before clearing the
toggle state for an Off slot. Pinned JUCE Button::setToggleState ignores updates
while disabled, leaving a disabled but checked power indicator. Root corrected
the state-update order. The second case covers this regression.

## Validation and limits

The original initial test version passed the warning-as-error C++20 syntax
check before the previous environment failed. Its final 291-line form removed
unnecessary mask-index plumbing and retained stronger all-host-value checks.
The recovered source has not been compiled in this replacement environment;
dependencies/native CI must provide that gate.

Recovered file whitespace validation passed. Strict manifest validation result
is appended below. Runtime results and screenshots remain pending the linked
native workflow.

Independent handlers, focus eligibility, hit targets and accessible toggle
actions are covered. Ancestor accessibility-tree reachability was an optional
follow-up interrupted by the old environment failure and is not claimed here.

Recovery SHA-256 of the test file immediately after replay:
`00d2a07bcee2d019042b8b693918f696ffb298e24895403e6ed1538df30bf123`.
Later assignments may legitimately extend that file.

## Recovered-tree strict validator

Exit code: 0

```text
PASS 01 Obsidian Precision: 14 panels, 83 controls
PASS 02 Ivory Atelier: 17 panels, 75 controls
PASS 03 Spectral Observatory: 18 panels, 69 controls
PASS 04 Bauhaus Modular: 25 panels, 79 controls
PASS 05 Space-Age 1977: 21 panels, 90 controls
PASS 06 Tokyo Industrial: 18 panels, 92 controls
PASS 07 Liquid Aurora: 21 panels, 99 controls
PASS 08 ClearSignal Accessible: 27 panels, 77 controls
  WARNING plot/control bounds overlap: panel.envelope.plot / envelope.mode (79 x 24); inspect rendered labels
  WARNING plot/control bounds overlap: panel.lfo.plot / lfo1.shape (60 x 21); inspect rendered labels
PASS 09 Signal-Flow Cartographer: 11 panels, 51 controls
  NOTE panel.flow: 19 live controls intentionally overlay routing plot
PASS 10 Live Performance Deck: 22 panels, 64 controls
PASS: 10 reference workspaces validated
```
