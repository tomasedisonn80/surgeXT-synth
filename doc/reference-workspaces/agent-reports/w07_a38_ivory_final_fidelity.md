# Assignment 38 — Ivory final fidelity refinement

> Curated historical assignment report. The implementation was subsequently included in passing native source `3ec3998`; the ledger records the exact run and results. Statements below about pending integration or unstaged work are the original completion-time record, not current branch status. See the [ten-wave ledger](../ten-wave-ledger.md) for later verification and publication.
> Temporary execution paths were normalized, and recovery-only or temporary-image inventory sections were omitted. Labels such as `analysis-only/` identify uncommitted inspection intermediates, not delivered screenshots. Native-artifact labels identify the source-specific captures described below.

## Evidence and scope

- Reference: `doc/reference-workspaces/references/02_Ivory_Atelier.png`, 1586 × 992.
- Native comparison: `native-artifact-22b9c29/02-ivory-atelier.png`, compiled source `22b9c299e6aa74093c9e96677cb32df5f2f74e74`.
- Reviewed AGENTS.md, the user guide, current continuation log and relevant engineering-log history.
- Inspected a labelled overview and a 1:1 macro crop. Analysis-only JPEGs are `analysis-only/w07_a38_ivory_before.jpg` and `analysis-only/w07_a38_macros_before.jpg`.
- Owned repository file: `resources/data/skins/reference-02-ivory.surge-skin/workspace.json` only. No renderer, fixture, engine, test or publication changes.

## Implemented correction

The selected default Chorus detail showed Time, Rate, Depth and Feedback.
The reference prioritizes Rate, Depth, Mix and Width. The actual `ChorusEffect.h`
`chorus_params` enum confirms these native zero-based indices:

| Existing stable component ID | Previous FX1 binding | New FX1 binding | Native Chorus role |
| --- | --- | --- | --- |
| `ivory.fx1.param0` | `fx1.param0` | `fx1.param1` | Rate |
| `ivory.fx1.param1` | `fx1.param1` | `fx1.param2` | Depth |
| `ivory.fx1.param2` | `fx1.param2` | `fx1.param6` | Mix |
| `ivory.fx1.param3` | `fx1.param3` | `fx1.param7` | Width |

Changed both each control's base binding and its `bindings.fx1` entry. All
FX2–FX8 mappings remain byte-for-byte equivalent as JSON values. The controls
still use actual parameter labels, ranges, units, disabled state, typed entry
and gesture paths. No fixed reference captions or numerical values were added.
Chorus Width remains the engine's dB control; the illustration's percentage
is not substituted. The existing adaptation note now documents this priority.

## Geometry and remaining visible differences

- Kept all 17 panels, 75 control IDs, palette values, rectangles, face diameters
  and selection definitions unchanged. Large outer panel boundaries and the
  two macro rows already align closely in the actual image pair.
- The 1:1 macro crop shows matching dial placement and near-matching diameters.
  Different pointer positions reflect genuine assigned macro values.
- The reference uses heavier serif section headings and macro labels. The
  shared renderer's large-Ivory-heading branch explicitly selects a nonbold
  font. Reported this renderer-owned difference to the coordinator; no size
  substitution or manifest-only workaround was introduced.
- Reference filter-title power glyphs are absent in the native filter headers.
  Reported this separately; no nonfunctional icon or unrelated toggle was added.
- Reference Hybrid/Sub/extra dedicated-envelope concepts retain the documented
  native Classic/ring-modulation/LFO-envelope adaptations. Actual traces,
  response shapes, output spectra, route values and preset metadata remain
  engine-derived and therefore are not asserted identical to concept artwork.

## Validation

- Strict manifest validation passed for all ten skins after the binding changes.
- Explicit invariants verified: all IDs preserved; panels/palette identical;
  only the four intended controls' binding fields changed; all other selected
  FX-slot mappings unchanged.
- `git diff --check` passed; final patch is nine changed lines, including one
  adaptation paragraph. Original JSON Unicode escaping was preserved.
- Final manifest SHA-256: `d4d8447ba04057e2d4d8656d848e6ef1e6e26a189e9c5daecd24c425106e0ece`.
- No new linked-native or post-edit capture pass is claimed. The next native
  run must confirm the four Chorus labels/values and all alternate selections.

Changes remain unstaged. No commits, branch updates or pushes were performed.
