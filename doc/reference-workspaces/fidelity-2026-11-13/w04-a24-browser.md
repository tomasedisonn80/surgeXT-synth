# Wave 4 / assignment 24 — preset browser

## Evidence and finding

Compared the Bauhaus, Space-Age, Tokyo and Aurora browser crops directly with
their supplied references at the 1586 × 992 canvas. Native evidence is source
`a362667839b8e1d742cf3af94bb8710fa3c6d1fd`, workflow `43486541422`, retained in
`native-gallery`; no new native capture was produced by this assignment.

Every capture expanded **Templates**, although its actual live patch category
was **Reference Showcases**. `browserRows()` preferred the retained `patchid`
over current metadata. The same stale ID could also enter session history or
make a selected installed preset display the unrelated live patch's metadata.

## Changes

Added a small shared `currentCatalogPatchId()` query that accepts an ID only
when the catalog name and category match the live patch. Selection, history and
patch-card identity use that query. Category expansion follows matching live
metadata; a retained ID only disambiguates matching Factory/User category
names. An uncatalogued category leaves the genuine root folders collapsed.
No installed names, folders, counts or fixture presets were added.

Within `PresetBrowser`, tree text now uses 13–14 px nominal sizes for the four
reviewed designs, with distinct child indentation and ellipsis instead of
horizontal compression. Expanded folders show downward chevrons. Scrollbars
use the actual theme palette instead of the default cyan. Space keeps counts in
folder tooltips; Aurora/Tokyo show their actual counts in parentheses. Selected
rows use reference-specific purple, dark red and amber treatments. Aurora's metadata
card has a quieter inset surface. Cards show the actual category directly and
retain author attribution in accessible descriptions/tooltips; the bounded name,
comment and tag text also remains available in tooltips.

The focused native regression covers a matching loaded preset, renamed patches,
duplicate Factory/User category names, moved/uncatalogued metadata, collapsed
real folders, card identity, guarded preset loading, and genuine session history.
The browser-only synthetic catalog now sets a consistent live name; capture
fixtures and sound profiles are unchanged.

## Validation and next gate

Strict manifest validation passed all ten skins, retaining the existing
ClearSignal overlap warnings and Cartographer routing note. `git diff --check`
passed. Only the assigned browser class, data helper/query, focused browser
tests and this report were edited. No build, commit or push was run.

The coordinator's exact-source native workflow must execute the new regression
and recapture all ten skins. Text clipping, scrollbar placement and selected-row
appearance remain subject to that native visual check; this source review does
not establish pixel identity.
