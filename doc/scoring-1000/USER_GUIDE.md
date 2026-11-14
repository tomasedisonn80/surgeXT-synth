# Scoring 1000: playing and adapting the collection

Scoring 1000 contains **1,000 native Surge XT presets in ten categories**. The
collection comprises **200 authored synthesis families with five purposeful
variants per family**. Related variants change articulation, oscillator balance,
color, motion, or space while keeping a recognizable starting instrument.

The sounds cover recurring scoring roles: a small motif beneath dialogue, a
harmonic bed, an exposed melody, rhythmic movement, low support, and a transition
that follows an edit. They are original synthetic colors. Names such as felt,
bowed, brass, choir, and wood describe an impression or musical role; these
patches do not contain recorded acoustic instruments or vocal articulations.

Composer interviews informed this organization. Mac Quayle describes different
electronic palettes for different shows, while Thomas Newman discusses small
instrumental colors and musical transitions. Those examples support a flexible,
story-specific approach; they do not certify this new bank or establish an
industry usage ranking. See the [research and primary sources](RESEARCH.md),
including [Quayle's interview](https://composer.spitfireaudio.com/en/articles/mac-quayle-discusses-how-eight-hardware-synths-beat-a-thousand-plug-ins)
and [Newman's BFI interview](https://www.bfi.org.uk/interviews/scoring-mr-banks-composers-life-hollywood).

The [native audio previews](AUDIO_PREVIEWS.md) provide one example from each
category. Current fork builds include the bank and ten reference skins inside
their binaries; no separate bank installation is needed. The
[download and installation guide](DOWNLOADS.md) also covers older installations.

## Start with a short musical phrase

1. Install a current build of this fork, then open the factory patch
   browser and locate **Scoring 1000** and its numbered category folders.
2. Choose a role first: a melodic instrument, sustained harmony, rhythmic layer,
   bass, or accent. Play within the suggested MIDI range in the patch description
   or [family catalog](CATALOG.md). The [complete catalog](manifest.json) retains
   individual variant details and file hashes. MIDI numbers avoid the different
   octave labels used by different hosts.
3. Try a short phrase at soft and firm velocities. Hear it with the other parts
   and any dialogue that it must support. Hold a note long enough to hear a slow
   entrance, then leave time to hear the release.
4. Turn **Space to zero** for a dry comparison. Return it to **40%** for the
   saved ambience. Establish the dry articulation and track level before adding
   more width, movement, or effects.
5. Adjust one macro at a time, then save your adaptation as a user patch. Include
   the cue or song name and any important tempo or register in your own name.

Stable identifiers such as `SC01-002` identify the file and catalog entry. The
native patch's musical name may be displayed without that identifier. Search for
the name when the browser does not show the filename prefix.

## Find the right category

Every category has 20 families and 100 presets. The equal allocation helps
navigation; it does not represent measured demand in professional scores.

| Category | Main role | Five variants |
|---|---|---|
| 01 Keys | Intimate keyboard motifs, electric colors, resonant harmony | Close, Felt, Glass, Float, Bite |
| 02 Plucks | Defined patterns, short melodies, synthetic string articulations | Close, Tender, Spark, Cascade, Shadow |
| 03 Bells and Mallets | Tuned accents, woody attacks, harmonic and inharmonic metal | Close, Bloom, Glass, Alloy, Shadow |
| 04 Pads | Sustained harmony, gradual changes, supporting texture | Close, Open, Bloom, Drift, Veil |
| 05 Ensemble Hybrids | Synthetic string, brass, reed, organ, and vocal-like colors | Close, Section, Swell, Muted, Motion |
| 06 Drones and Air | Pedal tones, breath-like layers, suspense, open space | Near, Veiled, Open, Drift, Ripple |
| 07 Bass | Low foundations, melodic bass, articulated weight | Tight, Round, Edge, Pluck, Pulse |
| 08 Leads | Foreground melodies, solos, character motifs | Near, Warm, Clear, Float, Phrase |
| 09 Pulses and Sequences | Held-note patterns, timbral accents, selected pitch sequences | Soft, Drive, Sparse, Triplet, Glass |
| 10 Percussion and Transitions | Synthesized hits, noise accents, pitch gestures, swells | Close, Body, Edge, Drift, Bloom |

Use the variant names as a first approximation. Close and Near usually reduce
tails and secondary-layer weight; Bloom, Float, and Swell favor more gradual
phrasing. The catalog records the particular intent of every preset. A secondary
oscillator can supply a lower octave or same-register body, so increasing its
contribution does not always mean adding a higher layer.

## Eight controls with consistent jobs

All macros are unipolar. Saved values start at zero except **Space at 40%**.
Zero usually means the authored starting sound, rather than disabling the named
property. The precise routes are inspectable in the native modulation system
and [builder](../../scripts/scoring/build_bank.py).

| Macro | What it changes | How to use it |
|---|---|---|
| **Tone** | Filter cutoff and a small amount of oscillator color. On high-pass designs it lowers the cutoff, admitting more body. | Bring a part forward or add body. Choose a darker variant when the starting sound is already too bright. |
| **Motion** | Increases existing modulation and adds controlled amplitude/pan movement. Unsynchronized movement also speeds up. | Develop a held note or increase rhythmic emphasis. Zero preserves the family's authored motion; it is not a freeze switch. |
| **Contour** | Filter-envelope depth and decay; selected bright voices also shape amplitude decay and sustain. | Change the transient's spectral shape and how its color or body develops after the note starts. |
| **Space** | Delay and reverb mix. | Use zero for dry layering or a shared host reverb. Increase gradually for distant or exposed gestures. |
| **Color** | A source-specific parameter: oscillator shape, FM amount, partial blend, feedback, or string stiffness. | Change the instrument's character. Expect different behavior in different synthesis families. |
| **Width** | Native stereo width and effect width. | Broaden a part that has space in the arrangement. Zero retains the saved width and does not guarantee mono. |
| **Attack** | Amplitude-envelope attack time. | Soften an entrance or turn an articulated note into a swell. Short notes can become quieter because they end before the attack develops. |
| **Release** | Amplitude release; low-sustain designs also gain decay, with relevant String resonance extended. | Give a phrase more trailing body. A naturally decaying source will still lose energy while a key is held. |

For stillness, edit the native motion routes. For exact mono compatibility, use
the native width/pan controls or host processing and listen to the actual mono
sum. The macros deliberately retain each family's default character.

**Velocity** affects level and filter brightness. The **modulation wheel** adds
tone and motion. **Channel pressure** introduces vibrato. If a controller lacks
aftertouch, a host MIDI control can send channel pressure; ordinary key velocity
and the wheel remain useful. These mappings do not switch sampled articulations.

## Starting points for different stories and genres

The following are audition recipes, not guarantees of genre suitability. Use
the same short musical passage when comparing alternatives, and match perceived
level before deciding that a brighter or louder sound is better.

| Context | Concrete starting presets | First adaptation |
|---|---|---|
| Dialogue drama or intimate documentary | **SC01-002 Felt Ember Felt**, **SC04-005 Amber Canopy Veil** | Try Space around 0–20%, soft velocity, and a sparse voicing. Keep the pad below the keyboard phrase and check tails through speech. |
| Romance or reflective character writing | **SC01-007 Copper Tine Felt**, **SC08-092 Moonlit Triangle Warm** | Start with a simple melody and open accompaniment. Add Release only where phrase endings have room; introduce pressure on selected sustained notes. |
| Comedy, family scenes, or light animation | **SC03-081 Warm Toybox Close**, **SC08-041 Harbor Whistle Near** | Use separated notes and contrasting registers. Keep Space modest so timing and small melodic answers stay legible. |
| Action or investigative momentum | **SC07-031 Transit Saw Tight**, **SC09-012 Copper Transit Drive** | Set tempo first, establish the bass rhythm, then add the held-note pulse. Begin with moderate Motion and leave gaps for percussion. |
| Suspense, crime, or horror | **SC06-097 Deep Listening Veiled**, **SC03-071 Quiet Anvils Close** | Contrast a low sustained bed with isolated metal attacks. Develop Motion slowly and check whether added low energy masks the rest of the cue. |
| Science fiction or abstract mystery | **SC06-076 Metal Weather Near**, **SC09-096 Prism Arcs Soft** | Try isolated roots or widely spaced notes. Explore Color gradually; the whole-tone sequence deliberately changes the harmonic implications. |
| Fantasy, wonder, or ceremonial arrivals | **SC03-076 Starlit Drops Close**, **SC05-028 Bronze Swell Swell** | Separate upper accents from lower harmony. Shape the ensemble entrance with Attack and reserve extra Space for arrivals. |
| Travel, landscape, or quiet documentary continuity | **SC09-001 Quiet Engine Soft**, **SC06-031 Distant Choir Near** | Keep a stable harmonic bed and low rhythmic intensity. The choir impression is synthetic; avoid treating it as a lyrical vocal performance. |
| Pop, dance, hip-hop, or electronic songs | **SC07-006 Anchor Sub Tight**, **SC08-048 Soft Voltage Clear** | Establish the dry groove and bass balance, then add Color or Space in sections. Leave room for the kick and vocal. |
| Jazz, folk, rock, or acoustic hybrids | **SC01-007 Copper Tine Felt**, **SC04-005 Amber Canopy Veil** | Fit articulation to the performers. Use fewer notes and restrained effects when the synth supplements an acoustic ensemble. |
| Ambient or sparse instrumental music | **SC06-065 Clockless Room Ripple**, **SC05-083 Felt Pizzicato Swell** | Hold fewer notes for longer. Compare the regular Ripple motion with freer drone variants, and let Pizzicato Swell's nominal 400 ms Attack setting soften its entrance. |

For a more energetic version of an existing cue, first change playing density,
register, or articulation, then increase Tone, Contour, or Motion. For a quieter
version, a darker family variant and fewer overlapping notes may work better
than simply reducing track volume. No single setting can serve every scene,
arrangement, playback system, or genre without judgment.

## Pulses, harmony, and tempo

Category 09 creates repeating modulation while a note is held. It is not a MIDI
arpeggiator that reads a chord and outputs separate notes. Most families animate
amplitude or filter color while retaining the played pitch. Four families add
programmed pitch intervals:

| Family and first preset | Characteristic intervals in the non-Sparse variants |
|---|---|
| **Amber Ladder — SC09-081 Soft** | Major pentatonic: root, 2, 4, 7, 9, and 12 semitones |
| **Night Thirds — SC09-086 Soft** | Minor pentatonic: root, 3, 5, 7, 10, and 12 semitones |
| **Open Fifths — SC09-091 Soft** | Root, fifth, and octave |
| **Prism Arcs — SC09-096 Soft** | Whole-tone: root, 2, 4, 6, 8, 10, and 12 semitones |

Play one root at a time in these mono families. Changing the played note
transposes the pattern. Motion changes timbral activity without scaling the
programmed pitch intervals. Their Sparse variants use root/octave accents
instead of the full characteristic interval set. In the other families, Sparse
means widely spaced timbral accents over a continuing tone, not silent rests.

Synchronized movement follows BPM. The Triplet variant applies a **3:2 speed
increase relative to its family's base pattern**. It does not guarantee one
universal triplet subdivision or a one-bar cycle: families have different base
rates. Check the actual pattern against a metronome and the picture edit.

The native measurements exercise BPM changes. They do not establish behavior
for every host's transport start, stop, seek, loop, or automation mode. Audition
those operations in the intended session, particularly before printing a cue
with an exact rhythmic entry.

## Shape transitions and connected phrases

**Dawn Sweep** rises while held; **Falling Arc** descends while held. Both use a
filter envelope routed to pitch. On release, that envelope returns toward zero,
so Dawn Sweep's pitch returns downward and Falling Arc's returns upward toward
the played note. The return may be audible during the amplitude release.

The **Attack macro controls amplitude onset, not pitch-sweep duration**. To fit
these gestures to a particular edit, adjust the native filter-envelope attack
that drives pitch, then coordinate the amplitude envelope and note-off. For a
one-direction ending, audition an earlier amplitude fade or a shorter amplitude
release. Contour can change pitch-decay behavior as well as spectral contour
when this filter envelope drives pitch. A transition name does not promise a
fixed number of bars.

Bass and lead families marked mono use retriggering Mono mode. Overlapping notes
do not guarantee a continuous-envelope legato phrase, and a long Float attack
can restart on each note. For continuous-envelope legato, select an appropriate
native single-trigger mode, adjust portamento if wanted, and save the resulting
user patch. Poly families permit chords; the catalog records the mode of each
preset. See the [Surge XT manual](https://surge-synthesizer.github.io/manual-xt/)
for the native play-mode and envelope controls.

## Balance, installation, and verification

Patches use a 16-voice limit. Long releases and repeated chords can accumulate
voices even when each new chord contains only a few notes. Set track level with
the loudest intended passage, then check dense overlapping notes and the macro
positions used in the cue. Saved voice-gain trims provide measured headroom in
specified fixtures; they cannot guarantee arbitrary edits or every downstream
signal chain. Measurements use the saved unity (0 dB) master reference; raising
master gain or automating it can exceed that condition. Conservative polyphonic
headroom also makes some single-note previews quieter. Preserve headroom when
layering several instances and set mix gain against your actual passage.

The bank uses native oscillators and effects and requires no external samples
or separately purchased wavetables. Its files live under
`resources/data/patches_factory/Scoring 1000` in this repository. Current binaries
embed the complete bank and ten reference skin packages. Full installers also
install the resource tree; plugin-only builds prepare the included content
automatically in a private cache. Keep portable data folders for the remaining
factory library. An individual `.fxp` can also be loaded through
native patch handling. Save personal edits in the user patch location so future
factory-data updates do not replace them.

The [catalog](manifest.json) records names, roles, ranges, variant intent, modes,
and file hashes. The [measurement guide](../../scripts/scoring/QA_RUNNER.md)
explains the five automated technical gates and their limits. Read the actual
published evidence for the tested revision; this guide does not declare a test
run successful. Technical checks measure loading, signal integrity, controls,
and defined performance conditions. Human musical audition remains a separate
decision, and no composer endorsement is claimed. For a final choice, audition
the preset in the cue, through its intended mix and delivery format.

## What the measurements cover

The native protocol checks the saved-default release tail against measured decay
thresholds. At maximum Release, it observes at most six seconds after note-off;
a longer tail may still be sounding. Short sequence fixtures check control and
tempo responses, but a three-second hold does not traverse every slow 16-step
pattern.

The separate [sustained-note audit](reviews/w08-a48-sustains.md) adds two longer
fixtures for every preset, with held notes lasting approximately 12–76 seconds.
Those holds cover a complete 16-step cycle for Step-shaped modulation, or multiple
phase cycles for the other motion shapes, at 120 BPM. They exercise dense held
notes and aggressive macro settings. Their two-second post-note-off observation
does not establish complete release settlement. These are defined technical
conditions, so audition the complete phrase and ending when fitting a cue.

Measured gain adjustments provide headroom under the tested conditions and lift
some very quiet defaults. They are saved synth settings; the test audio is not
normalized. Peak calibration does not match perceived loudness between sounds.
The very bright **Air Swish Edge** remains a useful listening priority when
balancing upper-frequency texture against dialogue or other instruments.
