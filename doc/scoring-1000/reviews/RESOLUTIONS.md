# Independent review resolutions

The reviews describe the pre-release inputs identified in each report. They
are retained as evidence of issues found, rather than rewritten into clean
reports after corrections. This page explains the sequence of fixes. Historical
measurements below belong to their stated catalog hashes; the generated catalog
and published QA records identify the later preset bytes that were tested.

## Changes before the full native run

- Removed compiler floors that replaced quiet authored tremolo and pan values.
  A zero-default companion LFO follows the same motion shape, rate and tempo;
  raising Motion or the wheel adds movement without homogenizing the saved sound.
- Corrected Drawbar Lantern Glass to strengthen its existing upper octave.
  Pluck Spark describes a secondary layer, which can support body or upper color.
  Tin Carousel accurately describes its FM-and-pulse architecture.
- Gave Felt Pizzicato Swell a 400 ms nominal Attack setting and longer String
  decay. Bass Pluck and Tight variants cap nominal attack at 20 ms and 35 ms to
  support their articulated roles.
- Added family-specific role/intent overrides. Clockless Room and Deep Listening
  Ripple describe synchronized motion. Copper Transit and Tilted Grid Sparse
  describe their replaced accent pattern; all four pitched Sparse families
  explicitly describe root/octave material instead of their other scale patterns.
- Clarified Triplet as a 3:2 acceleration of the family rate, corrected Linen
  Motor's four-step wording, and removed an unsupported continuous-legato promise.
- Raised the suggested lower note of Harbor Hum and Deep Listening to MIDI 36,
  keeping their principal sub-octave fundamentals above the subsonic region.
- Documented that Dawn Sweep and Falling Arc pitch envelopes return toward the
  played note on release, and that Attack controls amplitude onset.
- Corrected the native integer type of Delay parameter 9 and explicitly activated
  its intended low/high filtering. These changes are checked by semantic roundtrip.

## Measurement corrections

The renderer rejects durations that round to zero audio frames. Fresh engine
instances plus seeded generator, noise, and C-library random states isolate every
scenario; the repeat fixture measures any remaining uncertainty rather than
assuming deterministic audio.

Attack response compares only the common pre-note-off window, preventing a longer
held note from masquerading as a control response. DC checks include active audio
as well as the full render, preventing post-release silence from diluting offset.

A 2,000-scenario native calibration exposed excessive peaks under dense polyphony
or aggressive macro combinations. Measured voice-VCA reductions were applied to
375 presets, targeting 0.72 sample peak in those calibration fixtures. These are
saved synth parameters; no test PCM was normalized, clipped or limited to conceal
failures. Final tests independently render the corrected presets and may justify
further measured corrections, recorded in
[gain_trims.json](../../../scripts/scoring/gain_trims.json).

## Remaining interpretation limits

Some individual shared-variant operations naturally do nothing on a model to
which they do not apply; independent ablation found no wholly unchanged variant.
Distinct active definitions do not establish perceived uniqueness. Short-signal
metrics, finite output and generous headroom do not replace listening in an actual
cue. Human audition and composer endorsement remain explicitly separate from
technical passes.

## Later gain-reference and control review

The complete audio run was restarted with a saved 0 dB master reference and a
common −6 dB voice VCA, plus the measured individual trims. The earlier 266
completed results are superseded, not combined with final results. This makes
the reference condition explicit and moves attenuation ahead of scene/global
effects.

The initial concern that normal browser loading would preserve the master for
these patches was corrected after reading the implementation: removal of saved
master volume during preset loading applies only to patch revisions below 17.
This bank uses revision 31. The unity-reference change is a conservative test and
design choice, not evidence of a revision 31 browser-loading defect. User
automation or a raised master gain can still increase output beyond the tested
condition.

Tin Toy Keys Glass received an additional Contour-to-amplitude-decay route after
its filter-only response was unresolved in the initial run. The saved default is
unchanged by that zero-default route. Eight filter-motion depths now retain their
authored fractional-semitone settings instead of a compiler floor of 1 semitone.
Analog ADSR times are described as nominal parameter settings rather than exact
acoustic arrival times; the native analog curve and sustain law remain unchanged.

