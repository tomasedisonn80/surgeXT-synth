# Ten-wave continuation ledger

This ledger tracks the current numbered continuation from `786db23`, with six
substantive assignments in each of ten waves. The earlier reconstruction
[engineering log](../reference-workspaces-work-log.md) and the historical portions
of the [continuation](../reference-workspaces-continuation.md) describe earlier
work; their agent counts are not added to these sixty assignments.

**Progress snapshot at assignment 48:** assignments 1–46 and 48 are complete;
47 and 49 are active; 50–60 have not started. Completing a source or visual
review assignment does not mean its later code has passed native verification,
or that its skin matches the concept pixel for pixel.

The runtime permits three workers alongside the coordinator. Six assignments
in a wave therefore run in batches; this is not a claim of sixty concurrent
workers. The user authorized ordinary fast-forward publication to `main`.

## Verified checkpoints and publication

| Source / branch at this snapshot | Independent evidence | Result and limits |
| --- | --- | --- |
| `22b9c299e6aa74093c9e96677cb32df5f2f74e74` — published main checkpoint | [Native run 43382049170](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049170) | 33 behavior cases, 9,988,099 assertions; ten baseline/edited pairs, 24,095,778 capture assertions; normal and ThreadSanitizer audio-display checks passed. |
| Same `22b9c29` source | [Platform run 43382049249](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43382049249) | All seven platform build jobs passed: macOS, Ubuntu 20 Docker, Windows ARM64EC, Windows ARM64, Ubuntu ARM, Windows MSVC/JUCE 7, and Windows 64-bit. Release/nightly publication jobs were skipped; no installer or nightly publication is claimed. |
| `3ec3998877eab793f8bf2aa4acb367c69de17979` — verification branch | [Native run 43389991532](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43389991532) | 39 behavior cases, 10,298,881 assertions; all twenty native PNGs, 24,095,754 capture assertions; normal and ThreadSanitizer checks passed. Includes the new MIDI lanes, FX-power coverage, keyboard correction and first material/gallery integration. |
| `eccf282722f719b10a918424663244af0b5688fa` — verification branch | [Native run 43391258392](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43391258392) | 39 behavior cases, 10,298,881 assertions; all twenty native PNGs, 24,095,766 capture assertions; normal and ThreadSanitizer checks passed. Includes the measured Spectral/Bauhaus/Space/Tokyo/Aurora integration. Fresh images exist, but a successful capture does not establish visual identity. |

Additional ClearSignal, Flow and Live refinements and the active control fixes
follow the last verified source above. They need their own exact-source native
run and image review before final promotion.

The [gallery publication script](../../scripts/reference-workspaces/publish_native_gallery.py)
validates the successful main run's exact artifact, complete twenty-image set,
dimensions, logs, clean source provenance and reference names. Its dedicated
workflow job commits unmodified PNGs, evidence, hashes, README and HTML under
`doc/reference-workspaces/native-gallery` using an ordinary push. Verification
branch runs intentionally skip that publication job. At this snapshot the
pipeline is implemented and tested; publication of the final main gallery is
still pending.

## Wave 1 — audit, recovery and native foundation

| Assignment | Status | Substantive result |
| --- | --- | --- |
| 01 — Reference audit, Obsidian through Bauhaus | Complete | Measured typography, plots, browsers and control discrepancies against actual native captures. |
| 02 — Reference audit, Space-Age through Live | Complete | Prioritized material, browser, routing and widget corrections. |
| 03 — Native build investigation | Complete | Recovered exact Actions build/capture evidence and identified the local runtime limitations. |
| 04 — Fixture range repair | Complete | Corrected Space/Tokyo attack values outside the native parameter range, with explicit adaptations. |
| 05 — Native preset browser | Complete | Added real catalog scopes, categories, metadata cards, history and accessible navigation. |
| 06 — Cross-platform release audit | Complete | Corrected the Windows ARM RPC `small` macro collision without suppressing warnings. |

## Wave 2 — native displays and interaction

| Assignment | Status | Substantive result |
| --- | --- | --- |
| 07 — State-derived routing | Complete | Added actual modulation/audio connections, junctions, FX status and Wide/Serial3 routing corrections. |
| 08 — Wavetable materials | Complete | Added real-frame terrain surfaces, perspective floor, harmonic ribbons and luminous source projections. |
| 09 — Native segmented choices | Complete | Added Bauhaus oscillator/routing and Space scene/voice choices with full menus, gestures and accessibility. |
| 10 — Accessibility and interaction coverage | Complete | Corrected seven unnamed Live controls and added browser, scope, segment and transfer-state regressions. |
| 11 — Native FX transfer preview | Complete | Added a cached actual Distortion waveshaper preview; excluded unsafe shared-random Fuzz initialization. |
| 12 — Showcase state and metadata | Complete | Added genuine LFO/envelope routes with exact identities/depths and real patch metadata. |

## Wave 3 — first six individual skins

