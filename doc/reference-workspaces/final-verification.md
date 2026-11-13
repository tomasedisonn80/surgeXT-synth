# Ten reference workspaces — verification record

**The native implementation, audio checks, screenshots, published gallery and all seven platform build/package jobs are verified for source `453df569`.** Final visual reviews found no new functional, clipping or spacing regression in the inspected areas, while documenting remaining differences from the reference images. Literal 1:1 pixel identity is not established.

## Source and verification status

| Item | Identity and result |
| --- | --- |
| Tested source | [`453df569c977350c3a4da2caf2f0ab7cbd1a0e71`](https://github.com/tomasedisonn80/surgeXT-synth/commit/453df569c977350c3a4da2caf2f0ab7cbd1a0e71) |
| Source tree | `41b53daf055b5fbf9505dc43b72e06b378879901` |
| Native main workflow | [Run 43400507719](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507719), job 131801876413 — **success** |
| Native behavior | **42 test cases / 10,316,058 assertions passed** |
| Native captures | **10 baseline and 10 edited images; 24,100,990 capture assertions passed** |
| AudioTap checks | **Normal and ThreadSanitizer checks passed** |
| Exact main artifact | **14024468912**, a verified 11,019,495-byte ZIP |
| Published gallery | [`29de74bee7e5a953d7d7102408ceef0541d04919`](https://github.com/tomasedisonn80/surgeXT-synth/tree/29de74bee7e5a953d7d7102408ceef0541d04919/doc/reference-workspaces/native-gallery) — **direct byte comparison passed** |
| Platform matrix | [Run 43400507710](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710) — **success: all seven build/package/upload jobs** |
| Visual review | **Completed for the final source, with remaining differences recorded below** |

The latest source changes are confined to bounded Aurora projection/light/inset adjustments, real glyph outlines for two captions, and Spectral cap gradient coordinates. They do not change source assets, the audio engine, bindings or declared layout.

The machine-readable companion is [final-verification.json](final-verification.json). The [structured platform evidence](agent-reports/w10_a59_final_platform_evidence.json) preserves the seven upload/log comparisons, seventeen listed packages and explicit distribution limits. The [ten-wave ledger](ten-wave-ledger.md) records the assignments and integration history; the [curated archive](agent-reports/README.md) preserves source-qualified reviews.

## Inventory and work record

The implementation contains **ten native skin packages, 194 manifest panels and 779 declared controls**, on a **1586 × 992** logical canvas. Generated native child components are separate from these declared-control counts.

| Skin | Panels | Declared controls |
| --- | ---: | ---: |
| Obsidian Precision | 14 | 83 |
| Ivory Atelier | 17 | 75 |
| Spectral Observatory | 18 | 69 |
| Bauhaus Modular | 25 | 79 |
| Space-Age 1977 | 21 | 90 |
| Tokyo Industrial | 18 | 92 |
| Liquid Aurora | 21 | 99 |
| ClearSignal Accessible | 27 | 77 |
| Signal-Flow Cartographer | 11 | 51 |
| Live Performance Deck | 22 | 64 |
| Total | 194 | 779 |

**Sixty numbered assignments were carried out in ten waves of six.** The scoped implementations and reviews were delivered, with later follow-up turns retaining their original assignment numbers. Up to three child workers ran alongside the coordinator. These counts describe actual assignments, not sixty simultaneous workers or proof of reference-image identity.

## Native execution and audio

The exact main run compiled the native component and passed the behavior, capture and audio-display gates. Capture evidence records a clean source tree, source `453df569`, 48 kHz audio and 512-sample blocks. All 779 declared controls resolved.

Each skin has four measured active stages: baseline, edited, host-updated and restored. **All 40 active peaks** were finite, greater than 0.0001 and less than 1. Their measured range was **0.013976696878672–0.692262887954712**. **All 10 post-reset peaks** were below 1e-6 in absolute value; the maximum was **5.429840577164669e-9**. These are the unchanged gates.

The filter-response correction passed on this final source. Independent reviews reproduced all ten response-plot pixel counts and native sample deltas. Aurora and Live showed 3,028 and 1,735 changed curve-only pixels after an edit, respectively, compared with zero in the earlier stale-capture checkpoint. The regression exercises consecutive real edits after one production refresh.

## Native screenshot and gallery provenance

Screenshots use `Component::createComponentSnapshot` attached to the real `SurgeSynthProcessor`, with actual audio, parameter and MIDI state. They are native component renders, not operating-system desktop or DAW screenshots.

Artifact **14024468912** explicitly belongs to **main / source 453df569 / run 43400507719**. Its independently calculated ZIP SHA-256 matches the API digest:

`4054102ba04157671f0d3464616fdd0575514f657a8c22b089f4eeabd74b7246`

The audit downloaded all 31 gallery files from immutable commit `29de74b` and compared them with this same main-run artifact:

- All **20 native PNGs** fully decode at **1586 × 992** and are byte-for-byte identical.
- All **28 copied artifact files**, including the evidence and logs, are byte-for-byte identical.
- All **30 recorded file hash/size entries** and **10 retained-reference hashes** match.
- All ten full-image edited-pixel counts were independently reproduced.
- All local HTML and Markdown links resolve. The HTML contains 30 separately labelled reference/native/edited images.

The gallery commit is a single-parent child of the tested source. The verified ancestry is `8588c50 → 453df569 → 29de74b`, with no divergent commits. Its 25 changed files are confined to the gallery directory; six unchanged files complete the 31-file gallery.

## Final visual findings and remaining differences

The final reviews inspected genuine native PNGs and retained the distinction between reference concepts, native captures and temporary engineering studies. No new functional, caption-clipping or control-overlap defect was found in the inspected regions.

| Area | Verified final-source result | Remaining difference |
| --- | --- | --- |
| Aurora hero | Bright/saturated coverage is 37.005%, versus 39.997% in the reference. Its measured vertical extent is y6–176, versus y4–177. Controls remain clear; no pixels outside the hero changed in the bounded correction. | Actual source-family crest positions and crossing topology differ; the reference has finer cross-weave variation, peripheral wisps and a higher mean blue level. |
| Spectral hero and caps | The actual audible peak family, signed source mesh, lower plane and native position marker remain intact. The hero matches the previously reviewed 4dbb414 crop exactly. Sampled upper/lower cap colors are within three/one RGB channel levels of the reference. | The native surface remains a cleaner wire/ribbon mesh than the reference's denser, craggier terrain. |
| Bauhaus output heading | Measured ink bounds exactly match the reference: 175 × 13 pixels. Dark-pixel count is 1,005, versus 1,012 in the reference. | Matching bounds and close weight do not establish identical glyph rasterization. |
| Space macro captions | Left/top alignment and nine-pixel height match. All four real captions fit without clipping or overlap. | The sampled caption is 44 pixels wide versus 43; its dark-pixel count is 182 versus 160. |

These measurements are **diagnostics, not fidelity percentages**. Actual algorithms, units, preset content, two-scene behavior, unsupported reference feature concepts and changing signals also retain the documented native adaptations. The [user guide](../reference-workspaces-user-guide.md) and each manifest's adaptation notes explain those boundaries.

The working UI uses native parameter bindings, loaded source frames, filter models, incoming MIDI, actual routing and preset data, and output-tap measurements. Reference images are comparison inputs, not UI backplates. The renderer does not substitute frozen meters, invented signal traces or fake parameter values.

## Seven-platform gate

All seven required jobs completed successfully for the exact main-source push. Each passed its build, installer-directory inspection and artifact-upload steps. This matrix compiles and packages native distribution targets; the behavior and capture tests are verified by the separate native run above.

| Platform job | Job link | Final outcome |
| --- | --- | --- |
| Ubuntu 20 Docker | [131802758475](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758475) | **Passed** |
| Windows MSVC JUCE 7 | [131802758498](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758498) | **Passed** |
| Windows ARM64EC | [131802758523](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758523) | **Passed** |
| Ubuntu ARM | [131802758559](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758559) | **Passed** |
| Windows 64-bit | [131802758571](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758571) | **Passed** |
| Windows ARM64 | [131802758590](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758590) | **Passed** |
| macOS | [131802758650](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/job/131802758650) | **Passed** |

“Ubuntu 20 Docker” is the workflow's historical label; the configured image uses Ubuntu 22/GCC 12. The macOS job builds universal arm64/x86_64 targets and labels its fork artifact unsigned and unnotarized. The upstream-owner-gated release/nightly publication jobs were skipped. No release or nightly publication is claimed. These compile/package results do not certify every DAW or target operating system at runtime.


### Build artifacts from the tested source

These are artifacts uploaded by the successful workflow. They are separate from a published release or nightly.

| Target | Workflow artifact | Included distribution forms |
| --- | --- | --- |
| Linux x64 | [build-docker](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14026661290) | DEB, RPM, full archive, binaries archive and portable data |
| Linux ARM64 | [build-ubuntu-arm](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14025099917) | DEB, full archive and binaries archive |
| Windows x64 | [build-Windows 64-bit](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14026243663) | Installer, portable archive and binaries archive |
| Windows ARM64 | [build-Windows ARM64](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14026718414) | Binaries archive; matching factory data required |
| Windows ARM64EC | [build-Windows ARM64EC](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14027547557) | Binaries archive; matching factory data required |
| Windows x64 / JUCE 7 | [build-Windows MSVC JUCE 7](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14026147413) | Binaries archive; matching factory data required |
| macOS universal | [build-macOS-unsigned](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43400507710/artifacts/14025504610) | DMG, binaries archive and unsigned-build notice |

## Installation and commit identity

Use a build of **this modified fork** with compatible factory data containing the ten skin folders. Copying the folders into an unmodified upstream executable does not add the new renderer. Archives named `pluginsonly` omit factory data; install matching data separately or choose a distribution that contains it. The [user guide](../reference-workspaces-user-guide.md) describes the Windows ARM64, ARM64EC and JUCE 7 packaging limits.

In the modified build, select **Settings → Skins → Reference Workspaces**.

The tested source SHA identifies the code and resources compiled for these captures. The gallery and subsequent documentation commits have different SHAs. **The documentation commit is the commit containing this record, resolved from Git history.** Its own SHA is not embedded as a self-referential placeholder.

Before treating a later main commit as the same tested runtime, review its ancestry and confirm its diff is confined to documentation/gallery content. Preserve the captures' original source and run fields. The coordinator reports the actual final main SHA after publication and checks that descendant separately.

## Previous verified checkpoint

[Source 4dbb414](https://github.com/tomasedisonn80/surgeXT-synth/commit/4dbb414370c94932670e1b58cf6f5caecbe4c6f4), [native run 43398309849](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43398309849) and [gallery 8588c50](https://github.com/tomasedisonn80/surgeXT-synth/tree/8588c50e06902e6a924a472e76762fbc4e795313/doc/reference-workspaces/native-gallery) remain historical verified evidence. The final-source results above were independently repeated for 453df569; the earlier images were not relabelled.
