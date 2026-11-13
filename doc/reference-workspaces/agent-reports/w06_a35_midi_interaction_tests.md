# Assignment 35 — native MIDI interaction regressions

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

Status: four native test cases implemented in the recovered checkout; linked execution pending.
No commit or publication was performed by this agent.

## Owned changes

- New file: src/surge-xt/xt-tests/XTTestReferenceMidi.cpp.
- One explicit source entry in src/surge-xt/xt-tests/CMakeLists.txt.
- No changes to agent 33's FX tests, renderer, processor implementation, schema or fixtures.
- The new test file contains 443 lines and four cases tagged reference-workspaces/reference-midi.
- SHA-256: 22ff96a220378fda7a889eecba6f0fbea027aa53fbb31c6ba5e61d53062291fc.

## Native delivery and range coverage

The first case enqueues Pitch, Mod, Expression, Aftertouch and existing Sustain input,
requires that enqueueing alone leaves applied engine state unchanged, then processes
a real audio block and checks actual native values. It verifies storage.pitch_bend
and the real ControllerModulationSource targets in both scenes, rather than relying
only on renderer properties or protocol counters.

Both integer extremes are clamped to native MIDI ranges. NOTE is rejected by the
controller helper and cannot add a command through that API.

## Saturation and ordering

The second case exercises both Pitch and Touch:
- A spring zero release survives a full GUI MIDI queue.
- A full queue rejects later nonzero movement rather than claiming it was applied.
- A newer accepted movement takes precedence over an older emergency zero release.
- An older accepted packet delivered after the emergency release cannot revive it.

The last case also fills the actual queue before switching skins with an owned
Pitch gesture and before destroying a workspace with an owned Touch gesture.
Both UI lifetime paths must deliver zero through saturation. Latched Mod and
Expression retain their applied values.

## Native UI and accessibility

The third case sends genuine host MIDI messages through processor.processBlock.
It checks all four displayed values, raw ranges, accessible current values, slider
roles, names, descriptions, focus eligibility, native child containment and exact
measured centres 38/76/114/152 at y831 with height128.

Refreshing, painting, switching to Ivory, and returning to Live must leave the
MIDI queue empty, applied incoming values intact, host parameters unchanged,
the patch clean, and the audible RNG state unchanged. A second host event verifies
that the restored controls continue following incoming values.

The fourth case uses real virtual keyboard methods, mouseDown/mouseDrag/mouseUp,
the asynchronous ValueEntry callout's actual input/apply callbacks, and native
accessibility value setters. It covers coarse/fine pitch steps, key release,
focus loss, signed pitch input, exact text bounds, latched Mod, pointer Touch,
malformed integer input (including '+-1'), NaN rejection, a valid '+87' entry,
and real CC11 delivery.

## Validation and limits

- Recovered-runtime Python schema self-test passed: 28 accepted / 140 rejected
  shared cases, plus existing JSON/path/schema cases.
- Strict validation passed all ten manifests, including Live's 22 panels and
  64 ordinary controls plus four generated MIDI lanes.
- git diff --check and explicit whitespace/source registration checks passed.
- Pinned JUCE headers were not available in the recovered runtime, so no C++
  syntax, linked runtime, screenshot, or concurrency-stress pass is claimed.
- Native GitHub execution is required before acceptance. These tests verify
  sequentially controlled queue ordering; they do not replace thread sanitizer
  coverage or certify a DAW/OS input stack.
