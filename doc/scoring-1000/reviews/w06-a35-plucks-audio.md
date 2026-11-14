# Wave 6 / Agent 35: Plucks native-audio review

## Outcome and exact scope

All **100 Plucks presets**, covering 20 families and five variants, have current
hash-bound results for **2,300 native scenarios and 500 technical gates**. Every
gate passes; there are no recorded scenario failures. One warning remains:
**SC02-033 Glass Filament Spark / Contour produces exactly zero measured change**.
That is an actionable control issue despite the overall technical pass. Three
additional Contour responses deserve targeted correction or listening review.
These findings were sent to the parent before this report was finalized.

The companion [machine-readable evidence](w06-a35-plucks-evidence.json) retains
all 100 input patch and QA-report hashes, distributions, response priorities, and
four additional dry comparison renders. This review describes the inspected
catalog, not any subsequent corrected files. Human musical audition remains
pending; nothing here establishes composer endorsement or suitability for every
cue.

## Independent verification

For every preset, I compared the actual FXP SHA-256 with the catalog and report,
then verified the renderer, QA-script, configuration, NumPy, preset-definition and
fixture bindings. All 23 fixture definitions matched the current runner. Native
note, tempo, velocity, pressure, wheel, macro, sample-rate, hold, frame-count,
format and loaded-name fields agreed with their fixtures. I independently
recomputed scenario failure checks from the retained metrics rather than relying
solely on the top-level `pass` labels. All 100 repeated defaults are bit-identical;
all 100 default PCM fingerprints differ.

The tested reference is the saved **0 dB master**, with the saved voice-gain
corrections. These are raw float32 sample-peak measurements, not reconstructed
true peaks. The 16-note stress fixture is deliberately dense; its role is gain
stress, not a suggested pluck voicing.

| Measurement | Observed result |
|---|---:|
| Default sample peak, minimum / median / maximum | 0.020986 / 0.050943 / 0.084490 |
| Default active RMS, minimum / median / maximum | 0.000265 / 0.003622 / 0.010503 |
| Maximum sample peak across all 2,300 scenarios | 0.719966 |
| Maximum absolute active-channel DC | 0.000000640 |
| Default mono-energy ratio, minimum | 0.991475 |
| Mono-energy ratio across every scenario, minimum | 0.520640 |
| Largest default final-500-ms tail RMS | 0.000000000131 |
| Hard/soft velocity active-RMS increase, minimum / median / maximum | 10.616 / 13.463 / 16.452 dB |

There are no nonfinite samples, silence-floor violations, clipping-threshold
failures, DC failures or excessive default release tails. The maximum peak is
Water Threads Shadow's dense-polyphony fixture. Default mono retention is strong;
the minimum across all controls occurs in an extended-stereo condition and remains
above the runner's warning threshold. Low three-second active RMS is expected for
brief, decaying articulations and should not be confused with a silent transient.
Bone Tines Spark has the lowest default peak and active RMS, so it is a useful
level-setting audition target, not an automatic gain-boost recommendation.

## Control findings

**799 of 800 macro comparisons resolve above the protocol's measurement floor.**
That count describes detectable change, not artistic usefulness. The additional
screen below uses both relative waveform delta and feature distance below 0.01
to identify small responses. It is an explicit review-priority rule, not a
psychoacoustic threshold.

| Preset / Contour | Relative waveform delta | Spectral/envelope distance | Interpretation |
|---|---:|---:|---|
| SC02-033 Glass Filament Spark | 0 | 0 | No measured response; correction needed |
| SC02-034 Glass Filament Cascade | 0.001037 | 0.0000413 | Barely clears the protocol; prioritize |
| SC02-078 Velvet Mandolin Spark | 0.005218 | 0.0000637 | Small measured response; prioritize |
| SC02-083 Water Threads Spark | 0.007084 | 0.0000860 | Small measured response; prioritize |

Their bright low-pass settings make an additional opening envelope less effective
in the tested register. A targeted amplitude-decay Contour route is a reasonable
design option, already demonstrated elsewhere in the bank; new FXP hashes must
receive fresh native tests. No other Plucks macro meets both small-response
screening conditions. Every default, range endpoint, expression, tempo and
44.1 kHz fixture remains technically valid.

## String decay and Release

Eleven families use String oscillators: Silk Plectrum, Cedar Thread, Harbor Harp,
Copper Plectrum, Bone Tines, Woven Strings, Dusk Zither, Velvet Mandolin, Water
Threads, Moon Marquetry and Tremor Lute. Their **55 variants** all have resolved
Release responses, with relative waveform changes from **0.15943 to 1.07698**.

The [compiler](../../../scripts/scoring/build_bank.py) extends amplitude release
and, for these low-sustain designs, amplitude decay and the String oscillator's
two feedback-decay controls. Across 130 inspected String-decay targets, maximum
Release produces parameter endpoints from 0.765 to 0.985. With the bank's
non-extended settings, the [native implementation](../../../src/common/dsp/oscillators/StringOscillator.cpp)
maps the largest endpoint to feedback **0.9990625**, below unity. These controls
are feedback amounts, not seconds. A naturally decaying excitation can therefore
lose energy during a held note; increasing only post-note-off release would not
necessarily extend its body. The combined routing addresses that limitation,
while the native release fixtures remain the actual evidence of output behavior.

## Two closest-family comparisons

The lowest matched-fixture spectral/envelope distances belong to Rubber Lantern
Close/Spark (0.007305) and Palm Circuit Close/Tender (0.008941). Coarse quarter-second
envelope bins can obscure millisecond attack differences, so I rendered these
four presets again with **Space=0**, matched playing conditions, fresh engines
and the same seed. No native audio was normalized. For the comparison calculation
only, one signal was scaled to equal active RMS, removing a simple level offset.

The dry RMS-matched waveform differences remain **0.10835** and **0.27416**,
respectively. These pairs are consequently not explained solely by gain or
ambience. Rubber Lantern changes nominal attack, decay, cutoff, resonance and
triangle-layer balance; Palm Circuit Tender more than doubles nominal attack,
lengthens decay, darkens the filter and increases its saw layer. The comparison
supports actual articulation/source differences, while their usefulness as
separate musical choices still needs listening in context. All four supplemental
native renders succeed without nonfinite output.

## Provenance and follow-up

- Catalog SHA-256: `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
- Renderer SHA-256: `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.
- QA-script SHA-256: `203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681`.
- Supplemental fixtures, raw PCM and native rows: scratch directory
  `/workspace/scratch/9118c5c72ad8/a35-plucks/`.

Only this report and its compact evidence JSON were written to the repository.
The parent should re-test any corrected Contour presets and preserve these
pre-correction findings rather than relabeling them as a completed listening pass.
