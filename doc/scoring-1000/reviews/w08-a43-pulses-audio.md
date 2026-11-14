# Wave 8 / agent 43: Pulses and Sequences native evidence

## Scope and result

Reviewed **all 100 current category-09 presets**, covering 20 families and five
variants each. All **2,300 native scenarios and 500 technical gates pass**. All
**800 macro responses resolve** above the existing measurement thresholds; all
100 default repeats are byte-identical, and the 100 default PCM hashes differ.
There are five quiet-default warnings and one additional small-response priority,
listed below. These observations do not certify human audition, perceived
uniqueness, musical usefulness, or composer endorsement.

The audit rechecked each actual FXP hash, catalog-entry binding, renderer hash,
QA source/configuration/fixture bindings, scenario identities, native frame
counts, notes, velocity, tempo, macro settings, controller settings, stereo
format, loaded name, native success and roundtrip result. It independently
reapplied the finite-output, sample-peak, silence and DC bounds to every recorded
scenario, and checked default-tail limits. The catalog and all report bytes
remained stable while the evidence was read. The
[bound evidence JSON](w08-a43-pulses-evidence.json) records these inputs and
per-preset report hashes.

There are **30 mono and 70 polyphonic presets**. Mono fixtures use one held note
and hard velocity; they do not establish overlapping-note priority or legato
behavior. The 70 polyphonic presets additionally face the 16-note stress fixture.
No new native audio was rendered for this review: the existing complete evidence
and prior isolated timing investigation resolved the relevant questions.

## Audio integrity and playing range

| Measurement | Observed result |
|---|---:|
| Highest scenario sample peak | 0.712732 — Velvet Spokes Triplet, 16 voices |
| Lowest scenario mono-energy ratio | 0.405350 — Velvet Spokes Soft, Width maximum |
| Highest absolute active/full channel DC | 0.000126690 — Low Lantern Triplet, Motion maximum |
| Default sample-peak range | 0.004377–0.155513 |
| Default active-RMS range | 0.000600–0.070475 |
| Default spectral-centroid range | 103.90–3675.31 Hz |
| Hard-to-soft velocity active-RMS difference | 8.07–10.99 dB |

No recorded scenario reaches the 0.98 peak limit, the silence floor, the DC limit,
or the mono-loss warning threshold. This includes the declared register
endpoints, 44.1 kHz fixtures, individual controls and the aggressive control
combination. All tests use unnormalized PCM and the saved unity master reference.
The range of spectral centroids is consistent with a palette spanning Low
Lantern's low foundation and Threaded Reed's brighter synthetic voice; a centroid
is not a harshness rating or proof of bass audibility on a particular speaker.

All saved-default tails satisfy the release gate, with measured tail-to-active
ratios below −127 dB. Extreme ratios can be dominated by numerical floors and
should not be interpreted as an acoustic noise-floor measurement. Maximum-Release
fixtures observe at most six seconds after note-off. Threaded Reed Soft remains
about **22.91 dB below active RMS** at that endpoint, so complete settlement at
every possible Release setting is not established.

## Concrete corrections and priorities

All five Breath Relay variants trigger the quiet-default warning. Their roles
specify an airy veil around dialogue, but their measured stress margin permits a
more practical saved preview level. The following additions target a default
sample peak of 0.03 using `20 log10(0.03 / measured default peak)`:

| Preset | Default peak | Largest measured scenario peak | Proposed VCA addition | Linearly predicted largest peak |
|---|---:|---:|---:|---:|
| SC09-056 Breath Relay Soft | 0.007693 | 0.065036 | +11.821008 dB | 0.253633 |
| SC09-057 Breath Relay Drive | 0.004654 | 0.036427 | +16.184970 dB | 0.234786 |
| SC09-058 Breath Relay Sparse | 0.004945 | 0.039778 | +15.659744 dB | 0.241338 |
| SC09-059 Breath Relay Triplet | 0.007376 | 0.062375 | +12.186387 dB | 0.253706 |
| SC09-060 Breath Relay Glass | 0.004377 | 0.035886 | +16.718543 dB | 0.245952 |

The parent accepted these candidates for integration under a measured +18 dB
positive-trim ceiling. **These are predictions for new saved parameters, not
verified corrected presets.** Changing voice gain changes the level entering
effects, whose behavior need not be perfectly linear. All five require the full
23-scenario native re-test after their new hashes are generated. No PCM was
normalized, and this target is not a perceptual loudness standard.

**SC09-085 Amber Ladder Glass / Contour** resolves technically but has a small
relative waveform change of **0.009723** and feature distance **0.000182**. It is
the category's only control below both of the parent's 0.01 diagnostic screening
values. That screen prioritizes adjustment or listening; it is not an established
threshold of human audibility. The parent has a targeted amplitude-decay route
available for filter-only contours that are too subtle.

## Pattern, pitch and tempo interpretation

A fresh structural pass with the independent
[sequence audit](../../../scripts/scoring/check_sequences.py) validates all 100
current FXPs, including the 20 tuned presets. They retain 16 steps, zero shuffle,
zero deformation and zero trigger masks. Sixteen families modulate timbre and
level without a sequencer pitch route. Four families use fixed pitch intervals
from raw LFO output, with unscaled extra amplitude: major pentatonic, minor
pentatonic, root/fifth/octave, or whole-tone material. Playing another root
transposes those intervals. This is not a MIDI arpeggiator that interprets an
input chord or emits separate MIDI notes.

Every Sparse variant has accents at steps 0 and 8. A zero step returns the VCA's
dB modulation to its base level; it does not set the voice amplitude to zero.
Trigger mask zero also leaves amplitude/filter envelopes unretriggered by those
steps. Accordingly, Sparse's corrected description of accents over a continuing
bed is accurate. Tuned Sparse variants replace their family scale with
root/octave material, as their role overrides explicitly state.

The current stored rates confirm the documented family distribution: one family
at 1 Hz, five at 2 Hz, three at 3 Hz and eleven at 4 Hz at 120 BPM. Each Triplet
variant is exactly 1.5 times its family's rate. The native
[LFO implementation](../../../src/common/dsp/modulators/LFOModulationSource.cpp)
advances one step per LFO phase wrap; BPM scales that rate. A 16-step sequence is
therefore not automatically a one-bar or sixteenth-note loop.

Every current preset changes its rendered output between the tempo fixtures.
The 60-versus-180 BPM spectral/envelope feature distance spans **0.0140–0.4417**.
Those whole-output comparisons include tempo-synchronized delay, so they do not
alone isolate sequencer timing. The
[earlier isolated timing audit](w05-a28-pitch-tempo.md) used dry native renders at
60/120/180 BPM and verified step centers independently. Its earlier input hashes
remain explicit; this review rechecks current interval/rate definitions without
claiming a new isolated timing render.

The ordinary three-second hold does not traverse every complete slow loop:
Distant Relay's 16 steps take 16 seconds at 120 BPM and 32 seconds at 60 BPM.
Complete slow-loop audition and host seek, loop, stop/restart behavior remain
separate session checks. Close family-feature pairs, especially Night Thirds
Soft/Drive and Low Lantern Soft/Glass, also remain useful listening comparisons;
different hashes and small feature distances are not perceived-uniqueness verdicts.

## Evidence binding

- Catalog SHA-256: `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
- Renderer SHA-256: `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.
- Scratch audit and summary: `/workspace/scratch/9118c5c72ad8/a43-pulses/`.

Only this report and its evidence JSON were added to the repository. No preset,
recipe, compiler, renderer, QA protocol or commit was changed. Later gain or
Contour corrections must be assessed against fresh, correctly bound evidence.
