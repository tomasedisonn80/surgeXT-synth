# Wave 5 / assignment 26 — actual capture state

Inspected all ten references, capture evidence, native parameters and repository
guidance. Editing began after wave-4 clearance at `411578a`. Changes are
confined to `configureShowcaseFixture` and capture MIDI/evidence in
`XTTestReferenceWorkspaces.cpp`; the browser regression and production renderer
are untouched. Selecting a skin never installs these test patches.

## Supported values

ADSR units: milliseconds / milliseconds / normalized sustain / seconds.

| Profile | Actual fixture changes |
| --- | --- |
| Obsidian | Amp `5 / 200 / 1.00 / .32`; LFO 2 Hz; filter keytracking 50%/0%. |
| Ivory | Native Legacy Ladder at 2400 Hz/17%; BP12 at 824 Hz/48%; amp `5 / 280 / .70 / .48`; wavetable positions 0/.32 and OSC3 eight voices. |
| Spectral | Amp `5 / 286 / .76 / 1.25`; OSC2 −3.2 dB; Serial 1; LFO 3 Hz, corresponding to the pictured quarter-note triplet at 120 BPM. |
| Bauhaus | Wavetable position .427; Serial 1; filters 2600 Hz/.38 and 1150 Hz/.71; amp `10 / 320 / .76 / 1.20`. |
| Aurora | Amp `12 / 680 / .86 / 2.40`; filter EG `24 / 420 / .73 / 1.80`; OSC3 position .71; Classic width .72, seven voices and 14-cent detune. |

Bauhaus macros now use `.76,.42,.68,.53,.31,.57,.85,.47`; Aurora uses
`.71,.68,.54,.76,.21,.67,.19,.63`. ClearSignal's real LFO1→filter1 cutoff
assignment is +.64; other unmeasured route depths remain modest.

LFO3 envelopes use `10/1000/.50/2.40` for Space,
`4/120/0/.25` for Tokyo, and `5/1200/0/4.50` for Aurora.

## Genuine audio history

The 768-block, 48 kHz capture sends this channel-1 phrase. All events are at
sample offset zero; numbers after colons are integer MIDI velocities.

| On block | Off block | Pitches:velocities |
| --- | --- | --- |
| 0 | 72 | 48:48 |
| 96 | 168 | 60:58, 67:53 |
| 192 | 264 | 72:44 |
| 288 | 360 | 48:64, 55:57 |
| 384 | 456 | 55:51, 62:47, 69:43 |
| 480 | 552 | 60:60, 64:55, 67:50 |
| 576 | 624 | 48:49 |
| 640 | 672 | 72:42 |
| 704 | Held until reset | 48:70, 60:76, 67:73 |

Evidence records each onset, release, time, pitch and velocity. Real release/FX
tails remain; Live's four incoming controller values remain unchanged.

## Caveats and verification

Obsidian's concept contradicts itself (upper ENV1 70%, AMP/OUT 100%); the real
amp uses 100% and filter EG 70%. Spectral retains native LP24 because “Peak 24”
has no identical native choice; its Series diagram takes precedence over its
contradictory Dual caption. Unnumbered macros retain prior demonstration values.
Tokyo's illustrated 1 ms third-envelope attack uses the supported 4 ms
adaptation; existing Space/Tokyo attack adaptations remain explicit in evidence.

All 100 scoped assertions remain unchanged. Whitespace checks
pass; schedule simulation confirms 17 onsets, 14 releases and at most three
gated notes. New envelope values fit native ranges. Native audio/headroom,
reset and capture verification await CI; no build,
commit, push or new screenshot is claimed.
