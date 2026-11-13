# Wave 2 / assignment 10 — header chrome

## Evidence and changes

Inspected the original and committed native headers for Spectral, Bauhaus,
Space-Age and Aurora at their 1586 × 992 logical size, plus enlarged logo crops.
The native baseline is the gallery source identified in the wave-1 audit; this
assignment does not claim a new native capture.

- Added `luminousRibbonLogo` in `ReferenceOrnaments.h`: bounded, static Bézier
  artwork with five distinct strands. Spectral uses a translucent folded violet
  ribbon; Aurora uses a pink-to-cyan ascending fan. The latter occupies the
  reference's taller decorative area, clear of the wordmark. The helper has no
  signal, parameter, time or random input and never loads reference pixels.
- `drawHeader` now renders the Bauhaus and Space wordmarks through the existing
  glyph-outline helper, with restrained added ink weight. Space explicitly uses
  the embedded bold-italic face. Other wordmarks retain their existing path.
- Aurora's horizontal meter segments now progress from cyan through pale violet
  to pink. Channel peaks, dB mapping, segment count and illuminated-length
  threshold remain unchanged, including the silent state.
- Space's lower utility strip now contains a recessed, read-only display of the
  actual major/minor version and abbreviated build hash. Its `(1278, 51, 191, 20)`
  rectangle clears Browser/Mod/FX and the vents. Header actions were not moved.

## Verification and next gate

`git diff --check` passed. A focused source check confirmed `layoutHeader` is
byte-identical to HEAD, both real meter peaks and the existing activation
threshold remain present, and the Space inset does not intersect the three
navigation rectangles or vents. No event code, manifests or parameter bindings
were changed by this assignment.

The coordinator's linked native build and fresh captures remain the verification
gate for font mass, ribbon opacity and final visual balance. These source checks
are not a compiler, runtime or pixel-fidelity pass. No commits or remote actions
were performed.
