# Assignment 51 — audio, RNG and native-state integrity

> Curated historical assignment report. The original evidence below is for eccf282. The later a9d0d0a native runs also passed, including the real Live MIDI fixture and spring acknowledgement. New Spectral/Aurora source assets require their own audio and capture gates. Statements below about pending integration, unstaged work or handoff state are the original completion-time record. See the [ten-wave ledger](../ten-wave-ledger.md) and [verification record](../final-verification.md) for current source-qualified outcomes.
> Temporary execution paths were normalized; recovery-only details and temporary image inventories were omitted. Labels such as `analysis-only/` identify uncommitted review intermediates, not delivered screenshots. Native-artifact labels identify the exact-source captures named in each report.

## Result

No additional audio/RNG defect was found in this scoped source and evidence review. The verified checkpoint is **eccf282722f719b10a918424663244af0b5688fa**, native workflow [43391258392](https://github.com/tomasedisonn80/surgeXT-synth/actions/runs/43391258392). GitHub independently reports completed/success for that exact source. Its clean capture evidence is complete for all ten skins and twenty native PNGs.

The pending generic-linear input correction, GUI spring acknowledgement and new Live incoming-MIDI fixture are distinct working-tree changes. This report reviews their relevant source boundaries but does **not** treat the older passing run as runtime verification of those changes.

## Verified native evidence

- Behavior log: **39 cases / 10,298,881 assertions**, all passed.
- Native capture log: **one all-ten capture case / 24,095,766 assertions**, all passed.
- Normal AudioTap test: PASS for boundaries, wrap, stalls, stereo ordering, subscription lifecycle and no audio allocations.
- ThreadSanitizer AudioTap test: the same PASS, with no reported sanitizer failure in the successful job.
- Evidence source: `capture-evidence.json` in `native-reference-workspaces-eccf282722f719b10a918424663244af0b5688fa`.
- Evidence generated UTC: **2026-11-13T08:15:10.261Z**.
- Capture conditions: **48,000 Hz**, **512-sample audio blocks**, actual native JUCE component snapshots; sourceDirtyAtConfigure=false; expectedCaptures=10; capturedCount=10; complete=true.

The stress program deliberately stalls/resubscribes its display consumer. Its dropped-frame counts describe omitted **display copies**, not samples removed from the host output.

### Per-skin actual output peaks

These are maximum absolute main-output samples recorded by the native capture run, before and after its actual filter edit, host update, restoration and host reset. Values are linear full-scale amplitude.

| Skin | Baseline | Edited | Host update | Restored | Post-reset |
| --- | ---: | ---: | ---: | ---: | ---: |
| 01-obsidian-precision | 0.261424869 | 0.113724068 | 0.119866624 | 0.127924308 | 5.317832e-9 |
| 02-ivory-atelier | 0.040943258 | 0.019059643 | 0.031454571 | 0.027996022 | 0 |
| 03-spectral-observatory | 0.576480865 | 0.308395863 | 0.326737881 | 0.398536265 | 5.429841e-9 |
| 04-bauhaus-modular | 0.559619367 | 0.412809670 | 0.306386381 | 0.329579949 | 5.317832e-9 |
| 05-space-age-1977 | 0.692262888 | 0.382475466 | 0.509262979 | 0.538379788 | 5.248026e-9 |
| 06-tokyo-industrial | 0.462193817 | 0.319761902 | 0.391117334 | 0.394573092 | 5.406882e-9 |
| 07-liquid-aurora | 0.699898899 | 0.465921193 | 0.558170021 | 0.553183079 | 3.875818e-9 |
| 08-clearsignal-accessible | 0.030222584 | 0.015988145 | 0.013976811 | 0.017169848 | 3.397016e-9 |
| 09-signal-flow-cartographer | 0.297005773 | 0.149249315 | 0.278736919 | 0.260015994 | 0 |
| 10-live-performance-deck | 0.097936898 | 0.038358137 | 0.035061885 | 0.042957362 | 5.321060e-9 |

All forty active-state values are finite, greater than 0.0001, and below the existing 1.0 gate. The highest is **0.699898898601532**, Aurora baseline (approximately **−3.0993 dBFS**). The highest post-reset value is **5.429840577164669e−9**, below the unchanged **1e−6** reset gate. These measurements apply to the documented offline fixtures, not every possible patch.

Restored-state peaks need not equal baseline peaks: the synthesizer continues processing between captures, with evolving phase, envelopes and effects. Parameter restoration and host-adapter behavior have their own native assertions.

## Source boundary review

### Display tap and audio callback

`src/surge-xt/gui/reference/AudioTap.h` provides a fixed-capacity, stereo SPSC ring (default8192 frames). The producer writes only free slots and release-publishes its write index; the consumer acquire-loads it and reclaims after copying. A saturated ring drops new display frames instead of overwriting unread samples. A partial fit retains the newest part of that callback.

Only the consumer owns the read index, subscription changes and vector-returning `popall()`. Producer `push()` has no lock, allocation or host-buffer modification; its copy loop is bounded by the fixed capacity. The 32-bit index atomics are explicitly required to be always lock-free. Per-frame generations reject stale frames after hide/reopen.

`SurgeSynthProcessor.cpp::publishReferenceAudio` only copies from the active float-output path into this independent ring. It does not consume the original oscilloscope stream or alter the supplied buffers. The standalone stress test compiles this production header with warnings as errors and instruments audio-side allocation.

`Workspace.cpp` subscribes only while the workspace is visible. It discards stale history when display samples are dropped or the sample rate changes. FFT/history work and vector allocation run in the message-thread refresh path; history uses sample-clock timestamps and at most256 analysis frames/eight seconds.

### Oscillator and wavetable displays

`Visuals.cpp` renders owned preview data from actual oscillator parameters and source tables. It does not load the reference PNGs. Wavetable snapshots allocate before taking a bounded try-lock, copy real adjacent source frames, retain actual source silence and release the lock before graphics work. The ribbon/mesh projection changes affect drawing coordinates and color, not source samples.

Ordinary oscillator previews use caller-owned aligned storage, a private oscillator instance and display mode. `DisplayPreviewScope.h` is thread-local and restores the previous context on exit; it does not save/reseed a global generator. Native drift and Alias constructor randomness consult this context so previewing cannot consume the audible generator. Twist, live-memory Alias modes and live audio input explicitly report unavailable source previews instead of fabricating signals.

The native display-isolation test processes matching seeded processors with and without a workspace, checks every sample is finite, keeps maximum audio difference below1e−6, checks audible RNG is unchanged across refresh, and checks host parameters and active gestures remain unchanged. Separate table-snapshot tests verify data ownership and RNG preservation.

### Filter, LFO and effects previews

`PreviewCurves.cpp` evaluates filter response using the native filter plotter and actual algorithm, subtype, cutoff, resonance and gain. The fixed96kHz base-response scope is recorded in its caption. UI-axis clipping/remapping remains a display choice and does not write filter state.

LFO preview copies the LFO storage, step sequence and parameter array into invocation-owned state, uses the native evaluator and bounded block counts, and leaves audible LFO instances unchanged. It identifies seeded noise examples, excluded envelopes and unsupported Formula/MSEG operations.

Distortion previews use private initialized SIMD state and actual native model/drive conversion. They explicitly exclude EQ, feedback and output stages. The shared-random Fuzz table case is excluded and shown as an algorithm-family illustration instead. Its native regression checks audible RNG remains unchanged.

Material grain is an immutable coordinate-hash texture. `ReferenceOrnaments.h` contains bounded decorative engraving and labelled algorithm-family pictograms; these do not supply parameter, meter or signal data. A source search found no reference-image load path in the reference workspace renderer.

### Skin/view state

The native editor lifecycle regression covers all ten packages, full-editor/return, skin reload, sceneB retention and editor reconstruction. It compares all host parameter values, dirty state and latched-note-release flags around these view changes.

The display and appearance paths reviewed do not install the showcase patch. `configureShowcaseFixture` belongs to the test-owned processor in `XTTestReferenceWorkspaces.cpp`; choosing a skin in the application does not invoke it.

## Pending changes reviewed, not yet runtime-certified

### GUI spring acknowledgement

Assignment49 adds an optional accepted sequence output to `queueMidiController` and two atomic acknowledgements for Pitch/Touch. The current diff publishes each acknowledgement **after** the actual native controller call, using release stores; the message-thread reader uses acquire loads. Rejected input does not expose an accepted token. Saturated-queue zero releases keep the existing ordered fallback; newer applied packets also acknowledge older superseded tokens.

This adds constant work per existing GUI spring packet/fallback and no new audio-side lock, allocation, UI call, DSP algorithm or RNG access. The lane's pending owned sequence/value is UI-only. The focused rapid-release and external-MIDI tests still require the next linked native run.

### Genuine Live MIDI capture fixture

The working test file now submits actual incoming MIDI before Live's baseline note-on events: signed pitch+6616 (14-bit14808), CC1=104, CC11=56 and channel pressure85. It asserts the synthesizer's applied telemetry and records a structured `incomingMidiControllers` object. The existing finite, audible, headroom and post-reset gates are unchanged.

Those input values are **not** present in the verified eccf282 capture above. Fresh native captures must establish the updated Live peak/reset results and show its real lane positions. No old peak is substituted for that pending run.

## Files and audit scope

Reviewed:
- `src/surge-xt/gui/reference/AudioTap.h`
- `src/surge-xt/SurgeSynthProcessor.cpp`, `SurgeSynthProcessor.h`
- `src/surge-xt/gui/reference/Workspace.cpp`, `Visuals.cpp`, `WorkspaceData.cpp`, `PreviewCurves.cpp`, `ReferenceOrnaments.h`
- `src/common/dsp/oscillators/DisplayPreviewScope.h`, native Alias/drift call sites
- `src/surge-xt/xt-tests/XTTestReferenceWorkspaces.cpp`
- `scripts/reference-workspaces/audio_tap_test.cpp`, `test-audio-tap.sh`
- Repository implementation contract, continuation, engineering log and user guide.

No source, manifest, test, workflow, captured image or existing document was changed by this assignment. This report is source/evidence review, not DAW certification or a pixel-identity assessment.