| Assignment | Status | Substantive result |
| --- | --- | --- |
| 13 — Obsidian | Complete | Refined real WT/FM/Classic control priorities, menus and oscillator dials. |
| 14 — Ivory | Complete | Added oscillator mute controls and actual Formant bindings; refined mixer/filter/ADSR geometry. |
| 15 — Spectral | Complete | Refined filter floors, live captions and LFO framing. |
| 16 — Bauhaus | Complete | Measured oscillator/filter/envelope/LFO centers, scene rows and compact native choices. |
| 17 — Space-Age | Complete | Refined actual Unison/Detune controls, LFO alignment and master placement. |
| 18 — Tokyo | Complete | Calibrated graphite panels, ADSR alignment and real Classic/FM2 binding order. |

## Wave 4 — remaining skins and real showcase state

| Assignment | Status | Substantive result |
| --- | --- | --- |
| 19 — Aurora | Complete | Refined dark hero backdrop, real-frame ribbon colors, source previews and Classic controls. |
| 20 — ClearSignal | Complete | Improved readable values, cutoff/macro typography and LFO geometry; checked intentional corner selectors. |
| 21 — Cartographer | Complete | Separated actual source cables, bottom routing lane and count caption; clarified orthogonal audio paths. |
| 22 — Live | Complete | Refined curved macro columns, truthful XY heading, filter containment and master/meter placement. |
| 23 — Actual source families | Complete | Used existing factory/native oscillator families and LFO rates, with genuine Live XY values. |
| 24 — Header interaction/status | Complete | Added real preset-browser actions and atomic CPU telemetry in Live, Tokyo and Aurora. |

## Wave 5 — plots, keyboard, power and placement

| Assignment | Status | Substantive result |
| --- | --- | --- |
| 25 — Filter/LFO plots | Complete | Added actual unity-gain grid/captions, interpolated cutoff markers and bounded traces. |
| 26 — Live keyboard | Complete | Added MIDI 0–120, key-derived C−2…C8 labels and native Single/Split/Dual/Channel Split range strips. |
| 27 — Space-Age materials | Complete | Separated brass/olive reflections, upright headings and annular dial details. |
| 28 — FX power | Complete | Added 48 accessible native slot switches in six skins with actual disable bits and suspension. |
| 29 — Interaction regressions | Complete | Covered real header actions, focus/skin changes, key identities and scene routing. |
| 30 — Placed-control audit | Complete | Checked outer/nested bounds, power children and keyboard footer; found the covered Tokyo Browser button. |

The stock hidden FX-disable bitmask path has no dedicated host-automation or
undo gesture. Slot power is separate from selecting an FX card or global bypass.

## Wave 6 — MIDI, regression coverage and capture delivery

| Assignment | Status | Substantive result / report |
| --- | --- | --- |
| 31 — Native MIDI lanes | Complete | Actual Pitch/Mod/Expression/Touch input, applied-state telemetry and bounded queue/spring ordering. [Report](agent-reports/w06_a31_native_midi_lanes.md) |
| 32 — MIDI schema and layout | Complete | Matching native/Python contract, generated-ID protection, measured four-lane layout and 168 shared cases. [Report](agent-reports/w06_a32_midi_schema_manifest.md) |
| 33 — FX-power regressions | Complete | Exact slot-mask/listener behavior, independent selection/bypass, Off/reassignment and all 48 switches. [Report](agent-reports/w06_a33_fx_power_tests.md) |
| 34 — Actual-source projection | Complete | Refined perspective and luminous material while retaining every real sample/frame/phase ordinate. [Report](agent-reports/w06_a34_wave_projection.md) |
| 35 — MIDI interaction tests | Complete | Four native cases covering engine targets, incoming MIDI, saturation, accessibility, entry and spring lifecycle. [Report](agent-reports/w06_a35_midi_interaction_tests.md) |
| 36 — Native gallery pipeline | Complete | Validated same-run artifact publication, unmodified images, provenance, normal push and stale-source guards. [Report](agent-reports/w06_a36_native_gallery_pipeline.md) |

The first linked MIDI test build found ambiguous JUCE point construction in two
test calls; explicit `juce::Point<float>` arguments corrected it. The next run
found a real stale-value keyboard defect after pitch release. Starting a new
key gesture from applied input fixed that case without weakening its assertion.
The passing `3ec3998` run verifies these changes. The later rapid-release
sequence found in assignment 47 is tracked separately as assignment 49.

Gallery validation used the genuine `22b9c29` artifact: twenty byte-identical
PNGs, thirty file hashes, all local image links, deterministic regeneration,
31 rejected malformed-input cases and normal/stale/rejected-push Git fixtures.
This verifies the pipeline; it is not a fidelity score or a completed main
gallery publication.

## Wave 7 — measured visual pass, Obsidian through Tokyo

