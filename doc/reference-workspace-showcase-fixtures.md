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

Space-Age's second oscillator additionally reuses the checked-in factory
`resources/data/wavetables/Basic/Sine To Square.wt`. Its 31 frames of 128 int16
samples occupy 7,948 bytes; it is not a newly generated showcase asset. At the
fixture's real morph value of 0.612, it supplies a square harmonic family. The
native loader retains **Sine To Square** as the actual source name; the concept's
unavailable **Pulsar 16** name is not substituted into the UI. Per-source capture
metadata distinguishes this reused factory table from the original generated
families and verifies each source's frame and sample counts independently.

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

Ivory's second oscillator uses native **Classic**, with Shape 0, Width 1 at
0.50 and four unison voices. That produces the reference's saw family through
real Shape/Width/Unison controls. The native algorithm name is retained; the
concept's Hybrid / Saw + FM labeling is not claimed as an additional engine mode.

Space-Age now uses the actual native **Wavetable** algorithm for Oscillator 2,
at +1 octave, and the native **Sine** algorithm for Oscillator 3, at -1 octave.
Oscillator 3 uses Sine shape 0, zero feedback, zero detune and one voice.
Oscillators 1/2 each use four native unison voices, with 12/7 cents of detune.
These parameters use the normal bridge setters and native readback checks. The
concept's generic Analog label, oscillator phase controls and independent spread
settings are not fictionalized as unsupported parameters.

Mixer gains and cutoff frequencies use the native text parser with explicit dB
and Hz units. A value such as 0.68 in the engine's amplitude parameter is not a
linear amplitude and does not mean -3.35 dB: its native display is about -10 dB.
The new fixture uses 0/-3/-6 dB explicitly for the common mixer configuration.
Scene volume is 0 dB and master volume is -6 dB. The native Distortion pre/post
high-cut filters are set to 18 kHz, drive to +6 dB, and output to -6 dB; this
preserves real upper partials instead of the initialized 440 Hz high-cuts.
Conditioner threshold is -6 dB wherever that real algorithm is present.

Two illustrated attack times need explicit native adaptations. Space-Age's
amplitude-envelope attack and Tokyo's filter-envelope attack both use **4.0 ms**
in their actual showcase patches, in place of the concepts' 2.0 ms and 3.2 ms.
The engine's envelope-time range is log2(seconds) from -8 to 5. Its lower
endpoint has special zero-time display semantics, and the amplitude/filter
envelope skips the attack when the value is less than 0.01 above that endpoint.
Four milliseconds lies above this instantaneous-attack region and remains near
the shortest supported nonzero attack. The native range assertions are retained;
there is no general clamp that could hide other invalid fixture values. The
recorded parameter displays, envelope plots and rendered audio use these real
4.0 ms settings.

The eight macro knobs have real native controller values and modulation routes.
Capture profiles use their reference-inspired labels, with each meaningful
label connected to an appropriate native target: Drive to Distortion drive,
Space to Reverb 2 mix, FX Mix to Chorus mix, Motion to LFO rate, and Attack/Release
to the amplitude envelope, for example. The default fixture used by independent
behavior tests retains its original eight macro target bindings and eight
active effect types.

Live's Macro 1 and Macro 2 are actually set to **0.62 and 0.37**, respectively,
so its XY position and readouts derive from the real controller values. Their
existing Drive and Motion assignments and normalized route depths remain intact.
The fixture records both normalized values and native displays in `settings`.

The visible LFO families are also native patch settings:

| Workspace | LFO 1 | LFO 2 | LFO 3 |
| --- | --- | --- | --- |
| 04 Bauhaus Modular | Sine, 2 Hz | Triangle, 4 Hz, 90° phase | Square, 8 Hz |
| 05 Space-Age 1977 | Sine, 0.25 Hz | Triangle, 1 Hz, 90° phase | Sample & Hold, 4 Hz |
| 06 Tokyo Industrial | Triangle, 2 Hz | Template default | Template default |
| 07 Liquid Aurora | Sine, 0.24 Hz | Triangle, 2 Hz, amplitude 0.62 | Sample & Hold, 4 Hz, amplitude 0.48 |
| 08 ClearSignal Accessible | Sine, 0.25 Hz | Template default | Template default |
| 09 Signal Flow Cartographer | Sine, 0.25 Hz | Triangle, 0.5 Hz | Sine, 0.125 Hz |
| 10 Live Performance Deck | Sine, 0.28 Hz | Triangle, 0.5 Hz | Template default |

All explicitly configured LFOs are key-triggered; their phases are zero unless
listed otherwise. Styled LFO 1 and the newly configured Bauhaus/Space-Age/Aurora
LFOs use zero deform and bipolar output. Bauhaus's 2/4/8 Hz and Aurora's 4 Hz
are free-rate equivalents of the concepts' note values at 120 BPM, not a claim
that tempo synchronization is enabled. Sample & Hold uses the native random
step algorithm; its preview remains a separately seeded example. The extra
Bauhaus/Space-Age/Aurora LFO settings add no modulation routes.

Live's reference draws approximately two sine periods while the native thumbnail
currently normalizes periodic shapes to one cycle. Its actual Sine family and
0.28 Hz rate remain correct; changing the audible rate to imitate that drawing
would not fix the preview's time axis. Cartographer retains LP24 for its first
filter, as named both on the reference node and in its 24 dB/oct inspector.

Styled profiles also contain actual voice modulation assignments. All ten add
LFO 1 to Filter 1 cutoff at +0.025 normalized depth and Filter EG to Oscillator 1
wavetable morph at +0.06. Cartographer and Live additionally route Amp EG to
Oscillator 3 morph (+0.035) and a key-triggered 0.5 Hz triangle LFO 2 to
Oscillator 2 pitch (+0.003). Cartographer adds a key-triggered 0.125 Hz sine
LFO 3 to Filter 2 cutoff (-0.02). These deliberately modest native depths are
not copied concept-art numbers: they preserve the eight useful macro mappings
while providing audible movement and genuine source-to-destination connections
for the matrix and routing schematic. The runtime audio headroom and strict
reset checks remain unchanged.

Profiles 01–08 therefore declare and resolve **10 routes**, Cartographer
**13**, and Live **12**. The independent default fixture still has **8**. The
fixture checks every declared parameter/source/scene/index tuple and its
normalized depth against the engine's resolved route rows, in addition to the
exact count and active/finite-value checks. The metadata records the declared
native IDs, depths and resolved display labels.

The current patch's category is `Reference Showcases`; its author, license,
description and tags identify the generated showcase and its actual wavetable,
filter routing and modulation. These fields are assigned to the real isolated
patch and read back into `patchFixture`, so browser cards no longer inherit an
unrelated template comment. No invented preset row, catalog ID, factory folder
or patch count is injected into the installed browser library.

These are native FX slots with the engine's actual insert/send/global topology;
the test does not turn all eight into a fictional serial insert chain. Every
algorithm selection and changed parameter is recorded. Some concept controls
and algorithms have no exact engine equivalent; the fixture is not an assertion
that every illustrated setting or audio response has been reproduced.

## Capture evidence

The capture case passes the workspace ID into the fixture. `patchFixture`
records the actual patch name, skin ID, template path, every loaded wavetable's
file/display name/frame count/sample count/byte size, per-source provenance,
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
