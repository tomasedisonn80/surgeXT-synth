# Assignment 54 — schema, controls and accessibility audit

> Curated historical assignment report. The a9d0d0a native run that was pending at this report's handoff subsequently passed. Manual screen-reader/DAW certification remains outside this scope; later renderer and source-asset changes need fresh native evidence. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Scope and result

Read-only audit of candidate `a9d0d0a838632daf178255561def4622cd208258`
on `work/reference-wave6`. No additional blocking defect was found in this
scope. This is source and existing-validator evidence, not manual screen-reader
or DAW certification.

The candidate's [native run 43393400045](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43393400045)
was still in progress when this audit was completed. Its new linear-pointer and
rapid-MIDI regressions therefore remain subject to linked native verification.
The earlier `eccf282` run is a separate passing checkpoint.

## Schema parity and identities

Compared the native validator in `src/surge-xt/gui/reference/Manifest.cpp`
with `scripts/reference-workspaces/validate.py` and their shared
`fixtures/selection-schema.json`.

The MIDI rules match:

- Supported names are exactly pitch, mod, expression and aftertouch.
- A panel contains one through four names, with each name unique across the
  entire workspace.
- Generated `workspace.midi.<controller>` identifiers enter the same reserved
  ID set as declared panels and controls. Collisions are rejected whether the
  conflicting declaration appears before or after the MIDI panel, including
  the panel's own ID.
- Each lane receives at least 24 logical pixels; panel height is at least 64.
- The title is empty. titleHeight is omitted or numeric zero, including
  rejection of Boolean false as a substitute for a numeric value.
- index, plot, plot2, selection, variant and titleInset are unsupported on MIDI
  panels. A controller list on an ordinary panel/control is rejected.

All 37 shared MIDI cases were inspected: four accepted and 33 rejected. The
complete fixture set contains 28 accepted and 140 rejected cases. The existing
Python self-test passed all of them, plus its JSON, path and overlap checks.

Independent inventory of the actual ten manifests found 194 panels and 779
declared controls. There are 48 generic linear controls affected by assignment
47. Live adds these four generated MIDI components:

| ID | Logical rectangle |
| --- | --- |
| workspace.midi.pitch | 19, 831, 38, 128 |
| workspace.midi.mod | 57, 831, 38, 128 |
| workspace.midi.expression | 95, 831, 38, 128 |
| workspace.midi.aftertouch | 133, 831, 38, 128 |

The four lanes partition the actual 152-pixel panel exactly. No current
manifest/generated-MIDI ID collision was found.

## Native editing and accessibility paths

### Parameter controls

BoundSlider maintains the actual native binding in its properties, uses native
enabled/deactivated state, and falls back to the native parameter name when a
compact visual caption is empty. Its accessible numeric value reads the
ParameterBridge; its accessible string reads the engine's formatted value.
Disabled controls expose a read-only value interface.

Accessible numeric writes reject nonfinite values and use the same bridge as
pointer edits. Accessible string writes use the native typed-value parser,
rather than treating Hz, dB or time text as a normalized number. Typed callouts
provide validation feedback. Rebinding ends the old gesture and invalidates
open callouts; queued submissions must still match the scene, binding,
generation and native parameter name.

BoundMenu similarly checks its popup generation, scene, binding and complete
choice list before accepting an asynchronous result. Current-source/FX tabs
retain native rebinding and selection state rather than substituting captions
for processor operations.

ParameterBridge starts a host gesture and captures native parameter/macro undo
before editing. It prevents simultaneous alias gestures through the same
adapter, rejects nonfinite writes, ends a gesture when a target becomes
inactive, and releases host edit flags during completion. Focus loss,
visibility changes, destruction and rebinding retain the control-level cleanup.

### Linear pointer geometry

Assignment 47 installs the explicit linear layout for generic faders and
ordinary horizontal sliders. Painting and JUCE input share the rounded rail
geometry; handles and active travel read getPositionOfValue. Existing compact
and performance layouts remain separate. This preserves component bounds,
bindings and accessible identities.

The new source regression invokes real mouse callbacks for Spectral, Live and
Bauhaus controls, checks that clicking the visible handle does not jump the
native value, reaches both endpoints and balances gestures. This audit
inventoried the 48 affected controls without adding an implementation-mirroring
test. The linked test is the runtime gate.

### MIDI input lanes

The MIDI accessible values report actual applied processor telemetry, with
signed pitch range -8192 through 8191 and other lanes 0 through 127. Numeric
inputs reject nonfinite values; string inputs accept only complete in-range
integers. The source tests exercise incoming host MIDI, typed/accessibility
inputs, queue saturation, spring ownership, focus/skin/destruction cleanup,
and preservation of external held values.

Assignment 49 adds the accepted packet sequence and the audio acknowledgement
of actual Pitch/Touch application. The GUI retains the latest accepted owned
spring input until acknowledgement; a rapid release followed by a new key can
start from its pending zero. Once consumed or superseded, applied external MIDI
is authoritative. Its focused regression is present in the candidate.

I also checked the pinned JUCE accessibility/slider implementation rather than
inferring defects from an empty action list: the native Slider handler itself
uses that action pattern; AccessibilityHandler.takeFocus routes to the
component's keyboard focus when applicable. JUCE rejects modifier-key input
before its ordinary arrow-key fallback, so the custom MIDI modifier guard does
not introduce an unowned spring edit through that path.

## Correctly limited automation and undo claims

| Operation | Native behavior |
| --- | --- |
| Ordinary parameters and macros | Native bridge, host gestures and existing undo capture |
| Source/FX view selection | Selects/rebinds the view; it is not an audio parameter edit |
| MIDI lanes | Actual MIDI input, rather than host-automatable parameter controls |
| Per-slot FX power | Existing hidden disable bitmask, suspend/listener/dirty path; no dedicated native host gesture or undo operation |

The FX-power source resolves its toggle state before disabling an unassigned
slot, preserving the assignment 33 Off/reassignment correction. Its description
states that global FX bypass remains independent.

## Verification performed

At the inspected candidate HEAD:

- `python3 scripts/reference-workspaces/validate.py --strict`: passes all ten.
- `python3 scripts/reference-workspaces/validate.py --self-test`: passes
  42 schema rejections, 24 malformed-JSON rejections, nine confined-path checks,
  and the complete 28/140 shared cases.
- `git diff --check`: passes.
- Independent manifest inventory saved in `analysis-only/w09_a54_inventory.json`.

The two already-reviewed Clear plot-corner overlaps remain explicit warnings;
the Flow plot's 19 real overlaid controls remain intentional. This audit did
not move them or reinterpret static validation as visual fidelity.

No repository source, manifest, test, workflow or existing document was changed.
No staging, commit or push was performed. Manual OS accessibility clients,
manual DAW automation and fresh candidate captures remain outside this report's
completed checks.

## Sources

- [Candidate source](https://github.com/tomasedisonn80/surgeXT-synth/tree/a9d0d0a838632daf178255561def4622cd208258)
- `src/surge-xt/gui/reference/Manifest.cpp`, `Workspace.cpp`, `ParameterBridge.cpp`
- `scripts/reference-workspaces/validate.py` and `fixtures/selection-schema.json`
- `src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp`, `XTTestReferenceMidi.cpp`
- [Pinned JUCE AccessibilityHandler](https://github.com/juce-framework/JUCE/blob/a12f0de/modules/juce_gui_basics/accessibility/juce_AccessibilityHandler.cpp)
- [Pinned JUCE Slider](https://github.com/juce-framework/JUCE/blob/a12f0de/modules/juce_gui_basics/widgets/juce_Slider.cpp)
