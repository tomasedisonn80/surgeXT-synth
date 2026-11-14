# Wave 7 / Agent 40: Drones and Air native evidence review

## Scope and result

Independently reviewed **all 100 Drones and Air presets**, covering the complete
unity-master run for catalog `f8e0f6fd…81331`. All **2,300 native scenarios and 500
technical gates pass**. Nine Motion responses remain unresolved, with seven more
small-response priorities; these precede the parent's planned companion Noise-LFO
phase correction. Three dense-chord headroom warnings and one quiet-default
warning also require explicit disposition. This is historical, hash-bound
technical evidence, not certification of the later corrected bank or human
musical audition.

The [evidence JSON](w07-a40-drones-evidence.json) records every preset and QA-record
hash. I checked the catalog entry, actual FXP bytes, renderer, QA implementation,
configuration and regenerated fixture bindings. Every native report agrees with
its planned notes, timing, velocity, sample rate, tempo, macros and expression;
frame counts match the recorded PCM length. All inputs remained unchanged during
the analysis. The existing renderer produced the audio; this audit did not
re-render the category or reconstruct deleted PCM from its fingerprints.

| Coverage | Result |
|---|---:|
| Current preset/record bindings | 100 / 100 |
| Native scenarios | 2,300 |
| Technical gates passed | 500 / 500 |
| Nonfinite, silence, DC or peak-limit failures | 0 |
| Resolved macro responses | 791 / 800 |
| Byte-identical default repeats | 100 / 100 |
| Distinct full default PCM hashes | 100 |

Distinct hashes establish different rendered data, not perceived uniqueness.

## Signal integrity and gain decisions

Default sample peaks range **0.009615–0.095432**, with median **0.051043**. Active
RMS ranges **0.001329–0.054145**, reflecting deliberately different tonal/noise
balances rather than a matched perceived-loudness target. Hard velocity raises
active RMS by **4.80–9.41 dB** over the soft fixture in every preset.

The maximum measured sample peak is **0.843821**, Harbor Hum Open under 16-note
stress. It passes the 0.98 limit but leaves less than the warning threshold's
1.94 dB of sample-peak headroom. Three presets merit the parent's planned saved
voice-gain correction:

| Preset | Dense-stress peak | Approximate additional trim to target 0.72 |
|---|---:|---:|
| SC06-026 Harbor Hum Near | 0.828831 | −1.22 dB |
| SC06-028 Harbor Hum Open | 0.843821 | −1.38 dB |
| SC06-098 Deep Listening Open | 0.801614 | −0.93 dB |

These are starting calculations from measured output, not substitutes for
re-rendering new preset bytes. Default audio remains healthy at those reductions.
The maximum absolute active/full channel mean is **0.000371**, below the 0.003
DC threshold. The worst mono-energy ratio is **0.353464** at Low Cloud Open's
Width maximum; its default ratio is **0.752644**. This is appreciable spatial
change, but no mono-loss warning occurs under the protocol's 0.1 threshold.

**SC06-038 Glass Air Open** is the only default-peak warning: peak **0.009615**,
active RMS **0.002218**, and maximum across all scenarios **0.200242**. Its airy
role supports a restrained level, but a moderate measured gain lift is also
feasible. A hypothetical lift to 0.03 default peak is approximately +9.88 dB and
would predict about 0.625 at the old worst fixture under linear scaling. Effects
and the forthcoming control correction make native retesting necessary. Peak
matching alone does not establish equal perceived loudness.

## Low-register evidence

Harbor Hum and Deep Listening now advertise **MIDI 36–60**. Their principal
sub-octave layers therefore start around **32.7 Hz**, above their 20 Hz low cut.
All ten variants produce finite, clearly nonzero low-endpoint signals:

| Family | Low-endpoint peak range | Active RMS range | Measured energy below 40 Hz |
|---|---:|---:|---:|
| Harbor Hum | 0.05865–0.06851 | 0.02809–0.03895 | 92.0–95.9% |
| Deep Listening | 0.05281–0.06973 | 0.02241–0.03277 | 86.8–88.4% |

This confirms a functioning low-register signal, not dependable audibility on
small speakers or a diagnosis of fundamental pitch from the strongest FFT bin.
The bass-heavy energy distribution remains a deliberate audition/mix concern.

Other ranges deserve precise wording. Low Cloud's lowest saw base is approximately
20.6 Hz, just below its 23 Hz low cut, while its second layer is around 41.3 Hz.
Its five low-endpoint renders have peaks **0.04560–0.07909** and substantial
40–160 Hz energy. It is neither silent nor exclusively subsonic. Several other
families intentionally place an oscillator base below a high-pass cutoff, including
Hollow Distance and Friction Veil; filtering an underlying pitch does not by
itself establish a tuning error or an unusable sound.

Silver Static, Metal Weather and Orbital Dust explicitly use fractional FM ratios
and inharmonic descriptions in the [recipes](../../../scripts/scoring/recipes/06_drones_air.json).
Their non-harmonic spectra should not be judged against an unsupported acoustic
orchestral-realism promise. Silver Static Ripple's **4.218 kHz** centroid is the
category's brightest default, consistent with its high FM/noise role; comfort in
an actual cue still needs listening.

## Controls and release limits

All unresolved responses are **Motion**: SC06-014, SC06-041–044 and SC06-061–064.
The additional small-response screen identifies SC06-011–013 and SC06-076–079.
The screen requires both relative waveform change below 0.01 and feature distance
below 0.01; it is a prioritization rule, not a perceptual audibility standard.
All other controls resolve under the main protocol, and no additional Contour
problem was found.

These families agree with the independently diagnosed
[Noise startup mechanism](w07-a37-motion-diagnosis.md). The chosen fix changes
only the companion Noise phase to 0.5, retaining the main random LFO and zero
saved companion magnitude. It affects 24 category presets, including already
resolved faster-noise families, so all 24 require fresh evidence. Their Ripple
variants use a different shape and are outside that conditional adjustment.

Default renders run **7.64–17.42 seconds** in total. Every default tail settles;
the least attenuated final 500 ms is still approximately **120 dB below active
RMS**. This must not be extended into a claim about maximum Release. Those
fixtures observe at most six seconds after note-off. Deep Listening Drift retains
**−9.35 dB** relative tail energy at that point, consistent with its much longer
extended envelope. The audit does not establish complete maximum-Release decay
or arbitrary repeated-note voice behavior.

## Related variants and provenance

The closest matched-note/hold feature pair is Porcelain Mist Near/Open
(distance **0.03530**), followed by Harbor Hum Near/Open (**0.04225**). They contain
real non-FX differences: Porcelain Mist's nominal release changes 1.32→1.92 seconds,
cutoff 2,635→5,580 Hz, and secondary String weight 0.231→0.1815. Harbor Hum likewise
changes envelope timing, filter cutoff/resonance and Modern-layer balance.
These support purposeful related variants without proving perceived separation.
No additional native probe was necessary to resolve a remaining engineering
question in this bounded review.

| Input | SHA-256 |
|---|---|
| Catalog | `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331` |
| Renderer | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| QA implementation | `203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681` |

Full recipe/compiler/configuration and per-preset bindings are in the companion
JSON. The analysis script is retained at
`/workspace/scratch/9118c5c72ad8/a40-audit.py`. Only this report and its evidence JSON
were added to the repository. Presets, source, recipes and QA definitions were
not modified. Human audition and composer endorsement remain unclaimed.
