# Wave 9 / assignment 54 — Modulation state and inspector refresh

## Confirmed finding

Reviewed source `ea674878313006afa0b9d1a2b1d3495b69755627`, native
[run 43497923810](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43497923810)
and gallery `762d888f7159558db472836a8f9b5f015c1f9427`. Assignment 52 reported
the ClearSignal mismatch; independent image comparison confirms that its
inspector ROI `(1197, 548)–(1576, 759)` is pixel-identical in the baseline and
edited PNGs. The edited cutoff reads **115.14 Hz**, while the inspector still
reads **11.62 < 1420.00 > 50175.42 Hz**. Its **+0.64** normalized route depth
correctly remains unchanged.

This is a bounded stale-display defect, not fabricated data or a second,
independent base quantity. `WorkspaceData::modulationRows` calls the native
`Parameter::get_display_of_modulation_depth(..., InfoWindow, ...)`; its string
includes the destination's current base value and physical range. `Workspace`
previously cached that snapshot under `ticks++ % 8`, although bound controls
and filter responses update on every refresh. At the 20 Hz UI timer this can
leave seven intervening refreshes with old route text. The capture harness
exposes it by photographing the first refresh after the actual cutoff edit.

## Fix

Moved the existing route snapshot and `syncRouteControls` block into the
per-refresh path in `Workspace.cpp`. All skins now synchronize their native
route rows, accessible descriptions and tooltips with the current destination
values. ClearSignal's inspector paints that same refreshed data. Oscillator,
LFO, FX-preview and browser cadence remains unchanged.

The snapshot still runs only on the message thread, returns at most 128 rows,
and uses the existing try-lock. Route identity, selected-scene lookup,
zero-crossing retention and the active-edit guard are preserved. No processor
write, audio-thread operation, formatter alteration or painted value override
was added. Only ClearSignal has the full physical-range inspector; all ten
matrices use the shared route snapshot and native interaction components.

## Regression and checks

Added one native case, **Native modulation ranges follow base edits and state
restore on the first refresh**, adjacent to the existing modulation tests. It
checks all ten skins against the live native formatter after a native cutoff
control edit, host adapter update and actual saved processor-state restore,
with exactly one public refresh per change. It also checks row accessibility
text, stable normalized depth, Scene A/B destination separation, and preservation
of all host values, patch dirty state and both shared RNGs by refresh.

For ClearSignal, actual inspector snapshots must change after the edits and
return exactly to the baseline image after restore. The actual native depth
row must also survive a zero-depth crossing across refreshes and continue
editing its original route; the other scene's depth remains unchanged. There
is no timer pumping, forced slow tick, delayed capture or relaxed assertion.

Passed locally: strict validation of all ten manifests, schema self-test and
`git diff --check`. The existing ClearSignal plot/control advisory warnings
remain. Pinned native dependencies are unpopulated in this checkout; this
assignment does **not** claim a compiled or runtime pass for the new code.
The coordinator's final exact-source native behavior/capture gate must verify
the regression and updated images. No commit or push was performed.

## Follow-up from assignment 59 — routing contention

The final compatibility review identified another concrete cache defect:
`modulationRows` returned the same empty vector for a failed try-lock and a
genuinely empty patch. Refresh could therefore clear a nonfirst selection
during contention and choose the first route on recovery.

Added an optional, defaulted `available` output to the snapshot API. It is false
for an unavailable processor or busy lock and true for a successful snapshot,
including an empty one. Refresh retains the last valid same-scene routes and
selection while unavailable. It tracks the current bridge scene explicitly,
cancels old route gestures and clears the old cache before displaying another
scene, including direct `bridge().setScene()` callers. An active drag retains
its last physical amount during contention rather than misreporting zero.
Successful empty snapshots still clear the displayed routes and inspector.
The snapshot remains a nonblocking try-lock; no audio-side work was added.

Added a separate focused native case, **Busy native route snapshots preserve
selection without leaking across scenes**. A second thread holds the real
routing mutex through a promise handshake; a bounded watchdog only prevents a
deadlock and must be explicitly released for the test to pass. The test checks
nonfirst inspector selection and row identity during contention and recovery,
active-edit amount retention, both direct bridge and public scene-switch paths,
genuine empty clearing and the other scene's unchanged route. Thrown assertions
release and join the worker safely. No timing sleep is used.

Assignment 59 independently reviewed this focused source and test and found no
semantic or API compatibility blocker. Whitespace checks still pass. Native
execution of both A54 regressions remains part of the final exact-source gate.
