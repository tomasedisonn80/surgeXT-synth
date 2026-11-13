# Wave 8 assignment 47 — renderer state review and linear pointer correction

> Curated historical assignment report. The linear pointer correction was subsequently included in a9d0d0a and passed its native behavior/capture gates. The separately identified rapid-release defect was handled by assignment 49. Later renderer changes still require their own final-source evidence. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Scope and evidence

Reviewed the public source transition `3ec3998877eab793f8bf2aa4acb367c69de17979`
to `eccf282722f719b10a918424663244af0b5688fa`, then the current shared
renderer changes in the working checkout. Read AGENTS, the
continuation, historical work log and user guide. The native run at 3ec3998
is the established prior checkpoint; the changes from this assignment require
a new linked native run.

The review covered the new Space dial materials, Spectral rail/cap styling,
Tokyo numeric/menu changes, Aurora and Bauhaus coatings, selected-FX title
inheritance, the actual-source wave projection, Clear external filter axes,
Flow dial caps and the prior MIDI keyboard spring fix. Pure paint changes
remain confined to graphics and read real parameter/sample state. No DSP,
audible RNG or fixture source change was introduced by this assignment.

## Defect found and corrected

Generic linear controls painted a smaller travel area than JUCE used for
pointer input. For Spectral `c.osc1-level` (58 × 91), the painted rail ran
from local y22 to y68, but the stock JUCE NoTextBox layout ran from y12 to y79.
Clicking the painted maximum handle at y22 therefore requested approximately
0.851 instead of 1. This predates the latest material change but directly
affects the controls being refined.

The source proof used the repository's exact JUCE submodule commit,
`a12f0de5a3d1e3a43cca626889ac2ee1c0cc0dbe`: LookAndFeel_V2's slider layout
applies the virtual V4 thumb inset, V4 returns 12 for this component, and
Slider's absolute drag maps pointer position through that resulting region.
SurgeJUCELookAndFeel overrides neither slider layout nor thumb radius.

With coordinator authorization, modified only these source files:

- `src/surge-xt/gui/reference/Workspace.cpp`
- `src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp`

### Implementation

- Added one rounded standard rail-geometry helper to the existing slider
  look-and-feel. It reserves the actual caption and value bands.
- Generic faders and noncompact horizontal sliders now opt into that exact
  JUCE layout. All 48 such controls have positive tracks inside their existing
  component bounds.
- Painted active travel and handle position now use JUCE's
  `getPositionOfValue`, so drawing and absolute pointer input share the same
  mapping.
- Preserved compact horizontal and performance-fader layouts, native parameter
  callbacks, host gesture lifetime, keyboard input, typed value entry,
  accessibility identity, manifest rectangles and captions.
- Set Live's ordinary fader-cap width to 20 pixels, as requested by the
  coordinator following assignment 46's measured reference width.
- Preserved the coordinator's existing uncommitted Clear filter/Flow paint
  changes. No unrelated source was reset or staged.

### Meaningful native regression added

`Painted linear handles match native pointer positions and endpoints` uses
three genuine workspace controls with independently specified visible travel:

| Skin / control | Component | Position at normalized 0 | Position at normalized 1 |
| --- | --- | --- | --- |
| Spectral / c.osc1-level | 58 × 91 vertical | y68 | y22 |
| Live / c.mixer-osc1 | 42 × 130 vertical | y107 | y22 |
| Bauhaus / output-scene-level | 195 × 54 horizontal | x7 | x188 |

For each, the test renders the actual control, clicks the displayed handle at
five values, and checks that the real synth parameter and host adapter do not
jump. It then drags to both visible endpoints and checks endpoint attainment.
It also checks balanced host gestures, no lingering edit flag, keyboard focus
capability, accessible name preservation and unchanged audible RNG during paint.
This exercises actual JUCE mouse callbacks and the native parameter bridge,
rather than calling a test setter in place of a pointer action.

## Separate open finding

The previous MIDI keyboard fix handles release → audio block → next key.
A distinct rapid sequence remains for focused follow-up: applied pitch256,
key release queues0, Shift+Up before audio processes that release reads the
still-applied256 and queues272 after0. The next block then ends at272.
The coordinator accepted this finding and is delegating a separate
queue/gesture design and native regression. This assignment did not modify
MIDI queue or controller code.

## Verification performed

- Strict validation passes all ten manifests. The two known Clear plot-corner
  overlap warnings remain unchanged and are unrelated to this correction.
- `git diff --check` passes.
- Static geometry audit passes for all 48 generic linear controls.
- The three regression fixture dimensions and manually measured travel
  coordinates agree with current manifests and painted rail geometry.
- No native compiler/runtime pass is claimed for this new regression in the
  recovered environment; the next GitHub native workflow is the required gate.
- No screenshot fidelity or pixel-identity claim follows from these checks.

Validation data: `analysis-only/w08_a47_validation.json`.

At handoff, Workspace.cpp SHA-256:
`eec2add135523ccd8636ea5b9d44be3aa61cee1f0d57aa315af28fad28c5e521`.

At handoff, XTTestReferenceWorkspaces.cpp SHA-256:
`d44f07cbbfe463131c31ec64fcbf79e33a807ae7f48fc4f20f93c833d363347f`.

Changes remain unstaged. No commit or push was performed.
