# Assignment 57 — final native artifact and gallery provenance

> Curated historical assignment report. The completed original audit below concerns the a9 branch artifact. Assignment 60 subsequently verified the exact 453df569 main artifact and committed gallery byte for byte. The historical pending-publication statements below are preserved; current results are in the ledger and final verification record. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Candidate verification

Independently verified source `a9d0d0a838632daf178255561def4622cd208258`
from [native run 43393400045](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43393400045).
The native job and every build, behavior, capture and audio-display step passed.
The gallery job correctly skipped publication on the verification branch.

The archive identifies a clean source tree and a complete set of ten skins.
All twenty PNGs were fully decoded with Pillow and are exactly 1586 × 992.
The file names, dimensions, capture IDs, native method, manifest paths and
source/run provenance agree. SHA-256, Git blob SHA-1 and byte size were recorded
for every PNG independently of the gallery generator.

Pixel differences were independently recounted from each decoded baseline and
edited image. Every count exactly matches its capture-evidence record.

| Skin | Independently recounted changed pixels | Declared controls resolved | Maximum active peak | Post-reset peak |
| --- | ---: | ---: | ---: | ---: |
| 01-obsidian-precision | 5332 | 83 | 0.261424869 | 5.31783e-09 |
| 02-ivory-atelier | 5873 | 75 | 0.040943258 | 0 |
| 03-spectral-observatory | 4445 | 69 | 0.576480865 | 5.42984e-09 |
| 04-bauhaus-modular | 1012 | 79 | 0.559619367 | 5.31783e-09 |
| 05-space-age-1977 | 530 | 90 | 0.692262888 | 5.24803e-09 |
| 06-tokyo-industrial | 881 | 92 | 0.462193817 | 5.40688e-09 |
| 07-liquid-aurora | 832 | 99 | 0.699898899 | 3.87582e-09 |
| 08-clearsignal-accessible | 30108 | 77 | 0.030222584 | 3.39702e-09 |
| 09-signal-flow-cartographer | 17054 | 51 | 0.297005773 | 0 |
| 10-live-performance-deck | 23984 | 64 | 0.103609890 | 5.32106e-09 |

The forty baseline, edited, host-updated and restored peaks are finite,
greater than 0.0001 and below the unchanged 1.0 gate. The maximum is
0.699898898601532. The largest absolute post-reset peak is
5.429840577164669e-9, below the unchanged 1e-6 gate.

## Native test summaries and real Live MIDI input

- Behavior: **41 cases / 10,299,242 assertions passed**.
- Native capture: **24,095,770 assertions passed**.
- Normal AudioTap stress and ThreadSanitizer AudioTap stress: passed.
- All ten strict manifests and the schema self-test: passed.

The new behavior count includes the generic linear pointer alignment and rapid
owned MIDI spring ordering regressions. Their source tags include
`[reference-workspaces]`, the workflow's selected behavior suite; the passing
summary is from the exact compiled source above.

Live's capture evidence records real incoming MIDI channel 1 values:

| Input | Value |
| --- | ---: |
| Signed pitch bend | +6616 |
| MIDI 14-bit pitch wheel | 14808 |
| Modulation CC1 | 104 |
| Expression CC11 | 56 |
| Channel pressure | 85 |

The exact source sends these events through `processBlock` before the held
notes and checks all four applied native telemetry fields before capturing.
The passing capture run therefore corroborates the recorded inputs. Only Live
has this extra incoming-controller fixture. The lanes are real native controls;
the image is not edited to place the indicators.

## Main publication — pending verification

The exact candidate was promoted to main. Its distinct main native/gallery run
is [43394267074](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43394267074).
At this report stage, that run and its gallery publication are still in progress.
The published gallery must be compared with this main run's artifact, even
though the source SHA is the same as the earlier branch run.

The final comparison will verify the gallery commit, all twenty committed PNG
bytes, its source/run fields, hashes and links against that exact main artifact.
No final publication claim is made yet.

## Scope

This is artifact integrity, source/run provenance and native execution evidence.
It is not a claim of pixel identity, manual screen-reader testing, desktop/DAW
capture, or cross-platform host behavior. Source and images were read-only;
only this report and scratch audit evidence were written.

Detailed candidate evidence: `analysis-only/w10_a57_candidate_artifact_evidence.json`.
