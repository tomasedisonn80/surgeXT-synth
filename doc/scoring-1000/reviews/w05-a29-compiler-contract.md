# Wave 5 / agent 29: compiler-to-DSP contract review

## Scope and evidence

This is an independent source and generated-definition review. It asks whether
the recipe compiler addresses the native controls it intends to address. Native
serialization can preserve a wrongly chosen parameter perfectly, so this review
is complementary to the separate save/reload audit.

I read the compiler, the native parameter definitions and voice/oscillator/effect
DSP, and inspected all **1,000 generated presets** against the **200 families ×
five variants**. The initial inspected catalog SHA-256 was
`a5408903059ac1d15540fb5700b2d8ce40a42ebe6344a37ab4edf46dbab1b1db`.
During the review, the parent changed the master/VCA reference; the subsequent
catalog was `6b4505d6d31844371250dc330352d07387503a4b028844a40ab833bc82610b23`.
This report does not substitute results from one snapshot for another.

The all-bank inspection checked oscillator model IDs, cube-root mixer values,
eight connected macro sources, disabled cross-oscillator FM and waveshaping,
neutral scene pitch, companion/vibrato LFO defaults, unscaled extra LFO output,
and absence of additional MSEG/formula payload. No mismatch was found in those
checks. There are **2,510 active oscillator instances**: 430 FM2, 205 FM3, 205
String, 675 Sine, 390 triangle configurations of Modern, 175 other Modern,
275 pulse configurations of Classic, and 155 saw configurations of Classic.

I did not rebuild, regenerate, render audio, alter recipes or edit the test
runner. Only this report was written. This is not a listening or endorsement
assessment.

## Findings and decisions

### 1. Make the output-gain reference explicit

The initial bank used a saved master of −3 dB. The native renderer restores that
master, so its peak results have that precise gain condition. A hypothetical
0.72 peak at −3 dB becomes approximately 1.02 at unity master; an assertion about
headroom must not silently change the master condition.

My first message incorrectly generalized a comment in `SurgeSynthesizer.h` into
a claim that normal browser loading would preserve master for this bank. The
implementation is narrower: `SurgePatch::load_xml` removes the serialized volume
for preset loading **only for revisions below 17**. These patches use revision
31 and normally load the saved volume. I corrected that statement immediately
after inspecting the implementation. This is **not a demonstrated revision-31
browser-loading defect**.

The parent nevertheless chose a clearer unity-master testing reference: saved
master 0 dB and the common voice VCA reduced by 3 dB, with measured per-preset
trims retained. The subsequent inspected first patch reflects that change.
This approximately preserves the output level while moving attenuation ahead
of the effects. It is a defensible reference choice, not a remedy for the
incorrectly generalized browser claim.

The placement is valid: voice VCA gain is applied after the voice's filters and
waveshaper and before scene/global effects. Effects containing nonlinear
feedback may not be exactly invariant under redistributing gain, so the parent's
decision to regenerate and repeat native QA is appropriate. User changes to
master, voice gain or downstream host processing remain outside a fixed test
condition.

Sources: [patch loading](../../../src/common/SurgePatch.cpp),
[browser call path](../../../src/common/SurgeSynthesizerIO.cpp),
[voice gain and filter processing](../../../src/common/dsp/SurgeVoice.cpp), and
[wide filter chain](../../../src/common/dsp/QuadFilterChain.cpp).

### 2. A remaining minimum changes eight restrained motion settings

At review time `vary()` clamps transformed filter-motion depth to **1–18
semitones**, whereas the recipe contract permits 0–18. This raises eight
authored transformed values to 1:

| Preset | Requested transformed depth |
|---|---:|
| SC05-064 Chapel Pipes Muted | 0.720 |
| SC08-011 Willow Breath Near | 0.816 |
| SC08-012 Willow Breath Warm | 0.960 |
| SC08-041 Harbor Whistle Near | 0.748 |
| SC08-042 Harbor Whistle Warm | 0.880 |
| SC08-043 Harbor Whistle Clear | 0.935 |
| SC08-066 Satin Harmonics Near | 0.884 |
| SC08-076 Chalk Melody Near | 0.952 |

This was a small design-contract mismatch, not evidence of bad audio. With the
new companion LFO supplying additional Motion response, the floor is no longer
necessary solely to give that macro something to change. The parent accepted
the correction to a zero lower bound, preserving these eight requested settings.
The final resolution record and regenerated catalog identify the delivered bytes;
the in-progress native run must use those bytes.

### 3. Envelope and reverb times are native control settings

The compiler correctly stores log2 time values for the native controls. Both
ADSRs use mode 1, Surge's analog capacitor model. That model squares the sustain
control and charges/discharges an internal capacitor; its measured time to peak
is not a rectangular timer equal to `2 ** attack`. Consequently a description
such as Felt Pizzicato Swell's “400 ms entrance” should say **400 ms nominal
Attack setting**, or simply describe a slower entrance, unless the actual onset
has been measured. The same distinction applies to claims of exact decay or
release duration.

Reverb2's displayed decay control also cannot by itself establish a measured
RT60: its feedback calculation, damping, room scale and diffusion shape the
tail. The compiler uses the intended native Decay Time parameter. I found no
reason to replace that mapping, but documentation should avoid treating the
recipe number as a measured acoustic tail duration.

