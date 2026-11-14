# Embedded-content integration evidence

This continuation preserves the frozen 1,000-preset catalog and its complete
25,000-scenario measurements. It adds verification of the compiled payload,
ordinary startup, factory discovery, and distribution rules.

| Check | Recorded outcome | Evidence |
|---|---|---|
| Compiled CMakeRC resource versus source | All 1,020 files reproduced byte for byte | [Native content tests](native-content-tests.log) |
| Installed reuse and cache recovery | Matching install reused without a cache; partial install preserved; missing/damaged cache files repaired | [Native content tests](native-content-tests.log) |
| Malformed inputs and cache links | Rejected with existing data preserved | [Native content tests](native-content-tests.log) |
| Independent-process race | Four processes completed successfully; same cache root; all 1,020 hashes matched | [Process probe](process-concurrency-probe.json) |
| Ordinary implicit engine startup | Discovered 1,000 factory presets and ten complete reference skins without a data-root override | [Startup probe](implicit-discovery.log) |
| New native audio rendering | Ten presets, one per category; 230 scenarios; 50 technical gates; zero errors/warnings | [Fresh summary](fresh-audio-summary.json) · [Original fresh reports](fresh-audio-reports.zip) |
| Audio comparison with retained full-bank evidence | All 230 corresponding PCM hashes match exactly | [Fresh summary](fresh-audio-summary.json) |
| Cross-platform packaging commands | Nine platform/configuration branches evaluated successfully | [Distribution contract](distribution-contract.json) |
| Windows installation selections | All 1,020 required files selected once in Full/Compact/Custom/Minimal | [Distribution contract](distribution-contract.json) |
| Payload source binding | 1,000 presets, ten skins, 190,454 compressed bytes | [Source check](source-payload-check.json) |
| Compiled CLAP, native skin discovery and manifest/schema tests | Five test cases; 5,911 assertions; zero failures | [Native UI summary](ui-native-summary.json) · [Executed log](ui-native-tests.log) |

The audio regression includes **SC07-064 Soft Pedal Pluck**, the highest-peak
preset in the preserved whole-bank tests. These ten fresh reports belong to the
newly compiled renderer identified by SHA-256 in the summary. They do not
replace the original 1,000-preset evidence or imply that every preset was
rerendered during this integration pass.

The private-cache regression suite also verifies case-insensitive suppression
of a stale installed bank while preserving unrelated factory and user presets.
Its malformed-input fixtures cover truncated/trailing compressed data, invalid
container versions and lengths, path traversal, and invalid cache identifiers.
Temporary concurrency fixtures were removed after checking; the ordinary-startup
probe uses the normal per-user cache path printed in its log.

The compiled UI test executable checks the real CLAP location callbacks and
native `SkinDB`, including all ten parsed skin names, stale installed copies,
unrelated factory skins and user skins. It also checks the ten workspace
manifests and shared malformed-JSON fixtures. An initial run exposed a fixture
construction bug: JUCE array equality treated numeric strings as equal to
numbers and skipped a requested replacement. The fixture mutator now removes
the property before replacing it, preserving its exact JSON types. The native
validator and rejection expectations are unchanged. The
[initial failure record](ui-native-initial-failure.json) and
[original log](ui-native-tests-initial.log) are retained alongside the passing
rerun. These local discovery/schema tests do not capture or render an editor.

The packaging checker evaluates actual CMake control flow and Inno Setup source
selection. It does not run a Windows or macOS installer. Native frontend tests,
hosted platform builds, signing status, and artifacts must retain their separate
reported scopes in the [main verification record](../VERIFICATION.md).

The source and bank are unchanged by these checks. Human musical audition,
professional adoption, acoustic realism, all-host compatibility, and arbitrary
control/phrase combinations are not certified by these technical results.
