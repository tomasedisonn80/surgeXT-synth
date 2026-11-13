# Wave 4 assignment 21 — Flow graph finish

## Evidence and measurements

Compared the supplied `references/09_Signal_Flow_Cartographer.png` with the
1586 × 992 native gallery capture from `a362667839b8e1d742cf3af94bb8710fa3c6d1fd`
(workflow `43486541422`). These are the before-change images; this assignment
does not claim a new compiled capture.

The native graph's placement is already close. Measured bright lettering showed
OSC 1 at y158–167 versus reference y159–168, MIXER starting x541 versus x559,
OUT starting x1210 versus x1217, and the modulation-bus heading starting
x700/y489 versus x715/y491. A straight oscillator cable's brightest native
pixel was RGB 131/245/210 versus approximately 171/255/240 in the reference.
Active native output ports were 5.4 px diameter; the reference ports occupy
roughly 8–9 px. Reference card edges have a pale double rim and softly lit lip;
the native edges were thin, saturated cyan.

## Local changes

- Replaced only the Flow graph card finish with a navy gradient, pale double
  rim, narrow dark inset and small upper reflection; increased corner radius
  from 7 to 8.5 px.
- Added a restrained wider cable halo and pale mint core, increased audio
  stroke width 2.2 → 2.4 px, and enlarged output ports to 8.4 px. Input-port
  diameters, inactive-state opacity and routing topology remain intact.
- Centered MIXER/OUT headings, adjusted oscillator/filter/FX/AMP lettering,
  and moved the modulation-bus heading by the measured +15/+2 px. The mixer
  card moves x530 → 527 and widens 96 → 99 px; its controls stay fixed.
- Reduced the small oscillator traces' glow and positioned them clear of
  native menus. Their actual sample values, finite-sample handling and ±1.35
  display clamp are preserved. Filter miniatures gain source-derived shading.
- Added subtle FX row-number capsules and a legible hollow legend port.

No shared knob, bezel or header function changed. Topology, route aggregation,
labels, live counts, menus, selections and click behavior remain unchanged.
Read-only filter/FX indicators remain status dots rather than invented buttons.

## Validation and next check

`validate.py --strict-overlap` passes all ten skins; the existing ClearSignal
warnings and intentional Flow overlay note remain. `git diff --check` passes.
A parsed manifest comparison confirms only the mixer card x/width changed;
all 51 control definitions and bindings are identical.

No builds, commits or remote operations were performed. The coordinator must
capture the combined source natively and inspect card rims, port coverage and
text fit. Actual waveforms, filter responses and route topology necessarily
differ from the static concept; no pixel-identity result is claimed.
