# Wave 6 / Agent 31: saved gain and native headroom

## Outcome and scope

The current bank's saved gain contract is internally consistent: **all 1,000
FXPs store a 0 dB master reference**, a common **−6 dB voice VCA**, and the
documented per-preset trim. All **375 measured trims** reproduce the calibration
formula exactly to its six-decimal precision. A bounded native recheck of the
five worst pre-calibration presets passed **15 current-fixture renders**.

This review covers saved parameters, calibration arithmetic, native gain
placement, and a targeted headroom probe. It does not replace the full-bank
23-scenario run, certify perceived loudness, or establish musical suitability.
Only this review document was added to the repository. Native fixtures and
measurements were written to scratch; no preset, recipe, compiler, renderer,
or QA implementation was changed by this reviewer.

## Independent checks across all 1,000 definitions

I parsed each FXP's native XML independently using its binary XML-length field
and compared its master and scene-A VCA parameters with the catalog and
[gain records](../../../scripts/scoring/gain_trims.json). Both parameters use
native float type `2` in every file. All saved master values are exactly zero;
VCA values range from **−18.561247 to −6 dB**. Every VCA equals `−6 + trim_db`
within the serialized decimal precision, and every catalog trim agrees with
the gain record. There were no missing or unmatched preset names.

The original calibration evidence contains **2,000 successful native rows for
1,000 distinct preset names**, with no reported nonfinite samples. I independently
selected each preset's larger peak across the two supplied stress scenarios.
For every preset whose maximum exceeded 0.72, the recorded correction equals:

`trim_db = round(20 × log10(0.72 / maximum_calibration_peak), 6)`

All 375 trim values, stored peaks, and worst-fixture labels matched. The remaining
625 presets correctly receive zero additional trim. The measured cuts range
from **−0.018566 to −12.561247 dB**, with a median of **−3.975911 dB** among the
trimmed presets. The original maximum was **3.05769968033**; **251 calibration
rows, belonging to 250 presets**, reached or exceeded the 0.98 QA threshold.
These are distinct counts and should not be conflated.

| Category | Presets receiving measured cuts |
|---|---:|
| Keys | 69 |
| Plucks | 64 |
| Bells and Mallets | 96 |
| Pads | 19 |
| Ensemble Hybrids | 12 |
| Drones and Air | 18 |
| Bass | 11 |
| Leads | 18 |
| Pulses and Sequences | 30 |
| Percussion and Transitions | 38 |
| **Total** | **375** |

The historical calibration JSON preserves a catalog hash and native measurements,
but does not embed the original FXP bytes or a full original per-file hash map.
Consequently, this audit independently confirms the numerical correction against
the retained measurements, but cannot reconstruct every original pre-trim patch
from that JSON alone. The gain records' historical `input_sha256` values are
provenance assertions, not independently recovered original files. Current
behavior is verified against current FXP hashes in the new native run.

## What the native gain controls actually do

In [SurgeVoice.cpp](../../../src/common/dsp/SurgeVoice.cpp), `SetQFB` computes voice
gain from the VCA dB value, velocity sensitivity, and amplitude-envelope output.
For this bank's Wide filter configuration,
[QuadFilterChain.cpp](../../../src/common/dsp/QuadFilterChain.cpp) applies that
gain after voice filters and any waveshaper, before scene summation and effects.
The bank disables voice feedback. In
[SurgeSynthesizer.cpp](../../../src/common/SurgeSynthesizer.cpp), the global master
multiplies the signal after the global effects. Moving 3 dB of attenuation from
master to voice therefore preserves the nominal combined dry gain while reducing
the level presented to subsequent effects. Nonlinear effects mean this is not a
promise of exact waveform equivalence; re-rendering is appropriate.

The current contract changes the earlier `master −3 / common VCA −3` reference
to `master 0 / common VCA −6`. This is a clear unity-reference design choice.
It must **not** be described as repairing a demonstrated revision-31 browser
volume bug. The actual implementation in
[SurgePatch.cpp](../../../src/common/SurgePatch.cpp) removes a streamed master
volume on preset loading only when `revision < 17`. These patches use revision31.
A broad header or README description does not override that implementation.
Later host automation or user gain changes remain outside the measured fixture.

