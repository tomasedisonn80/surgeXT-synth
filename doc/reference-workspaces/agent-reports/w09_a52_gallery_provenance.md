# Assignment 52 — independent native gallery and provenance audit

> Curated historical assignment report. This is a historical independent dry run, not the final artifact comparison. The real a9 main gallery was later published in 575d63d. Assignment 60 subsequently checked the final 453df569 gallery bytes against the exact successful main-run artifact; the final verification record contains those results. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Outcome and scope

The gallery generator and publication workflow passed this independent review and
a dry run against the real native artifact from source
`eccf282722f719b10a918424663244af0b5688fa`, workflow run
[43391258392](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43391258392).
GitHub independently reports that run as completed/success on
`work/reference-wave6`. This is a historical verified source checkpoint; the
coordinator's later source edits and final main/gallery publication are outside
this result.

No concrete source defect was found. No repository source, workflow, manifest,
test, reference image or committed gallery was edited. The only generated
deliverables are the isolated dry-run gallery and this report.

## Files reviewed

- `scripts/reference-workspaces/publish_native_gallery.py`
- `.github/workflows/reference-workspaces.yml`
- `AGENTS.md` and the relevant user-guide gallery instructions
- The actual native capture bundle's evidence, provenance, logs and twenty PNGs
- The official [download-artifact v8 action definition](https://github.com/actions/download-artifact/blob/v8/action.yml),
  confirming current-run/current-repository artifact selection when no cross-run
  token is supplied and support for `digest-mismatch: error`

## Dry-run evidence

The generator was run with source `eccf282722f719b10a918424663244af0b5688fa`,
run ID `43391258392`, and the actual downloaded artifact directory.

| Independent check | Result |
| --- | --- |
| Generated managed files | 31 |
| File size and SHA-256 records independently rechecked | 30 of 30 |
| Native PNGs fully decoded with Pillow | 20 of 20 |
| Native dimensions | All 1586 × 992 |
| Native bytes compared with original artifact | All 20 identical |
| Source reference SHA-256 hashes independently checked | 10 of 10 |
| HTML image elements | 30, with explicit reference/native/edited labels |
| HTML links and anchors checked | 78 |
| Markdown links checked | 67 |
| README inline native baselines | 10 |
| Baseline-to-edited pixel differences independently recounted | All 10 match the evidence exactly |
| Capture evidence | Complete, ten unique skins, clean source, matching full SHA/run |
| Behavior log | 39 cases / 10,298,881 assertions |
| Capture log | 1 capture case / 24,095,766 assertions |
| Audio-display logs | Normal and ThreadSanitizer pass markers present |

The thirty manifest entries cover the twenty PNGs, eight original evidence/log
files, HTML and README. The gallery manifest itself is the thirty-first file and
does not claim to contain its own hash.

Relative reference and guide links were resolved against the intended committed
location, `doc/reference-workspaces/native-gallery`, rather than the temporary
dry-run directory. Every local link exists under that production mapping.
Reference concepts, compiled baselines and edited native states remain visibly
and semantically distinct.

### Independent changed-pixel counts

These counts compare each decoded RGBA baseline with its edited native image;
they validate evidence consistency, not reference fidelity.

| Skin | Changed pixels |
| --- | ---: |
| Obsidian Precision | 5,332 |
| Ivory Atelier | 5,873 |
| Spectral Observatory | 4,445 |
| Bauhaus Modular | 1,012 |
| Space-Age 1977 | 530 |
| Tokyo Industrial | 881 |
| Liquid Aurora | 874 |
| ClearSignal Accessible | 30,098 |
| Signal-Flow Cartographer | 16,958 |
| Live Performance Deck | 25,501 |

## Publication review

The native build/test job retains `contents: read`. The dedicated publisher alone
requests `contents: write`, depends on the native job, and is restricted to this
repository's `main` ref. It selects the exact SHA-named artifact from the same
workflow run with digest mismatch treated as an error.

The publisher checks main before downloading/generating and again before
committing. It stages only `doc/reference-workspaces/native-gallery`, handles an
unchanged gallery without creating a commit, and uses ordinary
`git push origin HEAD:main`. There is no force push. If a competing push advances
main, the generated obsolete commit is not forced onto it; an otherwise
unexplained push rejection still fails the job.

The gallery directory does not match any native-workflow push path, so the
gallery-only commit does not create a native workflow loop. The HTML and README
link both the capture source commit and its native run. Neither document claims
pixel identity or DAW certification.

## Limits and integration

This audit deliberately did not repeat assignment 36's 31 malformed-input cases
or temporary Git push simulations; no new risk required that repetition. It
instead independently decoded actual output, rechecked all hashes and links,
recounted pixel differences, confirmed the live GitHub run result and reviewed
the publication logic.

The output under the isolated audit directory is a dry run. No main/gallery
commit or successful final publication is claimed. The final source must complete
its own native gate, and its resulting gallery must be checked against that
source/run rather than relabelling these earlier images.

Detailed machine-readable results are in
`analysis-only/a52-gallery/audit-validation.json`. That validation file was
written after verifying the generator's 31-file output and is not part of the
generated gallery manifest.

No staging, commit or push was performed.
