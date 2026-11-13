# Assignment 58 — delivery documentation and asset provenance

> Curated historical assignment report. These guide corrections were integrated before final source 453df569. The report's older asset/gallery checks remain qualified by their original source. Later native asset, gallery and image evidence is recorded separately in the final ledger and verification record. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Outcome

Completed the user-facing delivery audit against source
`a9d0d0a838632daf178255561def4622cd208258`. No blocking installation,
navigation or provenance defect was found. Two agreed documentation corrections
were applied to `doc/reference-workspaces-user-guide.md`; the file remains
unstaged for the coordinator's final commit.

The final source may receive three cosmetic changes following assignments
55–56. This report does not claim those later renders or the pending platform
matrix have passed. It also does not claim that the final gallery is published.

## Corrections implemented

1. Added an explicit explanation that build archives named `pluginsonly` omit
   factory data. Users must choose a distribution containing that data or
   provide matching factory data, including the ten skin folders, separately.
   The guide now identifies the Windows ARM64, ARM64EC and JUCE 7 variants as
   producing those archives without a full installer or portable package
   containing resources.
2. Moved the existing Live incoming-MIDI capture paragraph from after
   Troubleshooting into **Validation and real captures**, immediately before
   **Committed native gallery**. Its values and meaning are unchanged.

The distribution wording follows the actual
[distribution CMake source](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/cmake/lib.cmake):
the plugins-only archive is created before resource copying, and the Windows
ARM64/ARM64EC/extra-variant branches omit the full-resource portable and installer
paths. This is distinct from the much smaller skin-only data ZIP.

Only the user guide was edited by this assignment: 11 added and 7 removed lines.
No C++, manifest, fixture, workflow, image, wavetable or other document changed.

## Installation and navigation findings

- The opening requirement is explicit: these layouts need a compatible build
  of this modified fork. A normal upstream Surge XT binary does not acquire the
  new renderer by copying the skin folders.
- The ten displayed skin entries and package folders match their `skin.xml`
  names. Some manifest display names omit their numeric prefix, but the native
  skin menu entries carry the prefixes shown in the guide.
- The guide keeps `skin.xml` and `workspace.json` together, describes user-copy
  precedence, and uses the actual user skin folder rather than guessing
  platform paths.
- Verified that **Install a New Skin...** opens `storage.userSkinsPath`,
  **Rescan Skins** rescans and restores the selected entry when available, and
  developer menus expose the user data folder. These operations appear in
  [SurgeGUIEditorMenuStructures.cpp](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/surge-xt/gui/SurgeGUIEditorMenuStructures.cpp).
- **Full editor**, the **Return to reference workspace** footer, Ctrl/Command
  plus Alt/Option+R, and the workspace's Ctrl/Command+K skin menu agree with
  [SurgeSynthEditor.cpp](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/surge-xt/SurgeSynthEditor.cpp)
  and [Workspace.cpp](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/src/surge-xt/gui/reference/Workspace.cpp).
- The guide accurately separates Macro 1/2 XY control, native scenes, real
  browser contents and original-editor navigation from unsupported concept
  operations. It does not promise that a skin creates a separate sample player,
  arbitrary cable engine or scene-morph processor.
- MIDI lanes are documented as input events rather than host parameter knobs:
  Pitch/Touch return, Mod/Expression latch, and resulting sound follows the
  current native patch. The capture paragraph describes actual incoming channel
  1 events: signed pitch +6616 / 14-bit 14808, CC1 104, CC11 56 and pressure 85.

These are source/document checks, not a claim of manual installation or DAW
operation on every platform.

## Signal displays, assets and licensing statements

The earlier obsolete “stacked current waveform only” sentence is corrected.
The current guide says terrain/ribbon views project copied source frames and
distinguishes them from actual output-waveform, spectrum and meter data. It also
correctly limits filter curves to base parameters and LFO thumbnails to their
documented native models.

The [wavetable generator](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/scripts/reference-workspaces/generate_showcase_wavetables.py)
carries `SPDX-License-Identifier: GPL-3.0-or-later` and describes original
periodic harmonic signals consumed by Surge's normal loader and oscillator.
Its source contains analytic sine/harmonic families, floating-point WT encoding,
periodicity/DC/peak checks and deterministic hashes. It does not read image
pixels.

Ran the existing generator with `--check`. All **10** checked-in native
wavetables matched regenerated bytes, totaling **2,982,008 bytes**. No output
asset was written by that check. The project [LICENSE](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/LICENSE)
is present, and the native-gallery publisher also carries the GPL-3.0-or-later
identifier. No new external font or image asset was introduced by this audit.

The retained
[reference design notes](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/doc/reference-workspaces/references/DESIGN_NOTES.md)
identify the ten images as generated concept art, and explicitly distinguish
them from implemented interfaces and exact engineering specifications. The
user guide maintains that distinction and points to the real adaptations.
This audit makes no additional ownership or legal-licensing assertion about
the supplied reference artwork.

## Gallery labels and publication boundary

Inspected the actual
[gallery generator](https://github.com/tomasedisonn80/surgeXT-synth/blob/a9d0d0a838632daf178255561def4622cd208258/scripts/reference-workspaces/publish_native_gallery.py).

Its HTML and README distinguish **Reference concept**, **Native baseline** and
**Native edited state**, include source/run/evidence/hash links, and explicitly
limit the images to JUCE component snapshots. They do not claim OS desktop
captures, DAW certification or pixel identity. The README correctly says the
HTML comparison page must be downloaded with the gallery and sibling reference
directory, while the Markdown gallery renders directly on GitHub.

At **08:41 UTC on 13 November 2026**, public main still identified `a9d0d0a`
and the future gallery README returned 404 because publication had not occurred.
The guide describes that link conditionally (“After the complete native
workflow passes...”); it is not represented here as an already available file.
The coordinator/assignment 57 must verify the final published target, image bytes
and links after the source and gallery are settled.

The guide contains no claim that a release/nightly has been published. Its
macOS signing language distinguishes unsigned fork artifacts from signed and
notarized upstream releases. Its accessibility and fidelity statements remain
qualified by actual evidence.

## Checks and handoff

- `git diff --check -- doc/reference-workspaces-user-guide.md`: pass.
- Exactly ten workspace table rows: pass.
- Live MIDI paragraph occurs once and lies in the capture section: pass.
- Explicit `pluginsonly`/factory-data distinction: pass.
- Existing original-wavetable `--check`: all ten match, no writes.
- Only prospective relative link in this guide: native-gallery README, pending
  publication as recorded above.
- No staging, commit or push performed.

Updated user-guide SHA-256:
`c4c6c30049044264bea2c361a9c727ce98d07a3283641646db028066903a1684`.

The guide correction is ready for the coordinator's final source/documentation
commit. The report's gallery-publication status is deliberately historical;
later integration evidence belongs in the final ledger.
