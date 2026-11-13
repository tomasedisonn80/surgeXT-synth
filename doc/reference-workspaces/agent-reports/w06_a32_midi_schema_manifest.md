# Assignment 32 — native MIDI lane schema and Live layout

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Exact contract

- New panel kind: midi.
- Required controllers array: 1–4 supported strings selected from pitch, mod,
  expression, aftertouch. Each controller occurs only once per workspace.
- Generated IDs workspace.midi.<controller> are reserved in the shared
  component-ID set. Earlier/later declared panels or ordinary parameter controls
  cannot reuse an ID, and the MIDI panel cannot reuse one as its own ID.
- Full panel rectangle divides equally into left-to-right lanes without a gap.
  Minimum width is 24 pixels per lane; minimum height is 64 pixels.
- Title must be empty; optional titleHeight must be numeric zero.
- No index, plot, plot2, selection, variant or titleInset. The controllers field
  is invalid on other panel kinds or ordinary parameter controls.
- No invented ParameterBridge bindings or host parameter IDs are introduced.

## Live layout

Added p.midi-performance at (19, 831, 152, 128). Agent 31 confirmed that four
38-pixel native children with 20-pixel glass tracks place their centers at
38, 76, 114 and 152, matching the reference performance strip.

Child bounds are (19,831,38,128), (57,831,38,128), (95,831,38,128) and
(133,831,38,128). They end at x171, leaving 14 pixels before Scene Controls.
Generated MIDI hit areas are included in Python overlap validation.

Removed exactly c.performance-octave, c.performance-glide, c.performance-width
and c.performance-pan. Scene settings, the playable keyboard and all remaining
parameter bindings are unchanged. Live now contains 22 panels and 64 ordinary
manifest controls plus four MIDI lanes.

The adaptations and user guide describe actual Pitch bend, Modulation CC1,
Expression CC11 and channel pressure. Pitch and Touch return to zero after an
active interaction; Mod and Exp retain native controller values. Unsupported
scene morphing, six snapshots and three independent keyboard zones are not
claimed.

## Verification repeated after recovery

- Python self-test passes all 168 shared cases: 28 accepted and 140 rejected,
  including 37 added MIDI contract cases. Existing malformed JSON, Unicode and
  confined-path checks also pass.
- Cases cover missing, empty, overlong or malformed arrays; unsupported members;
  duplicates within and across panels; minimum dimensions; unsupported fields;
  title semantics; invalid field scope; and generated-ID collisions.
- All ten manifests pass strict overlap validation. Existing intentional
  ClearSignal plot/menu warnings remain unchanged.
- Explicit Live lane bounds, centers, adjacency, the 14-pixel scene-card gap and
  complete interactive overlap checks pass.
- git diff --check passes.
- Manifest.cpp passed the C++20 warning/deprecation-as-error syntax check before
  environment loss. The same source changes were replayed, but that compiler
  check was not rerun in the fallback without restored dependencies.
- The native Catch test already consumes the same shared JSON fixture. Linked
  execution, MIDI interactions and native screenshots remain separate CI gates.

All changes remain unstaged; the coordinator owns commits and publication.

## Restored file hashes

- src/surge-xt/gui/reference/Manifest.cpp: 5fd81c404557e09680970f1358eca78bcca9ef36939bf960011f5683245c305e
- scripts/reference-workspaces/validate.py: e79321321e7c318ebc5c6b61a65bfce96481ac72aa4394f3f1f53a7b72d1a83a
- scripts/reference-workspaces/fixtures/selection-schema.json: dcaf86c82bb356e54b299cf9ef7f91df44261e4c05e971e2e4599e54d3822e25
- resources/data/skins/reference-10-live.surge-skin/workspace.json: 2f2b7d9808b8505de7c19774a3a8b963906c58a1eb5ae2338f3870a22a13d270
- doc/reference-workspace-selection-schema.md: 2fe1175d216616617d5fc8dae561897d8488c71bff1192b1c2905f0f8e5dbba4
- doc/reference-workspaces-user-guide.md: a0f0a9fd87787840415b71810dd44b550aaed7a15bad6bd73ae90da7f0eb907b
