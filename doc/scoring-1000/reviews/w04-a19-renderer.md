# Wave 4, agent 19: native renderer audit

## Scope and result

Independent read-only audit of `src/scoring-tools/ScoringRender.cpp`, with scratch executions of the native `surge-scoring-render` binary. Focus: scenario isolation, RNG and controller initialization, patch loading, roundtrip claims, tempo advancement, output errors, and false-success reports. No listening or artistic-quality evaluation was performed.

One concrete false-success defect was reproduced and then corrected by the root agent during the review. The rebuilt executable rejects the reproduction with a valid failure record. No remaining defect demonstrated by this audit invalidates the bank's current multi-second, fresh-engine fixtures. This is a scoped result, not exhaustive proof for all presets or engine modes.

Final inspected source SHA-256: `d6e7d6ed947ce6af7a2c5f476c0cedb3e52e2d92b05195d2507b6855c024366c`.

Verified executable SHA-256: `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361`.

## Confirmed defect, now corrected: a positive duration could render zero frames

Before the correction, scenario validation accepted any positive duration. At 48 kHz, `--seconds 0.0000001 --hold 0` rounded to zero frames. The command exited successfully, created empty PCM, reported `ok:true`, and serialized RMS and mean as `-nan`, which is invalid JSON. Thus a caller relying on the native success flag could falsely accept a render that never processed audio.

Reproduction uses the existing native bank patch:

```sh
/workspace/scratch/9118c5c72ad8/build-scoring/src/scoring-tools/surge-scoring-render \
  --patch 'resources/data/patches_factory/Scoring 1000/01 Keys/SC01-001 Felt Ember Close.fxp' \
  --out /workspace/scratch/9118c5c72ad8/a19-renderer-audit/zero-current.f32 \
  --seconds 0.0000001 --hold 0 --fresh-engine 1 \
  --report /workspace/scratch/9118c5c72ad8/a19-renderer-audit/zero-current.jsonl
```

The root agent added the minimum rounded-frame check in `checkScenario()`. Repeating the command against the rebuilt executable returned exit code 1 and valid JSON with `ok:false` and `Scenario duration must produce at least one audio frame`. Current bank fixtures already use durations far above this boundary, so the original defect did not affect those fixtures.

## Reproductions and inspection supporting the current workflow

- Two identical three-note scenarios, each using a fresh engine and the same seed, produced bit-identical PCM in this sample test. The engine initializes storage RNG, noise RNG, and the upstream C RNG; the fresh instance prevents prior scenarios' effect/LFO history from carrying forward. One repeated sample demonstrates the fixture's isolation behavior, not determinism for every possible patch.
- A two-row batch with an invalid zero sample rate followed by a valid 48 kHz scenario emitted `ok:false` for the invalid row and successfully rendered the following row. Scenario exceptions are represented in the report and do not silently discard later rows.
- Patch loading restores the serialized gain. The renderer applies sustain, modulation wheel, expression, pressure, and pitch-bend settings after loading; specified macro values follow patch loading. Fresh-engine mode also resets engine state that these explicit controller calls do not address.
- Tempo is assigned and PPQ advances during rendering. Controller smoothing receives a fixed preroll. Audio and report stream errors are checked, and nonfinite audio makes the native result fail.

## Boundaries to preserve in the QA claims

1. Native roundtrip currently verifies container/name validity, saves, and reloads successfully. It does not compare every parameter and modulation route before and after serialization. Describe this as native save/reload validation; do not call it proof of complete semantic preservation.
2. The native `ok` field expresses successful finite rendering. Clipping limits, level balance, per-channel DC, macro audibility, and other quality gates belong to the Python QA layer. Native success alone is insufficient for sound-quality acceptance.
3. The fixture does not explicitly set host transport `isPlaying` true. Advancing tempo/PPQ is suitable for the bank's key-triggered rhythmic fixtures, but it does not establish host-transport start/stop behavior for arbitrary transport-dependent patches.
4. Fresh-engine construction applies the requested sample rate before `render()` performs validation. The tested invalid rate was subsequently rejected without preventing the next row, so no false pass was demonstrated. Validating before engine construction would avoid passing untrusted out-of-range settings into the engine at all.

Scratch evidence was saved under `/workspace/scratch/9118c5c72ad8/a19-renderer-audit/`, including the original zero-frame result, invalid/valid batch records, repeated PCM, and the corrected zero-frame failure. No claim of professional composer endorsement or human audition follows from this audit.
