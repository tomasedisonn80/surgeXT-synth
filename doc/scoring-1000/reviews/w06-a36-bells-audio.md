# Wave 6 / Agent 36: Bells and Mallets native evidence review

## Result and scope

All **100 current Bells and Mallets presets** have complete, matching evidence:
**2,300 native scenarios, 500 technical gates, and 100 bit-identical default
repeats**. Every gate passes, with no nonfinite output, clipping-limit failure,
silent fixture, excess DC, or default-tail failure. **795 of 800 macro responses
resolve above the runner's thresholds. Five Contour responses are exactly zero
in the tested fixture and require correction.** A pass with those warnings is
not approval of their musical usefulness.

This review covers `SC03-001` through `SC03-100`, all 20 families and five variants.
It is pinned to catalog SHA-256
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
The complete metrics, per-preset and report hashes, warnings, and four extra dry
renders are indexed in [the evidence file](w06-a36-bells-evidence.json). This is a
technical category review, not human audition or composer endorsement.

## Independent consistency checks

For every record I compared its identity, actual FXP SHA-256, catalog-entry hash,
renderer hash, QA-script hash, configuration hash, NumPy version, and fixture hash
with the current files. All 23 expected fixtures are present per preset. Every
native row agrees with its requested notes, sample rate, hold, duration, velocity,
tempo, macro setting, wheel, pressure, and expression. Frame counts agree with
requested duration and rate; the format is stereo interleaved float32.

The native binary is
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`;
the measured QA script is
`203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681`.
Its engine label records base revision `dfdd2f94a`; it is the actual compiled
renderer addition, not a claim that this bank existed at that base commit.
The [runner](../../../scripts/scoring/qa_bank.py) restores the saved unity master
reference, preserves raw output, and measures repeat uncertainty explicitly.

## Audio integrity and playing range

| Measurement | Observed category result | Interpretation |
|---|---:|---|
| Default sample peak | 0.029963–0.098267; median 0.055265 | No unusually quiet-default warning under the runner's stated threshold |
| Default active RMS | 0.001754–0.017586; median 0.007534 | A short transient and a long resonator should not be judged by RMS alone |
| Maximum peak across all scenarios | 0.719979 | Quiet Anvils Close, 16-note stress; about 2.85 dB below sample full scale |
| Maximum absolute full/active channel DC | 0.000010612 | Well below the 0.003 protocol limit |
| Default mono energy ratio | 0.989538–0.999981 | Little default energy cancellation in the measured sum |
| Minimum mono ratio across all scenarios | 0.662829 | Quartz Gong Bloom under aggressive macros; approximately 1.79 dB energy loss |
| Hard/soft velocity active-RMS difference | 13.45–17.95 dB; median 14.96 dB | All tested voices retain a clear measured dynamic response |

Declared low endpoints produce peaks of 0.020504–0.098567; high endpoints produce
0.035176–0.096752. These establish non-silent, stable output at the tested
endpoints, not a listening judgment for every intervening note or voicing.

Default centroids span 222.6–8,079.9 Hz. **Quiet Anvils Glass, SC03-073**, is the
bright extreme. Its FM3 ratios, deliberately inharmonic tag, and industrial
accent role make high spectral energy plausible; the metric alone is no reason
to remove that character. **Hollow Slats Close, SC03-006**, has the lowest active
RMS and a short, dry rhythmic role. Cross-family centroids also use different
played middle notes, so they are diagnostics rather than matched-pitch rankings.

All default release fixtures settle deeply: the largest final half-second RMS
is approximately `1.0e-9`. Extended Release is a separate scope. The macro fixture
caps its observed tail at six seconds; **Quartz Gong Bloom** still has a tail
about 14.43 dB below active RMS at that endpoint. Its long extended envelope makes
that unsurprising, but this test does **not** establish complete settlement of
every maximum-Release tail. The [player guide](../USER_GUIDE.md) appropriately
asks users to fit tails to the passage and avoid dense accumulated releases.

## Contour corrections and weaker candidates

The following five records have **zero waveform and feature difference** at
Contour maximum in this fixture:

| ID | Preset |
|---|---|
| SC03-028 | Frost Celeste Glass |
| SC03-029 | Frost Celeste Alloy |
| SC03-053 | Silver Nodes Glass |
| SC03-078 | Starlit Drops Glass |
| SC03-079 | Starlit Drops Alloy |

Consistent with their open filters, filter-only Contour is ineffective at the
tested note. This does not establish identical behavior at every register. A family/variant-specific amplitude-decay route is a
reasonable correction that preserves the saved default and gives the control
an articulation function. The parent is collecting these changes; **this review
did not edit the bank**, and it must not be cited as verification of a later fix.

Five other Contour responses are measurable but small: Rain Tines Glass
`SC03-018` (relative waveform delta 0.001818), Frost Celeste Close `SC03-026`
(0.004992), Pocket Musicbox Glass `SC03-033` (0.009300), Ceramic Hands Glass
`SC03-063` (0.009867), and Starlit Drops Close `SC03-076` (0.009280). Their feature
distances are also below 0.01. This is a focused review list, not an invented
hearing threshold. No other macro in this category falls below both of those
prioritization values.

## Related variants and dry comparison

All 200 within-family pairs use matching active fixtures: the same note, hold,
velocity, tempo, controller state, and sample rate. Normalized spectral/envelope
features differ for every pair. Full default PCM hashes are also distinct, but
some render lengths differ; neither fact proves perceived uniqueness.

The closest pairs include **Hollow Slats Glass/Shadow** and **Warm Toybox
Glass/Shadow**. I rendered each pair dry with Space zero, fresh engines, matched
seeds, a three-second hold, and one second of release. Raw peaks were preserved;
positive RMS matching was applied only during comparison, never to repair a
headroom measurement.

| Dry pair | Matched active waveform difference | Normalized feature distance |
|---|---:|---:|
| Hollow Slats Glass / Shadow | 0.428729 | 0.027732 |
| Warm Toybox Glass / Shadow | 0.219071 | 0.031908 |

These are not simply constant-gain or reverb-only copies. Hollow Slats changes
its secondary FM contribution from 0.176 to 0.48, FM amount from 0.1365 to 0.0546,
cutoff from 4,650 to 1,488 Hz, and attack/release settings. Warm Toybox likewise
changes FM amount, secondary triangle weight and detune, envelope timing, and
cutoff. Their broad spectral profiles remain close, as expected within related
families; phase-sensitive waveform distance is not a musical quality score.
A matched-level audition in a real phrase remains the appropriate next judgment.

No code, recipes, presets, renderer, or QA protocol was modified. Raw probe files
and the analysis scripts are retained under `a36-bells` in the task workspace;
the report and compact evidence are the only repository additions.
