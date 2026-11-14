# Agent 50: evidence integrity and reproducible export

This is the fiftieth actual subagent assignment, in wave 9. It audits technical
evidence and provides a repeatable exporter. It does not perform human listening
or certify composer adoption.

## Result

The current-catalog method check passed for catalog
`ddbb4781424430cc950daf6d610ec7025c743d7c435b404747c5821e65ec9369`.
Subsequent wording or preset changes require another export against their exact
catalog; this dated review is not a claim about an unmeasured later revision.

| Evidence | Independently checked result |
|---|---|
| Primary preset reports | 1,000, each with the expected 23 scenarios |
| Primary technical gates | 5,000, all passed |
| Primary native audio scenarios | 23,000 |
| Primary macro measurements | All 8,000 resolve above their recorded thresholds |
| Repeated default audio | 1,000 byte-identical pairs |
| Exact default PCM collisions | None |
| Sustained-note reports | 1,000, each with two expected scenarios |
| Sustained-note scenarios | 2,000 |
| Primary and sustained warnings | Zero after the ten final gain adjustments |
| Nonfinite samples or native/Python frame-count mismatches | Zero |
| Highest sample peak in either audit | 0.7811339497566223 |
| Native/Python peak or RMS discrepancy | Below 5.2 × 10⁻¹³ |

The primary fixtures represent approximately 28.781455 hours of rendered audio.
The sustained-note fixtures add 8.610699 hours, with held-note durations from
12.000667 to 76.191333 seconds. These are rendering durations, not listening
hours. They are bounded, deterministic fixtures and do not cover every random
state, host transport state, performance, or combination of controls.

Before the ten final gain adjustments, I also independently compared the existing
1,000-entry native report archive with every original report file and its report
index. Every archived byte, report hash, and matching preset hash agreed. That
earlier snapshot had a maximum measured peak of 0.782223105430603. Its results are
retained as historical evidence in the accompanying machine-readable review.

## Export helper

[`export_evidence.py`](../../../scripts/scoring/export_evidence.py) validates the
complete current bank before publishing evidence files. It checks exact preset,
catalog-definition, fixture, renderer, QA-helper and configuration bindings;
the sustained supplement additionally binds the expanded recipe and compiler.
It checks report identity, scenario cardinality, all five gate records, macro
measurement/status consistency, repeat fingerprints, warning counts, CSV rows,
and summary aggregates. It also cross-checks the native renderer's peak, RMS,
mean DC, frame count, format and performance settings against the recorded
Python measurements and planned fixture.

The exporter preserves original report and summary bytes. The two report ZIPs
use sorted entries, fixed ZIP timestamps and permissions, and verified CRCs and
SHA-256 payload hashes. Report indexes pair each report hash with its exact FXP
hash. `checksums.json` identifies the files in an export; `integrity-audit.json`
records independently recomputed totals, and `provenance.json` records the
renderer, source, recipe, gain-adjustment and catalog hashes. Source inputs are
captured before collection and checked again before delivery so an overlapping
generation cannot silently produce a mixed snapshot.

Optional full native semantic roundtrip and factory staging/discovery evidence
must match the same current catalog, renderer and relevant implementation
hashes. Historical correction plans can be included separately through repeated
`--extra NAME=PATH` arguments. They remain historical source evidence rather
than being relabeled as current results.

A typical complete refresh, from the repository root, is:

```bash
python scripts/scoring/export_evidence.py \
  --qa-dir ../qa-scoring-final \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --static-report ../static-final.json \
  --sustain-dir ../sustain-final \
  --roundtrip-report ../a49-roundtrip-final/summary.json \
  --staging-evidence doc/scoring-1000/reviews/w09-a49-final-native-evidence.json \
  --extra correction-plan.json=../polish-plan.json \
  --extra sustain-correction-plan.json=../sustain-headroom-plan.json \
  --output doc/scoring-1000/evidence
```

The roundtrip path must point to the latest full-catalog run. A stale run is
rejected. `--verify-only` performs the checks without writing files. The initial
method exports were written into a separate scratch directory so the release
evidence was not overwritten while the final revisions were still in progress.

## Focused rejection checks

Four isolated negative fixtures were rejected for their intended reasons:

1. Removing one preset report failed the complete-catalog cardinality check.
2. Changing one report's preset hash failed its exact binding check.
3. Altering the native peak while retaining the Python measurement failed the
   independent measurement-consistency check.
4. Changing the aggregate warning total failed summary recomputation.

No production preset, generator, renderer, QA threshold or existing measured
report was modified by these fixtures. The machine-readable results are in
[`w09-a50-evidence-integrity.json`](w09-a50-evidence-integrity.json).

## Interpretation and limits

These five primary gates are technical checks, not five listening passes.
Maximum values are unnormalized sample peaks at the saved unity master
reference; reconstructed true peaks are not measured. The sustained supplement
has only a two-second release tail, while the primary maximum-Release fixture
has a bounded observation window. Neither establishes complete release
settlement at every possible setting. A different definition or PCM hash does
not establish perceptual uniqueness. Human musical audition remains pending,
and Hollywood or other composer endorsement is not claimed.

The exporter checks already measured evidence; it does not generate new audio
or independently reproduce every raw PCM-derived metric. Raw PCM hashes and
renderer fixtures make those measurements reproducible, and full native
rerendering remains the authority when any input binding changes.
