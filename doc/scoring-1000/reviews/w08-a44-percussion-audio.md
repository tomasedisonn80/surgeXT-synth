# Wave 8 / Agent 44: Percussion and Transitions native evidence

## Scope and reviewed state

This independent review covers **all 100 presets, SC10-001 through SC10-100**,
their 20 authored families and five variants, and all retained native QA records.
It is a **pre-correction snapshot**: catalog SHA-256
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
The parent is retaining the completed run in
`/workspace/scratch/9118c5c72ad8/qa-scoring-pre-polish` before applying targeted
changes. Findings here must not be presented as verification of later bytes.

The [evidence JSON](w08-a44-percussion-evidence.json) records every input FXP and
QA-report hash, native executable, measurement implementation, configuration,
recipe, compiler, distributions, warning IDs and gain candidates. The native
executable hash is
`563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.
All report files remained stable through the completed audit.

I checked catalog-entry and fixture bindings, all native note/control/rate/frame
identities, metadata, PCM format, finite metrics, declared thresholds, every gate,
and the relevant programmed pitch/envelope routes. This re-evaluates retained
measurements; it does not claim new waveform analysis of deleted PCM. No additional
native render was necessary. The scratch audit script and its hash are retained
in `/workspace/scratch/9118c5c72ad8/a44-percussion/` and the evidence JSON.

## Complete technical coverage

| Check | Result |
|---|---:|
| Current, correctly bound presets | 100 / 100 |
| Native scenarios | 2,300 / 2,300 passing |
| Technical gates | 500 / 500 passing |
| Protocol-resolved macro responses | 794 / 800 |
| Bit-identical default repeats | 100 / 100 |
| Distinct default PCM hashes | 100 |
| Warning messages / affected presets | 20 / 18 |
| Integrity, silence, DC, headroom or default-tail failures | 0 |

Every preset is polyphonic and receives the distinct 16-note stress fixture.
Maximum sample peak is **0.720002**, Iron Footfall Body (SC10-037), under that
fixture. The minimum mono-energy ratio is **0.339979**, Air Swish Edge (SC10-048)
with aggressive macros, above the protocol's 0.1 warning threshold. Maximum
absolute full/active channel DC is **0.000010488**. These are sample-domain
measurements at the saved unity master, not reconstructed true-peak or listening
certifications.

Default peaks span **0.003600–0.103678**; active RMS spans
**0.000138–0.013173**. Hard-versus-soft velocity increases active RMS by
**4.15–17.21 dB**. A low whole-hold RMS is expected for a short hit followed by
silence, so peak, envelope and role must be considered together.

## Controls requiring targeted correction

The Contour findings are concentrated in Glass Tick:

| Preset | Relative waveform change | Feature distance | Disposition |
|---|---:|---:|---|
| SC10-026 Close | 0.007500 | 0.000707 | Small resolved change |
| SC10-028 Edge | 0 | 0 | Unresolved |
| SC10-029 Drift | 0.000320 | 0.000000586 | Unresolved |
| SC10-030 Bloom | 0 | 0 | Unresolved |

The bright source and open filter leave limited filter-only contrast. The parent's
planned additional amplitude-decay mapping is a reasonable focused intervention;
the resulting hashes need complete native re-testing. The `<0.01` waveform and
`<0.01` feature screen prioritizes investigation and is not a perceptual standard.

Unresolved Motion responses occur at **SC10-021 Paper Brush Close**, **SC10-023
Paper Brush Edge**, and **SC10-041 Silk Shaker Close**. Additional small Motion
changes occur at SC10-022, 036, 038, 042 and 043. These agree with the independently
diagnosed Noise interpolation-history startup issue in
[agent 37's review](w07-a37-motion-diagnosis.md).

The selected correction changes only the companion Noise LFO's initial phase to
0.5. All **30 category presets using Noise motion** require new-hash evidence:
SC10-021–025, 036–050, 056–060 and 091–095. Main random-motion settings remain
authored; the intended companion exception must be documented explicitly.

## Quiet defaults and brightness

Fourteen defaults are below the 0.01 peak warning level:

| Family / IDs | Default peak range | Maximum tested peak range |
|---|---:|---:|
| Paper Brush, SC10-021–025 | 0.003770–0.006000 | 0.027821–0.049206 |
| Clay Pulse, SC10-031 and 033–035 | 0.004589–0.008445 | 0.057115–0.163431 |
| Air Swish, SC10-046–050 | 0.003600–0.005431 | 0.022342–0.036741 |

Their measured stress margin supports saved-VCA gain candidates. Targeting roughly
0.03 default sample peak predicts maxima **0.128–0.581** across these fixtures.
This calculation is a starting estimate, not a native pass or perceptual loudness
match. Effects may respond nonlinearly, and the changed Noise startup also needs
testing. The evidence JSON gives exact candidate additions, bounded at **+18 dB**.
That cap leaves Paper Brush Bloom near 0.02995 and Air Swish Drift near 0.02859;
there is no reason to widen the guard simply to hit an arbitrary decimal target.

**Air Swish Edge is an explicit audition priority after gain adjustment.** Its
default spectral centroid is **16.12 kHz**, with approximately **74.3%** of measured
spectral energy above 10.24 kHz. A bright filtered-air gesture plausibly occupies
that range, but raising its peak does not establish balanced loudness, comfortable
brightness or translation to different listeners and playback systems. The
available measurements do not establish an aliasing defect.

## Transients, transitions and variant identity

Short articulations behave differently from sustained transitions. Felt Drum
Close's second 250 ms envelope bin is only **0.00745** of its first; Dawn Sweep,
Breath Lift and Velvet Arrival retain appreciable energy through their held-note
windows. These coarse bins support the broad articulation distinction, while
omitting sub-bin attack detail.

All **45 pitch-envelope assignments across nine families** match the
[recipes](../../../scripts/scoring/recipes/10_percussion_transitions.json).
Dawn Sweep uses positive depth with a growing filter envelope; Falling Arc uses
negative depth. Their envelopes return toward the played note on release. Attack
targets amplitude-envelope attack, not the pitch-driving filter-envelope attack.
Contour extends that shared filter envelope's decay and can therefore also alter
pitch-decay behavior on these drums and gestures. None of this implies an exact
bar duration, a one-direction ending, or reversed recorded audio.

The closest of 200 within-family feature pairs is **Felt Drum Close / Edge**,
distance **0.003237** under matching playing conditions. Their nominal decays
differ (0.1736 versus 0.224 seconds), as do source balance, filtering and noise.
The short hit is poorly resolved by 12 bins over a three-second hold; this pair
belongs on a focused listener A/B list. Different PCM hashes alone do not prove
perceptually valuable variation, and no dry level-matched comparison is claimed.

All saved-default release fixtures settle at least **109.08 dB below active RMS**
within their measurement windows; some reach the numerical reporting floor.
Maximum-Release fixtures observe at most six seconds after note-off. Velvet
Arrival Bloom remains **26.14 dB** below active RMS there, with tail RMS 0.000548.
The audit therefore does not certify complete settlement at every macro setting.

Only this report and its evidence JSON were written to the repository. Human
musical audition, acoustic realism and composer endorsement remain unclaimed.
