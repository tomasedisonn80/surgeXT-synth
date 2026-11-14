# Wave 5 / Agent 26 — native preset save/reload audit

## Result

**All 1,000 presets passed independent serialized-state comparison after loading,
saving, and reloading through the actual native Surge engine. All 20 selected
original-versus-saved audio pairs were byte-identical.** The run completed in
222.987 seconds with stable input files throughout.

This report applies to the exact catalog and executable below. Later preset or
compiler changes require a new audit of the affected files; a changed catalog
must not be presented as having this exact catalog hash.

| Evidence | Value |
| --- | --- |
| Scope | 1,000 presets, all ten categories |
| Native semantic comparisons | 1,000 pass; 0 fail |
| Matched native audio pairs | 20 pass; 0 fail |
| Audio probe format | 48 kHz, stereo float32, 8 seconds, 4-second held note |
| Audio probe performance | Velocity 96, tempo 120 BPM, seed 1000, fresh engine for each render |
| Catalog SHA-256 | `a5408903059ac1d15540fb5700b2d8ce40a42ebe6344a37ab4edf46dbab1b1db` |
| Renderer SHA-256 | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| Audit script SHA-256 | `d178eeaf6827082ad8d2789e5b47430a89944115fe13c5aaa796d16734fe2fb9` |
| Native renderer source SHA-256 at report finalization | `d6e7d6ed947ce6af7a2c5f476c0cedb3e52e2d92b05195d2507b6855c024366c` |

The native executable reports engine base Git identity `dfdd2f94a`. This is the
repository base, not a claim that the new renderer addition was already committed.
The binary hash binds the tested executable; the source hash above records the
renderer source at report finalization. The machine-readable audit additionally
binds its own script, catalog, and each preset file.

The complete machine-readable report is in the task workspace at
`../a26-roundtrip-final/summary.json`. Its 1,000 records retain original and native
saved-file SHA-256 values, per-preset status, native engine identity, and expected
serializer expansions. The report also retains both audio hashes for each of the
20 comparisons. The saved native files and native JSONL reports remain in that
directory; temporary audio was removed after comparison.

## What was checked

The new `scripts/scoring/check_roundtrip.py` is independent of the recipe compiler
and the existing structural validator. It parses the FXP headers and XML itself,
invokes the native renderer using `--roundtrip-dir` and `--fresh-engine 1`, and
compares original definitions against files written by Surge's serializer.

For every preset, the comparison checks:

- Every source parameter's value at the engine's actual float32 precision, its
  storage type, and its meaningful flags, including filter activation, tempo
  synchronization, range extensions, oscillator deformation, and portamento.
- All modulation routes, including source, target, depth, source index, scene,
  and mute status. No route is ignored because it happens to be inaudible during
  a particular test note.
- All eight macro labels, saved values, and polarity flags.
- Step values, loop boundaries, shuffle, and effective trigger masks; zero steps
  omitted by the native serializer are reconstructed as zero.
- Extra oscillator state, LFO extra-amplitude data, nonparameter voice behavior,
  compatibility state, metadata, license text, comments, and tags.
- The absence of wavetable payload dependencies. The optional native arbitrary
  payload is decompressed and must be exactly the empty 16-slot FX data map; the
  script does not accept arbitrary unexamined binary content.

The native pass uses a short 10 ms render only to exercise successful native
load/save/reload. That short render is **not** a sound-quality probe. The separate
8-second original/saved pairs exercise actual output equality with independent
fresh engines and identical seeding.

The selected pairs are the first and last preset in every category:
`SC01-001`, `SC01-100`, through `SC10-001`, `SC10-100`. Together they cover all six
oscillator types used in this bank, mono and polyphonic voices, six LFO shapes,
and all ten categories. This is a sampled acoustic preservation test, alongside
complete serialized-state comparison; it is not an assertion that all 1,000
presets were separately auditioned by a human.

## Expected native serialization changes

These were accepted because their behavior is directly defined in
`src/common/SurgePatch.cpp`, `src/common/Parameter.cpp`, and the Reverb2 effect
parameter definitions. Other parameter differences fail the audit.

| Native change | Count across the bank | Why it preserves the state |
| --- | ---: | --- |
| Materialized or omitted zero-valued optional flags | 46,838 | Loader defaults and explicit zero encode the same flag value. |
| Omitted parameters belonging to disabled FX slots | 14,000 | `save_xml` skips FX slots whose effect type is off. |
| Added unused Reverb2 slots 10 and 11 | 2,000 | The generic serializer writes twelve FX parameters; Reverb2 uses ten, and both unused values are zero. |
| Added attached-sustain-pedal default | 1,000 | `detachSustainPedalFromVoicing=0` preserves the original default. |
| Added default stored tempo | 1,000 | Revision 31 loading defaults missing `tempoOnSave` to 120 BPM; the renderer also saves at 120 BPM. |

These are narrow schema interpretations. The audit does not ignore changed
values, dropped routes, arbitrary clamping, unexpected parameter insertion,
changed macro defaults, or changed step patterns.

## Corrections found before the final run

The first exploratory native serialization revealed two builder details, which
were reported to the root agent and corrected before this final catalog was
frozen:

1. Delay parameter `fx7_p9` was written as float storage even though the native
   parameter is integral. The builder now writes native integer type `0`.
2. Delay low/high filter values were present without explicit activation flags.
   Native loading defaulted both filters to bypassed. The builder now writes
   `deactivated=0` for `fx7_p4` and `fx7_p5`, making their intended filter settings
   effective.

The final 1,000 comparisons confirm the corrected types and activation flags
survive native save/reload.

## Deliberate failure probes

Seven independently modified copies were rejected by the comparator:

| Deliberate corruption | Observed rejection |
| --- | --- |
| Change saved voice gain | Parameter value changed |
| Change an oscillator parameter's storage type | Parameter type changed |
| Delete a filter modulation route | Modulation routing changed |
| Change the Tone macro label | Custom-controller state changed |
| Bypass the saved delay low filter | `deactivated` changed from 0 to 1 |
| Insert an unexpected parameter | Unexpected native parameter added |
| Change a step-sequence value | Step/loop/shuffle/trigger state changed |

The development pilot also tested three patches from Keys, Pulses and Sequences,
and Percussion and Transitions; all three passed semantic comparison and matched
audio equality before the complete bank run. These preliminary passes are not
added to the 1,000-preset final coverage count.

## Reproducing the audit

From the repository root, with the native renderer built:

```sh
python scripts/scoring/check_roundtrip.py \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --output ../scoring-roundtrip-check \
  --audio-per-category 2
```

Python's standard library is sufficient apart from decompression of native saved
arbitrary data: the script uses either the Python `zstandard` module or the
system `libzstd` library. The completed run used the available system library.
Only one native rendering process is launched by the script.

For a bounded investigation, `--ids SC01-001,SC09-001` or `--limit N` marks the
result as **partial**. `--audio-per-category 0` performs serialization comparison
without sampled audio, and `--keep-audio` retains the float32 probe files. A
nonzero exit code indicates failure; the final JSON explicitly reports input
hash stability and the actual coverage.

## Limits

This establishes native state preservation, correct storage behavior, and
sampled acoustic equivalence. It does not establish artistic preference,
perceptual uniqueness, a host's automation behavior, or acceptance by a Hollywood
composer. Human audition and composer endorsement were not performed or claimed.
The root bank QA suite separately evaluates sound generation, note ranges,
headroom, controller response, release behavior, and other performance scenarios.
