# Wave 8 / assignment 45 — playable sources and display integrity

## Scope and result

Read-only audit of the ten showcase `.wt` assets, their generator and ordinary
loader/oscillator path, the Wave 7 display/material diff, and view/skin selection.
Read AGENTS, the current-pass README, continuation, engineering/user records,
and assignments 38–39. The reviewed working tree was based on
`facc04210a6c93e8ef846d31cce3e7c65649b3d0`, with the pending Wave 7 changes.
No source-integrity defect was found in this bounded review. Only this report
was written; no assets, implementation, tests, commit or push were changed.

## Independent asset checks

Parsed the actual bytes with Python `struct`, independently counted finite and
unique frames, calculated peaks/DC with `math.fsum`, and compared complete files
against local Git blobs. This did not regenerate assets or repeat the full
generator `--check` already reported by assignment 38 and the coordinator.

All ten headers are little-endian `vawt`, flags `0`, with exactly
`12 + frames × samples × 4` bytes. Spectral Bloom is 256 × 2048 / 2,097,164 bytes;
the other nine are 48 × 512 / 98,316 bytes each. All **745,472 float32 samples**
are finite; every source frame is distinct. Total asset size is 2,982,008 bytes.
Every table's absolute peak is `0.9200000166893005`, the expected float32
representation of the generator's 0.92 ceiling.

| Asset | Frame-peak range | Maximum absolute frame DC | Equal to Wave 5 `0708bc7` bytes |
| --- | --- | --- | --- |
| Spectral Sweep | 0.9200000 | 1.352e-9 | Yes |
| Analog Stack | 0.9200000 | 2.153e-9 | Yes |
| Spectral Bloom | 0.3325945–0.9200000 | 3.448e-10 | No |
| Bauhaus Waves | 0.6757296–0.9200000 | 1.063e-9 | No |
| Orbiting Saw | 0.9200000 | 1.359e-9 | Yes |
| JT Saw RM | 0.9200000 | 1.264e-9 | Yes |
| Aurora Waves | 0.5268980–0.9200000 | 9.268e-10 | Yes |
| Glass Sweep | 0.9200000 | 1.608e-9 | Yes |
| GlassWavetable 03 | 0.9200000 | 8.156e-10 | Yes |
| VocalFormant A | 0.9200000 | 9.341e-10 | Yes |

Thus precisely the newest two assets changed; the other eight are byte-identical
to the tested Wave 5 source. Comparing with pre-Wave-5 `4872523` independently
identifies exactly four changed recipes/assets across Waves 5 and 7: Aurora
Waves and GlassWavetable 03 in Wave 5; Spectral Bloom and Bauhaus Waves in Wave 7.

The new asset SHA-256 values match assignments 38 and 39 exactly:

- Spectral Bloom: `2a6b9be1590a279d800cf916ab61c1fe8fe0a58a356c8c5c8c14240aa59b8020`
- Bauhaus Waves: `64b604245a16c812e2abc92278b9eec2020479baa5f62b4bf4fb5d5ec3604824`

Evaluating each analytic recipe at phase 0 and phase 1 for every stored morph
position gives maximum endpoint disagreement `1.560e-14` across all tables;
Spectral is `1.568e-15`, Bauhaus is exactly zero. This is the periodic endpoint
test; the final stored sample intentionally precedes the omitted duplicate
endpoint. All recipes are finite harmonic sums/products. The changed families'
highest analytic harmonics are Spectral 880, Bauhaus 38, Aurora 8 and Horizon 7,
below their source Nyquist limits. DC subtraction and constant scaling preserve
periodicity. No image data or random generator enters these recipes.

## Actual audio and bounded display path

`XTTestReferenceWorkspaces.cpp` selects the named file under
`resources/data/wavetables/Reference Showcases/`, invokes
`SurgeStorage::load_wt`, and checks the resulting source-frame count, size and
display name. The normal loader selects `load_wt_wt`, reads the header/payload
and invokes `Wavetable::BuildWT`. `WavetableOscillator.cpp` reads those resulting
`TableF32WeakPointers` for audio. These are playable source assets; the four
recipe changes intentionally change their sound when explicitly loaded.