| Assignment | Status | Substantive result / report |
| --- | --- | --- |
| 37 — Obsidian | Complete | Preserved close geometry, corrected the FX adaptation note and identified the real selected-algorithm title/transfer-ink correction. [Report](agent-reports/w07_a37_obsidian_final_fidelity.md) |
| 38 — Ivory | Complete | Bound the selected Chorus detail to genuine Rate/Depth/Mix/Width and identified heading/macro weight refinements. [Report](agent-reports/w07_a38_ivory_final_fidelity.md) |
| 39 — Spectral | Complete | Repositioned Noise/Ring captions, aligned two filter columns and reduced cutoff faces to measured diameter 35. [Report](agent-reports/w07_a39_spectral_final_fidelity.md) |
| 40 — Bauhaus | Complete | Calibrated red/filter-blue/heading-blue/LFO-yellow surfaces while preserving geometry; identified dark filter menus and output coating. [Report](agent-reports/w07_a40_bauhaus_final_fidelity.md) |
| 41 — Space-Age | Complete | Exposed actual Delay Mix/Depth/Width in the reference positions and measured stronger dial materials. [Report](agent-reports/w07_a41_space_final_fidelity.md) |
| 42 — Tokyo | Complete | Lifted the measured canvas background and identified cyan-only real wavetable ink, brighter readouts and scoped capsules. [Report](agent-reports/w07_a42_tokyo_final_fidelity.md) |

These reviews inspected actual `22b9c29` captures. The coordinator integrated
shared paint changes alongside the manifests. The `eccf282` native run covers
the combined implementation, but the old comparison crops do not certify the
changed pixels; inspect the new exact-source captures.

## Wave 8 — remaining skins and integration review

| Assignment | Status | Substantive result / report |
| --- | --- | --- |
| 43 — Aurora | Complete | Reduced four filter faces to diameter 35; measured darker panel/macro surfaces and genuine selected-FX title inheritance. Inspected the actual `3ec3998` source ribbons. [Report](agent-reports/w08_a43_aurora_final_fidelity.md) |
| 44 — ClearSignal | Complete | Aligned Filter 2 cutoff by +2/−1 px; retained the matching outer graph and verified free external-axis bands. [Report](agent-reports/w08_a44_clear_final_fidelity.md) |
| 45 — Cartographer | Complete | Aligned the cutoff center/outer radius and identified recessed caps, stronger real-value arcs and external dB captions. [Report](agent-reports/w08_a45_flow_final_fidelity.md) |
| 46 — Live | Complete | Reduced Amp Gain face to measured diameter 56 and measured thin rails/caps. Suggested genuine MIDI demonstration inputs, explicitly not implemented by this assignment. [Report](agent-reports/w08_a46_live_final_fidelity.md) |
| 47 — Renderer/control-state review | Active | Found a real generic linear-control mismatch between painted travel and JUCE hit geometry; implementing aligned layouts and a pointer regression. Also identified the rapid MIDI-release sequence assigned to 49. |
| 48 — Integration ledger and report archive | Complete | Created this truthful sixty-assignment progress ledger, updated the continuation and curated reports 31–46 with source/evidence context. |

## Wave 9 — verification assignments

| Assignment | Status | Scope / result |
| --- | --- | --- |
| 49 — Rapid MIDI release ordering | Active | Fix and regress release → new key before the next audio block, preserving external-input semantics and queue ordering. |
| 50 | Not started | No assignment result recorded. |
| 51 | Not started | No assignment result recorded. |
| 52 | Not started | No assignment result recorded. |
| 53 | Not started | No assignment result recorded. |
| 54 | Not started | No assignment result recorded. |

## Wave 10 — final verification and delivery

| Assignment | Status | Scope / result |
| --- | --- | --- |
| 55 | Not started | No assignment result recorded. |
| 56 | Not started | No assignment result recorded. |
| 57 | Not started | No assignment result recorded. |
| 58 | Not started | No assignment result recorded. |
| 59 | Not started | No assignment result recorded. |
| 60 | Not started | No assignment result recorded. |

## Fidelity and evidence boundaries

All delivered plots, values, routes, meters, browser rows, MIDI state and control
feedback must follow the real processor. Reference pixels are comparison inputs,
not a frozen interface. Source-preview geometry can change its projection,
lighting and material; it must not substitute invented samples for actual data.

Native adaptations remain material to comparison: actual algorithms and units,
unsupported sample-player or arbitrary-routing concepts, two real scenes,
performance operations without a direct native counterpart, and current patch
values. The [user guide](../reference-workspaces-user-guide.md) and each manifest's
`adaptations` list describe those distinctions. Aurora's real source ribbons,
for example, remain simpler than the concept's crossing traces; passing the
native gate does not erase that difference.

Native images are 1586 × 992 JUCE component snapshots attached to the processor.
They are not OS-desktop or DAW screenshots. Baseline and edited images, references
and comparison overlays must stay separately labelled. Build success, geometry,
behavior, audio checks and visual inspection answer different questions.

## Next integration gate

Finish the active control fixes, inspect their source changes, and run the
unchanged native behavior, audio-display and ten-pair capture gates on the exact
candidate. Review its new images, finish assignments 50–60, fast-forward the
verified source to main, verify the required platform builds, and let the
successful current-main gallery job publish its own evidence. Update this ledger
with actual results as those steps occur; do not turn planned work into completion.
