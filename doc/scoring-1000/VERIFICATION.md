# Scoring 1000: verification record

## Release identity and current status

The bank contains **1,000 original native FXP presets**, organized as **200
authored families with five functional variants each**, in **ten categories of
100**. Its purpose is an editable synthesis palette for scoring and other music.
The [research](RESEARCH.md) explains the musical design; the
[player guide](USER_GUIDE.md) explains performance and adaptation.

**Local technical verification passed for the frozen catalog below.** All
1,000 presets passed the complete 25,000-scenario audio protocol, with zero
warnings, as well as exact static regeneration, native semantic serialization
and factory staging/discovery. The exported evidence matches this catalog.
The completed diversity review and its archive also bind this frozen catalog.
Remote native CI and platform installer completion remain pending observed
workflow results.

| Identity | Value |
|---|---|
| Frozen catalog SHA-256 | `67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2` |
| Native patch revision | 31 |
| Measurement renderer SHA-256 | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| Embedded native engine identifier | `1.4.main.dfdd2f94a` |
| Engine base commit | `dfdd2f94adec9c7bee4f3cfd4f757860fb875c3e` |
| Recovered local feature commit | `946ed442572d5fa29e6d6467813b4fa6cb150908` |
| Published feature commit and platform runs | Awaiting remote publication and observed workflow completion |
| Human musical audition | Pending |
| Composer endorsement | Not claimed |

The engine identifier reflects when the local renderer was compiled, before the
bank's feature commit. It is not presented as the later release commit. The
[renderer implementation](../../src/scoring-tools/ScoringRender.cpp) links the
actual `surge-common` synthesis engine. The executable hash, source hashes and
exact inputs bind the local measurements independently of that embedded label.

## Publication recovery checks

This publication pass recovered the existing local feature commit identified
above and the completed supplemental review files. The
[committed-tree audit](reviews/w10-a59-repository-integrity.md) identifies the
original feature tree; subsequent documentation updates do not imply that the
native measurements were rerun. Publication and the exact remote source tree
must be verified separately.

Independent checks repeated during recovery passed exact regeneration of all
1,000 presets and confirmed all 1,000 preset hashes in the standalone ZIP.
Reconciliation of the preserved QA evidence checked both 1,000-report archives,
their report and current-preset hashes, all 23,000 primary and 2,000 sustained
scenario records, and all 5,000 passing technical-gate records. It also verified
18 evidence-file checksums, 12 audio/metadata checksums, 21 source-provenance
bindings, and all 12 members of the refreshed diversity archive against its
recorded hashes and current catalog identity. These are artifact-integrity and
record-consistency checks, not fresh DSP renders or human listening passes.
The current recovery runtime has no CMake executable; its read-only source
review does not substitute for the pending native CI or platform builds.

## What every preset receives

The [primary runner](../../scripts/scoring/qa_bank.py) records five distinct
**automated technical gates for every preset**. Its
[protocol](../../scripts/scoring/QA_RUNNER.md) defines the fixtures and limits.
These are 5,000 technical gate records, rather than five human listening passes
for each sound.

| Gate | Required evidence | Boundary of the result |
|---|---|---|
| 1. Native definition and identity | Exact catalog/FXP hash, successful native loading, expected metadata, and native save/reload identity | Complete parameter semantics are checked separately by the independent native roundtrip audit. |
| 2. Default audio | Finite, non-silent raw output; sample headroom and DC checks; observed default release decay | It does not measure subjective loudness, acoustic realism or artistic quality. |
| 3. Playing range and dynamics | Both declared register endpoints, velocities 32 and 127, and a chord or single-note mono fixture | It is bounded register and performance coverage, not every note/velocity/phrase combination. |
| 4. Controls and stress | Eight macro maxima, dry Space, combined controls, wheel, pressure, tempo, sample rate, and dense polyphony or hard mono playing | A measurable control response does not establish musical usefulness in every cue. |
| 5. Evidence completeness | Complete scenario measurements, raw PCM hashes and reproducible input bindings | Different fingerprints alone do not establish perceptual uniqueness. |

The independent [static validator](../../scripts/scoring/validate_bank.py)
checks all 1,000 containers, parameter types and values, metadata, connected
macros, self-contained resources, exact paths and reproducible generation. Its
duplicate screen ignores names, global gain, effects and uniform transpose when
comparing active synthesis definitions. All 1,000 definitions are distinct under
that screen; a different definition remains a structural finding rather than a
listening judgment.

## The 23 primary scenarios

Each fixture starts a fresh seeded native engine. Audio is stereo float32,
measured without normalization, limiting, clipping repair or removal of failed
samples. The saved master is **0 dB**. Protective level adjustments live in the
patch's voice VCA and therefore apply when the preset is played normally.

