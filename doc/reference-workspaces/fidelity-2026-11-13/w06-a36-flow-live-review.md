# Wave 6 / assignment 36 — Flow and Live measured review

## Evidence and pending-source caveat

Read-only comparison at 1586 × 992 of `references/09_Signal_Flow_Cartographer.png`
and `references/10_Live_Performance_Deck.png` against their native baseline PNGs.
Initial inspection used source `2a5a621241e28e15eda57ff5be3bad6dc3f36ff3`, gallery
`48725235d6576e19d05ff24b524bb90b5d91f2ae`. Wave 5 was then pending, so its Live
audible-source, wire-luminance, macro-caption, scene-card and MIDI-phrase changes
were excluded from stale-image conclusions.

The coordinator subsequently supplied gallery
`facc04210a6c93e8ef846d31cce3e7c65649b3d0`; I visually rechecked both new PNGs.
All three findings below persist on exact source
`0708bc76bb194a127064cf18b8849cf77373a7e7`, workflow `43493343847`, captured
`2026-11-13T19:37:53.210Z`. Evidence records clean-source native JUCE component
snapshots attached to the processor, 48 kHz / 512 samples. Fixtures are
“Evolving Glass Engine” and “Evolving Horizons.” The pending-source limitation
is therefore resolved for these findings; this is not a new runtime test or
pixel-identity claim.

New baseline SHA-256:

- Flow: `4ed883c2debc8147537248b876ffebbb1666f6b1095a1d1437d2b1cf652c790b`
- Live: `8595f22b023ecd55109633a16c7953221687c7a2d6b31c792cd66833e1ba28ce`

## Three actionable material/layout gaps

1. **Flow response plot wastes its already-small well.** The concept’s grid
   occupies approximately x1323–1565 / y431–565 (242 × 134). Native
   `panel.inspector.response` declares `[1325,432,236,134]`, but `drawFilter()`
   applies `reduced(7,15)`, mapping data into `[1332,447,222,104]`: roughly 29%
   less area and a 16 px lower grid top. Use a Flow-specific near-edge data
   rectangle and keep dB/frequency captions outside it. Hooks:
   `Workspace.cpp::drawFilter`, especially `outerDbLabels` and the generic
   reduction; `reference-09-flow.surge-skin/workspace.json` response `plot`.
   Preserve the genuine magnitude response and its actual numerical axes.

2. **Live master meters are broad pastel slabs.** At y880 and y930, the
   reference’s bright columns span x1499–1507 and x1521–1529 (9 px each);
   native spans x1497–1510 and x1519–1532 (14 px each). A shared illuminated
   patch, x1500–1505 / y880–924, has median RGB 93/230/131 in the concept
   versus 162/233/196 natively. Narrow the active cores around the existing
   channel centres, add the saturated green glass gradient, and restore the
   external dB scale in the unused right gutter. Hooks:
   `Workspace.cpp::drawMeters` continuous-meter branch and Live `p.master.plot`
   `[1496,804,38,155]`. Scale labels must follow the existing −60…0 dB mapping;
   preserve real peaks and clipping indication.

3. **Flow’s macro bank lacks its organizing cell chrome.** The concept has
   vertical rules at x106/199/292, a horizontal rule at y883, and 18 px numbered
   badges. Native has neither, despite near-matching dial centres. At (199,860),
   reference separator RGB is 25/47/67; native equals its surrounding fill,
   8/27/45. Add this quiet 4 × 2 partition and slot-number treatment in a
   Flow-only `Workspace.cpp::drawPanel` branch for `panel.macros`; use the
   existing `macro1`…`macro8` identities and retain actual names, values and
   control bounds. No decorative substitute controls are needed.

No source changes, git mutations or additional build were performed.
