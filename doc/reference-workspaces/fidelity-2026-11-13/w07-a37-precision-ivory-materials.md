# Wave 7 / assignment 37 — Obsidian, Ivory and Bauhaus materials

## Evidence and scope

Implemented the measured priorities from [assignment 32](w06-a32-obsidian-ivory-review.md)
and the matte-material finding in [assignment 33](w06-a33-spectral-bauhaus-review.md).
Read AGENTS, continuation, engineering log, user guide and this pass's README.
The reviewed native gallery was subsequently refreshed to source
`0708bc76bb194a127064cf18b8849cf77373a7e7`, run `43493343847`; its
[provenance](../native-gallery/build-provenance.txt) does not verify this patch.

Changes are confined to `Workspace.cpp::bezel`, the Ivory brand treatment,
the Ivory master slider's paint layout and reference-02's existing master
definition. No bindings, events, scenes or source assets were changed.

## Decisions

- **Obsidian:** enclosure panels whose fill is exactly `theme.panel` receive a
  bounded RGB lift and a 30-pixel title lip. With the current palette, gradient
  targets are `26/31/36` at the upper lip, `24/29/33` across its title band, and
  `17/21/24` at the lower body. Display wells and selected-control fills are
  outside this correction. These are paint inputs, not new screenshot samples.
- **Ivory:** the cream interpolation, paper grain and bright inset highlight
  now require a pale, warm fill. Dark green preset/selection surfaces retain
  their state-dependent input fill and receive a restrained green finish.
  Exact display fills retain their separate dark base treatment.
- **Ivory header:** brand geometry moves to `[102,20,190,39]`, with 34-pixel
  type and `.985` horizontal scale; the tagline moves down two pixels and
  right one. Direct green-arc measurements found reference x1399–1460 versus
  native x1392–1459, refining the earlier approximate seven-pixel shift: the
  correction also needs a smaller dial. Its 66-pixel paint face is centred at
  `(1430,44)`, versus the old 72-pixel face at `(1426,46)`.
- **Master readout:** the existing slider now occupies `[1312,10,206,72]`.
  Its caption sits left, with the actual `bridge.display(binding)` value right
  of the dial; no reference value is substituted. The previous header action
  ends at x1310 and meters begin at x1536. Existing drag, entry, automation,
  focus and accessibility paths remain in use.
- **Bauhaus:** non-display panels gain clipped deterministic paper tooth.
  Blue coatings use a cached cosmetic hash tile with restrained asymmetric
  light/dark variation; red/cream panels reuse the existing neutral grain.
  Blue lower-fill RGB offsets are `+2/0/-2`. An additional lower-red blank
  measurement was reference `193/56/36`, native `197/61/41`, so the red lower
  finish is adjusted as well. No reference pixels or engine RNG are used.

## Checks and handoff

`python3 scripts/reference-workspaces/validate.py --strict-overlap` passes all
ten manifests. It retains the existing two ClearSignal plot/control warnings.
`git diff --check` passes. Reviewed master paint extents and neighboring header
geometry in source; actual placed child bounds and new rendered pixels remain
for the combined exact-source native gate. No paint-only tests were added.

The shared `bezel()` region is released for the next Space/Tokyo worker.
Fresh captures must check Obsidian lip/body samples, Ivory green medians and
header glyph bounds, and Bauhaus grain channel ranges before visual acceptance.
No build, runtime or pixel-identity result is claimed for these uncommitted edits.
