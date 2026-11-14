# Wave 6 / Agent 32: macro and expression contracts

## Result and scope

The independent XML audit covered **all 1,000 generated FXPs** and found **no
missing macro, wrong label, wrong saved macro default, disconnected macro, dry-mix
base error, or companion-LFO mismatch**. All eight controls have active parameter
routes in every preset. This establishes the control definition; it does not
establish that every route produces a useful audible difference.

The current audio run was still in progress at review finalization. Its
**184 current-hash reports contain 1,472 measured macro responses**: 1,471 are
resolved above their repeated-render measurement floor. **SC02-033 Glass Filament
Spark / Contour** produces zero measured change in that fixture and needs a
specific correction and re-test. Root has queued that correction. All 184 reports
pass their technical gates; the unresolved control remains an explicit warning.
These are a bounded snapshot, not final results for the remaining 816 presets.

The added Contour-to-amplitude-decay route in **SC01-048 Tin Toy Keys Glass** is
present only there in this catalog. Its current native result measures relative
waveform RMS change **0.35585** and spectral/envelope feature distance **0.14091**,
above thresholds 0.001 and 0.002. Those measurements resolve the earlier no-response
issue; they do not certify artistic preference.

## Method and provenance

I parsed the actual FXP containers independently, checked every SHA-256 against
the catalog, inspected modulation identities/depths and controller state, and
compared macro endpoint sums with **native parameter dumps from six oscillator
models**. The six dumps ran sequentially through the already-built native
executable; no audio-engine or bank code was changed. Dynamic parameter bounds
were matched to each oscillator's actual model, rather than assuming that the
same numbered parameter means the same thing in different models.