| Fixture group | Scenarios per preset | Exact coverage |
|---|---:|---|
| Saved default and independent repeat | 2 | Midpoint of the declared range; velocity 96; default macro values; observed release tail |
| Register endpoints | 2 | Declared lowest and highest MIDI notes |
| Playing dynamics | 2 | Velocity 32 and 127 at the midpoint |
| Chord or mono performance | 1 | Three-note major voicing for poly patches; one note for mono patches |
| Individual macro maxima | 8 | Tone, Motion, Contour, Space, Color, Width, Attack and Release at maximum, one at a time |
| Dry ambience | 1 | Space = 0; saved Space is 0.4 |
| Combined macro stress | 1 | Macros 0–5 at maximum, Attack/Release at zero, velocity 127, chord or mono equivalent |
| MIDI performance controls | 2 | Wheel maximum and channel-pressure maximum separately |
| Tempo changes | 2 | 60 and 180 BPM |
| Secondary sample rate | 1 | 44.1 kHz; other fixtures use 48 kHz |
| Dense performance stress | 1 | Sixteen simultaneous notes within the declared range, velocity 127; one hard note for mono patches |
| **Total** | **23** | **23,000 native scenarios across the bank** |

The default holds for at least three seconds, or the nominal attack plus one
second if longer. Its tail is at least four seconds, or three times the stored
release plus two seconds if longer. Attack-max fixtures extend the hold to let
the slower onset develop. Release-max fixtures observe a bounded tail of at
most six seconds; a long tail can still be sounding afterward. Analog envelope
times are nominal parameter settings, not exact acoustic arrival times.

The acceptance limits are project criteria, not claimed industry standards.
Hard failures include a native error, missing output, any nonfinite sample,
sample peak at or above **0.98**, peak at or below **0.00001**, or absolute mean
DC at or above **0.003** in either the active segment or complete render. The
default release additionally rejects a final 500 ms RMS exceeding the larger
of 0.002 and one quarter of active RMS. Separate warnings flag limited headroom
(peak above 0.8), quiet defaults, substantial mono energy loss, incomplete
default decay, and unresolved macro response.

Macro comparisons account for repeat variation. They use a relative waveform
threshold of at least 0.001 or a spectral/envelope feature threshold of at least
0.002, raised when the measured repeat uncertainty requires it. Attack is
compared only over the common held-note window, so a longer render cannot
masquerade as a control response. These numerical distances are not
psychoacoustic audibility scores.

## Sustained notes and slow modulation cycles

The [sustain supplement](../../scripts/scoring/check_sustains.py) adds two
prolonged fixtures for every preset:

1. Saved controls with the same sixteen-note hard-velocity cluster, or its mono
   equivalent.
2. Aggressive macros 0–5 with a hard-velocity chord, or its mono equivalent.

Holds last at least 12 seconds and cover four nominal Noise-LFO cycles, all
16 Step positions, or two cycles of the other shapes, plus a native block beyond
the boundary. The faster unsynchronized rate at maximum Motion is included in
the duration calculation. Actual holds span **12.000667–76.191333 seconds**.
This closes the specific short-fixture gap for slow movement; it is still a
bounded sample of the random and modulation behavior.

These 2,000 fixtures have a two-second post-note-off tail. They test sustained
headroom, finite output, DC and mono behavior. They do **not** establish complete
release settlement. The primary default release fixture serves that separate,
bounded observation purpose.

## Measured outcomes for the frozen catalog

The following values describe the completed, current-catalog runs after the
gain and descriptive corrections. The [independent evidence audit](evidence/integrity-audit.json)
confirms every preset/fixture binding and recomputes the aggregates. Original
[primary](evidence/native-summary.json) and
[sustain](evidence/sustain-summary.json) summaries retain their measured scope.

| Measure | Primary protocol | Sustain supplement |
|---|---:|---:|
| Presets measured | 1,000 | 1,000 |
| Native scenarios | 23,000 | 2,000 |
| Failed presets | 0 | 0 |
| Protocol warnings | 0 | 0 |
| Nonfinite samples | 0 | 0 |
| Maximum sample peak | 0.7811339497566223 | 0.7811339497566223 |
| Largest absolute mean DC | 0.00026079776000784475 | 0.00010781144568979819 |
| Smallest measured mono-energy ratio | 0.20776092239067995 | 0.21419469260355545 |
| Native audio duration represented | 28.7814554204 hours | 8.6106992593 hours |

The maximum occurs at **SC07-064 Soft Pedal Pluck**, in the respective dense-note
stress fixtures, and corresponds to approximately **−2.14549 dBFS**. These are
sample peaks, not oversampled reconstructed true peaks. The combined 37.39215
hours describe generated test audio; they are not claimed listening hours.

