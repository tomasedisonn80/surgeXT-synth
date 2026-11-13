# Native showcase patch sources

The screenshot test creates an isolated processor and a real demonstration patch
for each reference workspace. `configureShowcaseFixture()` in
`src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp` starts with the checked-in
`Init Wavetable.fxp` template, calls the native wavetable loader, and changes
parameters through `ParameterBridge`. Loading a skin in the application does not
install these patches or change a user's sound.

## Original wavetable assets

`scripts/reference-workspaces/generate_showcase_wavetables.py` generates the ten
files in `resources/data/wavetables/Reference Showcases/`. They are original
periodic sums of integer-frequency sine harmonics. Frame interpolation changes
harmonic amplitudes and phases; the analog family uses a band-limited saw-like
harmonic series. All samples are audio source data used by the native oscillator.
No pixels, traced image coordinates, recorded factory audio, or frozen output
plots are included.

The binary layout follows `doc/WT fileformat.txt`: `vawt`, a little-endian sample
count, frame count, flags zero, and little-endian float32 frames. Every frame has
its DC component removed and is normalized to a 0.92 absolute sample peak. The
maximum harmonic is 48 cycles per table for the saw family; all files satisfy
their source sampling Nyquist limit. The native loader builds its usual mipmaps
and the oscillator performs the actual synthesis.

| File stem | Frames | Samples per frame | Bytes | Analytic family |
| --- | ---: | ---: | ---: | --- |
| Spectral Sweep | 48 | 512 | 98,316 | Smooth two-cycle harmonic sweep |
| Analog Stack | 48 | 512 | 98,316 | Two-cycle band-limited saw stack |
| Spectral Bloom | 256 | 2048 | 2,097,164 | Smooth three/four-cycle harmonic dunes |
| Bauhaus Waves | 48 | 512 | 98,316 | Five-cycle ribbon family |
| Orbiting Saw | 48 | 512 | 98,316 | Three-cycle band-limited saw stack |
| JT Saw RM | 48 | 512 | 98,316 | Three-cycle harmonic industrial family |
| Aurora Waves | 48 | 512 | 98,316 | Smooth three-lobe ribbon family |
| Glass Sweep | 48 | 512 | 98,316 | Two-cycle glassy harmonic sweep |
| GlassWavetable 03 | 48 | 512 | 98,316 | Three-lobe phase-shifted horizon family |
| VocalFormant A | 48 | 512 | 98,316 | Five/seven/nine-harmonic formant family |

The ten assets total **2,982,008 bytes**. Spectral Bloom intentionally contains
256 real frames of 2048 samples so the displayed metadata agrees with its
reference concept. The nine remaining assets total 884,844 bytes. The UI's
existing bounded preview sampling remains unchanged.

Regenerate from the repository root:

```sh
python3 scripts/reference-workspaces/generate_showcase_wavetables.py
python3 scripts/reference-workspaces/generate_showcase_wavetables.py --check
```

The check validates the header and exact byte length, finite values, DC offset,
peak normalization, distinct frames, periodic analytic endpoints, and equality
with the regenerated files. It prints SHA-256 hashes for all ten files. Generation
uses Python's standard library and no external audio or image assets. The script
and generated assets use the repository's GPL-3.0-or-later license.

## Per-workspace native patch settings

| Workspace | Actual patch name | Primary wavetable | Native filter routing | Oscillator mixer levels (dB) |
| --- | --- | --- | --- | --- |
| 01 Obsidian Precision | Init | Spectral Sweep | Serial 1 | 0 / -3 / -6 |
| 02 Ivory Atelier | Init | Analog Stack | Serial 1 | 0 / -3 / -6 |
| 03 Spectral Observatory | Init | Spectral Bloom | Dual 1 | 0 / -3 / -6 |
| 04 Bauhaus Modular | Bauhaus Harmonic Lab | Bauhaus Waves | Dual 1 | 0 / -4.6 / -2.1 |
| 05 Space Age 1977 | Space-Age Instrument 1977 | Orbiting Saw | Dual 1 | 0 / -3.2 / -6 |
| 06 Tokyo Industrial | Tokyo Bass Sequence | JT Saw RM | Serial 1 | 0 / -3.1 / -6 |
| 07 Liquid Aurora | Liquid Aurora | Aurora Waves | Dual 1 | 0 / -3.1 / -4.6 |
| 08 ClearSignal Accessible | ClearSignal Pad | Glass Sweep | Serial 1 | 0 / -1.2 / -3 |
| 09 Signal Flow Cartographer | Evolving Glass Engine | Glass Sweep | Dual 1 | 0 / -3 / -6 |
| 10 Live Performance Deck | Evolving Horizons | GlassWavetable 03 | Serial 1 | 0 / -2.4 / -1.8 |

Workspaces 02, 07, 08, 09 and 10 also load VocalFormant A into their actual third
wavetable oscillator. Other native oscillator algorithms remain visibly named
by the engine; a reference's unavailable algorithm is not represented by a
fictional preset label. Noise is audible at -18 dB in 07/10 and -12 dB in 08;
the remaining fixtures mute it. Ring modulation is muted in all fixtures.

Mixer gains and cutoff frequencies use the native text parser with explicit dB
and Hz units. A value such as 0.68 in the engine's amplitude parameter is not a
linear amplitude and does not mean -3.35 dB: its native display is about -10 dB.
The new fixture uses 0/-3/-6 dB explicitly for the common mixer configuration.
Scene volume is 0 dB and master volume is -6 dB. The native Distortion pre/post
high-cut filters are set to 18 kHz, drive to +6 dB, and output to -6 dB; this
preserves real upper partials instead of the initialized 440 Hz high-cuts.
Conditioner threshold is -6 dB wherever that real algorithm is present.

The eight macro knobs have real native controller values and modulation routes.
Capture profiles use their reference-inspired labels, with each meaningful
label connected to an appropriate native target: Drive to Distortion drive,
Space to Reverb 2 mix, FX Mix to Chorus mix, Motion to LFO rate, and Attack/Release
to the amplitude envelope, for example. The default fixture used by independent
behavior tests retains its original eight macro target bindings and eight
active effect types.

These are native FX slots with the engine's actual insert/send/global topology;
the test does not turn all eight into a fictional serial insert chain. Every
algorithm selection and changed parameter is recorded. Some concept controls
and algorithms have no exact engine equivalent; the fixture is not an assertion
that every illustrated setting or audio response has been reproduced.

## Capture evidence

The capture case passes the workspace ID into the fixture. `patchFixture`
records the actual patch name, skin ID, template path, every loaded wavetable's
file/display name/frame count/sample count/byte size, generator provenance,
normalized parameter values and native displays, and resolved modulation rows.

The capture plays held MIDI notes 48, 60 and 67 on channel 1 at velocity 0.65.
Its metadata derives block count, block size, sample rate and duration from the
test constants, rather than repeating a stale pre-roll length. Spectrum,
spectrogram and meters read the real processor audio tap. Source-table peak
normalization is not proof of output headroom: `offlineMainOutputPeak` and the
native audio checks provide the actual rendered-output evidence. Strict reset
silence remains below 0.000001 after the four post-reset audio blocks.

The generator check and a source syntax check do not establish a linked build,
audible headroom, reset correctness, or screenshot fidelity. Those require the
native workflow and comparison of its newly captured screenshots.
