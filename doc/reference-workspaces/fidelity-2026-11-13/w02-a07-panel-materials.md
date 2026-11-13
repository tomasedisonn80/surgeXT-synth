# Wave 2 assignment 07 — enclosure materials

Implemented changes in `Workspace.cpp::materialGrain` and `bezel` only. Other agents own the controls, header and panel contents.

## Reference-driven changes

- Aurora receives a 29-pixel translucent title lip with purple stops around `#422E5F`, followed by a short transition into its dark body. The existing native oscillator header was RGB `(43,30,67)` at `(450,90)`, versus reference `(66,46,95)`. Its body was much closer, so the change is localized vertically rather than lifting the whole panel. A thin lower lip, clipped upper reflection and inner luminous border give the glass depth.
- Live enclosures receive a restrained neutral upper reflection, fine inner rim and dark lower recess. Tokyo receives narrow edge finishing without globally lightening its already close charcoal body.
- Space-Age chassis shading retains more of the warm base color at the bottom. The prior `(1100,940)` native material was `(163,140,108)`, versus reference `(194,164,127)`. Olive inserts now retain a warmer lower tone. Reflected bands and brushed texture are stronger while displays retain their own dark fill.
- Grain uses deterministic coordinate hashing with smoothly interpolated horizontal fibres, avoiding the former hard ends of 24-pixel noise blocks. A separate fine grain supplies machining scratches and a slightly stronger Ivory paper texture. Images are generated once and cached; they use no synth RNG, source artwork or processor state.

All new material layers remain within the existing panel geometry. ClearSignal and Bauhaus retain their flat material branches. No controls, hit regions, labels, routes, values or audio behavior changed.

## Validation and remaining gate

`git diff --check` passes. Strict manifest validation passes for all ten skins, retaining the two previously documented ClearSignal plot/control warnings and the deliberate Cartographer control overlay. No local native compilation or post-change visual success is claimed. The integrated native workflow must compile this shared renderer and recapture every skin.

The coordinator separately adds Aurora's hero-control trays and macro reflection in `drawPanel`; this assignment does not modify that function. The next review should sample the same header/body points in the new native PNGs and inspect Space-Age lower-panel brightness, texture visibility and all dark borders at 100% scale.
