# Native preset measurements

Run from any directory with Python 3 and NumPy installed:

```sh
python scripts/scoring/qa_bank.py \
  --renderer /absolute/path/to/surge-scoring-render \
  --output /absolute/path/to/scoring-evidence --workers 4
```

The renderer must support `--fresh-engine 1`. Each scenario starts with a new
native engine, uses seed 1000, restores the serialized master level, and renders
unaltered stereo float32. A default-repeat scenario measures whether that
fixture is actually reproducible. File hashes bind results to the preset,
renderer binary, QA script, recipe catalog entry and exact scenario settings.
Changing any of these invalidates the successful-result cache. Failed presets
are retried. `--force` also reruns successful results.

Use `--ids SC01-001,SC04-001` or `--limit 2` for a clearly scoped smoke run.
The summary reports the actual tested count; a limited run is not a bank pass.
Raw PCM is deleted after measurement unless `--keep-audio` is used. Successful
working directories are removed. Failure logs and native roundtrips remain for
diagnosis. Do not edit presets or replace the renderer during a running audit.

## Coverage and interpretation

Each preset receives 23 native scenarios. The default fixture holds for at least
three seconds (longer for a slow attack), then releases for at least four seconds
or three times its stored release plus two seconds, whichever is longer. Other
fixtures exercise both documented register endpoints, velocities 32 and 127,
a three-note chord (or one note for mono patches), all eight macro maxima, dry
Space, an aggressive macro combination, maximum wheel and pressure, 60 and 180
BPM, 44.1 kHz, and sixteen-note polyphony (or mono hard velocity). The polyphony
fixture is a dense cluster within the declared range that exercises all 16
voices; it is a stress test, not a suggested musical voicing. Other macros'
zero endpoints are their saved defaults; Space's saved value is 0.4 and has a
separate zero-endpoint fixture. Attack-macro fixtures extend the held window to
allow the slower sound to develop.

Reports contain five **automated technical gates**:

1. Manifest file identity and native load/save/reload identity.
2. Default audio integrity, sample headroom, DC and release-tail decay.
3. Register, velocity and poly/mono performance integrity.
4. Controller, tempo, sample-rate and polyphony stress integrity.
5. Complete scenario measurements and audio fingerprints.

The separate static validator, semantic duplicate review and installed-package
verification remain required. Gate 1 does not assert full semantic equivalence
of every serialized parameter merely because the roundtrip loads successfully.
Gate 5 does not equate differing audio hashes with perceptual distinctness.

Hard failures include native errors, missing output, nonfinite samples, a sample
peak at or above 0.98, a peak at or below 0.00001, or excessive mean DC. The
default release fixture also rejects a substantial remaining tail. Quiet
defaults, extreme mono energy loss, limited headroom and uncertain control
responses are separately reported warnings. Sample peaks are not oversampled
true-peak estimates; the measurements do not establish perceptual loudness,
alias-free synthesis, real-time CPU performance, or all possible host behavior.

Macro-response comparisons account for measured repeat variation. A resolved
waveform or spectral/envelope change establishes measurable response, not an
expert judgment that it is musically useful. A numerical pass never changes
`musical_audition: pending` or `composer_endorsement: not_claimed`.

Outputs are `presets/<ID>.json`, a compact `summary.csv`, and `summary.json`.
The script exits nonzero if any tested preset fails a hard technical gate.
All warnings and unresolved macro responses must be reviewed before making
broader quality claims.

## Master-level reference

Final-bank measurements use the saved unity (0dB) master reference. Protective
voice gain is stored in each patch; tests do not normalize the resulting PCM.
A user's later master automation or gain increase is outside this fixture.
Revision31 patch loading normally restores the serialized master; the legacy
exception for revisions below17 is not a defect affecting this bank.
