# Native workspace selection and widget schema

These fields extend version 1 of the native workspace manifest. `Manifest.cpp`
and `scripts/reference-workspaces/validate.py` apply the same static rules. The
shared cases in `scripts/reference-workspaces/fixtures/selection-schema.json`
run in the Python self-test and the native `XTTestReferenceManifest.cpp` test.

All coordinates are absolute logical pixels in the 1586 × 992 workspace unless
otherwise stated. Numbers must be finite; booleans are not numbers. An identifier
contains 1–80 ASCII letters, digits, `_`, `.`, or `-`. All existing global limits,
including the one MiB file limit, remain in force. These optional fields are not
required by older manifests.

## Panel selection

An optional panel `selection` object creates real native tab buttons and selects
which source the panel displays. A selection can also rebind existing controls.
Selection itself changes view state, not a synth parameter.

```json
{
  "id": "mod-source",
  "kind": "source",
  "tabsRect": [950, 540, 320, 30],
  "default": "env1",
  "choices": [
    { "id": "env1", "label": "ENV 1", "source": "envelope", "index": 1 },
    { "id": "lfo1", "label": "LFO 1", "source": "lfo", "index": 1 }
  ],
  "columns": 2,
  "fontSize": 11
}
```

The example `tabsRect` must be inside the panel using it.

| Field | Contract |
| --- | --- |
| `id` | Required identifier, unique across all panel selection groups. |
| `kind` | Required `source` or `fx`. |
| `tabsRect` | Required `[x, y, width, height]`, inside both the canvas and owning panel. |
| `default` | Required string naming one of the choice IDs. |
| `choices` | Required array of 1–16 objects. |
| `choices[].id` | Required identifier, unique within its group. Other groups may reuse it. |
| `choices[].label` | Required nonblank string of at most 160 characters. FX tabs display the live slot number and algorithm name. |
| `choices[].source` | Required source from the table below, compatible with selection `kind`. |
| `choices[].index` | Required integer-valued number in the source's range. |
| `columns` | Optional integer-valued number 1–8. The effective column count is capped at the number of choices. When absent, it equals the number of choices, including groups with 9–16 choices. |
| `fontSize` | Optional number 8–24. The renderer defaults to 13 for ClearSignal and 11 for other styles. |

| Source | Valid indices | Selection kind |
| --- | --- | --- |
| `envelope` | 1–2 | `source` |
| `lfo` | 1–12 | `source` |
| `lfo-envelope` | 1–12 | `source` |
| `oscillator` | 1–3 | `source` |
| `waveform` | 1–3 | `source` |
| `fx` | 1–16 | `fx` |

No extra restriction is imposed on the owning panel's `kind`. Each selection has
one owner, and its rectangle must remain inside that owner's bounds.

The renderer fills cells from left to right and then top to bottom with a 4-pixel
gap between adjacent columns and rows. For `n` choices and `c` effective columns,
the number of rows is `ceil(n / c)`. Each cell's width is
`(tabsRect.width - 4 * (c - 1)) / c`; height uses the corresponding row formula.
Both dimensions must be at least one pixel. Empty cells in a partial final row
create no button.

Generated component IDs are `workspace.selection.<group-id>.<choice-id>`.
`Workspace::selectView(group, choice)` and `selectedView(group)` expose the same
view state used by the buttons. Left and right arrow navigation operates within
a group. Schema validation does not establish keyboard usability or visual
fidelity; the native interaction and screenshot checks cover those separately.

## Controls bound to a selection

A participating control retains its required canonical base `binding` and adds
both `selectionGroup` and `bindings`:

```json
{
  "id": "source-rate",
  "binding": "env1.attack",
  "selectionGroup": "mod-source",
  "bindings": { "env1": "env1.attack", "lfo1": "lfo1.rate" },
  "x": 980, "y": 590, "w": 80, "h": 80
}
```

`selectionGroup` must identify a declared group. `bindings` must be an object
whose keys are **exactly** that group's choice IDs. Every value must be a
parameter identifier or JSON `null`. A null value disables an operation that is
unavailable in that alternative view. The value for the group's default choice
must be nonnull and exactly equal to the control's base `binding`.

