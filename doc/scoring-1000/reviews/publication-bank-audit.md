# Publication recovery: independent bank audit

## Result and identity

**Passed:** the recovered Scoring 1000 bank is complete, exactly regenerates,
and matches the preserved per-preset native evidence. This review found no
bank-integrity blocker to publication.

The audited catalog SHA-256 is
`67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`.
The [machine-readable record](publication-bank-audit.json) identifies the exact
validator, report archives and indexes. The review was performed by
`/root/publication_bank_audit` during publication recovery on 14 November 2026.
It is an additional publication audit, not another assignment counted within
the original sixty-agent ledger.

The validation ran read-only against the recovered source checkout at
`/workspace/scratch/617e34e0c38e/surgeXT-synth`. The manifest, validator and
evidence hashes were then read from the isolated publication checkout at
`/workspace/scratch/2cdb4f918ba8/surgeXT-publication` when this report was written.
Only this report and its JSON companion were authored by this reviewer. No
presets, synthesis sources, build files, verification narrative or agent ledger
were changed.

## Static validation

The actual command was:

```sh
PYTHONDONTWRITEBYTECODE=1 python scripts/scoring/validate_bank.py --check-generated
```

It exited **0** and reported:

| Check | Result |
|---|---:|
| Catalog entries and native FXP files | 1,000 / 1,000 |
| Categories | 10, each with 100 presets |
| Authored families and variants | 200 families, each with 5 variants |
| Exact deterministic regeneration | Passed for all 1,000 |
| Unique active synthesis definitions | 1,000 |
| Duplicate active definitions | 0 |
| Validation errors | 0 |

The duplicate screen excludes names, overall gain, effects and uniform
transposition. The validator also checks the native container, metadata,
parameter types, safe paths, self-contained oscillator choices and connected
macro routes. These are definition and resource checks, not listening results.

## Independent archive and current-preset reconciliation

A separate inline Python command, invoked as
`PYTHONDONTWRITEBYTECODE=1 python - <<'PY'`, read both complete report ZIPs using
`zipfile.ZipFile`, parsed their JSON records, and used `hashlib.sha256` to verify
the following assertions:

1. Each archive contains exactly 1,000 unique report paths, and each index
   contains exactly the current catalog's ID set and manifest hash.
2. Every report's bytes match its index's `report_sha256`.
3. For every preset, the current FXP's byte hash equals the catalog hash,
   index `patch_sha256` and report binding `patch_sha256`.
4. Every report has `status=pass`, `native_exit_code=0` and zero warnings.
   Each primary report contains 23 scenarios; each sustain report contains two.
5. Every scenario records native success, no failures or warnings, zero
   nonfinite samples, and a finite sample peak greater than `0.00001` and less
   than `0.8`.
6. Each primary report has five passing gates and eight macro responses marked
   `measurable_change`. The first, saved-default scenario's PCM hash was
   collected for the bank-wide exact-collision count.
7. Every report preserves `musical_audition=pending` and
   `composer_endorsement=not_claimed`.

This independent assertion pass completed successfully and printed:

| Evidence | Primary protocol | Sustain supplement |
|---|---:|---:|
| Reports reconciled against current presets | 1,000 | 1,000 |
| Native scenario records inspected | 23,000 | 2,000 |
| Passing technical gates | 5,000 | Not a second five-gate protocol |
| Measurable macro-response records | 8,000 | Not separately counted |
| Distinct default PCM hashes | 1,000 | Not separately counted |
| Failures, warnings or nonfinite-sample records | 0 | 0 |
| Maximum recorded sample peak | 0.7811339497566223 | 0.7811339497566223 |

The hashes recorded in the JSON companion were read with:

```sh
sha256sum doc/scoring-1000/manifest.json \
  doc/scoring-1000/evidence/native-preset-reports.zip \
  doc/scoring-1000/evidence/sustain-preset-reports.zip \
  doc/scoring-1000/evidence/report-index.json \
  doc/scoring-1000/evidence/sustain-report-index.json \
  scripts/scoring/validate_bank.py
```

## Controls, musical scope and delivery

Every preset uses the consistent macro labels **Tone, Motion, Contour, Space,
Color, Width, Attack and Release**. The static route checks and the preserved
native response measurements address different questions: a connected route
and a measured signal difference are both useful evidence, but neither is a
musical preference judgment.

The collection is 200 authored families with five related functional variants
each. The [diversity review](w09-a51-final-diversity.md) identifies close related
articulations without treating their similarity as a hidden failure. Felt Drum
Close/Edge and Felt Motor Edge/Pulse remain explicit phrase-level listening
priorities. Distinct definitions or PCM hashes do not establish that every
variant is perceptually valuable.

The [research](../RESEARCH.md) separates documented composer practices from
design inferences and provides 16 source entries. This audit read that research;
it did not conduct a new web-source verification or establish an industry-wide
popularity ranking. The guide, README, manifest and native records consistently
keep human musical audition pending and composer endorsement unclaimed.

The bank is factory resource data distributed alongside the application and
plugins in full packages. It is not embedded as preset bytes within each plugin
binary; plugin-only packages need their companion data. Remote publication,
source-tree identity, observed CI results and platform artifacts are separate
publication checks and are outside this bank audit's acceptance decision.

## Explicit limits

- No new native DSP renders or human listening passes were performed here.
- The 25,000 scenarios are preserved native records whose hashes and results
  were reconciled; this review does not claim to have regenerated their PCM.
- Five technical gates per preset are not five human quality auditions.
- Sample peaks are not oversampled reconstructed true peaks. Headroom applies
  to the measured fixtures and saved settings, not arbitrary later edits.
- The research and measurements do not certify Hollywood adoption, acoustic
  realism, perceived uniqueness or suitability for every genre or cue.
- This report does not independently attest that a remote push, platform build
  or installer completed successfully.
