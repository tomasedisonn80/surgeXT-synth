# Wave 10 / assignment 59 — adaptation and compatibility review

## Scope and result

Read-only review of all ten `workspace.json` adaptation lists against their
actual bindings/selectors, shared native renderer, parameter bridge, source
models, matrix data and full-editor navigation. Also read AGENTS, continuation,
engineering log, current pass record, updated user guide and package evidence.
Only this report was written; no implementation, manifest, asset, commit or
push was changed.

Review started at gallery HEAD
`762d888f7159558db472836a8f9b5f015c1f9427`, including the uncommitted A54
inspector refresh, A55 hero drawing, A56 material/Width clearance and A57 guide
changes. The latest verified native source remains
`ea674878313006afa0b9d1a2b1d3495b69755627` / run `43497923810`.
The combined final source still needs its own native acceptance and captures.

The coordinator accepted both findings below: root owns the two FX title
corrections and exact slot-scope guide sentence; A54 was reactivated to preserve
the last valid same-scene route snapshot and selection during contention,
without blocking, with a focused regression. Those handoffs are not yet a
published or natively accepted result. The remainder of the reviewed capability
adaptations is consistent with the implementation.

## Corrections identified

1. **ClearSignal and Live describe mixed FX locations as inserts.**
   ClearSignal's `panel.fx` title is `INSERT FX (1–8)` and Live's `p.effects`
   title is `Insert Effects (8)`. `ParameterBridge::parameter` maps `fx1`–`fx8`
   directly to native storage indices 0–7. `fxslot_positions` in
   `src/common/SurgeStorage.h` identifies those indices as A inserts 1/2,
   B inserts 1/2, sends 1/2 and globals 1/2. The adaptation lists correctly
   describe eight actual slots and the sixteen-slot full editor. Recommended
   minimal correction: `FX SLOTS (1–8)` / `Effects (8)`, with a concise guide
   explanation of those locations. Keep every parameter binding unchanged.

2. **The guide overstates route-selection preservation during contention.**
   Its new modulation paragraph says the refresh preserves the selected route.
   `WorkspaceData.cpp::modulationRows` returns an empty vector if its
   nonblocking `modRoutingMutex` acquisition fails. `Workspace::Impl::refresh`
   replaces the cached routes with that vector when no edit is active, and
   `syncRouteControls` clears `inspectedRoute`. The next successful snapshot
   then selects the first route. This is an existing UI edge case, now reached
   at A54's faster refresh cadence, rather than evidence of an audio mutation.
   Either qualify the guide's guarantee or let the owning implementation
   assignment distinguish a busy snapshot from a genuinely empty patch.
   A54 owns that focused correction and regression; this review made no code
   changes.

## Ten-manifest cross-check