The audit follows the actual modulation and DSP implementations in
[SurgePatch.cpp](../../../src/common/SurgePatch.cpp),
[Parameter.cpp](../../../src/common/Parameter.cpp),
[SurgeVoice.cpp](../../../src/common/dsp/SurgeVoice.cpp), and
[LFOModulationSource.cpp](../../../src/common/dsp/modulators/LFOModulationSource.cpp).
The [machine-readable evidence](w06-a32-macro-evidence.json) records the catalog,
renderer and inspected source hashes, exception IDs, ranges and partial audio
snapshot. The catalog SHA-256 is
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`;
the renderer SHA-256 is
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.
Later preset changes require evidence tied to their new hashes.

## Eight control contracts

All macros are unipolar. Saved values are zero except **Space = 0.4**.

| Macro | Verified behavior | Concrete boundary |
|---|---|---|
| Tone | Two routes per preset: filter cutoff plus a modest change to oscillator 1's color target. Cutoff depth is +18 semitones in 965 low/band-pass presets and −15 in 35 high-pass presets. | Lowering a high-pass cutoff adds lower body. Raising a band-pass center is a spectral shift, not a universal increase in loudness or perceived brightness. |
| Motion | Main and companion LFO amplitudes each receive +0.6. The 799 unsynchronized presets also receive +2 log2 rate units on both LFOs; 201 synchronized presets retain their programmed rate. | Free-running rate at maximum spans about 0.2–32 Hz. The highest settings can produce fast tremolo rather than a slow atmospheric movement. Zero retains authored movement. |
| Contour | +15 semitones of filter-envelope amount and +1.25 log2 units of filter-envelope decay in every preset; one current variant also extends amplitude decay. | An already-open filter can make the first two routes ineffective. The Glass Filament Spark measurement demonstrates why connected routes alone are insufficient. |
| Space | Both global effect mix bases are zero. The saved 0.4 macro applies the authored ambience; maximum delay mix is 0.02–0.35625 and reverb mix is 0.05–0.75. | Space zero removes wet contribution, although the effect processors still exist. Increasing Space changes the dry/wet balance; it is not an independent wet-gain fader. |
| Color | One oscillator-specific route per preset, described below. | The control changes source character. It does not map every synthesis model to an identical perceptual axis. |
| Width | Adds 0.5 to the native scene width and 6 dB to both effect widths. Saved scene width ranges 0.16–0.9. | Zero is the authored spread, not mono. In 520 presets the scene-width endpoint exceeds the ordinary UI maximum of 1, invoking the native extended pan law. |
| Attack | Adds 2.8 log2-time units to amplitude attack, a nominal factor of about 6.964. | Endpoint parameter settings span about 27.86 ms–20.89 s. The native analog envelope curve means these are nominal settings, not exact measured times to acoustic peak. |
| Release | Adds 2.2 log2-time units to amplitude release; low-sustain designs also extend amplitude decay and appropriate String decay parameters. | Nominal release endpoints span about 0.164–22.974 s. A long envelope does not force a naturally decaying oscillator to keep producing energy. |

Attack and Release endpoints remain below the native envelope parameter maximum
of 32 nominal seconds. The current low-sustain amplitude-decay endpoints are at
most about 16.97 seconds. Analog mode squares its sustain control internally;
this audit does not substitute the displayed sustain number for measured RMS.
That behavior is explicit in
[ADSRModulationSource.h](../../../src/common/dsp/modulators/ADSRModulationSource.h).

## Color mapping and endpoint interpretation

The actual oscillator-1 model and destination counts are:

| Model/recipe color | Presets | Destination |
|---|---:|---|
| Classic pulse | 175 | Pulse width |
| Classic saw | 90 | Shape |
| Modern mixed voice | 105 | Pulse mix |
| Modern triangle-based voice | 60 | Saw mix |
| Sine | 130 | Feedback |
| FM2 | 195 | Modulator amount |
| FM3 | 145 | Modulator amount |
| String | 100 | Stiffness/filter character parameter |

No individual Color endpoint exceeds its native parameter's UI range. With
**Tone and Color both at maximum**, the five **Feather Bridge** variants
(SC04-076 through SC04-080) request pulse width 1.015. The Classic oscillator
actually clamps pulse width to 0.999, so the final small part of this combined
control range saturates. This is a localized endpoint plateau, not a disconnected
control or proof of an audio failure. See
[ClassicOscillator.cpp](../../../src/common/dsp/oscillators/ClassicOscillator.cpp).

The five **Reed Crescendo** variants (SC05-091 through SC05-095) request Modern
pulse mix 1.1125 when both controls are maximal. That exceeds the UI's ±1 range,
but the Modern oscillator deliberately permits modulation to ±2 before its DSP
clamp. It therefore must not be reported as an unintended clamp at 1. See
[ModernOscillator.cpp](../../../src/common/dsp/oscillators/ModernOscillator.cpp).

**Nineteen Tone endpoints** exceed the cutoff control's UI upper value of 70;
the evidence lists every ID, including Glass Hammer Glass, Tin Toy Keys Glass and
Silver Nodes Glass. Their actual filter input also includes keytracking,
filter-envelope output and LFO motion. Consequently, neither a UI-range exceedance
nor an unclamped sum alone proves a DSP defect or predicts the exact response.
The oscillator-color component provides a second route; the native audio tests
must still establish a measured change.

## Motion, stereo and expression

The companion LFO has zero saved magnitude and matches the main LFO's other
parameters and rate routes in all 1,000 presets. **All 118 step-sequence pairs**
match their programmed step values, loop, shuffle and trigger masks. Tuned
step-pitch routes use raw output index 1, independently of the magnitude-scaled
output used for timbral and amplitude motion. Thus adding companion movement
does not scale the programmed interval sequence.

Motion and wheel together can bring the main LFO magnitude to **1.10** and the
companion to **0.75**. The native LFO permits modulation magnitudes to ±3, so the
UI maximum of 1 is not a universal processing clamp. The source deliberately
separates scaled output 0 from raw output 1.

The Wide scene pan law also allows positions outside ±1, where the opposite
channel's contribution can invert phase. Its implemented position bound is ±2;
see [basic_dsp.h](../../../src/common/dsp/vembertech/basic_dsp.h) and the Wide
routing in SurgeVoice. This is why Width maximum should be auditioned in the
arrangement and checked in mono rather than described as simply “more stereo.”
In the 184-report snapshot, the lowest measured mono-energy ratio under the
individual Width-maximum fixture is **0.96544** (SC01-079), well above the QA
warning threshold of 0.1. This limited snapshot does not establish the remaining
presets' mono behavior or every combined controller state.

All 1,000 presets route velocity to +6 semitones of cutoff and use VCA velocity
sensitivity between −24 and −8 dB. The native gain law scales attenuation by
`1 − velocity/127`: velocity 127 incurs no velocity attenuation, while softer
notes receive more. All presets route the modulation wheel to the filter and
both motion-LFO magnitudes. High-pass wheel cutoff direction is negative,
matching the Tone control's body-opening direction.

Channel pressure routes +0.7 to a 5.2 Hz vibrato LFO, whose pitch depth is 0.2
semitones: approximately ±14 cents at full pressure before normal LFO-envelope
behavior. This is **channel pressure**, not a demonstrated per-note MPE or
polyphonic-aftertouch articulation system. The renderer's separate pressure and
wheel fixtures test those individual paths. Its aggressive-macro fixture does
not simultaneously maximize wheel and pressure; a separate combined-controller
stress test would have a distinct scope.

## Effects and remaining actions

No macro routes to delay feedback or crossfeed. All 1,000 patches save feedback
0.22 and crossfeed 0.10, with extend-range disabled. The native delay converts
these amplitude controls with a cubic law, giving approximately 0.010648 and
0.001 respectively before the feedback processing; they are not 22% and 10%
linear feedback coefficients. The source is
[Delay.h](../../../libs/sst/sst-effects/include/sst/effects/Delay.h). There is no
observed macro path toward runaway delay feedback.

The required follow-up is targeted: correct and re-test **Glass Filament Spark's
Contour**, retain explicit extended-stereo/nominal-envelope descriptions, and
use the completed full-run evidence for final counts. The renderer README also
needs the already-identified master-loading wording correction so it does not
imply the legacy revision-below-17 exception applies generally to this bank.
No bank, compiler, renderer or QA code was modified by this review. No listening,
composer endorsement, universally useful endpoint, or acoustic-realism claim is
made.
