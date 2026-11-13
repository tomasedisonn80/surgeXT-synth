# Reference-workspace engineering log

The user requested ten waves of six independent agents (60 agents total), ten
working native skins based on their supplied 1586×992 concept images, main-branch
commits, and screenshots of the implementation. This log distinguishes source
checks from native build/runtime evidence. Concept art is not a functional spec;
unsupported engine features must not be represented by fake controls.

## Wave 1 — inspection (six agents, completed)

| Agent | Substantive result |
|---|---|
| w01_reference_a | Measured concepts 01–03; identified simultaneous-oscillator and terrain requirements |
| w01_reference_b | Measured concepts 04–06; extracted material, panel and control geometry |
| w01_reference_c | Measured concepts 07–10; identified routing/XY/performance behavior and engine mismatches |
| w01_skin_architecture | Inspected native skin discovery, editor seams, parameter adapters and fallback strategy |
| w01_build_readiness | Audited native targets, installed dependencies and genuine snapshot options |
| w01_quality_strategy | Defined state, automation, audio, accessibility and screenshot acceptance checks |

## Wave 2 — shared native infrastructure (six agents, completed)

| Agent | Substantive result |
|---|---|
| w02_parameter_bridge | Real parameter resolver, host gestures, undo, dynamic labels and scene isolation |
| w02_editor_integration | Normal skin discovery, native editor switching and safe original-editor fallback |
| w02_live_visuals | Actual oscillator-algorithm previews and real output levels |
| w02_manifest_validation | Bounded native JSON loader and dependency-free package validator |
| w02_native_test_harness | Native control/state/snapshot/scaling tests with capture provenance |
| w02_build_toolchain | Diagnosed blocked local package route; authored GitHub Actions native/Xvfb job |

The root implemented the shared native renderer, rotary/fader/menu controls,
header actions, macro XY component and playable MIDI keyboard. No reference PNG
is used as an operative UI. Source compilation checks are not a linked build.

## Wave 3 — first five designs and compilation review (six agents, completed)

| Agent | Substantive result |
|---|---|
| w03_obsidian_skin | 01 Obsidian Precision: independent layout and live controls |
| w03_ivory_skin | 02 Ivory Atelier: serif/cream/green layout, knob mixer and vertical routing |
| w03_spectral_skin | 03 Spectral Observatory: central waveform terrain and violet/cyan layout |
| w03_bauhaus_skin | 04 Bauhaus Modular: red/blue/yellow panels and distinct waveform-first hierarchy |
| w03_space_skin | 05 Space-Age 1977: metallic/olive layout, ADSR faders and vertical FX slots |
| w03_cpp_compile_audit | Syntax-checked new C++; found integration defects and lifetime/performance risks |

Root fixes included scene-selection recursion protection, narrow native scene
wrapper, keyboard note release on hide, removal of duplicate refresh work,
per-control ink, side-label knobs, and compact/vertical routing.

## Wave 4 — remaining designs and actual patch data (six agents, completed)

| Agent | Scope |
|---|---|
| w04_tokyo_skin | 06 Tokyo Industrial |
| w04_aurora_skin | 07 Liquid Aurora |
| w04_clear_skin | 08 ClearSignal Accessible |
| w04_flow_skin | 09 Signal-Flow Cartographer |
| w04_live_skin | 10 Live Performance Deck |
| w04_live_data | Read-only snapshots of real modulation routes and installed preset catalog |

## Wave 5 — interaction, lifecycle and truthful displays (six agents, completed)

| Agent | Substantive result |
|---|---|
| w05_interaction_hardening | Unit-aware typed entry, accessible controls, searchable native preset list, gesture cancellation |
| w05_bridge_lifecycle | Dirty-checked preset loading, native undo, scene-safe skin transitions and hot reload |
| w05_model_previews | Actual filter-algorithm response and private native LFO previews; unsupported displays labeled |
| w05_live_audio_tap | Independent processor-owned output tap; original oscilloscope consumer untouched |
| w05_runtime_tests | Extended native editor, state, menu, input, audio and lifecycle regression coverage |
| w05_schema_security | Strict bounded JSON syntax and metadata validation with malformed-input fixtures |

Root integrated filter/LFO previews and the independently subscribed output tap.
The first published checkpoint is `adc55c04ca8ec5f4bab4125ae7ae1f2cef52cac4`.
Its Actions run reached package validation (all ten passed), then exposed an
existing pluginval configuration assumption that `surge-fx` is always enabled.
Target guards correct that configuration error; it was not a successful native
build or a screenshot capture. The original release matrix also encountered a
missing macOS signing configuration in this fork; this is separate from UI code.

## Wave 6 — measured layout refinement and portability (six agents, completed)