The review also found broader master-retention wording in the renderer README
and a player-guide description treating Felt Pizzicato Swell's 400 ms nominal
Attack setting as an exact entrance. The parent confirmed both wording
corrections before this report was finalized; this reviewer did not edit those
files.

## Bounded native recheck at the new unity reference

The five largest original maxima were selected mechanically, including four
Felt Drum variants because they actually occupy four of those positions. Each
current FXP was rendered through its default single note, aggressive macro chord,
and 16-note velocity-127 stress fixture. The common native engine used 48 kHz,
fresh engine instances, seed1000, actual saved parameters, and raw stereo float32.
No output normalization or post-render gain adjustment was applied. Current FXP
hashes were rechecked after rendering.

| Preset | Original maximum | Saved trim dB | Current default peak | Aggressive chord peak | Current 16-note peak |
|---|---:|---:|---:|---:|---:|
| SC10-001 Felt Drum Close | 3.057700 | −12.561247 | 0.035617 | 0.142711 | 0.719683 |
| SC10-003 Felt Drum Edge | 2.952894 | −12.258308 | 0.035645 | 0.138510 | 0.719724 |
| SC10-004 Felt Drum Drift | 2.949495 | −12.248303 | 0.036406 | 0.133677 | 0.719365 |
| SC01-006 Copper Tine Close | 2.689710 | −11.447459 | 0.041684 | 0.113354 | 0.719890 |
| SC10-002 Felt Drum Body | 2.605408 | −11.170866 | 0.037602 | 0.140283 | 0.719671 |

All 15 renders passed the existing integrity checks, including finite samples,
sample-peak headroom, DC, and the applicable default-release check. The largest
current peak, 0.719890, is about **2.85 dB below sample full scale**. The dense
16-note test and aggressive three-note control test are separate scenarios;
this probe does not establish a combined all-controls/16-note bound.

Single-note previews are deliberately quieter than the densest supported
polyphony. The five defaults above have peaks around −29 to −28 dBFS, with
active RMS varying substantially by articulation. A transient drum's low RMS
does not by itself make it defective. These are sample/RMS measurements, not
integrated perceptual loudness or true-peak certification. The user should set
mix level with the intended passage and check overlap; there is no defensible
single “Hollywood loudness” target for individual synthesizer patches.

A separate snapshot of the restarted full QA contained **53 completed current
presets, all passing with zero warnings**. That snapshot is partial, restricted
to the beginning of the catalog, and is not a bank-wide final result. The earlier
266-result run was superseded when the gain contract changed and must not count
toward the new bank's passes.

## Evidence identity and limits

The current probe's full fixtures, native JSONL, and measurement summary are in
`/workspace/scratch/9118c5c72ad8/a31-gain/`. Raw PCM was measured and then removed
to limit scratch use; its SHA-256 remains in the measurement records.

| Input or result | SHA-256 |
|---|---|
| Current catalog | `f8e0f6fdd299cbc18ad7410030f123a45777bdaf70c899ee8ee1d30680581331` |
| Gain records | `eeca9cc0c3b23115a5107f08242df27d0f9d00e40be6e791ccf77a0cb002d032` |
| Compiler | `944b6d9207bd837ffa8c62870af576d069069b58060ef8d1cf15805ddd1f8bf0` |
| Native renderer | `563b87243d493035ae1ccb6a71a18c01c261b3dd88aceef66285694be2043361` |
| QA measurement implementation | `203c1af4bf533755ee68b94f622762684a4e50f25119513ca515d76fb4392681` |
| Retained original calibration JSON | `0e94790a49da5293418d1043f533dd854f703890760826648cdcab785b7d4db5` |
| Current 15-render probe summary | `3d6f670d24398e53b663d8ea42655a37f4e922db74f0dd243f0b9627921a428c` |

Final publication must use the completed full-bank evidence for its actual
catalog hash. Any subsequent preset or fixture change requires corresponding
revalidation. This audit establishes no human listening pass, perceived
uniqueness, composer endorsement, or universal genre suitability.
