# Surge XT — 10 UI/UX Redesign Concepts

Design references generated for the `tomasedisonn80/surgeXT-synth` fork of Surge XT. These images are **concept art, not implemented interfaces or pixel-exact engineering specifications**. Small labels and values in generated images may be imperfect; use the actual codebase and Surge XT manual as the source of truth for functionality.

| # | Concept | Core direction |
|---|---|---|
| 01 | Obsidian Precision | Highly structured dark professional studio UI; orange + cyan signal cues. |
| 02 | Ivory Atelier | Warm ivory / forest-green / brass, classic instrument tactility in light mode. |
| 03 | Spectral Observatory | Expansive central wavetable/spectrum display, violet/cyan scientific visualization. |
| 04 | Bauhaus Modular | Color-coded, modular cards with playful geometric hierarchy. |
| 05 | Space-Age 1977 | Retro-analog console with amber CRT readouts and hardware-like controls. |
| 06 | Tokyo Industrial | Condensed dense control panels, carbon-black surface and red/cyan states. |
| 07 | Liquid Aurora | Expressive spectral centerpiece, translucent luminous violet interface. |
| 08 | ClearSignal Accessible | Accessible high-contrast light mode, large controls and clear values. |
| 09 | Signal-Flow Cartographer | Patch signal flow is visible and editable as a node-based routing canvas. |
| 10 | Live Performance Deck | Performance-first workspace with large scene-morph XY control and macros. |

## Functional requirements to retain

- Scene A and Scene B, layering/split/preset workflows, and scene management.
- Three oscillators per scene and their algorithm-specific parameters, including wavetables and FM where applicable.
- Oscillator mixer, dual configurable filters, flexible routing, output stages.
- Modulation sources, envelopes, LFO editors, modulation assignment and matrix.
- Macro assignment, automation, effects, preset browser, meters and accessibility.
- Scalable/resizable VST3 and standalone layouts with keyboard navigation and readable state indicators.

## Suggested next step

Select one base visual language and one interaction pattern to prototype. In particular, consider combining concept 01's overall information density and legibility with concept 09's optional signal-flow inspector and concept 10's separate performance view. Validate keyboard accessibility, discoverability and control reach before implementing the full skin.

Reference: https://github.com/tomasedisonn80/surgeXT-synth and https://surge-synthesizer.github.io/manual-xt/
