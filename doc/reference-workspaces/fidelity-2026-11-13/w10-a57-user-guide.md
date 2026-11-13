# Wave 10 / assignment 57 — current user guide

## Scope and changes

Updated only [the user guide](../../reference-workspaces-user-guide.md) and this
report. Read AGENTS, continuation, engineering log, current-pass README and
assignments 49–54/56; coordinated directly with assignment 55 and inspected its
current `Visuals.cpp` changes. Review started at gallery HEAD
`762d888f7159558db472836a8f9b5f015c1f9427`, with the later corrections in the
shared working tree. Historical delivery/verification records remain intact.

The guide now explains:

- The modified binary and matching factory data requirement, including the
  playable `Reference Showcases` tables omitted by skin-only/`pluginsonly` ZIPs.
- Actual Settings → Skins selection/installation and the developer-menu Data
  Folders route, plus ordinary wavetable catalog selection and rescan behavior.
- All four revised sources: Spectral Bloom, Bauhaus Waves, Aurora Waves and
  GlassWavetable 03. Spectral/Bauhaus/Aurora retain relative frame levels with
  one table-wide source scale; GlassWavetable 03 retains per-frame peak scaling.
  Selecting a skin alone does not load these sources or the capture fixtures.
- The measured Space global strip's real scene/voice, Glide, Pitch, Width and
  master-output operations. Width retains its actual native percentage readout;
  the guide does not promise the artwork's unsupported tuning/morph operation.
- Route physical readouts refreshing with target values, host changes and state
  restore, separately from normalized depth; ClearSignal uses this native data.
- Spectral's whole-table camera/floor framing, source-driven ribbon lighting,
  actual waveform fallbacks and explicitly unavailable previews. A55's shared
  camera offset/reduction is distinguished from altering the playable samples.
- Stable repository links to this pass record and the gallery, whose own exact
  source/run identifies the verified images. No final native-pass claim is added.

## Checks

Inspected `SurgeGUIEditorMenuStructures.cpp`, `SurgeSynthEditor.cpp`, the actual
skin XML metadata, Space's manifest, `ParameterBridge.cpp`, `SurgePatch.cpp`,
`Workspace.cpp`, `Visuals.cpp`/`.h`, `XTTestReferenceWorkspaces.cpp`, the wavetable
generator and skin packager. The Settings/Skins path, invalid-manifest fallback,
dirty-patch prompt, native editor/overlay actions and return footer agree with
the guide. All ten displayed names/category values match the actual XML files.

Parsed the four source headers and sizes directly: Spectral is 256 × 2048;
the other three are 48 × 512, all with the expected `vawt` header/float payload.
The generator's scale-family set is exactly `dunes`, `bauhaus`, `aurora`.
No assets were regenerated. Package self-test passed: ten validated skins,
25 archive entries, reproducible ZIP bytes, hash/CRC/metadata checks and path/
overwrite guards. Scoped `git diff --check` passed.

## Delivery prerequisite

The latest inspected native gallery still records source
`ea674878313006afa0b9d1a2b1d3495b69755627` / run `43497923810`. It predates the
inspector refresh and final hero/material corrections. The coordinator must
complete the combined exact-source native build, behavior/capture checks and
visual review, then publish that source and its genuine gallery. This assignment
does not rerun or certify that native gate. No code, manifests, screenshots,
commit or push were changed by this assignment.
