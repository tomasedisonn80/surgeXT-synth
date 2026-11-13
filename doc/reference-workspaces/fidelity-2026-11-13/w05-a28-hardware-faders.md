# Wave 5 / assignment 28 — Bauhaus and Space faders

## Evidence and scope

Read `AGENTS.md`, continuation, engineering log and user guide. Compared the
1586 × 992 Bauhaus and Space references, including enlarged mixer/envelope
crops, with the genuine `a362667839b8e1d742cf3af94bb8710fa3c6d1fd` native gallery.
Implementation starts from `411578a`; its pending capture is not treated as
visual evidence. This assignment owns only these two style branches in
`BoundSlider::paint`.

## Changes

- Bauhaus receives a flat 7 px dark rail and a 30 × 12 px cream cap with a thin
  dark outline. This addresses the measured generic rail/cap difference of
  approximately 13 px and 24 × 8 px respectively, without adding metallic glow.
- Space receives a 5 px dark slot within a 7 px recessed rim, replacing the
  broad generic bar. Brass caps are 27–28 × 13 px across the existing envelope
  and mixer controls, with squared shoulders, side highlights, a dark underside
  and two horizontal ridges. The reference caps measure approximately
  26–28 × 13 px.
- Space graduations now have twenty intervals, with longer major marks every
  fifth division and visible short minor marks. Their positions follow native
  normalized travel; no invented numerical scale or reference values are added.

The existing `standardTrackBounds` remains unchanged. Cap centres still use
`getPositionOfValue(getValue())`; no component bounds, mappings, events,
bindings or manifests change. Thick caps are clipped to the existing body area
at travel endpoints so the caption/readout bands remain clear. The native
focus outline and unavailable-value display remain; disabled materials dim.
Tokyo wheels and every other style retain their prior source.

## Verification and handoff

`validate.py --strict-overlap` passes all ten manifests, retaining the existing
two ClearSignal overlap warnings and deliberate Flow overlay note.
`git diff --check` passes. A scoped before/after source comparison confirms
that removing the new branch restores the previous `BoundSlider` byte for
byte, including its original text bands, travel, event handling and other
styles.

No local build, native capture, commit or push was performed. The coordinator
must inspect exact-source native captures for cap clearance at both endpoints,
Space graduation contrast and material appearance at normal/scaled sizes.
These source checks do not establish runtime success or pixel identity.