| Workspace | Verified adaptation against current source |
| --- | --- |
| Obsidian | Native Wavetable/FM2/Classic fields, actual mute/solo, two dedicated EGs plus LFO 1/2 envelopes, selected FX algorithm plus four genuine parameter indices; no Sample label or sample-loop processor invented. |
| Ivory | Wavetable parameters 0/3/6 and FM2 0/1/6; Ring 2×3 replaces the reference Sub block; envelope/LFO source tabs rebind real stages; default Chorus details use native indices 1/2/6/7. |
| Spectral | Actual oscillator-source selection, seven rebound oscillator and FX detail fields, six visible macros, two EGs/two LFO envelopes/four LFO views; table terrain is a bounded copied-source view, separate from output spectrum. |
| Bauhaus | OSC 1/2/3 navigation, genuine scene mode and scene-level controls, two EGs plus LFO envelopes, five FX fields and shaper-only Distortion preview; no MIX-source or scene-morph processor. |
| Space-Age | `scene.width` is labelled Width and `scene.pitch` Pitch; OUTPUT binds `master.volume`. Third bank binds LFO 3 envelope; UNI is polarity; four native macros and eight FX details retain real units and labels. |
| Tokyo | Three native oscillator cards, algorithm-dependent parameter order, two EGs plus LFO 3 envelope, LFO 1–4 rebinding, eight algorithm menus; Pitch/FM faders are scene pitch/FM depth, not MIDI wheels. |
| Aurora | Source navigation rebinds real oscillator fields; Classic Width 1/Unison Detune/Unison Voices/Shape remain genuine slots; LFO 3 envelope, UNI, Ring level and selected Chorus indices match the notes. |
| ClearSignal | Two EGs plus LFO envelopes; inapplicable envelope model disables. Four macros, real signed route depth and physical range, eight actual FX slots with two fields each, scene level instead of Blend; FX heading caveat above. |
| Flow | Nineteen node controls comprise three oscillator menus, three mute controls, three mixer faders, two filter menus and eight FX menus. Eight real filter-routing modes and selected-scene modulation data back the schematic; no arbitrary rewiring. |
| Live | Macro 1/2 XY and eight macros; two actual scenes, selected-scene level, real split allocation and native MIDI lanes; no six snapshots or three independent zones. Eight selected FX slots use the genuine algorithm menu; FX heading caveat above. |

`SurgeStorage.h` confirms two scenes, three oscillators per scene, two dedicated
EGs, twelve LFO sources and sixteen FX slots. The available oscillator types
have no separate Sample or Granular algorithm. Native Wavetable sample behavior
is not denied or relabelled; unavailable dedicated concept controls simply are
not promised. `ParameterBridge` resolves the current scene and current algorithm
parameters, and rejects inactive/unused bindings through native control type and
deactivation state. `WavetableSelector` only enables catalog operations for a
wavetable-capable algorithm with an available catalog.

The compact matrix capacities are 6/6/5/6/8/8/6/8/6/5 rows in numbered skin
order; Tokyo uses two columns of four. They are display capacities, not invented
route counts. Actual rows, signed normalized depth, formatted physical range,
polarity and mute state come from patch assignments. `modulationRows` bounds
its snapshot to 128 assignments; `routePlacements` bounds visible children to
the layout capacity. `Edit routes` opens the complete native modulation editor.
Empty rows do not imply assignments. Full editor, FX, browser, scope and return
navigation resolve to the original editor or its real overlays.

## Delivery and integration boundary

The guide accurately requires this modified binary with matching factory data.
The skin packager copies twenty XML/JSON files plus guide, license, adaptation
notes and provenance; it bundles no binary, source wavetable or screenshot.
Its adaptation document is generated from the captured manifest bytes. The
factory-data route and `pluginsonly` distinction agree with the existing package
review and scripts; an upstream binary cannot acquire the C++ renderer by
copying the skin folders. Selecting a skin does not load a showcase fixture.

The guide/gallery identify the PNGs as native JUCE component snapshots attached
to the processor, not DAW or desktop screenshots. Historical platform packaging
and earlier native passes are not final-source or DAW-runtime certification.
Unsigned fork macOS artifacts, retained plugin/parameter identity, remaining
font/material/source-topology differences and the absence of a pixel-identity
claim are explicit. New playable source revisions are correctly distinguished
from paint-only camera and lighting changes.

Inspected the final `Workspace.cpp`, `Visuals.cpp` and native-test diffs. Apart
from the existing busy-route edge case above, no concrete new integration or
cross-platform API defect was apparent. The edits use existing JUCE drawing
interfaces and standard-library facilities already included; the terrain
iteration is bounded by the owned 48×256 snapshot, and the new source-floor
math uses double precision before projecting. Material noise is deterministic
and paint-local, without audible RNG or audio-thread state changes. This is
source review, not compilation or performance certification. Scoped
`git diff --check` passed; no broad test suite or expensive regeneration was
repeated. Exact-source build, new regressions and ten native capture pairs
remain the coordinator/A60 gate.