All **8,000 macro comparisons** resolve above their recorded thresholds. All
**1,000 default repeats** are byte-identical under the measured conditions, and
there are **no exact default PCM collisions** between different presets. A
secondary review screen requiring both waveform and feature distance below
0.01 has no remaining macro candidates. Its purpose is review prioritization,
not perceptual certification.

## Native serialization and factory discovery

The [semantic roundtrip audit](../../scripts/scoring/check_roundtrip.py) saves
and reloads every FXP through the native engine, then compares parameters,
types, flags, routes, macros, steps and other serialized state at native float32
precision. It allows only documented native default materialization and
omission rules. Those rules are not a broad tolerance for altered sound state.
Separate negative fixtures demonstrated rejection of deliberately corrupted
semantic fields.

The [current full-catalog run](evidence/roundtrip-summary.json) passed **1,000
semantic comparisons** and **20 fresh original/saved audio pairs** with
byte-identical PCM. Inputs remained stable through the run, and every recorded
source hash matches the frozen catalog. Audio equality is measured for those
20 selected pairs, not claimed for 1,000 full audio pairs.

The [CMake guard](../../cmake/verify-scoring-bank.cmake) verifies exact path
sets, ten category counts, every preset hash and the catalog-bound index at
configure, build and package time. Direct CMake installation runs it again.
The data-only stage is independently hashed, and native patch discovery checks
the exact expected 1,000 unique paths. The [current staging evidence](evidence/factory-staging.json)
records **1,000 matching hashes**, **1,000 unique native discovery paths**, and
**zero missing, extra or duplicate paths** against this frozen catalog. The
renderer was unchanged before and after staging. [The independent guard review](reviews/w08-a45-build-guard.md)
records rejection of missing, changed, extra, duplicate, unsafe and stale inputs,
including uppercase FXP extensions and Windows newline conversion cases.

## Diversity and musical interpretation

The [independent diversity review](reviews/w09-a51-final-diversity.md) evaluates
all 2,000 within-family pairs and records complete comparison data. Of those,
1,941 have matching active fixtures and 59 have different adaptive holds.
Matching cross-family candidates exist for 949 presets; the remaining 51 are
listed explicitly. Comparisons with different notes or hold durations are not
quietly treated as controlled timbral comparisons.

Ten additional dry native renders examine five closely ranked pairs. Fitting
out one overall gain scalar still leaves measured waveform and early-envelope
differences. Felt Drum Close/Edge and Felt Motor Edge/Pulse remain useful
listening priorities because they are deliberately related articulations.
Coarse spectral similarity, different hashes and dry waveform differences each
answer different technical questions; none proves that a listener will value
every variant equally. The completed review and all archived comparison data
now bind the frozen `67bd4a5d…` catalog. The
[refreshed evidence record](reviews/w09-a51-final-diversity-evidence.json)
preserves the earlier `ddbb4781…` catalog as historical provenance and records
ten fresh dry renders after the description corrections, all with PCM hashes
identical to their pre-correction counterparts. The refreshed archive SHA-256
is `ae7985fdbcbd37e60731241a467ac337da5b228f8a0c947bf9cb2d13b4a632d5`.
This resolves its input-binding refresh without converting the technical
comparison into a musical-audition claim.

The [native previews](AUDIO_PREVIEWS.md) provide one excerpt per category and a
94-second overview. They use saved controls and a common playback multiplier of
4 (+12.04 dB) for all clips. That declared preview adjustment preserves relative
levels and is separate from the unnormalized QA measurements. Ten excerpts are
audition material, not approval of all 1,000 sounds.

## Corrections and retest policy

The [resolution history](reviews/RESOLUTIONS.md) preserves problems found during
review instead of rewriting earlier reports as clean passes. Meaningful changes
included native parameter-type corrections, preserving authored movement,
prompt Noise-LFO companion response, measurable Contour routes, safer voice
gain and more accurate range, sequence and articulation descriptions.

Initial 2,000-scenario calibration applied negative voice trims to 375 presets.
The complete primary run then guided 25 bounded quiet-default boosts and three
additional headroom reductions. The long-hold sweep exposed ten further
headroom warnings; fresh renders after their VCA reductions placed those ten
peaks near 0.72. The earlier maximum of 0.889678 in the long sweep is retained in
historical evidence, not hidden or normalized away. Positive trims are capped
at +18 dB; the final voice level remains part of each serialized preset.

The last 18 descriptive corrections remove unsupported faster-attack wording
from eight variants at the 4 ms attack clamp and replace upper-layer wording
with secondary-layer wording in ten variants whose second oscillator is lower.
Native sound-state comparisons establish that these edits alter metadata only.
They still change FXP hashes, so affected per-preset evidence is refreshed.

