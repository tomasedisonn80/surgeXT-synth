# Wave 7 / Agent 41: Bass native evidence review

## Scope and result

Reviewed **all 100 Bass presets**, covering 20 families and five variants, against
catalog **`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`**.
All **2,300 recorded native scenarios and 500 technical gates pass**. All 800 macro
responses resolve above the protocol's measurement thresholds, and there are no
protocol warnings. One smaller Motion response remains a targeted correction
priority within the parent's already-planned Noise-LFO adjustment.

This is an independent review of native measurements and saved definitions. It
makes no claim of human listening, perceived bass audibility, composer endorsement,
or universal musical suitability. The [machine-readable evidence](w07-a41-bass-evidence.json)
records every reviewed preset/report hash, parameter observations, distributions,
and closest-family comparisons. Later changed preset bytes require fresh QA.

## Binding and measurement checks

The review checked each FXP against the catalog, then verified the report's preset,
catalog-entry, renderer, QA-script, configuration and fixture bindings. It checked
all 23 rows per preset against their requested notes, velocities, tempo, macro
values, MIDI expression, sample rate, frame count, fresh-engine seed, stereo format,
loaded name and source path. Recorded audio failures were independently recomputed
from the metrics using the current protocol. The catalog, all 100 FXPs and all
100 report files remained unchanged throughout the analysis.

| Coverage | Result |
|---|---:|
| Current preset/report bindings | 100 / 100 valid |
| Native scenarios | 2,300 / 2,300 passing |
| Technical gates | 500 / 500 passing |
| Measured macro responses | 800 / 800 resolved |
| Bit-identical default repeats | 100 / 100 |
| Distinct default PCM hashes | 100 |
| Protocol warnings | 0 |

The native executable SHA-256 is
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.
Measurements are raw float32 sample peaks at the saved **0 dB master reference**.
They do not establish reconstructed true-peak bounds or protect against subsequent
master automation and downstream gain. [QA implementation](../../../scripts/scoring/qa_bank.py)
and [native renderer](../../../src/scoring-tools/ScoringRender.cpp) define those conditions.

## Gain, dynamics and mono behavior

| Measurement across this category | Observed range or worst case |
|---|---|
| Default sample peak | 0.029552–0.194811 |
| Default active RMS | 0.002891–0.089245 |
| Maximum peak in any scenario | **0.781134**, SC07-064 Soft Pedal Pluck, 16-note stress |
| Maximum absolute active/full channel DC | 0.000099179 |
| Default mono-energy ratio | 0.999793–1.000000 |
| Lowest mono-energy ratio in any scenario | **0.990940**, SC07-025 Quiet Bow Pulse, aggressive macros |
| Hard-to-soft active RMS difference | 5.858–17.303 dB |

The saved basses maintain a strongly centered sum in these fixtures; even the
least favorable measured mono ratio loses only about 0.04 dB of energy. That is
useful technical evidence for low-frequency foundations, without certifying a
particular room or playback system. Every hard-velocity fixture is materially
stronger than its soft counterpart. The different active-RMS levels reflect
sustained sub voices versus brief plucks as well as gain: equal peak values would
not imply equal perceived loudness.

There are **75 mono presets and 25 polyphonic presets**. The latter belong to Wood
Finger, Quiet Bow, Soft Pedal, Muted Current and Low Lantern and receive distinct
16-note, velocity-127 stress clusters within their declared ranges. Mono presets
receive an isolated hard single note. Saved polymode and 16-voice limits agree
with those intended choices. The mono tests do **not** exercise overlapping-note
priority, retrigger sequences, portamento transitions or continuous legato. Neither
the report nor the user guide should convert isolated-note success into such a claim.

## Low register, filtering and pitch scope

At each declared lower endpoint, the lowest programmed oscillator fundamental is
**41.20–55.00 Hz**, including authored octave and detune offsets. Every value is
above its active **20–25 Hz scene lowcut setting**. Native low-endpoint peaks are
0.028447–0.186428 and active RMS is 0.002337–0.073522, so none of those fixtures is
silent or near the protocol floor. This does not imply an unattenuated fundamental:
filters have slopes, and the source/filter architecture changes the available spectrum.

Measured low-endpoint energy below 80 Hz spans **2.34% to almost 100%**. Anchor Sub
Round is overwhelmingly low-frequency material; Bronze Growl Tight has much more
energy in upper partials. Its FM-based role makes that contrast plausible. Spectral
centroids and band fractions are not fundamental trackers, and low-frequency
energy alone cannot establish audibility on small speakers or musical intonation.
The preserved evidence includes those quantities for every preset rather than
assigning a generic bass-quality score.

All 20 Pulse variants use synchronized square-wave timbral motion. No Bass patch
routes the main or companion movement LFO to pitch. The saved pitch routing is the
shared pressure-controlled vibrato LFO, with small authored oscillator detunes
where applicable. That confirms the absence of a programmed bass pitch sequence;
it does not replace measured fundamental tracking or listening for FM/String
inharmonicity. See the [recipes](../../../scripts/scoring/recipes/07_bass.json) and
[compiler assignments](../../../scripts/scoring/build_bank.py).

## Articulation, tails and meaningful variation

All **20 Tight** nominal Attack settings lie between 4 and 35 ms; all **20 Pluck**
settings lie between 4 and 20 ms. This verifies the earlier cap corrections for
Quiet Bow, Cello Shadow and Low Lantern. Native analog ADSR settings are nominal
control values, not exact acoustic times to peak. The existing three-second fixture
and quarter-second envelope bins should not be described as millisecond onset measurements.

All saved-default release tails settle deeply: the largest measured trailing level
is about **160.23 dB below active RMS**. Maximum-Release tests observe only their
specified, capped tail windows; Pulse Engine Round remains about **24.94 dB below
active RMS** at that endpoint. Complete settlement at every Release setting is
therefore outside this audit.

The closest of the 200 within-family feature comparisons is **Felt Motor Edge / Pulse**
(SC07-093/095), with distance 0.009123 under the same played-note/hold fixture. These
are related low, percussive voices. Their programmed differences include FM amount
0.1705 versus 0.11, secondary triangle level 0.217 versus 0.308, cutoff 1,656 versus
1,058 Hz, nominal Attack 10.5 versus 16.8 ms, and synchronized movement 1.5 versus
6 Hz at the reference tempo. Those are actual source, articulation and movement
changes; the small coarse-feature distance still makes this pair a useful listener
A/B priority. Different hashes alone do not establish perceived uniqueness.

## Targeted follow-up

**SC07-059 Ember Grit Pluck / Motion** passes the protocol but has a small relative
waveform change of **0.003532** and feature distance **0.001328**. It is the sole Bass
entry meeting the requested screening rule of both values below 0.01. That rule
prioritizes review; it is not a psychoacoustic audibility standard.

The [Noise-startup diagnosis](w07-a37-motion-diagnosis.md) supports the parent's
planned companion-only phase adjustment. It affects **SC07-056 through SC07-059**;
Ember Grit Pulse uses a square shape and is outside that conditional change. The
four new hashes must receive all 23 scenarios again. No gain correction is indicated
by this Bass set, and no additional audio probes were needed to resolve this scope.
Only this report and its evidence JSON were written; presets and implementation
files were left untouched.