Sources: [ADSR implementation](../../../src/common/dsp/modulators/ADSRModulationSource.h),
[parameter definitions and formatting](../../../src/common/Parameter.cpp), and
[Reverb2 DSP](../../../libs/sst/sst-effects/include/sst/effects/Reverb2.h).

## Confirmed mappings

| Area | Source-level conclusion |
|---|---|
| Oscillator IDs | Classic 0, Sine 1, FM3 5, FM2 6, Modern 8 and String 9 match the native enumeration and factory dispatch. |
| Classic saw/pulse | Shape 0 uses the saw configuration; shape 1 uses pulse with its Width 1 control. Sub mix is zero; cross-oscillator FM is disabled. |
| Modern triangle | Saw and pulse are zero, triangle mix is 0.8 with the standard triangle deformation. Its Color macro adds saw content. |
| FM controls | FM2 ratios are integer controls; FM3 ratios are float controls. The compiler's two amount/ratio pairs and feedback address the intended parameters. FM3's additional absolute-frequency modulator is deliberately zero. |
| String controls | Exciter IDs 0–6 are internal burst modes, not external audio inputs. Decay, second-string detune/balance and stiffness map correctly. Deformation 34 selects one-times processing plus sinc interpolation; 64 selects fixed filtering. |
| Mixer levels | `amp_to_linear(x)` is `x³`. The compiler's cube roots recover the authored linear balance after the common normalization. Current `Parameter::set_storage_value(float)` stores the numeric value directly; an old comment about saving mixer gain in dB is not the implementation. |
| VCA and velocity | VCA gain and velocity sensitivity use native dB units. Negative sensitivity attenuates softer notes through `velocity_sensitivity * (1 - velocity)`. Scene volume 0.95 is a cubic amplitude control, not 0.95 dB. |
| Filter topology | Configuration 7 is Wide. Route 1 supplies both left/right paths; filter balance −1 leaves Filter 1 fully active and Filter 2 absent. This preserves oscillator stereo paths. |
| Frequency units | Cutoff values use semitones relative to 440 Hz. Modulation depth 12 therefore means an octave in the parameter domain; filter keytracking and envelope modulation then modify the resulting cutoff. |
| Macro sources | Native sources 7–14 are the eight custom controllers, in the same order as Tone, Motion, Contour, Space, Color, Width, Attack and Release. Routes use native parameter deltas, not normalized knob fractions. |
| Mod wheel and pressure | Source 6 is wheel; source 4 is channel aftertouch. Pressure raises the separate 5.2 Hz vibrato LFO magnitude, whose pitch route is 0.2 semitone. |
| Motion companion | Source 19 is the third voice LFO (`a_lfo2`). Its magnitude is zero at the saved setting, so it adds no default tremolo/pan; Motion and wheel raise it. Its shape/rate/synchronization are copied from authored motion. |
| Tuned step output | The 20 tuned sequence patches use source index 1. Extra-amplitude state is zero, so this output bypasses the LFO magnitude; Motion does not scale the intended semitone intervals. |
| Ambience | FX7 and FX8 are the two initial global effect slots. Delay mix is parameter 10; Reverb2 mix is parameter 9. Space zero makes both dry; default 0.4 restores the authored amounts, subject to the documented minimum ambience. |
| Width and articulation | Width controls native scene/effect width, not an unconditional mono/stereo switch. Attack and Release modify logarithmic amplitude time controls. Low-sustain patches also extend decay; 415 presets receive that additional amplitude-decay route. |

Relevant implementations:
[oscillator dispatch](../../../src/common/dsp/Oscillator.cpp),
[oscillator controls](../../../src/common/dsp/oscillators),
[modulation source enumeration](../../../src/common/ModulationSource.h),
[voice modulation application](../../../src/common/dsp/SurgeVoice.cpp),
[LFO output construction](../../../src/common/dsp/modulators/LFOModulationSource.cpp),
[gain conversion](../../../src/common/dsp/utilities/DSPUtils.h), and
[effect parameter adapters](../../../src/common/dsp/effects).

## Initialized-state and boundary review

The Init Saw container has no external oscillator payload, MSEG or formula
content. The compiler removes existing modulation routes before constructing
its own. Scene A is the selected single scene; Scene B is inactive. Waveshaping,
cross-oscillator FM and both ring modulators are off. Inactive oscillator values
remain initialized but do not create an additional audible layer.

Oscillator-specific attributes are reset before the intended model flags are
set. This is particularly important for Modern deactivation flags, Sine's
optional filters, and String deformation bits. The delay's filter activation
flags and integer unused parameter are now explicit. No unsupported family,
oscillator, motion, filter or variant input key appeared in the current recipes.

The retained hardclip mode is the normal **+18 dBFS protection**, not a limiter
at 0 dBFS. Therefore a sub-unity native peak check is meaningful and is not being
made to pass by a hidden zero-dB output limiter. Standard parameter limits can
still make some combined macro movements saturate; connected routes are not a
proof that every endpoint creates an equally useful musical change.

No critical oscillator, routing, gain-unit or modulation-source misassignment
was found. The specific design and reporting boundaries above should remain
visible alongside the independent native audio and serialization results.
