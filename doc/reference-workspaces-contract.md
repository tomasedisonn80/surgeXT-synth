# Reference workspaces — implementation contract

This fork is implementing ten native JUCE workspaces from the user's ten
1586 × 992 concept images. They are live alternative editor layouts, selected
through ordinary `.surge-skin` packages. The stock editor remains available.
No reference image may be used as a fake live interface or screenshot.

## Shared interfaces (implementation coordination)

Namespace: `Surge::GUI::Reference`. Sources: `src/surge-xt/gui/reference/`.

* `ParameterBridge(SurgeSynthProcessor&)` resolves a string binding in the
  current `scene` (0 or 1). Methods: `setScene(int)`, `getScene() const`,
  `valid(const juce::String&) const`, `value(const juce::String&) const`,
  `display(const juce::String&) const`, `label(const juce::String&) const`,
  `begin(const juce::String&)`, `set(const juce::String&, float)`,
  `end(const juce::String&)`, `reset(const juce::String&)`,
  `choices(const juce::String&) const -> juce::StringArray`.
  Bindings: `master.volume`, `scene.volume`, `scene.pitch`, `scene.pan`,
  `scene.width`, `scene.portamento`, `scene.polylimit`, `scene.mode`,
  `scene.mix`, `routing`, `osc1.type`, `osc1.pitch`, `osc1.param0` through
  `osc1.param6`, `osc1.level`, `osc1.mute`, `osc1.solo` (likewise 2/3),
  `noise.level`, `ring12.level`, `ring23.level`,
  `filter1.type`, `filter1.subtype`, `filter1.cutoff`, `filter1.resonance`,
  `filter1.keytrack`, `filter1.envmod` (likewise 2),
  `filter.drive`, `filter.feedback`, `filter.balance`,
  `env1.attack`, `env1.decay`, `env1.sustain`, `env1.release` (likewise 2),
  `lfo1.rate`, `lfo1.shape`, `lfo1.phase`, `lfo1.magnitude`, `lfo1.delay`,
  `lfo1.attack`, `lfo1.hold`, `lfo1.decay`, `lfo1.sustain`, `lfo1.release`
  (likewise 2–12), `macro1` through `macro8`, `fx1.type`, `fx1.param0`
  through `fx1.param11` (likewise 2–16). Report unsupported bindings; do not
  silently point them at unrelated parameters. Normalized values are 0..1.

* `Workspace` derives `juce::Component`; constructor
  `Workspace(SurgeSynthProcessor&)`; methods
  `bool load(const juce::File& manifest, juce::String& error)`,
  `void refresh()`, `ParameterBridge& bridge()`,
  `juce::String currentId() const`, `void selectScene(int)`;
  callbacks `std::function<void()> onFullEditor`,
  `std::function<void()> onChooseSkin`,
  `std::function<void(const juce::String&)> onAction`.
  It renders in 1586×992 logical coordinates with aspect-preserving scaling.

* `Visuals` is stateless painting helpers. Call contract to be agreed by parent
  before integrating. Waveforms must be computed from the actual oscillator
  or explicitly identified as parameter previews. Output meters must use actual
  signal levels, never random or hard-coded activity. Prefer live waveform data.

## Manifest schema (version 1)

`workspace.json`: `schema: 1`, `id`, `name`, `reference`, `width:1586`,
`height:992`, `style` (`precision|ivory|spectral|bauhaus|space|tokyo|aurora|clear|flow|live`),
`font` (`sans|serif|mono`), `palette` object with HTML hex colors
(`background`, `panel`, `border`, `text`, `muted`, `accent`, `secondary`,
`display`, `knob`, `red`, `yellow`, `blue`). Optional `radius`, `glow`.

`panels`: objects `{id, kind, title, x, y, w, h, index?, variant?, color?}`.
Kinds: `oscillator`, `filter`, `mixer`, `envelope`, `lfo`, `macros`,
`matrix`, `effects`, `analyzer`, `waveform`, `browser`, `routing`,
`scene`, `output`, `xy`, `keyboard`, `inspector`, `amp`, `tabs`.
Indices are 1-based. `variant` can be `compact`, `horizontal`, `vertical`,
`large`, `combined`, `terrain`, `flow`, `faders`, `knobs`.
The header is a built-in top band; `headerHeight` gives its height.

`controls`: objects `{id, binding, label?, x, y, w, h, kind?, accent?}`.
Kinds `knob` (default), `fader`, `horizontal`, `menu`, `toggle`.
Coordinates are absolute logical pixels. Knob bounds include label/value.
All controls are real named, keyboard-accessible child components. Algorithm
parameters should use their real dynamic label when label is omitted.
Do not place a child control over a different interactive target.

`decorations` optional array for purely cosmetic vector elements; never bake
values, meters, knob indicators, or selected state into images.

`adaptations`: honest list of deviations from concept, retained features, and
any reference-only capability not implemented. No numerical fidelity score.

## Integration

Each skin.xml uses `<reference-workspace layout="workspace.json"/>` within
`globals`. Existing SkinDB handles discovery and persistence. Only load a
manifest confined to its package; malformed packages must keep the stock editor
available. Preserve stable plugin/parameter IDs and DSP behavior.

## Validation

Native screenshots must be captured with JUCE from the actual new component
attached to the synth, not generated from reference PNGs or HTML. Record
dimensions/build context and test real parameter mutation, scene isolation,
host updates and state restoration. User requested ten waves of six independent
agents; the work log records all sixty actual agent tasks and results.