`Visuals.cpp::sampleWavetable` remains a bounded read-only copy: allocate the
48 × 256 owned array before a `try_to_lock`, validate the live table, interpolate
its actual frames/phase samples, copy the real morph position, then unlock
before string conversion and drawing. A busy/invalid table yields an unavailable
snapshot. There is no peak normalization, display oscillator or RNG in this
copy. The renderer accepts the snapshot as const, clips to caller bounds, and
projects signed samples. Spectral's maximum 142 interpolated rows derive only
from those 48 rows. Bauhaus draws 32 original rows, both endpoints, and the real
selected-position trace; it omits strip fills. Its caller explicitly supplies
the expanded plot and the matching real-waveform fallback. Aurora colour/haze
changes leave the same source-derived vertices intact.

The ordinary oscillator-preview fallback still enters the thread-local
`DisplayPreviewScope` before construction. Shared-RNG Twist and unsafe Alias
memory modes remain unavailable. Existing preview tests explicitly compare
parameters, the C RNG sequence, Surge RNG state and noise RNG before/after
sampling; these tests were inspected, not rerun here.

## Selection, materials and truthful state

The manifest diff changes geometry, typography, face diameter, value position
and plot/title insets. Control bindings and selection identities/defaults are
unchanged. `selectView` updates local selection/rebinding/focus and refreshes;
control synchronization uses `dontSendNotification`. Skin loading constructs
the validated workspace and adopts the current scene with `notify=false`; it
does not load a showcase patch/table. Existing lifecycle cancellation still
ends gestures and releases UI-owned held notes/momentary inputs as appropriate.
This intentional cleanup is distinct from automatically replacing synth state.

The new Bauhaus paper tile is a once-created 512 × 256 image generated by fixed
unsigned coordinate hashing. Existing paper/brushed grain uses the same local
deterministic principle. Neither uses engine RNG, time, parameters or reference
pixels. `ReferenceOrnaments.h` adds bounded, clipped vector section pictograms;
their fixed shapes identify sections rather than displaying measurements.

Reviewed Workspace additions retain real values for dial arcs/captions and
read `visuals.left/right` for Space/Live meter illumination. New meter tick
labels describe the same -60..0 dB mapping; they do not supply measured values.
Flow badges identify existing macro slot numbers, not fixed macro values.
Source search found no reference-image/PNG/JPEG loading in the reference
renderer or the ten reference `skin.xml` files. No frozen signal, measurement,
selection or preset-state injection was found in the reviewed diff.

## Provenance and limits

`git diff --check` passed. Reviewed file SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `generate_showcase_wavetables.py` | `e64c2118263106c279964bc077dc1cca974e28be12f0b6f66d282052a0ce3c5e` |
| `Visuals.cpp` | `00398fac75b37550f8f5a6cbd2f1535a1785431d405c0130b45b87eb9c690a3e` |
| `Workspace.cpp` | `d815798eeb4a313968746644445d63dcfffd4a467c3bcbff0b54ab02bbbf7c85` |
| `ReferenceOrnaments.h` | `3bf0b3ab4b495fee828e31ebf736049a6760a33c7dafdde8b4de2ae5e9339d63` |

This is source/asset evidence, not a fresh native build, native image comparison,
perceptual sound assessment, audio/headroom/reset run or ThreadSanitizer result.
Those remain the coordinator's exact-source integration gate. No network/API
blob checks or expensive spectral/projection experiments were repeated.

Minor documentation follow-up sent to the coordinator: the user guide's source
paragraph still names only Spectral/Aurora as retaining table-wide frame levels;
the current generator also includes Bauhaus in `GLOBAL_SCALE_FAMILIES`.
