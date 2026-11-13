# Ten reference workspaces for this Surge XT fork

These are ten native, interactive editor layouts for
[`tomasedisonn80/surgeXT-synth`](https://github.com/tomasedisonn80/surgeXT-synth).
They use the actual Surge XT processor, parameters, presets and automation
adapters. They are not pictures placed over the original editor.

**A build of this modified fork is required.** Copying the `.surge-skin` folders
into an ordinary upstream Surge XT installation does not add the new C++
workspace renderer. The skin-only ZIP is a data package, not a plug-in,
standalone application or installer. Use a compatible binary built from this
fork, together with its factory data.

## Select a workspace

In the modified build, open **Settings → Skins → Reference Workspaces** and
choose one of the numbered entries. The workspace's **Skins** button opens the
native settings menu; choose its Skins submenu to change the layout.

| Entry | Package folder | Design direction |
|---|---|---|
| 01 Obsidian Precision | `reference-01-obsidian.surge-skin` | Dark graphite, orange/cyan, dense synthesis layout |
| 02 Ivory Atelier | `reference-02-ivory.surge-skin` | Cream/brass surfaces and green displays |
| 03 Spectral Observatory | `reference-03-spectral.surge-skin` | Violet/cyan with a large oscillator preview |
| 04 Bauhaus Modular | `reference-04-bauhaus.surge-skin` | Geometric red, yellow and blue sections |
| 05 Space-Age 1977 | `reference-05-space.surge-skin` | Brass hardware and olive display wells |
| 06 Tokyo Industrial | `reference-06-tokyo.surge-skin` | Graphite panels, red controls and cyan plots |
| 07 Liquid Aurora | `reference-07-aurora.surge-skin` | Violet surfaces and flowing cyan/magenta accents |
| 08 ClearSignal Accessible | `reference-08-clear.surge-skin` | Light surfaces, larger controls and teal/coral accents |
| 09 Signal-Flow Cartographer | `reference-09-flow.surge-skin` | Routing schematic with live node controls |
| 10 Live Performance Deck | `reference-10-live.surge-skin` | Central macro XY, performance controls and keyboard |

If a complete fork installer already includes these folders, there is no need
to install a second copy. For a skin-only package:

1. Extract the ZIP into a temporary folder.
2. In the modified synth, choose **Settings → Skins → Install a New Skin…**.
   This opens the actual user skin directory; it is preferable to guessing a
   platform-specific location. In developer-menu mode, use **Data Folders →
   Open User Data Folder…**, then its `Skins` directory instead.
3. Copy the ten complete `.surge-skin` directories from the archive's `skins/`
   directory into that user skin directory. Keep `skin.xml` and `workspace.json`
   together. Do not rename `workspace.json` independently.
4. Choose **Settings → Skins → Rescan Skins**, then select a numbered workspace.

Before replacing an existing same-named user skin folder, retain a backup if
you have edited it. An older user-installed copy can take precedence over newer
factory data; compare its manifest with the intended release when troubleshooting.

## Controls and navigation

| Action | Operation |
|---|---|
| Adjust a rotary, fader or compact numeric field | Drag; hold Shift for finer adjustment |
| Enter a value in real units | Focus the control and press Return, right-click it, or Shift+double-click it |
| Reset a control | Ordinary double-click |
| Choose an algorithm or mode | Open its native drop-down; choices follow the actual parameter |
| Navigate controls | Tab/Shift+Tab; focused native controls support their usual keyboard input |
| Move the macro XY pad | Pointer drag, or arrow keys; Shift+arrow uses a smaller step |
| Change editing scene | A/B scene buttons; these select actual Surge scenes, not a fabricated scene-morph engine |
| Open all synthesis features | **Full editor** |
| Return from the original editor | **Return to reference workspace** footer |
| Toggle the two editor surfaces | Ctrl+Alt+R on Windows/Linux; Command+Option+R on macOS |
| Choose another skin from the workspace | **Skins**, or Ctrl+K/Command+K while the workspace handles focus |

Hosts can reserve keyboard shortcuts. Use the visible buttons when a shortcut
does not reach the plug-in. Compact value fields use the same parameter parser
as typed entry; invalid text is rejected instead of silently converted into a
different unit. Disabled controls correspond to parameters unavailable in the
current algorithm or state.

The XY pad adjusts **Macro 1 and Macro 2**. It does not automatically create
modulation assignments: assign those macros in the native modulation editor.
Other macro controls display their real current assignments/names. A scene
change follows the synth's existing scene behavior; a mere skin change is not
intended to change the sound.

Live's **Pitch**, **Mod**, **Exp** and **Touch** lanes send native pitch bend,
modulation wheel (CC1), expression (CC11) and channel pressure. Pitch and Touch
return to zero when their interaction ends; Mod and Exp retain their current
controller values. They are MIDI inputs, separate from the host parameter
controls described above. Their sound depends on the current patch's native
pitch-bend settings and modulation assignments. The keyboard range strips show
the actual Single, Split, Dual or Channel Split allocation; they do not create
extra scenes, independent zones or patch snapshots.

Browser panels search the installed preset catalog. Use the list's keyboard
selection or double-click to load a preset; category rows narrow the list.
**Browse all presets** and the header preset button open the complete native
browser. Unsaved-patch confirmation may switch to the original editor so its
prompt remains visible. The next/previous preset buttons use the same native
dirty-patch protection. Use the return footer afterward if necessary.

**Edit routes**, **Full FX editor**, **Open scope** and similar navigation
actions deliberately expose the original editor or its native overlays. The
reference layouts are alternative working surfaces, not a replacement for
every specialized Surge editor. The original controls remain the route to
formula/MSEG editing, complete effects editing and other detailed operations.

## What the displays mean

- Oscillator traces are generated by the actual oscillator algorithms in
  private display state. They are previews at a display pitch, not a recording
  of every currently sounding voice. A terrain view stacks the current
  waveform; it is not a claim to sweep every wavetable frame.
- Filter curves are base-parameter responses from independent instances of the
  selected filter/subtype. They do not show every voice's live modulation.
- LFO thumbnails use the native LFO model. Their captions distinguish normalized
  shape views, envelope views and unsupported previews. Formula/MSEG operations
  remain available in their full editors.
- Output waveform/spectrum views use a separate tap of the actual main output.
  An inactive audio stream is not replaced by invented activity. The analyzer
  is a UI display, not a calibrated laboratory measurement.
- Modulation rows come from real patch assignments. A blank Init matrix is
  expected. The routing graph is a schematic of the available routing modes,
  not an arbitrary audio-cable engine.

## Compatibility and reference fidelity

The layouts are based on ten supplied 1586 × 992 concept images. That is their
logical design canvas; the editor scales it while retaining its aspect ratio.
OS fonts, host framing, display scaling, current patch values and native menus
can affect the rendered result. The ClearSignal name describes a design
direction, not an accessibility certification.

The concept artwork includes capabilities that are not present as those exact
controls in Surge XT, including a dedicated sample player, arbitrary cable
routing and some scene-morph/performance operations. Those are not implemented
by adding a skin. Every `workspace.json` includes an `adaptations` list, also
collected in the package's `ADAPTATIONS.md`. Read those lists before treating an
artwork label as a functional promise. No pixel-identical or production-ready
claim follows merely from the presence of these files or a passing static
validator.

The plug-in retains the fork's existing plug-in/parameter identity. Test it in
an isolated host setup or with backed-up sessions before replacing a binary
used in important projects. Do not confuse unsigned fork macOS artifacts with
signed/notarized upstream releases; use the build's own signing notice.

## Build the modified synth

Install the platform build dependencies described in the repository
`README.md` and `doc/Developer Guide.md`. Clone this fork, not the upstream
repository, and initialize its pinned dependencies:

```sh
git clone https://github.com/tomasedisonn80/surgeXT-synth.git
cd surgeXT-synth
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF \
  -DSURGE_BUILD_FX=OFF -DSURGE_BUILD_CLAP=OFF -DSURGE_COPY_AFTER_BUILD=OFF
cmake --build build --config Release \
  --target surge-xt_Standalone surge-xt_VST3 --parallel 2
```

These commands do not install or overwrite an existing plug-in. Use the
repository's distribution/install workflow to install both the binary and
factory data. For a development launch, `SURGE_DATA_HOME` can point to this
checkout's `resources/data`. The native screenshot test job disables audio
backends and is **not** itself a musician-facing application installer.

## Validation and real captures

From the repository root, the dependency-free package checks are:

```sh
python3 scripts/reference-workspaces/validate.py --strict-overlap
python3 scripts/reference-workspaces/validate.py --self-test
python3 scripts/reference-workspaces/package.py --self-test
```

Geometry/schema checks do not execute the synth. For native coverage, configure
with `SURGE_BUILD_TESTRUNNER=ON`, build `surge-xt-tests`, and run the
`[reference-workspaces]` test group. On a Linux test machine with the build
dependencies, Xvfb, Openbox and `xprop` (`x11-utils`) installed:

```sh
cmake --build build --target surge-xt-tests --parallel 2
SURGE_DATA_HOME="$PWD/resources/data" \
SURGE_REFERENCE_SCREENSHOTS="$PWD/build/reference-captures" \
bash scripts/reference-workspaces/run-native-ui.sh \
  build/src/surge-xt/xt-tests/surge-xt-tests '[reference-workspaces]'
```

The capture harness writes numbered workspace PNGs, edited-control PNGs and
`capture-evidence.json`. These are native JUCE component snapshots attached to
the actual processor, not OS-desktop screenshots and not renders of the supplied
reference pictures. Read the evidence file for the source revision, test
fixture, dimensions and actual results. A workflow may upload diagnostics even
when tests fail; an artifact's existence alone is not a passing test result.

## Create a reproducible skin-only ZIP

```sh
python3 scripts/reference-workspaces/package.py \
  --output build/surge-xt-reference-workspaces.zip
```

The packager validates all ten packages before writing, snapshots and
revalidates the bytes it will archive, and includes only their two data files,
this guide, the project license, adaptation notes and SHA-256 provenance. It
does not bundle binaries, supplied reference images or unverified screenshots.
Archive ordering, file modes and timestamps are fixed; identical captured
inputs/provenance produce identical ZIP bytes. Existing outputs are preserved
unless `--force` is explicitly supplied.

## Troubleshooting

- **The regular editor appears instead of the chosen layout:** verify that the
  binary is built from this fork and that both skin files are present. Invalid
  or unconfined manifests intentionally leave the original editor available.
- **The numbered category is missing:** use the actual user skin directory,
  keep each `.surge-skin` folder intact, and run Rescan Skins.
- **Plots or FX controls are empty:** check the patch and active algorithm,
  play MIDI through a running audio engine, and open the full editor to inspect
  the underlying state. Off/unused slots do not generate pretend content.
- **A feature in the artwork has no equivalent control:** consult the skin's
  adaptation notes and the original editor; do not assume it exists in the DSP.
- **The layout is too small:** enlarge the host window where supported. The
  workspace remembers its dimensions within an editor instance; it does not
  promise to restore those dimensions after the editor is destroyed.

For reproducible bug reports, include the fork commit, OS/host, selected numbered
skin, patch or minimal reproduction, expected/observed behavior and a real
screenshot. Do not include credentials or unrelated session files.
