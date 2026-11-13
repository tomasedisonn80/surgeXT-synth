# Wave 1 / assignment 4 — actual native visual gaps

## Evidence and scope

Compared all ten original PNGs in `references/` with the ten baseline PNGs in
`native-gallery/`. The native images are the real captures from implementation
`453df569c977350c3a4da2caf2f0ab7cbd1a0e71`, retained by current source checkpoint
`a0d016fd59ef9aee1c96a720c6a2c5a0b9a56f35`. Both sets are 1586 × 992. The
comparison used five labelled two-row sheets, with originals at left and native
captures at right, followed by source inspection and selected full-resolution
pixel samples. These are observations of existing native evidence, not a new
build, interaction pass, or whole-image fidelity score.

The main module positions are already substantially reconstructed. The largest
remaining visual differences come from surface lighting, control finishing,
type hierarchy, waveform projection, and chrome density. Repeating the layouts
from scratch would spend effort on the portion that already matches best.

## Highest-impact shared work

| Priority | Actual visible discrepancy | Concrete implementation site and change |
| --- | --- | --- |
| 1 | Aurora panels look flat, uniformly purple, and much darker at their title lips than the reference. Its bottom hero control strip lacks the distinct translucent raised bar. | `Workspace.cpp::bezel` around lines 261–394 and `drawPanel` around 6188–6311. Add a theme-specific top lip with rounded upper corners, a narrow specular edge, a recessed lower body, and localized purple/cyan reflections. Respect `titleHeight` and omit fake buttons. Give the hero's actual controls their own cosmetic tray. |
| 2 | Live, Aurora, and Spectral knob faces read as dark flat discs; Live's eight macro rings are particularly subdued. References have a bright fine metal rim, an illuminated value arc, a second outer rim, and an upper-left highlight. | `Workspace.cpp::knob` around 611–799. Keep the current value-derived arc and pointer. Add a fine neutral rim independent of value, strong but thin colored arc cores, dark lower lip and restrained upper-left radial reflection. Treat Live mint/amber separately from Aurora purple. Avoid widening the interactive bounds. |
| 3 | Several interfaces use uniformly thin type. Aurora/Live captions and headings lose hierarchy; Space's wordmark is too light; Bauhaus's logo is visibly less heavy than the original. | `Theme::type`, `text`, `weightedGlyphText`, `drawHeader::brand`, and `drawPanel`'s heading section. Tune individual semantic roles and styles. Do not apply a global font multiplier: many compact controls already fit tightly, and the serif skin needs a different treatment. |
| 4 | The hero waves in Spectral, Bauhaus, Aurora and Live have broad opaque bands and regular smooth geometry, while the references emphasize fine overlapping strands, a denser mesh, and more separated highlights. | `Visuals.cpp::wavetable` around 384–712; source-family fixtures in `generate_showcase_wavetables.py`. Lower broad surface opacity and tune line/bloom weighting before adding more curves. Preserve real signed samples. Geometry changes should come from actual periodic source tables or fixed camera projection, not reference-image trace overlays. |
| 5 | Dark cards often have only a thin uniform border; references have a subtle raised top edge and dark lower recess. Tokyo and Live particularly lose the hardware impression. | `Workspace.cpp::bezel`, `themedButton`, `ViewTabButton::paintButton`, `LiveChoice::paint`. Use a short bright top lip, narrow opposing lower shadow, and better selected-button material. Preserve hit areas, accessibility, and current selected state. |
| 6 | Fine visual cues are missing from module headings: there are fewer status glyphs, compact icon buttons, and section controls than in the references. | `drawPanel` and the native button creation near 4480–4550. Add only live actions or noninteractive decoration that is unmistakably ornamental. A power glyph must not appear to switch a module if no real action is bound. |

### Direct Aurora material samples

These samples are individual pixels in apparently empty panel material at the
same native coordinates, not similarity measurements:

| Pixel `(x, y)` | Reference RGB | Current native RGB | Implication |
| --- | --- | --- | --- |
| `(450, 90)` | `(66, 46, 95)` | `(43, 30, 67)` | Oscillator title lip needs substantially brighter purple material. |
| `(450, 100)` | `(66, 46, 95)` | `(42, 29, 66)` | This is a broad header treatment, not a one-pixel border issue. |
| `(450, 240)` | `(31, 25, 52)` | `(25, 21, 46)` | Body should remain dark; lifting the whole panel uniformly would be wrong. |
| `(750, 540)` | `(35, 33, 67)` | `(10, 9, 31)` | The bottom hero-control tray is markedly underlit/missing. |
| `(1100, 914)` | `(51, 32, 78)` | `(28, 25, 51)` | Macro panel needs the reference's localized violet upper reflection. |