## Full-bank warning corrections

The complete pre-polish run passed all 23,000 scenarios with 69 protocol warnings.
The original 1,000 reports, catalog and input FXPs were preserved in the authoring
checkpoint `qa-scoring-pre-polish`; category reports identify their reviewed
hashes explicitly.

The next corrections changed 136 FXPs: all 95 Noise-shaped Motion companions
start at phase 0.5, preserving the main random LFO and zero companion default;
27 additional variants received body-envelope Contour routes, with the previously
corrected Tin Toy Keys Glass also receiving bounded sustain shaping; 25 quiet
defaults received measured VCA increases; and three high stress peaks received
additional headroom trims. Positive trims are capped at +18 dB. Some corrections
overlap in the same preset. No test PCM was normalized.

All 136 changed presets were rendered again through all 23 scenarios. Unchanged
results were reused only with matching preset, catalog-entry, fixture, renderer
and QA hashes. At this stage, catalog
`8396df6694b92e644f027e90014be629e3c3f542c9d9403c69cb72a0b5b6bee1`
had 23,000 passing scenarios, 5,000 passing technical gates, 8,000 resolved macro
responses, 1,000 bit-identical default repeats, zero protocol warnings and a
maximum sample peak of 0.782223 at unity master. The additional small-response
screen (waveform distance below 0.01 **and** feature distance below 0.01) had no
remaining candidates. It is a prioritization screen, not a psychoacoustic
audibility standard. This result precedes the sustained-note corrections below.

## Sustained-note headroom corrections

The [long-hold audit](w08-a48-sustains.md) added two native scenarios for each
preset: dense notes at saved controls and a chord with aggressive macro settings,
using single-note equivalents for mono presets. Holds lasted approximately
12–76 seconds, covering a full 16-step pattern or multiple phase cycles according
to the modulation shape. The observation after note-off is only two seconds;
this supplement does not establish complete release settlement.

The first complete sweep passed all 2,000 scenarios but identified ten headroom
warnings, with a maximum sample peak of 0.889678. Further saved voice-VCA trims
targeted 0.72 in the worst measured fixture for each of those presets. The complete
rerun reused 990 exact matching records and rendered the ten changed presets
again. Catalog
`ddbb4781424430cc950daf6d610ec7025c743d7c435b404747c5821e65ec9369`
passed all 2,000 sustained-note scenarios with zero warnings. Its largest sample
peak was 0.781134; the ten corrected presets peaked approximately at 0.72. The
23-scenario primary protocol was also repeated for those ten changed presets.

The gain file retains the measured input hashes and individual corrections.
These are bounded stress measurements at saved unity master, not reconstructed
true-peak estimates or guarantees for arbitrary edits, stacked instances, or
every possible random state. The audio remains unnormalized.

## Description-only corrections

The [diversity review](w09-a51-final-diversity.md) identified eight Percussion and
Transitions Edge variants whose nominal attack remains at the 4 ms minimum.
Their original generic intent promised a faster attack, even though that
particular operation could not shorten the stored time. The affected IDs are
SC10-003, SC10-008, SC10-018, SC10-028, SC10-053, SC10-063, SC10-068 and SC10-078.
Their family-specific intents now describe brighter, more articulated attacks
without the unsupported speed comparison.

The later catalog review corrected ten descriptions that called a changed second
oscillator an upper layer even though it was tuned an octave below the played
note. They now say secondary layer: SC05-006; SC08-006, SC08-046 and SC08-091;
SC09-031, SC09-035, SC09-071, SC09-075, SC09-081 and SC09-085. This describes the
blend control without implying a register the source does not occupy.

Together, these 18 description-only corrections preserve every native sound-state
element outside metadata. Different decay, source and filter settings continue
to distinguish the variants. Changed preset bytes receive fresh native checks
even when the change is descriptive. None of these technical or editorial
reviews changes the separate status of human musical audition or implies
composer endorsement.
