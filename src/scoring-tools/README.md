# Native scoring preset renderer

This optional offline executable links the same `surge-common` engine as the
plugin. It does not add code to the plugin's audio path. Enable
`SURGE_BUILD_SCORING_TOOLS=ON` and build target `surge-scoring-render`. A headless
configuration may also set `SURGE_SKIP_JUCE_FOR_RACK=ON`,
`SURGE_BUILD_CLAP=OFF`, `SURGE_BUILD_XT=OFF`, `SURGE_BUILD_FX=OFF`, and
`SURGE_BUILD_TESTRUNNER=OFF`.

Run from the repository root, or provide an explicit `--data` directory:

```sh
surge-scoring-render --patch example.fxp --out example.f32 \
  --sample-rate 48000 --seconds 6 --hold 3 --notes 60,64,67 \
  --velocity 96 --tempo 120 --macro 0=1,2=0.5 --report example.jsonl
```

Output audio is stereo interleaved little-endian float32, with no normalization,
limiting or removal of nonfinite samples. The JSONL report includes peak, RMS,
mean, nonfinite count, frame count, engine version, patch identity, and exact
scenario arguments. `ok` establishes successful loading/rendering and finite
audio; it does **not** establish acceptable loudness or musical quality.
External analysis must apply the collection's acceptance criteria.

## Batch input

`--batch scenarios.tsv --report renders.jsonl` processes every nonblank line.
Lines beginning with `#` are comments. Columns are:

| Column | Value |
| --- | --- |
| 1 | FXP input path |
| 2 | Float32 output path |
| 3 | Sample rate, 8000–192000 Hz |
| 4 | Total rendered seconds, greater than 0 and at most 120 |
| 5 | Held seconds, from 0 through total duration |
| 6 | Comma-separated unique MIDI notes, 0–127 |
| 7 | MIDI velocity, 1–127 |
| 8 | Tempo, 10–400 BPM |
| 9 | Optional zero-based macro assignments, e.g. `0=1,1=0.5`; blank or `-` for defaults |
| 10 | Optional modulation wheel, 0–127 (default 0) |
| 11 | Optional channel pressure, 0–127 (default 0) |
| 12 | Optional expression CC11, 0–127 (default 127) |

Rows require eight to twelve fields. An invalid row produces an error report;
later rows still run. Exit status is 0 for success, 1 for row/render failures,
and 2 for command/configuration failures. Use separate processes for parallel
work. Each process reuses one engine but reloads the patch for every scenario;
the native loader stops voices, resets controls, and reloads effects. All notes
are released between scenarios.

`--seed` sets the native oscillator/control and noise random generators (default
1000; noise uses seed+1). This is reproducibility support, not a guarantee that
every third-party DSP module is deterministic on every platform. Controller
smoothing gets 128 silent preroll blocks. The host PPQ position then starts at
zero and advances each audio block. Note-off occurs at the first block boundary
at or after `hold`; its timing can differ by less than one block.

## Definition and integration checks

`--dump-parameters parameters.json [--patch example.fxp]` writes the native
parameter storage name, ID, type, control type, value, minimum, maximum and
default. When combined with a render it dumps the loaded state before applying
scenario macros. Dumping is single-patch only.

`--roundtrip-dir DIR` saves each loaded patch through the native serializer and
reloads it before rendering. It validates the saved FXP container and metadata
identity. Files use their original basenames; provide unique basenames in one
batch. The separate collection validator should compare serialized parameter
and modulation semantics, since identity alone is not complete state equality.

`--discover discovered.json` records the engine's actual factory/user patch
discovery paths and names. Point `--data` at a staged installation to audit its
content.

The renderer checks FXP magic/length, bounded XML and wavetable payload sizes,
valid XML metadata, the native loader result, engine error messages and loaded
identity. It calls `loadPatchByPath(..., false)` to restore serialized global
volume and FX bypass. The bank stores a unity (0 dB) master reference and its
protective gain in the voice path. Revision-31 preset loading normally restores
saved master volume; the legacy retention exception applies only below revision17.
User master automation and downstream gain remain outside this reference. Technical render
metrics and agent review cannot certify human audition or composer endorsement.
