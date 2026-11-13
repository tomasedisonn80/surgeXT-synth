# Wave 8 / assignment 47 — build compatibility and packaging

## Result and source

No concrete compilation or distribution-path defect was found in this bounded
review of the Wave 7/8 renderer, ornaments, placement regression and two audio
assertions. The combined source is published as
[`ea674878313006afa0b9d1a2b1d3495b69755627`](https://github.com/tomasedisonn80/surgeXT-synth/commit/ea674878313006afa0b9d1a2b1d3495b69755627).
The review began on `facc04210a6c93e8ef846d31cce3e7c65649b3d0` plus those
changes, includes the final `std::ceil` placement fixture and the Space global
parent-containment correction (`x=255`, `w=1323`), and makes no native pass claim
for the newly published source. Assignment 48 owns that exact-source gate.
Only this report was written; no implementation, asset, commit or push changed.

## Compatibility review

Checked the new rectangle constructors/centering, rounded fills/clipping,
component coordinate/hit-test APIs, retargeted mouse events and font horizontal
scaling against the actual dependency headers for
[`surge-7.0.12`](https://github.com/surge-synthesizer/JUCE/tree/surge-7.0.12/modules)
and pinned JUCE
[`a12f0de5a3d1e3a43cca626889ac2ee1c0cc0dbe`](https://github.com/surge-synthesizer/JUCE/tree/a12f0de5a3d1e3a43cca626889ac2ee1c0cc0dbe/modules).
The inspected overloads exist in both versions. New ornament rectangles are
explicitly typed; no repeat of the earlier brace-only template deduction fault
was found. `Font::withHorizontalScale` is available without a deprecation marker
in both checked headers. The final placement test's arrays, coordinate types,
`MouseEvent` calls and `std::ceil` usage are consistent with those interfaces.

Changed language constructs fit C++17: scoped enums, lambdas, ordinary arrays,
initializer lists, `std::clamp` and existing standard math. The project itself
explicitly selects **C++20** in `CMakeLists.txt`, and the inspected successful
platform logs confirm `CLAP_CXX_STANDARD=20`; this review does not relabel the
actual builds as C++17. Existing platform configuration and warning policy were
not changed. Local JUCE submodules are unpopulated, so API/source inspection is
not represented as local compilation or a linked test run.

Reviewed implementation SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `Workspace.cpp` | `d815798eeb4a313968746644445d63dcfffd4a467c3bcbff0b54ab02bbbf7c85` |
| `Visuals.cpp` | `00398fac75b37550f8f5a6cbd2f1535a1785431d405c0130b45b87eb9c690a3e` |
| `ReferenceOrnaments.h` | `3bf0b3ab4b495fee828e31ebf736049a6760a33c7dafdde8b4de2ae5e9339d63` |
| `XTTestReferenceWorkspaces.cpp` | `50bbabcf7a5b5014836e0c492f9af08b98b35cbfc7621d853d089b45ee5ed379` |

## Factory data and skin packages

Traced `resources/data/wavetables/Reference Showcases` and all ten skin folders
through the actual packaging scripts and the normal factory wavetable scan:

| Distribution | Data route |
| --- | --- |
| Linux DEB/RPM | Recursive factory-data copy into `/usr/share/surge-xt` |
| macOS full installer | Resources package rooted at `resources/data`; postinstall moves it to `/Library/Application Support/Surge XT` |
| Windows full installer | Recursive skin/data and optional Wavetables components under common application data; full installation selects Wavetables |
| Windows x64 / Linux x64 portable data | Recursive copy to `SurgeXTData` |
| `pluginsonly` archives | Omit factory data by design; matching data must be installed separately |
| Skin-only ZIP | Twenty XML/JSON skin files plus guide, license, adaptations and provenance; no binary, wavetable or screenshot payload |

The Windows ARM64, ARM64EC and JUCE 7 matrix variants intentionally omit the
full installer/resource portable ZIP. This and the matching-data requirement
are already explicit in the user guide. `SurgeStorage::refresh_wtlist` scans
the ordinary factory `wavetables` directory; the showcase assets are not confined
to a test-only data path.

Strict validation passed all ten manifests. The existing ClearSignal
plot/control-overlap warnings remain advisory, and Flow's control-over-routing
layout is explicitly identified. An in-memory `package.build_payload` check
validated the captured bytes and produced the expected 25-file data payload.
It correctly marked the reviewed prepublication tree dirty. `git diff --check`
passed. Independent header/size/hash checks of the two new `.wt` files match
assignment 45 exactly: Spectral Bloom `2a6b9be1…b8020`, Bauhaus Waves
`64b60424…04824`. No expensive regeneration was repeated.

## Existing CI and gallery evidence

At **2026-11-13 20:03:07 UTC**, platform
[run 43493343813](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43493343813)
was still running on **`0708bc76bb194a127064cf18b8849cf77373a7e7`**, not the new
source:

| Job | Observed result |
| --- | --- |
| Universal macOS | Success, artifact uploaded; explicitly unsigned and unnotarized |
| Linux ARM64 | Success, artifact uploaded |
| Linux x64 Docker | In progress |
| Windows x64 | In progress |
| Windows JUCE 7 | In progress |
| Windows ARM64 | In progress |
| Windows ARM64EC | In progress |

Inspected successful macOS/ARM64 logs confirm their distribution target and
actual package output. No new archive-content audit or platform/DAW runtime
test was performed. Optional unfinished builds were not awaited.

The committed gallery coherently identifies native
[run 43493343847](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43493343847)
and source `0708bc7`. All twenty native PNG SHA-256 hashes and 1586 × 992 sizes,
all ten separately labelled reference hashes, and manifest-path existence were
independently verified. Gallery and capture evidence source IDs agree. These
are valid earlier-source captures; fresh Wave 7/8 captures and pipeline results
remain pending. Build/package success, source review and native runtime/visual
acceptance remain distinct evidence.
