# Scoring 1000: native audition previews

[Play the 94-second overview](audio/scoring-1000-overview.mp3), or open an
individual lossless FLAC below. These ten excerpts cover one preset from each
category. They let you hear selected source colors, entrances, movement and
released tails before browsing the full bank.

Every excerpt comes from the actual `surge-scoring-render` executable, which
links Surge's native synthesis engine. Each is one held note or simultaneous
voicing. The overview plays them in category order, with 0.4 seconds of silence
between excerpts. There is no backing track, assembled note collage, added
reverb, mastering compressor or limiter. The effects you hear are the preset's
own saved effects.

## Listen by category

Each linked FLAC is 48 kHz stereo, encoded from 24-bit PCM. The MP3 overview is a
compact, lossy listening copy; use the individual FLACs when comparing details.
Note names use the convention **MIDI 60 = C4**. All examples run at 120 BPM.

| Category | Native preset / FLAC | Performance | Total length | Overview start |
|---|---|---|---:|---:|
| 01 Keys | [SC01-007 — Copper Tine Felt](audio/01-copper-tine-felt.flac) | A3, C4, E4; velocity 88; hold 2.5 s | 6.660 s | 0:00.00 |
| 02 Plucks | [SC02-014 — Harbor Harp Cascade](audio/02-harbor-harp-cascade.flac) | C4, E4, A4; velocity 88; hold 2 s | 8.080 s | 0:07.06 |
| 03 Bells and Mallets | [SC03-022 — Violet Vibra Bloom](audio/03-violet-vibra-bloom.flac) | F4, A4, C5; velocity 82; hold 3 s | 9.674 s | 0:15.54 |
| 04 Pads | [SC04-003 — Amber Canopy Bloom](audio/04-amber-canopy-bloom.flac) | A2, E3, C4; velocity 84; hold 4 s | 13.650 s | 0:25.61 |
| 05 Ensemble Hybrids | [SC05-022 — Ember Violas Section](audio/05-ember-violas-section.flac) | G3, D4, G4; velocity 92; hold 4 s | 8.916 s | 0:39.66 |
| 06 Drones and Air | [SC06-001 — Quiet Horizon Near](audio/06-quiet-horizon-near.flac) | C3, G3; velocity 78; hold 6 s | 11.795 s | 0:48.98 |
| 07 Bass | [SC07-033 — Transit Saw Edge](audio/07-transit-saw-edge.flac) | E2; velocity 100; hold 3 s | 7.000 s | 1:01.17 |
| 08 Leads | [SC08-092 — Moonlit Triangle Warm](audio/08-moonlit-triangle-warm.flac) | A4; velocity 88; hold 4 s | 8.053 s | 1:08.58 |
| 09 Pulses and Sequences | [SC09-082 — Amber Ladder Drive](audio/09-amber-ladder-drive.flac) | C3; velocity 96; hold 8 s | 12.000 s | 1:17.03 |
| 10 Percussion and Transitions | [SC10-052 — Tension Thud Body](audio/10-tension-thud-body.flac) | C2; velocity 104; hold 0.7 s | 4.700 s | 1:29.43 |

The last example is a low synthetic impact. It illustrates the percussion side
of category 10; that category also contains swells and transitions that are not
represented by this single clip. Likewise, one pad or mallet example cannot
represent all 100 presets in its category. The ensemble and instrument labels
describe synthesized colors, rather than recorded acoustic performances.

## Playback level and what was preserved

All ten clips use their **saved macro settings**: Tone, Motion, Contour, Color,
Width, Attack and Release at zero, with **Space at 40%**. Wheel and channel
pressure are zero; expression is 127. A new seeded engine is created for each
excerpt. Notes are within the preset's declared range, and mono presets receive
one note.

For comfortable preview playback, **one common multiplier of 4.0 (+12.04 dB)**
is applied after rendering to every excerpt and to the overview. Relative levels
are preserved. No clip receives individual peak normalization or loudness
matching, and no limiter hides an overload. The raw native float32 measurements
are retained separately from this playback adjustment in
[previews.json](audio/previews.json). The largest raw native excerpt peak was
**0.155511**; the largest decoded FLAC peak is **0.622044**, below the preview
ceiling of 0.8. The overview's decoded peak is **0.622032**.

The amplitude increase belongs only to these listening files. It does not change
any preset, voice-gain trim or unnormalized bank QA measurement. The performances
also use different registers, voicings and velocities to illustrate their roles,
so these clips are not controlled loudness comparisons between categories.

The observation tail is at least four seconds, or three times the saved nominal
Release plus two seconds, whichever is longer. Analog envelope times are nominal
settings; they do not guarantee an exact acoustic rise or decay time. These clips
show the saved defaults, not the longest possible Release-macro settings.

## Audition the editable sounds

1. Load the listed ID/name in the native Scoring 1000 patch browser and play a
   short passage from your own cue. The [player guide](USER_GUIDE.md) describes
   installation, categories and the eight common controls.
2. Compare the saved Space setting with **Space = 0**. This separates the source
   and articulation from the preset's delay and reverb.
3. Change one macro at a time using the same passage. Motion = 0 retains authored
   movement; Width = 0 retains authored width. Neither is an automatic off switch.
4. Check softer and harder playing, the register you need, and phrase endings.
   Longer Attack or Release settings may change how overlapping notes fit.
5. Compare in the actual arrangement and in mono. Adjust track level for that
   passage, and judge brightness, timing and space in context.

The files are **audition material**, not evidence that a listener has approved
all 1,000 presets. No human listening pass, perceived-quality score, acoustic
realism certification or composer endorsement is claimed. The separate
[QA protocol](../../scripts/scoring/QA_RUNNER.md) describes technical measurements.

## Reproduce and verify

The reusable [preview script](../../scripts/scoring/render_previews.py) requires
NumPy, FFmpeg/FFprobe with FLAC and MP3 encoders, and the compiled native renderer:

```sh
python scripts/scoring/render_previews.py \
  --renderer build-scoring/src/scoring-tools/surge-scoring-render \
  --work-dir /tmp/scoring-preview-work
```

It renders all ten inputs through one sequential native process, checks input
hashes before and after rendering, then encodes and decodes every delivery file.
All ten FLACs have exact fixture frame counts, 48 kHz stereo format, zero
nonfinite samples and conversion error below one 24-bit PCM least-significant
step. The MP3 overview decodes with **37 padding frames (about 0.77 ms)** beyond
the concatenated reference, recorded explicitly rather than treated as an exact
PCM-length match. No audio samples are removed to conceal a failure.

[previews.json](audio/previews.json) records each patch hash, native fixture and
raw metrics, encoded-file hash, decoded metrics, common gain, encoder version and
overview timeline. [checksums.json](audio/checksums.json) covers every delivered
audio file and the metadata. This delivery is bound to catalog SHA-256:

```
67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2
```

Future regeneration may use a different common playback multiplier if the
selected native outputs change; the newly written JSON is the authority for
that run. Refresh this recorded-delivery description when publishing new audio.