The static validators check identifier syntax and complete mappings. Before
exposing a workspace, `Workspace::load` additionally resolves the base and **all**
nonnull alternate bindings against the real `ParameterBridge`. A syntactically
valid invented binding therefore still fails native workspace loading. Tab
selection rebinds stable control components and cancels an in-progress gesture
or menu before the new binding is applied.

## Other panel fields

| Field | Scope and accepted value | When omitted |
| --- | --- | --- |
| `envelope` | Boolean on an `lfo` panel only; selects the LFO envelope view when true. | False. |
| `titleHeight` | Number from 0 to `min(120, panel.h)`; controls the title/content allocation. Zero hides the title. | The painted title/content allocation is 24 pixels. |
| `titleInset` | Number from 0 to `panel.w - 12`; horizontal title inset in logical pixels. | 12 pixels. |
| `wavetableRect` | `[x, y, width, height]` on an `oscillator` panel only; at least 60 × 18 and inside its panel and canvas. | No selector is created. |

A panel with `wavetableRect` must use oscillator `index` 1–3 when an index is
provided; otherwise it uses oscillator 1. The selector's generated component ID
is `workspace.wavetable.<panel-id>`. Its previous, browse, and next actions use
the real wavetable workflow. The static schema validates its layout, not whether
the current oscillator algorithm exposes a wavetable.

## Control appearance fields

An omitted control `kind` means `knob`.

| Field | Scope and accepted value | When omitted |
| --- | --- | --- |
| `faceDiameter` | Knob only; number from 8 to `min(160, control.w, control.h)`. The renderer may shrink it further to leave room for the label and value. | Uses the available face area. |
| `valuePosition` | Knob only; `bottom`, `right`, or `hidden`. | `bottom`. |
| `labelWidth` | Value control only; number from 0 to `control.w - 12`. | Based on the label length and available width. |
| `glyph` | Toggle only; exactly `power`. | Ordinary toggle presentation. |

These fields do not enlarge a control's interactive bounds or change the
parameter it edits. A power glyph on a mute binding shows the active source
state while preserving the mute parameter's native toggle behavior.

## Overlap validation

`validate.py --strict-overlap` checks declared controls, individual generated tab
cells, and wavetable selector rectangles against one another. It does not treat
the entire `tabsRect` as a control: the 4-pixel gaps and unoccupied cells remain
available. Selection and wavetable identifiers appear in overlap errors so a
manifest author can find the colliding element.

The existing overlap threshold is retained: an intersection greater than 0.5
logical pixels in both dimensions is rejected. This is a static logical-layout
check, not a replacement for checking the actual native child bounds after JUCE
rounding, generated actions, or the rendered labels. Panel backgrounds are
containers, so normal panel/control intersections are not errors.

## Verification commands

```sh
python3 scripts/reference-workspaces/validate.py --self-test
python3 scripts/reference-workspaces/validate.py --strict-overlap
```

The shared schema file contains 19 accepted and 85 rejected cases for the new
fields. The Python self-test also checks tab cell placement, a collision with a
tab, free gaps and unused cells, and selector/control and selector/tab collisions.
The native Catch test is named `Reference selection and widget schema matches
shared fixtures` and carries `[reference][manifest][schema]` tags.

## Native rendering details

A source selection on a `kind: tabs` container navigates among simultaneously
visible oscillator cards. It focuses the selected oscillator’s first enabled
rotary/slider without changing audio parameters or adding a hidden plot. Other
source panels repaint their selected real source and rebind their mapped native
controls. A right-hand tab strip leaves the available left title area visible.

Selected effects panels may supply `plot` as a bounded algorithm-category icon
well. It is decorative category information, not a measured transfer curve or
audio signal. Effect card captions always show the actual current slot algorithm.

`valuePosition: top` is also supported when `labelPosition: top`: label,
current native value and rotary face are stacked in that order. Both validators
reject a top value with a side/default label, and the shared fixture covers both.

A horizontal control may set `compact: true` for one row containing its label,
actual native slider and current value. Minimum size is 100 × 20; optional
`labelWidth` is 0..w−52 (default32). Painting and hit travel share the native
slider layout. Compact is a boolean valid only on horizontal controls.