Successful cached results are accepted only with matching preset bytes,
catalog entry, exact fixture, renderer and measurement-script/configuration
hashes. The sustain cache additionally binds the expanded recipe and compiler.
There is no reuse of a failed result and no unconditional reuse based on a
preset name. [The evidence exporter](../../scripts/scoring/export_evidence.py)
recomputes aggregates, checks native/Python measurement agreement and validates
all current bindings before preserving the original reports in deterministic
archives. Its negative fixtures reject missing reports, stale hashes, altered
native measurements and false summary totals.

## Build delivery and platform status

The bank is factory data installed alongside the program. It is not embedded
as preset bytes inside a VST3, CLAP, AU or application binary. The
[packaging audit](reviews/w09-a52-installer-pipeline.md) identifies the recursive
resource routes and the package-specific exceptions.

| Distribution | Factory-bank route |
|---|---|
| Windows x64 full setup or setup with Patches selected | Factory-data component |
| Windows x64 portable install | Complete portable resource folder |
| macOS full DMG/PKG | Required resources package |
| Linux x64 or ARM64 full DEB/tarball; Linux x64 RPM | Installed resource tree |
| Linux x64 portable content | Companion portable resource tree |
| Plugin-only archives, Windows Minimal, Windows JUCE 7, Windows ARM64/ARM64EC outputs | Companion preset data is required |

**Current platform-build completion is pending observation.** An earlier
successful run is evidence that the pipeline worked before this bank; it does
not certify new installers. The release record must name the exact feature
commit, run IDs, job conclusions and inspected artifact payloads. An Actions
artifact upload alone does not establish installer success. Unsigned macOS
artifacts must retain their actual signing status.

The [standalone archive](DOWNLOADS.md) provides a separate existing-installation
route. Its [metadata](downloads/Scoring-1000.zip.metadata.json) records this
catalog, **4,485,374 bytes**, **1,000 FXPs** and **1,006 archive entries**. Its
SHA-256 is `b794179791cfad604b2761e3c109aefd88821831c46ac168cb4048a2c69d41c4`.
The [archive inspector](../../scripts/scoring/inspect_archive.py)
can check supported ZIP/tar payloads, including nested Actions archives, without
extracting or executing their entries.

## Evidence and reproduction

The [evidence directory](evidence/) contains original per-preset report ZIPs,
raw JSON/CSV summaries, indexes, static and semantic results, staging records,
correction plans, [checksums](evidence/checksums.json) and
[provenance](evidence/provenance.json). The primary and sustain archives each
contain exactly 1,000 original reports. Historical reviews retain the hashes
they actually examined; they do not automatically become current because a
later preset keeps the same name.

Build the optional native tool using its [documented configuration](../../src/scoring-tools/README.md),
then run the independent checks from the repository root:

```bash
python scripts/scoring/validate_bank.py --check-generated --output static.json
python scripts/scoring/qa_bank.py \
  --renderer build-scoring/src/scoring-tools/surge-scoring-render \
  --output qa --workers 4
python scripts/scoring/check_sustains.py \
  --renderer build-scoring/src/scoring-tools/surge-scoring-render \
  --output sustains --workers 4
python scripts/scoring/check_roundtrip.py \
  --renderer build-scoring/src/scoring-tools/surge-scoring-render \
  --output roundtrip
cmake --build build-scoring --target stage-scoring-bank
```

The [native scoring workflow](../../.github/workflows/scoring-native.yml) builds
the actual engine, validates all source/staged definitions and runs category
smoke coverage on ordinary triggers. Its manual full-bank option runs all
presets. Smoke results must be reported with their actual ten-preset scope;
they are not substituted for this local complete-bank evidence. The
[agent ledger](agent-ledger.json) records actual assignments and completion
states; planned assignments do not count as completed reviews.

## Practical limits

The measured conditions do not exhaust all pitches, velocities, seeds, overlapping
phrases, control combinations, sample rates, host transport operations or later
user edits. No reconstructed true-peak, comprehensive aliasing, real-time CPU,
all-platform DAW-host, or subjective loudness certification is inferred. A
maximum-Release tail can outlast its test window, and raising the master or
downstream gain changes the measured headroom condition.

The palette contains synthesized keyboard, ensemble, voice-like and percussive
colors rather than recorded acoustic articulations. Research supports recurring
scoring roles and editing practices; it does not establish an industry-wide
popularity ranking or prove that a Hollywood composer will select these presets.
Human musical audition remains pending. Final selection requires hearing the
actual phrase, dynamics, register and release in its intended arrangement and
mix. The eight consistent macros and documented variants support that work;
they do not make every preset appropriate for every scene.
