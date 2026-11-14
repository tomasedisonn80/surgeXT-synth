# Wave 5 / agent 30: independent audio-diversity method

## Delivered scope

Added [`scripts/scoring/analyze_diversity.py`](../../../scripts/scoring/analyze_diversity.py).
It consumes native-render measurements from `qa_bank.py`; it does not generate,
modify, normalize, or repair presets or PCM. The method detects exact default-PCM
collisions and ranks pairs for targeted comparison. It does **not** assign a
musical-quality score or certify perceptual uniqueness.

The repository contains 200 families with five related variants each. A complete
run therefore examines **2,000 within-family pairs**, together with one nearest
neighbor for each preset and cross-family neighbors that match the active playing
fixture. Related versions are expected to share an instrument identity; a close
numerical match is not automatically a defect.

## Evidence acceptance

The script requires the complete 1,000-entry catalog, ten categories of 100, and
exactly 200 families of five. Every consumed report must match the current preset
bytes, complete catalog entry, QA source, measurement configuration and fixture
definitions. All reports must share the same renderer hash and measurement
bindings. Supplying `--renderer` also verifies the actual executable's hash.

The default fixture's native note, hold, sample rate, velocity, tempo, macro and
expression fields are checked against its declared fixture. Default frame count,
stereo float32 format, finite metrics, normalized spectral energy and fingerprint
format are validated before comparison. Stored input JSON and preset hashes are
retained in the exported per-preset records. A catalog change during reading is
rejected.

Final mode requires all 1,000 reports. `--allow-partial` permits missing reports
for development, but it never permits stale hashes, incompatible protocols,
malformed metrics or mismatched played notes. Partial output says `partial`, not
`complete`. Invalid inputs and failed input QA return exit code 1; exact PCM
collisions return exit code 2 for review. The script does not silently discard
invalid input and then pronounce the remaining bank complete.

## Comparisons and confounds

The ranking heuristic is explicitly defined:

`distance = L2(spectral fractions A − B) + 0.5 × L2(normalized envelope A − B)`

It uses the ten spectral energy fractions and twelve normalized active RMS bins
already measured by the native QA pipeline. The weight is a transparent way to
combine two feature sets, **not a calibrated perceptual distance**. There is no
invented distance threshold that turns a preset into a musical pass or failure.

Every pair records the spectral and envelope components separately, played notes,
held duration, full render duration, peak, active RMS, RMS ratio, centroid and
the default-repeat feature distances. Overall gain is largely removed from the
ranking features but remains visible in these companion measurements.

There are three comparison views:

- **Within family:** all ten pairs of the five variants, with an explicit flag
  for matching note, held window, rate, velocity, tempo and controller inputs.
- **Cross family, matched active fixture:** nearest neighbors outside the family
  whose played note and other active fixture fields match. Even the same MIDI
  note does not guarantee the same oscillator register or spectral structure.
- **Bank-wide:** nearest neighbors without matching restrictions, explicitly
  labeled as potentially confounded by pitch or timing. These are discovery aids,
  not controlled comparisons.

Adaptive default hold times matter: a slower preset may receive a longer held
window. Twelve envelope bins then span a different physical duration, and the
four FFT windows sample different moments. Such pairs are retained with a
confound flag and excluded from the primary matched-pair shortlist.

Full PCM fingerprints include every sample and the complete tail. Different
fingerprints can result from duration, phase, gain or tiny numerical changes;
they cannot prove a musically meaningful difference. An identical fingerprint,
however, identifies exactly identical rendered bytes in this fixture. The output
retains exact-collision groups separately from feature rankings.

## Relating audio rankings to recipe reviews

The script performs a bounded ablation of each requested variant transform in
the **expanded recipe state**. Explanatory text and override metadata are removed
from that comparison. A transform is annotated as inactive only if omitting it
leaves the expanded sound-design fields exactly unchanged. The current recipes
produce 1,000 annotation records and 364 such inactive transform occurrences.

These annotations are narrower than a complete native DSP ablation. An effective
recipe edit can still be ignored by an oscillator model, fall outside an audible
band, or change the sound only weakly. The independent compiler review and the
earlier wave-4 compiled-parameter reviews address those distinctions. Inactive
individual operations do not imply that a complete variant is unchanged.

The closest measured pairs can consequently be inspected alongside their exact
inactive-operation annotations. This supports a focused follow-up question:
which remaining envelope, balance or spectral changes distinguish the two
versions in a useful phrase?

## Actual bounded validation

The parent restarted native QA after correcting saved master/voice gain behavior.
The archived pre-correction evidence was rejected as stale; none is counted here.
At this report's bounded development snapshot, **73 current presets** passed
input validation, covering **14 complete families** and **143 within-family
pairs**. All 143 compared active fixtures matched. Their 73 full default PCM
hashes were distinct; there were no exact collisions. This is a **partial result**
from the keyboard category, not a full-bank conclusion.

Three leading matched feature pairs in that snapshot were:

| Pair | Ranking distance | Follow-up relevance |
|---|---:|---|
| Amber Clav Close / Felt | 0.0244568994 | Close's attack transform and Felt's FM transform are inactive; compare the remaining articulation and blend, especially the first 100 ms. |
| Felt Ember Close / Felt | 0.0339862879 | No expanded-recipe transform is inactive; a close feature result can still occur between deliberately related articulations. |
| Drawbar Lantern Felt / Glass | 0.0355787055 | FM changes are inactive on the sine layers; Glass's octave assignment is already present. Compare partial balance and phrase shaping rather than treating the names as evidence. |

The rankings do not establish that these pairs sound interchangeable. Short
attacks can be diluted in a three-second, twelve-bin envelope summary. Dry,
matched-register A/B phrases and a listener's judgment are the appropriate next
steps before changing such designs solely because of a small feature distance.

Meaningful negative checks also passed:

- A final-mode input containing one genuine current report was rejected as
  incomplete, while preserving that report as valid partial evidence.
- Stale preset hashes, a changed native played note, and a nonfinite feature
  value were each rejected even with `--allow-partial`.
- Six synthetic metric invariants checked identity, pitch mismatch, hold mismatch,
  a tail-only timing difference, PCM equality detection, and the stated distance
  formula. These are explicitly synthetic method checks, not sound evidence.

## Reproduction and output

Run from the repository root after the complete native QA run:

```sh
python scripts/scoring/analyze_diversity.py \
  --qa-dir ../qa-scoring-final \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --output ../diversity-final
```

The output contains `summary.json`, `preset_features.json`, `preset_features.csv`,
`within_family_pairs.csv`, `nearest_neighbors.csv`, and
`cross_family_matched_neighbors.csv`. The summary records all accepted bindings,
missing or invalid evidence, exact collisions and deduplicated ranked shortlists.
The full pair CSV retains all comparisons, not just favorable examples.

The development snapshot is at
`/workspace/scratch/9118c5c72ad8/a30-diversity-partial/`. Its catalog SHA-256 is
`f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331`.
The audit script SHA-256 at this report is
`70ca3cf0b9e3d13009d687b85577f593033113db5ef286c3b4cf8cf9da3ef64e`.
Later recipe or audio changes require a new analysis against matching native
evidence. The parent will run the complete final analysis separately.

Human musical audition remains pending. No Hollywood use, composer endorsement,
acoustic realism, universal genre suitability, or perceptual uniqueness is claimed.