| Agent | Substantive result |
|---|---|
| w06_geometry_01_02 | Reference-measured Obsidian numeric fields and macro centers; Ivory knob ordering and cream palette |
| w06_geometry_03_04 | Spectral envelope/LFO geometry; Bauhaus labels, scene rows and actual-output plot layout |
| w06_geometry_05_06 | Space-Age brass/olive material hierarchy and value fields; Tokyo four-column oscillator controls |
| w06_geometry_07_08 | Aurora oscillator geometry and global fields; ClearSignal label ordering and contrast |
| w06_geometry_09_10 | Flow node/control geometry; Live compact values, macro colors and honestly bound performance faders |
| w06_build_portability | Unsigned fork macOS workflow, independent matrix builds and actual JUCE 7 compatibility checks |

Root added native compact value controls, top-positioned knob labels, panel font
sizes, theme-specific plot/menu contrast, and an actual output FFT/spectrogram.
The initial native build configuration error is fixed; run `43353009245` reached
native compilation. Rendering and runtime results are still pending at this point.

## Wave 7 — behavioral regressions and delivery infrastructure (six agents, completed)

| Agent | Scope |
|---|---|
| w07_header_fidelity | Real header navigation, native widget styling and rendering collisions |
| w07_audio_safety | Producer/consumer race review and bounded output-tap stress tests |
| w07_runtime_isolation | Deterministic editor fixtures, parameter controls and test side-effect isolation |
| w07_preview_regression | Filter/LFO/oscillator preview correctness and state-preservation tests |
| w07_distribution_guide | Modified-build installation guide and deterministic validated skin packaging |
| w07_visual_evidence_tools | Genuine-render comparison, contact sheets and explicit reference/native labels |

The output tap passed independent normal, ASan/UBSan and TSan stress tests over
eight million generated frames per run. The new tap never overwrites unread
storage. Native sample/parameter assertions still need the full linked test run.
The preview audit found shared random-state consumption in oscillator display
work. A narrow thread-local preview scope now avoids it while keeping the normal
audible drift path bit-identical (512-sample standalone comparison passed).
Twist and unsafe Alias mini-previews explicitly report unavailable instead of
touching shared audio memory or RNG state.

Packaging and visual-comparison tools passed their synthetic/reproducibility
self-tests. No reference-versus-native fidelity metric has yet been measured.
The full build at `4ffd53a` stopped on a GCC C++20 deprecation under `-Werror`;
`550d469` fixes platform-native path conversion without suppressing the warning.

## Wave 8 — reference geometry and native desktop triage (six agents, completed)

| Agent | Substantive result |
|---|---|
| w08_visual_01_02 | Widened Obsidian OCT labels using measured font bounds; reviewed Ivory geometry |
| w08_visual_03_04 | Reviewed Spectral/Bauhaus reference geometry and identified renderer-owned color/label differences |
| w08_visual_05_06 | Corrected Space mixer gutter, top macro labels and six-plus-two FX layout; reviewed Tokyo |
| w08_visual_07_08 | Measured Aurora/ClearSignal surfaces and identified gradient/modulation-column differences |
| w08_visual_09_10 | Corrected Live knob placement and selected the genuine spectrogram mode; reviewed Flow |
| w08_native_ci_triage | Isolated native GUI test initialization failure; separated behavior/capture steps and added a ready Xvfb/Openbox desktop |

All control IDs and bindings were retained. The static geometry/overlap validator
passed all ten manifests. These five visual assignments completed reference and
source review, **not native screenshot approval**; the actual capture comparison
is assigned to wave 9.

The complete native synthesizer test executable built successfully in runs
`43353929473` (`550d469`) and `43354634410` (`d6a06dd`). The latter also passed
normal and ThreadSanitizer audio-tap stress checks. Both GUI runs then stopped on
X11 `BadAtom` before producing images: bare Xvfb lacked window-manager atoms
required by JUCE. The Openbox wrapper starts an owned window manager, checks
readiness with a bounded timeout, preserves the native exit status and cleans up
only its child. Its success, exit-code propagation and startup-failure mock tests
passed. Its real native run is a separate, still-pending verification step.

## Verification status at this checkpoint

* Package validation checks geometry, IDs, field bounds, package confinement,
  and overlap. It is not evidence of visual identity or complete functionality.
* GCC syntax-only checks passed for shared sources after identified fixes.
* Native build/runtime and screenshot capture are delegated to the committed
  GitHub Actions job because local development-header downloads are denied.
* Each layout contains an explicit `adaptations` list. No 1:1 match is claimed
  without a completed native render comparison.
* Waves 1–8 are complete (48 distinct agents); waves 9–10 remain. Work is
  recorded as complete only when actually executed.
