# Wave 9 — Agent 49: final native serialization and factory staging

## Result

**Pass for catalog `8396df6694b92e644f027e90014be629e3c3f542c9d9403c69cb72a0b5b6bee1`.** All 1,000 presets passed native load/save/reload semantic comparison. All 20 selected original/saved audio pairs were byte-identical. The CMake data-staging target produced the expected 1,000 factory files with matching catalog hashes, and native discovery returned exactly those 1,000 paths. The measured renderer executable remained unchanged throughout this work.

The [compact evidence JSON](w09-a49-final-native-evidence.json) records the exact hashes, serialization-normalization counts, all 20 audio comparisons, and the source evidence locations.

| Check | Observed result |
| --- | --- |
| Native serialization | 1,000 checked; 1,000 semantic passes; zero record errors |
| Original/saved PCM comparisons | 20 pairs; 20 byte-identical results |
| Input stability during native roundtrip | Pass; 217.394 seconds elapsed |
| `stage-scoring-bank` | Exit 0; CMake 4.4.4 |
| Staged file hashes | 1,000 of 1,000 match the catalog |
| Staged categories | 10 categories with 100 presets each |
| Static active definitions | 1,000 unique definitions; zero duplicate groups |
| Native discovery | 1,000 total records, 1,000 unique expected bank paths |
| Discovery differences | Zero missing, unexpected, or duplicate bank paths |
| Renderer before/after SHA-256 | Identical |

## Native serialization scope

[check_roundtrip.py](../../../scripts/scoring/check_roundtrip.py) used the existing native renderer, with one process. Each preset was loaded, saved by the native engine, and reloaded. The comparator checks float32 parameter values, parameter types and relevant flags, modulation routes, macro state, metadata, oscillator extra state, LFO/MSEG/formula state, sequence values and timing, and compatibility/configuration data. Float32 comparisons use exact equality. The audit rejects unexpected additions or differences outside its explicitly enumerated native serialization rules.

Native saving changes representation in several documented ways. Across this bank, the accepted changes were 46,838 zero-default flags materialized or omitted, 14,000 disabled-effect parameters omitted, 2,000 unused Reverb2 parameters materialized as zero, 1,000 attached-sustain defaults materialized, and 1,000 save-tempo defaults materialized. These represent serialization conventions, with the effective sound definition preserved by the comparison. The script also checks compressed extra state and normalized sequence trigger masks.

The audio probe selected the first and last preset in each category, giving two probes per category. Every original/saved pair used fresh engine instances, seed 1000, stereo float32 output at 48 kHz, 120 BPM, velocity 96, and the midpoint of the preset's declared note range. Each render contained a four-second held note and four further seconds, with no normalization. All 384,000-frame pairs matched exactly. Audio probe hashes are retained in the evidence JSON. These 20 PCM comparisons supplement the semantic check of all 1,000 definitions; they do not establish PCM equality for every note or control position.

## Factory staging and discovery scope

Only the `stage-scoring-bank` CMake target was requested. CMake reconfigured the existing build directory, ran the source-bank verification target, copied the bank into the factory-data staging layout, and verified the staged bank. No renderer compilation or linking occurred, and the executable hash remained equal to the recorded value below.

[validate_bank.py](../../../scripts/scoring/validate_bank.py) then independently checked the staged data root. Every staged preset also received a direct SHA-256 comparison against its catalog entry. Native `--discover` enumerated the staged factory hierarchy; its complete bank path set matched the 1,000 expected catalog paths without omissions, extras, or duplicates.

This verifies the factory-data layout consumed by Surge's native patch discovery. This audit did not execute a complete macOS, Windows, or Linux platform installer, load the plugin in a DAW host, or conduct human musical audition. Unique parameter definitions do not establish perceptual uniqueness, and these checks do not establish Hollywood composer adoption or endorsement.

## Exact input and evidence hashes

| Item | SHA-256 |
| --- | --- |
| Catalog | `8396df6694b92e644f027e90014be629e3c3f542c9d9403c69cb72a0b5b6bee1` |
| Package index | `93432d2ce636edda3c0609dd7556a3bbf5afb134d9210ff505b96080c2824a78` |
| Renderer before and after | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| Roundtrip script | `d178eeaf6827082ad8d2789e5b47430a89944115fe13c5aaa796d16734fe2fb9` |
| Full native roundtrip summary | `34aa828bff03d84244d7ee46af99d23b2776ce423fe83d9bb835775681840d8e` |
| Staged static report | `ae85c0a980600a0d3ff614a17731ad592369d2170e26755337dd984ae1291689` |
| Native discovery report | `7907a3ce27f14ecfd3bc6b8e58a8023e019fbfdcf6ca0c836f54a2b4feade188` |

The renderer identifies its engine as `1.4.main.dfdd2f94a`. This is the engine label embedded when the measured executable was built; it is not asserted to be the eventual feature-publication commit.

## Reproduction commands used

Commands were executed from the repository root. Output paths are sibling audit/build directories.

```sh
python scripts/scoring/check_roundtrip.py \
  --renderer ../build-scoring/src/scoring-tools/surge-scoring-render \
  --output ../a49-roundtrip-final

/workspace/scratch/641ca55ce6de/native-toolchain/cmake/data/bin/cmake \
  --build ../build-scoring --target stage-scoring-bank

python scripts/scoring/validate_bank.py \
  --data-root ../build-scoring/scoring-bank-stage/share/surge-xt \
  --output ../a49-staged-static.json

../build-scoring/src/scoring-tools/surge-scoring-render \
  --data /workspace/scratch/9118c5c72ad8/build-scoring/scoring-bank-stage/share/surge-xt \
  --discover /workspace/scratch/9118c5c72ad8/a49-native-discovery.json
```

The full native report remains at `../a49-roundtrip-final/summary.json`; native saved presets and per-record output remain alongside it. The compact evidence is stored beside this review so its aggregate results and individual audio hashes are available without the scratch output directory. This report's conclusions are bound to the exact catalog hash above; subsequent preset edits require the affected checks to be repeated.