## Per-skin changes visible in the current native pairs

| Skin | Most useful next visual corrections |
| --- | --- |
| **01 Obsidian** | Panel silhouette is close. Add the reference's finer dark metallic rim and stronger amber knob outlines. Native modulation rows lack the same visual density and vivid centered amount bars. Current waveforms and filter shapes differ partly because actual algorithms/patch values differ; do not freeze traces. Keep the cyan/orange separation stronger in source previews. |
| **02 Ivory** | Layout and serif treatment are broadly convincing. The native green plot backgrounds are flat and the wave mesh sparse; the reference adds luminous green depth and detailed strands. The cream surface needs slightly warmer irregular paper grain. Current title/control type and engraved floral motifs are much smaller/fainter in several places. FX tiles need stronger illustrated icon weight. |
| **03 Spectral** | Hero terrain is smooth, broad, and low-frequency compared with the reference's irregular mountain mesh. Lower wire opacity where many bands accumulate, raise spatial detail in the actual source recipe, and improve camera depth. The logo is a simple repeated sine mark instead of a small ribbon mark; the observatory emblem is notably flat and icon-like. Top section bars and colored active tabs need more glow/depth. |
| **04 Bauhaus** | Panel positions and block colors are close. The large wordmark is too thin, and the header's black text has insufficient visual mass. The hero is translucent broad ribbons instead of fine interleaved yellow/orange/red strands. Small knobs and headings should keep hard graphic edges; do not introduce metallic bloom into this skin. The reference's chart graticules are more subdued than some native small plots. |
| **05 Space Age** | Native chassis is flatter and its vertical darkening more uniform; original shows stronger short-range brushed texture, warmer highlights, and sharper recessed seams. The top wordmark is too light. Metal knob caps now have detail, but the four black macro knobs still need a richer rim/scale. The upper-right original is a dark version display beside vents; native utility labels occupy a plain chassis region. Where valid, put actual build/version information into a dark recess. |
| **06 Tokyo** | Native panel locations are close. Increase hardware title-lip contrast and narrow silver edge highlights. The original has a distinct red square module-power marker on envelope cards; native headings currently omit it, so use real native power/navigation behavior if added. Browser density differs greatly: the native Templates tree is expanded and dominates, while the reference shows a selected bank and lower patch list. This is a browser-state/design problem, not solved by copying fictional preset names. |
| **07 Aurora** | Highest material priority. Bright translucent title lips, purple edge bloom, the hero control tray, macro-panel reflections, and finer luminous knob rings are missing/weak. The hero is a dense opaque pair of smooth lobes; target has finer crossing ribbons, more small peaks, airy edge fade, and cyan/pink strand separation. Header meters should use the reference's cyan-to-pink hue ramp while their illuminated length remains real. |
| **08 ClearSignal** | Geometry is good. Current UI is visually more sparse: section iconography is missing, headings/values are less bold, and the dual filter plot has less luminous cyan/coral fill. Knob arcs and black titles can be strengthened without darkening cards. The inspector displays verbose numeric destination text instead of the reference's large amount emphasis; preserve real amount/destination information but improve hierarchy and wrapping. |
| **09 Cartographer** | The native routing topology is recognizably present but the source/node groups sit somewhat lower and cables have less depth. Add subtle mint glow and a bright thin cable core, leaving modulation lines dashed. Node edges and the right inspector could use stronger metallic blue top highlights. Native main-canvas toolbar is mostly absent in the current capture; functional views should occupy that header when supported. Browser state again differs substantially. |
| **10 Live** | Macro knobs have the largest control-material miss: mostly dark matte circles versus reference bright mint/amber concentric rings. The XY glass has almost invisible reflections; strengthen the existing layered translucent reflections and localized cursor halo. The center hero wave is broad smooth sheets, while the reference is an airy dense strand mesh. Main panel surfaces need restrained raised lips and polished borders. The lower keyboard/performance area has different real features; keep those adaptations explicit rather than painting fictional zone/snapshot controls. |

## Suggested implementation split

1. One owner for `bezel` / panel material and title lips.
2. One owner for `knob` / faders / selected button finish.
3. One owner for live sample projection and source recipe refinement.
4. Separate per-skin manifest/type/chrome passes once those shared changes are
   integrated, followed by fresh native screenshots.

The new captures should be compared to these existing captures as well as to
the originals, so a shared improvement cannot silently regress a different
skin. Current remaining differences are plainly visible; this audit does not
certify pixel identity or assert that the unsupported reference controls exist.
