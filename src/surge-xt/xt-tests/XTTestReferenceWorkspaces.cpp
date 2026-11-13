/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Released under GPL-3.0-or-later; see LICENSE in the repository root.
 */

#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthEditor.h"
#include "SurgeSynthProcessor.h"
#include "PatchDB.h"
#include "UserDefaults.h"
#include "gui/reference/Manifest.h"
#include "gui/reference/ParameterBridge.h"
#include "gui/reference/Workspace.h"
#include "gui/reference/WorkspaceData.h"
#include "gui/reference/Visuals.h"
#include "gui/reference/PreviewCurves.h"
#include "gui/reference/ModulationInteraction.h"
#include "UndoManager.h"
#include "DistortionEffect.h"
#include "sst/filters/FilterConfiguration.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <future>
#include <limits>
#include <set>
#include <vector>

namespace
{
using Surge::GUI::Reference::ParameterBridge;
using Surge::GUI::Reference::Workspace;

constexpr int referenceWidth = 1586;
constexpr int referenceHeight = 992;
constexpr int sampleBlock = 512;
constexpr double sampleRate = 48000.0;
constexpr int showcaseAudioBlocks = 768;

juce::File sourceRoot() { return juce::File(juce::String(SURGE_REFERENCE_SOURCE_DIR)); }

/** The processor constructor may read platform defaults. Before exercising an
    editor, redirect all subsequent preference/DB/keymap/backup writes into this
    unique test-owned directory. Declare this BEFORE the processor so its DB
    worker and editor are destroyed before the directory is removed. This does
    not change HOME, XDG settings, or a user's configured data-path override. */
struct IsolatedUserData
{
    IsolatedUserData()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("surge-reference-tests-" + juce::Uuid().toString()))
    {
        REQUIRE(!directory.exists());
        REQUIRE(directory.createDirectory().wasOk());
    }
    ~IsolatedUserData() { directory.deleteRecursively(); }

    void attach(SurgeSynthProcessor &processor, const juce::String &name = "processor")
    {
        REQUIRE(processor.surge != nullptr);
        auto &storage = processor.surge->storage;
        const auto path = directory.getChildFile(name);
        REQUIRE(path.createDirectory().wasOk());
        storage.patchDB.reset();
        storage.userDataPath = fs::path(path.getFullPathName().toStdString());
        storage.localAppDataPath = storage.userDataPath / "Application";
        storage.initializeUserDataPaths();
        storage.createUserDirectory();
        storage.userDefaultsProvider = std::make_unique<Surge::Storage::UserDefaultsProvider>(
            storage.userDataPath, "SurgeXT", Surge::Storage::defaultKeyToString,
            [&storage](const std::string &message, const std::string &title) {
                storage.reportError(message, title);
            });
        storage.patchDB = std::make_unique<Surge::PatchStorage::PatchDB>(&storage);
        storage.patchDBInitialized = false;
        // An editor starts from a deterministic factory skin and empty test
        // preferences, never a developer's selected external skin.
        storage.userDefaultsProvider->addOverride(Surge::Storage::DefaultSkin, "");
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCIn, 0);
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCOut, 0);
    }

    juce::File directory;
};

/** Typed-entry callouts are asynchronous by design. These synchronous unit tests
    do not spin a nested modal event loop; dispose the test's modal manager after
    checking the real callbacks, before its Workspace and GUI runtime disappear. */
struct ScopedModalCleanup
{
    ScopedModalCleanup()
    {
        REQUIRE(juce::ModalComponentManager::getInstance()->getNumModalComponents() == 0);
    }
    ~ScopedModalCleanup()
    {
        if (auto *manager = juce::ModalComponentManager::getInstanceWithoutCreating())
            manager->cancelAllModalComponents();
        juce::ModalComponentManager::deleteInstance();
    }
};

juce::Array<juce::File> workspaceManifests()
{
    auto skins = sourceRoot().getChildFile("resources/data/skins");
    auto packages = skins.findChildFiles(juce::File::findDirectories, false, "*.surge-skin");
    juce::Array<juce::File> result;
    for (const auto &package : packages)
    {
        auto manifest = package.getChildFile("workspace.json");
        if (manifest.existsAsFile())
            result.add(manifest);
    }
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        return a.getFullPathName() < b.getFullPathName();
    });
    return result;
}

juce::var readManifest(const juce::File &path)
{
    juce::var data;
    juce::String error;
    INFO("Manifest: " << path.getFullPathName().toStdString());
    const bool loaded = Surge::GUI::Reference::loadManifest(path, data, error);
    INFO("Manifest error: " << error.toStdString());
    REQUIRE(loaded);
    REQUIRE(error.isEmpty());
    return data;
}

juce::Component *findComponent(juce::Component &parent, const juce::String &id)
{
    if (parent.getComponentID() == id)
        return &parent;
    for (auto *child : parent.getChildren())
        if (auto *match = findComponent(*child, id))
            return match;
    return nullptr;
}

juce::Slider *findSlider(juce::Component &component)
{
    if (auto *slider = dynamic_cast<juce::Slider *>(&component))
        return slider;
    for (auto *child : component.getChildren())
        if (auto *slider = findSlider(*child))
            return slider;
    return nullptr;
}

juce::Component *findBindingControl(Workspace &workspace, const juce::var &manifest,
                                    const juce::String &binding)
{
    if (auto *controls = manifest["controls"].getArray())
        for (const auto &control : *controls)
            if (control["binding"].toString() == binding)
                if (auto *component = findComponent(workspace, control["id"].toString()))
                    return component;
    return nullptr;
}

SurgeParamToJuceParamAdapter *adapterFor(SurgeSynthProcessor &processor, ParameterBridge &bridge,
                                         const juce::String &binding)
{
    auto *parameter = bridge.parameter(binding);
    if (parameter == nullptr)
        return nullptr;
    auto found = processor.paramsByID.find(processor.surge->idForParameter(parameter));
    return found == processor.paramsByID.end() ? nullptr : found->second;
}

struct GestureProbe final : juce::AudioProcessorParameter::Listener
{
    explicit GestureProbe(juce::AudioProcessorParameter &target) : parameter(target)
    {
        parameter.addListener(this);
    }
    ~GestureProbe() override { parameter.removeListener(this); }

    void parameterValueChanged(int, float value) override
    {
        ++changes;
        lastValue = value;
    }
    void parameterGestureChanged(int, bool starting) override
    {
        starting ? ++starts : ++ends;
        balance += starting ? 1 : -1;
        minimumBalance = std::min(balance, minimumBalance);
    }

    juce::AudioProcessorParameter &parameter;
    int starts = 0, ends = 0, changes = 0, balance = 0, minimumBalance = 0;
    float lastValue = 0.0f;
};

void edit(ParameterBridge &bridge, const juce::String &binding, float value)
{
    bridge.begin(binding);
    bridge.set(binding, value);
    bridge.end(binding);
}

float processBlock(SurgeSynthProcessor &processor, juce::MidiBuffer midi = {})
{
    juce::AudioBuffer<float> audio(6, sampleBlock);
    audio.clear();
    processor.processBlock(audio, midi);
    float peak = 0.f;
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
        {
            REQUIRE(std::isfinite(audio.getSample(channel, sample)));
            if (channel < 2)
                peak = std::max(peak, std::abs(audio.getSample(channel, sample)));
        }
    return peak;
}

std::vector<float> hostParameterValues(const SurgeSynthProcessor &processor)
{
    std::vector<float> result;
    for (const auto *parameter : processor.getParameters())
    {
        const auto value = parameter->getValue();
        REQUIRE(std::isfinite(value));
        result.push_back(value);
    }
    return result;
}

void checkHostParameterValues(const SurgeSynthProcessor &processor,
                              const std::vector<float> &expected, float margin = 0.00001f)
{
    const auto &parameters = processor.getParameters();
    REQUIRE(static_cast<size_t>(parameters.size()) == expected.size());
    for (int index = 0; index < parameters.size(); ++index)
    {
        INFO("Host parameter " << index << ": " << parameters[index]->getName(128).toStdString());
        CHECK(parameters[index]->getValue() ==
              Catch::Approx(expected[static_cast<size_t>(index)]).margin(margin));
    }
}

Surge::GUI::SkinDB::Entry skinEntry(const juce::File &manifest)
{
    const auto directory = manifest.getParentDirectory();
    Surge::GUI::SkinDB::Entry entry;
    entry.rootType = Surge::GUI::FACTORY;
    entry.root =
        (directory.getParentDirectory().getFullPathName() + juce::File::getSeparatorString())
            .toStdString();
    entry.name = (directory.getFileName() + juce::File::getSeparatorString()).toStdString();
    entry.parseable = true;
    return entry;
}

juce::MouseEvent mouseEvent(juce::Component &component, juce::Point<float> position)
{
    const auto now = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(),
            position,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),
            1.f,
            0.f,
            0.f,
            0.f,
            0.f,
            &component,
            &component,
            now,
            position,
            now,
            1,
            false};
}

bool writePng(const juce::Image &image, const juce::File &destination)
{
    juce::FileOutputStream stream(destination);
    if (!stream.openedOk())
        return false;
    // FileOutputStream may open an existing file. Truncate so repeated captures cannot retain
    // bytes from an older, larger PNG after the new image's end marker.
    stream.setPosition(0);
    if (stream.truncate().failed())
        return false;
    juce::PNGImageFormat png;
    const bool written = png.writeImageToStream(image, stream);
    stream.flush();
    return written && stream.getStatus().wasOk();
}

int changedPixels(const juce::Image &a, const juce::Image &b)
{
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return -1;
    int changed = 0;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
            changed += a.getPixelAt(x, y) != b.getPixelAt(x, y) ? 1 : 0;
    return changed;
}

juce::var object() { return juce::var(new juce::DynamicObject()); }

void property(juce::var &object, const char *key, const juce::var &value)
{
    object.getDynamicObject()->setProperty(juce::Identifier(key), value);
}

/** A test-owned native catalog with deliberately identical Factory/User folder
    names and nonsequential display order. No file is loaded or written. */
void installBrowserTestCatalog(SurgeSynthProcessor &processor, const juce::File &directory)
{
    auto &storage = processor.surge->storage;
    storage.patch_category.clear();
    const auto category = [&](const char *name, bool root, bool factory, int count) {
        PatchCategory entry;
        entry.name = name;
        entry.isRoot = root;
        entry.isFactory = factory;
        entry.numberOfPatchesInCategoryAndChildren = count;
        storage.patch_category.push_back(entry);
    };
    category("Shared", true, true, 2);
    category("Shared/Child", false, true, 1);
    category("Third Party", true, false, 1);
    category("Shared", true, false, 2);
    category("Shared/Child", false, false, 1);
    category("User Only", true, false, 1);
    storage.firstThirdPartyCategory = 2;
    storage.firstUserCategory = 3;
    storage.patch_list.clear();
    const auto patch = [&](const char *name, int folder, bool favorite) {
        Patch entry;
        entry.name = name;
        entry.category = folder;
        entry.isFavorite = favorite;
        entry.path = fs::path(directory.getChildFile(juce::String(folder) + ".fxp")
                                  .getFullPathName().toStdString());
        storage.patch_list.push_back(entry);
    };
    patch("Test Factory Root", 0, true);
    patch("Test Factory Child", 1, false);
    patch("Test Third Party", 2, false);
    patch("Test User Root", 3, false);
    patch("Test User Child", 4, true);
    patch("Test User Other", 5, false);
    storage.patchOrdering = {4, 2, 0, 5, 1, 3};
    storage.patchCategoryOrdering = {0, 2, 3, 5, 1, 4};
    storage.favoritesOrdering = {4, 0};
    processor.surge->patchid = 0;
    storage.getPatch().category = "Shared";
}

/** An explicitly documented real patch, not a painted imitation of the concept
    values. It makes the capture exercise active oscillators, filters, FX and
    modulation rows. Only this test-owned processor is changed; choosing a skin
    in the application never installs the fixture or changes a user's patch. */
juce::var configureShowcaseFixture(SurgeSynthProcessor &processor,
                                  const juce::String &skinId = {})
{
    // These are real demonstration patches for this isolated capture process.
    // Their labels and plots come from the native patch and loaded audio data.
    struct Profile
    {
        const char *name{"Reference Showcase"};
        const char *wavetable{"Spectral Sweep"};
        const char *thirdWavetable{nullptr};
        int secondOscillator{ot_FM2};
        int thirdOscillator{ot_classic};
        int routing{fc_dual1};
        int firstFilter{sst::filters::fut_lp24};
        int secondFilter{sst::filters::fut_bp12};
        float position{.37f}, verticalSkew{.12f};
        float secondLevelDb{-3.f}, thirdLevelDb{-6.f};
        bool noiseEnabled{false};
        float noiseLevelDb{-18.f};
        float firstCutoffHz{1420.f}, secondCutoffHz{3780.f};
        float firstResonance{.27f}, secondResonance{.61f};
        std::array<float, 4> ampEnvelope{.005f, .2f, .8f, .32f};
        std::array<float, 4> filterEnvelope{.005f, .2f, .7f, .32f};
        float lfoHz{.70710678f};
        int lfoShape{lt_sine};
        std::array<const char *, 8> macroNames{"Tone", "Shape", "Color", "Width",
                                                "Motion", "FM", "Release", "Drive"};
        std::array<float, 8> macroValues{.72f, .48f, .81f, .67f, .35f, .56f, .27f, .43f};
        std::array<int, 8> effects{fxt_distortion, fxt_chorus4, fxt_phaser, fxt_eq,
                                    fxt_delay, fxt_reverb2, fxt_conditioner, fxt_eq};
    } profile;
    const auto skinNumber = skinId.substring(0, 2).getIntValue();
    if (skinNumber >= 1 && skinNumber <= 10)
        profile.effects = {fxt_distortion, fxt_chorus4, fxt_phaser, fxt_delay,
                           fxt_reverb2, fxt_eq, fxt_conditioner, fxt_eq};
    switch (skinNumber)
    {
    case 1:
        profile.name = "Init";
        profile.routing = fc_serial1;
        profile.macroNames = {"Tone", "Edge", "Drive", "Space", "Motion", "Width", "Env Amt", "FX Mix"};
        break;
    case 2:
        profile.name = "Init";
        profile.wavetable = "Analog Stack";
        profile.thirdWavetable = "VocalFormant A";
        profile.secondOscillator = ot_classic;
        profile.thirdOscillator = ot_wavetable;
        profile.routing = fc_serial1;
        profile.effects = {fxt_chorus4, fxt_delay, fxt_reverb2, fxt_distortion,
                           fxt_eq, fxt_conditioner, fxt_phaser, fxt_eq};
        profile.macroNames = {"Tone", "Motion", "Color", "Space", "Drive", "Width", "Attack", "Release"};
        break;
    case 3:
        profile.name = "Init";
        profile.wavetable = "Spectral Bloom";
        profile.position = .723f;
        profile.verticalSkew = .412f;
        profile.firstCutoffHz = 1740.f;
        profile.secondCutoffHz = 7400.f;
        profile.secondFilter = sst::filters::fut_lpmoog;
        profile.macroNames = {"Atmos", "Motion", "Color", "Space", "Drive", "FX Mix", "Width", "Release"};
        break;
    case 4:
        profile.name = "Bauhaus Harmonic Lab";
        profile.wavetable = "Bauhaus Waves";
        profile.secondLevelDb = -4.6f;
        profile.thirdLevelDb = -2.1f;
        // The illustrated quarter/eighth/sixteenth-note rates correspond to
        // 2/4/8 Hz at 120 BPM. The native LFOs remain key-triggered.
        profile.lfoHz = 2.f;
        profile.macroNames = {"Harmonic", "Motion", "Texture", "Space", "Drive", "Color", "Width", "FX Mix"};
        break;
    case 5:
        profile.name = "Space-Age Instrument 1977";
        profile.wavetable = "Orbiting Saw";
        profile.secondOscillator = ot_wavetable;
        profile.thirdOscillator = ot_sine;
        profile.position = .347f;
        profile.firstCutoffHz = 1800.f;
        profile.secondCutoffHz = 720.f;
        profile.firstResonance = .32f;
        profile.secondResonance = .68f;
        profile.secondLevelDb = -3.2f;
        // The concept's 2 ms attack is below the native log2-time range.
        // Use a real 4 ms attack above the -8 zero-time/snap sentinel.
        profile.ampEnvelope = {.004f, .18f, 1.f, .42f};
        profile.filterEnvelope = {.005f, .48f, .72f, 1.2f};
        profile.lfoHz = .25f;
        profile.macroNames = {"Brightness", "Motion", "Space", "Vintage", "Color", "Width", "Attack", "Release"};
        break;
    case 6:
        profile.name = "Tokyo Bass Sequence";
        profile.wavetable = "JT Saw RM";
        profile.position = .412f;
        profile.secondOscillator = ot_classic;
        profile.thirdOscillator = ot_FM2;
        profile.routing = fc_serial1;
        profile.firstFilter = sst::filters::fut_lpmoog;
        profile.firstCutoffHz = 2400.f;
        profile.secondCutoffHz = 560.f;
        profile.firstResonance = .72f;
        profile.secondResonance = .46f;
        profile.secondLevelDb = -3.1f;
        profile.ampEnvelope = {.005f, .198f, .78f, .382f};
        // As in the hardware profile, 4 ms is an explicit native adaptation
        // of the concept's unsupported 3.2 ms value, not a silently clamped edit.
        profile.filterEnvelope = {.004f, .42f, .42f, 1.2f};
        profile.lfoShape = lt_tri;
        profile.lfoHz = 2.f;
        profile.effects[6] = fxt_off;
        profile.effects[7] = fxt_off;
        profile.macroNames = {"Macro 1", "Macro 2", "Macro 3", "Macro 4", "Macro 5", "Macro 6", "Macro 7", "Macro 8"};
        profile.macroValues = {.64f, 0.f, .27f, .82f, .50f, 0.f, .36f, .71f};
        break;
    case 7:
        profile.name = "Liquid Aurora";
        profile.wavetable = "Aurora Waves";
        profile.thirdWavetable = "VocalFormant A";
        profile.secondOscillator = ot_classic;
        profile.thirdOscillator = ot_wavetable;
        profile.position = .38f;
        profile.verticalSkew = .62f;
        profile.firstFilter = sst::filters::fut_lp12;
        profile.secondFilter = sst::filters::fut_comb_pos;
        profile.firstCutoffHz = 4200.f;
        profile.secondCutoffHz = 1370.f;
        profile.firstResonance = .36f;
        profile.secondResonance = .71f;
        profile.secondLevelDb = -3.1f;
        profile.thirdLevelDb = -4.6f;
        profile.noiseEnabled = true;
        profile.lfoHz = .24f;
        profile.macroNames = {"Brightness", "Motion", "Color", "Space", "Grit", "Ensemble", "Attack", "Release"};
        break;
    case 8:
        profile.name = "ClearSignal Pad";
        profile.wavetable = "Glass Sweep";
        profile.thirdWavetable = "VocalFormant A";
        profile.thirdOscillator = ot_wavetable;
        profile.routing = fc_serial1;
        profile.secondFilter = sst::filters::fut_hp12;
        profile.secondCutoffHz = 4760.f;
        profile.firstResonance = .34f;
        profile.secondResonance = .22f;
        profile.secondLevelDb = -1.2f;
        profile.thirdLevelDb = -3.f;
        profile.noiseEnabled = true;
        profile.noiseLevelDb = -12.f;
        profile.ampEnvelope = {.12f, 1.2f, .76f, 2.8f};
        profile.lfoHz = .25f;
        profile.effects = {fxt_eq, fxt_distortion, fxt_chorus4, fxt_phaser,
                           fxt_delay, fxt_reverb2, fxt_conditioner, fxt_eq};
        profile.macroNames = {"Brightness", "Motion", "Texture", "Space", "Drive", "Width", "Attack", "Release"};
        break;
    case 9:
        profile.name = "Evolving Glass Engine";
        profile.wavetable = "Glass Sweep";
        profile.thirdWavetable = "VocalFormant A";
        profile.thirdOscillator = ot_wavetable;
        profile.firstCutoffHz = 4200.f;
        profile.firstResonance = .36f;
        profile.ampEnvelope = {.012f, .476f, .68f, 1.48f};
        profile.lfoHz = .25f;
        profile.effects = {fxt_chorus4, fxt_delay, fxt_reverb2, fxt_distortion,
                           fxt_eq, fxt_conditioner, fxt_phaser, fxt_off};
        profile.macroNames = {"Motion", "Texture", "Harmonics", "Space", "Width", "Time", "Drive", "Tone"};
        profile.macroValues = {.74f, .62f, .38f, .81f, .66f, .47f, .21f, .58f};
        break;
    case 10:
        profile.name = "Evolving Horizons";
        profile.wavetable = "GlassWavetable 03";
        profile.thirdWavetable = "VocalFormant A";
        profile.thirdOscillator = ot_wavetable;
        profile.routing = fc_serial1;
        profile.firstFilter = sst::filters::fut_lpmoog;
        profile.firstCutoffHz = 1360.f;
        profile.secondCutoffHz = 4780.f;
        profile.firstResonance = .28f;
        profile.secondResonance = .62f;
        profile.secondLevelDb = -2.4f;
        profile.thirdLevelDb = -1.8f;
        profile.noiseEnabled = true;
        profile.ampEnvelope = {.0124f, .62f, .74f, 1.52f};
        profile.lfoHz = .28f;
        profile.macroNames = {"Drive", "Motion", "Formant", "Space", "Color", "Width", "FX Mix", "Evolve"};
        profile.macroValues[0] = .62f;
        profile.macroValues[1] = .37f;
        break;
    default:
        break;
    }
    auto &synth = *processor.surge;
    REQUIRE_FALSE(synth.audio_processing_active.load());
    const auto templatePath = "resources/data/patches_factory/Templates/Init Wavetable.fxp";
    const auto wavetableDirectory = "resources/data/wavetables/Reference Showcases/";
    const auto wavetablePath = juce::String(wavetableDirectory) + profile.wavetable + ".wt";
    const auto initialPatch = sourceRoot().getChildFile(templatePath);
    REQUIRE(initialPatch.existsAsFile());
    REQUIRE(synth.loadPatchByPath(initialPatch.getFullPathName().toRawUTF8(), -1,
                                  profile.name));
    synth.processAudioThreadOpsWhenAudioEngineUnavailable();
    ParameterBridge bridge(processor);
    bridge.setScene(0);
    juce::Array<juce::var> settings;
    const auto record = [&](const juce::String &binding) {
        auto setting = object();
        property(setting, "binding", binding);
        property(setting, "normalized", bridge.value(binding));
        property(setting, "display", bridge.display(binding));
        settings.add(setting);
    };
    const auto select = [&](const juce::String &binding, int nativeValue) {
        CAPTURE(binding.toStdString(), nativeValue);
        auto *parameter = bridge.parameter(binding);
        REQUIRE(parameter != nullptr);
        REQUIRE(bridge.enabled(binding));
        const auto index =
            parameter->valtype == vt_bool ? nativeValue : nativeValue - parameter->val_min.i;
        REQUIRE(index >= 0);
        REQUIRE(index < bridge.choices(binding).size());
        edit(bridge, binding, bridge.choiceValue(binding, index));
        // FX and oscillator algorithms commit through the normal deferred path.
        // No audio thread exists for this isolated fixture yet.
        synth.processAudioThreadOpsWhenAudioEngineUnavailable();
        REQUIRE(bridge.choiceIndex(binding) == index);
        record(binding);
    };
    const auto setNative = [&](const juce::String &binding, float nativeValue) {
        CAPTURE(binding.toStdString(), nativeValue);
        auto *parameter = bridge.parameter(binding);
        REQUIRE(parameter != nullptr);
        REQUIRE(parameter->valtype == vt_float);
        REQUIRE(bridge.enabled(binding));
        REQUIRE(nativeValue >= parameter->val_min.f);
        REQUIRE(nativeValue <= parameter->val_max.f);
        // Use the native parameter's normalization without mutating its storage
        // directly or assuming every display unit has a linear 0..1 mapping.
        auto converted = *parameter;
        converted.val.f = nativeValue;
        edit(bridge, binding, converted.get_value_f01());
        REQUIRE(parameter->val.f == Catch::Approx(nativeValue).margin(.0001f));
        record(binding);
    };
    const auto setDisplay = [&](const juce::String &binding, const juce::String &displayValue) {
        CAPTURE(binding.toStdString(), displayValue.toStdString());
        REQUIRE(bridge.enabled(binding));
        juce::String error;
        bridge.begin(binding);
        const bool accepted = bridge.setText(binding, displayValue, error);
        bridge.end(binding);
        INFO(error.toStdString());
        REQUIRE(accepted);
        record(binding);
    };

    select("osc1.type", ot_wavetable);
    select("osc2.type", profile.secondOscillator);
    select("osc3.type", profile.thirdOscillator);
    auto &patch = synth.storage.getPatch();
    auto &scene = patch.scene[0];
    juce::Array<juce::var> wavetableSources;
    const auto loadWavetable = [&](int oscillator, const char *name, const char *directory,
                                   int expectedFrames, int expectedSamples,
                                   const char *provenance) {
        const auto path = juce::String(directory) + name + ".wt";
        const auto file = sourceRoot().getChildFile(path);
        REQUIRE(file.existsAsFile());
        auto &source = scene.osc[oscillator];
        synth.storage.load_wt(file.getFullPathName().toStdString(), &source.wt, &source);
        REQUIRE(source.wt.SourceFrameCount() == expectedFrames);
        REQUIRE(source.wt.size == expectedSamples);
        REQUIRE(source.wavetable_display_name == name);
        auto metadata = object();
        property(metadata, "oscillator", oscillator + 1);
        property(metadata, "file", path);
        property(metadata, "displayName", juce::String(source.wavetable_display_name));
        property(metadata, "frames", source.wt.SourceFrameCount());
        property(metadata, "samplesPerFrame", source.wt.size);
        property(metadata, "fileBytes", file.getSize());
        property(metadata, "provenance", provenance);
        wavetableSources.add(metadata);
    };
    constexpr auto originalWavetableProvenance =
        "Original analytic periodic harmonic family generated by "
        "scripts/reference-workspaces/generate_showcase_wavetables.py";
    const bool largeTable = juce::String(profile.wavetable) == "Spectral Bloom";
    loadWavetable(0, profile.wavetable, wavetableDirectory, largeTable ? 256 : 48,
                  largeTable ? 2048 : 512, originalWavetableProvenance);
    if (profile.thirdWavetable != nullptr)
    {
        loadWavetable(2, profile.thirdWavetable, wavetableDirectory, 48, 512,
                      originalWavetableProvenance);
        setNative("osc3.param0", .48f);
    }
    setNative("osc1.param0", profile.position);
    setNative("osc1.param1", profile.verticalSkew);
    if (skinNumber == 2)
    {
        // Classic's zero-shape, symmetric-width setting supplies Ivory's saw
        // family with native Shape/Width labels and the illustrated four voices.
        setNative("osc2.param0", 0.f);
        setNative("osc2.param1", .5f);
        select("osc2.param6", 4);
    }
    else if (skinNumber == 5)
    {
        // The factory table already supplies the reference's square harmonic
        // family. Keep its actual name instead of inventing a Pulsar preset.
        loadWavetable(1, "Sine To Square", "resources/data/wavetables/Basic/", 31, 128,
                      "Existing Surge XT factory Basic wavetable; reused native source samples");
        setNative("osc2.param0", .612f);
        select("osc2.octave", 1);
        setDisplay("osc1.param5", "12 cents");
        REQUIRE(bridge.parameter("osc1.param5")->val.f == Catch::Approx(.12f).margin(.0001f));
        select("osc1.param6", 4);
        setDisplay("osc2.param5", "7 cents");
        REQUIRE(bridge.parameter("osc2.param5")->val.f == Catch::Approx(.07f).margin(.0001f));
        select("osc2.param6", 4);
        // Native Sine shape 0, zero feedback and one unison voice produce the
        // actual sine shown in the reference's third oscillator.
        select("osc3.param0", 0);
        setNative("osc3.param1", 0.f);
        setNative("osc3.param5", 0.f);
        select("osc3.param6", 1);
    }
    else
        setNative("osc2.param0", .38f);
    for (int index = 1; index <= 3; ++index)
    {
        select("osc" + juce::String(index) + ".mute", 0);
        select("osc" + juce::String(index) + ".retrigger", 1);
    }
    // Amplitude controls are not linear amplitudes: use their native dB parser.
    setDisplay("osc1.level", "0 dB");
    setDisplay("osc2.level", juce::String(profile.secondLevelDb) + " dB");
    setDisplay("osc3.level", juce::String(profile.thirdLevelDb) + " dB");
    if (profile.noiseEnabled)
        setDisplay("noise.level", juce::String(profile.noiseLevelDb) + " dB");
    else
        setNative("noise.level", 0.f);
    select("noise.mute", profile.noiseEnabled ? 0 : 1);
    setNative("ring12.level", 0.f);
    setNative("ring23.level", 0.f);
    select("ring12.mute", 1);
    select("ring23.mute", 1);
    select("osc3.octave", skinNumber == 10 ? 1 : -1);
    setNative("scene.drift", 0.f);
    select("routing", profile.routing);
    select("filter1.type", profile.firstFilter);
    select("filter2.type", profile.secondFilter);
    setDisplay("filter1.cutoff", juce::String(profile.firstCutoffHz) + " Hz");
    setNative("filter1.resonance", profile.firstResonance);
    setDisplay("filter2.cutoff", juce::String(profile.secondCutoffHz) + " Hz");
    setNative("filter2.resonance", profile.secondResonance);
    setDisplay("scene.volume", "0 dB");
    setDisplay("master.volume", "-6 dB");
    setNative("scene.width", .82f);
    for (int index = 1; index <= 2; ++index)
    {
        const auto prefix = "env" + juce::String(index) + ".";
        const auto &values = index == 1 ? profile.ampEnvelope : profile.filterEnvelope;
        setNative(prefix + "attack", std::log2(values[0]));
        setNative(prefix + "decay", std::log2(values[1]));
        setNative(prefix + "sustain", values[2]);
        setNative(prefix + "release", std::log2(values[3]));
    }
    select("lfo1.shape", profile.lfoShape);
    setNative("lfo1.rate", std::log2(profile.lfoHz));
    select("lfo1.trigmode", lm_keytrigger);
    if (skinNumber >= 1 && skinNumber <= 10)
    {
        setNative("lfo1.phase", 0.f);
        setNative("lfo1.deform", 0.f);
        setNative("lfo1.magnitude", 1.f);
        select("lfo1.unipolar", 0);
    }
    if (skinNumber == 4 || skinNumber == 5 || skinNumber == 7)
    {
        const auto configureLfo = [&](int index, int shape, float hz, float phase,
                                      float magnitude) {
            const auto prefix = "lfo" + juce::String(index) + ".";
            select(prefix + "shape", shape);
            setNative(prefix + "rate", std::log2(hz));
            setNative(prefix + "phase", phase);
            setNative(prefix + "deform", 0.f);
            setNative(prefix + "magnitude", magnitude);
            select(prefix + "unipolar", 0);
            select(prefix + "trigmode", lm_keytrigger);
        };
        configureLfo(2, lt_tri, skinNumber == 4 ? 4.f : (skinNumber == 5 ? 1.f : 2.f),
                     skinNumber == 7 ? 0.f : .25f, skinNumber == 7 ? .62f : 1.f);
        // Aurora's illustrated eighth note is 4 Hz at the fixture's 120 BPM.
        // Random steps are generated by the native seeded S&H preview/engine.
        configureLfo(3, skinNumber == 4 ? lt_square : lt_snh,
                     skinNumber == 4 ? 8.f : 4.f, 0.f, skinNumber == 7 ? .48f : 1.f);
    }
    for (size_t index = 0; index < profile.effects.size(); ++index)
    {
        const auto binding = "fx" + juce::String(static_cast<int>(index) + 1);
        select(binding + ".type", profile.effects[index]);
        if (profile.effects[index] == fxt_distortion)
        {
            // The initialized effect has 440 Hz pre/post high-cuts. Open both
            // real filters so the demonstration retains audible upper partials.
            setDisplay(binding + ".param3", "18000 Hz");
            setDisplay(binding + ".param9", "18000 Hz");
            setDisplay(binding + ".param4", "6 dB");
            setDisplay(binding + ".param10", "-6 dB");
        }
        else if (profile.effects[index] == fxt_conditioner)
            setDisplay(binding + ".param4", "-6 dB");
    }

    std::array<juce::String, 8> targets{"filter1.cutoff", "osc1.param0", "filter2.cutoff",
                                         "scene.width", "lfo1.rate", "osc2.param0",
                                         "env1.release", "fx1.param0"};
    const auto effectParameter = [&](int type, int parameter) {
        const auto found = std::find(profile.effects.begin(), profile.effects.end(), type);
        REQUIRE(found != profile.effects.end());
        return "fx" + juce::String(static_cast<int>(found - profile.effects.begin()) + 1) +
               ".param" + juce::String(parameter);
    };
    if (skinNumber >= 1 && skinNumber <= 10)
        for (size_t index = 0; index < targets.size(); ++index)
        {
            // A named macro really controls the function named on its knob.
            const juce::String label = profile.macroNames[index];
            if (label == "Tone" || label == "Brightness")
                targets[index] = "filter1.cutoff";
            else if (label == "Color")
                targets[index] = "filter2.cutoff";
            else if (label == "Motion")
                targets[index] = "lfo1.rate";
            else if (label == "Drive" || label == "Grit" || label == "Edge" || label == "Vintage")
                targets[index] = effectParameter(fxt_distortion, 4);
            else if (label == "Space" || label == "Atmos")
                targets[index] = effectParameter(fxt_reverb2, 9);
            else if (label == "FX Mix")
                targets[index] = effectParameter(fxt_chorus4, 6);
            else if (label == "Ensemble")
                targets[index] = effectParameter(fxt_chorus4, 2);
            else if (label == "Width")
                targets[index] = "scene.width";
            else if (label == "Attack")
                targets[index] = "env1.attack";
            else if (label == "Release")
                targets[index] = "env1.release";
            else if (label == "Env Amt")
                targets[index] = "filter1.envmod";
            else if (label == "Time")
                targets[index] = effectParameter(fxt_delay, 0);
            else if (!label.startsWith("Macro "))
                targets[index] = "osc1.param0";
        }
    const std::array<float, 8> depths{.08f, .12f, .10f, .08f, .07f, .12f, .10f, .06f};
    juce::Array<juce::var> routes;
    for (int index = 0; index < 8; ++index)
    {
        const auto binding = "macro" + juce::String(index + 1);
        juce::String(profile.macroNames[index])
            .copyToUTF8(patch.CustomControllerLabel[index], CUSTOM_CONTROLLER_LABEL_SIZE);
        edit(bridge, binding, profile.macroValues[index]);
        REQUIRE(bridge.value(binding) == Catch::Approx(profile.macroValues[index]).margin(.0001f));
        auto *target = bridge.parameter(targets[index]);
        REQUIRE(target != nullptr);
        REQUIRE(synth.setModDepth01(target->id, static_cast<modsources>(ms_ctrl1 + index), 0, 0,
                                    depths[index]));
        auto route = object();
        property(route, "macro", index + 1);
        property(route, "label", profile.macroNames[index]);
        property(route, "sourceId", static_cast<int>(ms_ctrl1 + index));
        property(route, "sourceScene", 0);
        property(route, "sourceIndex", 0);
        property(route, "parameterId", target->id);
        property(route, "target", targets[index]);
        property(route, "normalizedDepth", depths[index]);
        routes.add(route);
        record(binding);
    }
    if (skinNumber >= 1 && skinNumber <= 10)
    {
        const auto addRoute = [&](modsources source, const char *binding, float depth,
                                  const char *label) {
            CAPTURE(binding, label, depth);
            auto *target = bridge.parameter(binding);
            REQUIRE(target != nullptr);
            REQUIRE(bridge.enabled(binding));
            REQUIRE(synth.isValidModulation(target->id, source));
            REQUIRE(synth.setModDepth01(target->id, source, 0, 0, depth));
            REQUIRE(synth.getModDepth01(target->id, source, 0, 0) ==
                    Catch::Approx(depth).margin(.00001f));
            auto route = object();
            property(route, "label", label);
            property(route, "sourceId", static_cast<int>(source));
            property(route, "sourceScene", 0);
            property(route, "sourceIndex", 0);
            property(route, "parameterId", target->id);
            property(route, "target", binding);
            property(route, "normalizedDepth", depth);
            routes.add(route);
        };
        // These routes are audible engine assignments, not painted matrix rows.
        // Modest depths leave the existing capture headroom/reset assertions
        // meaningful while giving each styled patch actual envelope/LFO motion.
        addRoute(ms_lfo1, "filter1.cutoff", .025f, "LFO 1");
        addRoute(ms_filtereg, "osc1.param0", .06f, "Filter EG");
        if (skinNumber == 9 || skinNumber == 10)
        {
            addRoute(ms_ampeg, "osc3.param0", .035f, "Amp EG");
            select("lfo2.shape", lt_tri);
            setNative("lfo2.rate", std::log2(.5f));
            select("lfo2.trigmode", lm_keytrigger);
            addRoute(ms_lfo2, "osc2.pitch", .003f, "LFO 2");
        }
        if (skinNumber == 9)
        {
            select("lfo3.shape", lt_sine);
            setNative("lfo3.rate", std::log2(.125f));
            select("lfo3.trigmode", lm_keytrigger);
            addRoute(ms_lfo3, "filter2.cutoff", -.02f, "LFO 3");
        }
        // Metadata belongs to this actual isolated patch. It does not add a
        // fictional preset or folder to the installed browser catalog.
        patch.category = "Reference Showcases";
        patch.author = "Surge XT reference workspace fixtures";
        patch.license = "GPL-3.0-or-later";
        patch.comment = "Original " + std::string(profile.wavetable) +
                        " harmonic wavetable, three native oscillators and " +
                        bridge.display("routing").toStdString() +
                        " filtering. Eight assigned macros with real envelope and LFO motion.";
        patch.tags.clear();
        for (const auto *tag : {"Wavetable", "Modulated", "Reference showcase"})
            patch.tags.emplace_back(std::string(tag));
    }
    const auto resolved = Surge::GUI::Reference::modulationRows(processor, 0);
    REQUIRE(resolved.size() == static_cast<size_t>(routes.size()));
    for (const auto &declared : routes)
    {
        const auto parameterId = static_cast<int>(declared["parameterId"]);
        const auto sourceId = static_cast<int>(declared["sourceId"]);
        const auto found = std::find_if(resolved.begin(), resolved.end(), [&](const auto &row) {
            return row.key.parameterId == parameterId && row.key.sourceId == sourceId &&
                   row.key.sourceScene == 0 && row.key.sourceIndex == 0;
        });
        REQUIRE(found != resolved.end());
        REQUIRE(found->normalizedDepth ==
                Catch::Approx(static_cast<float>(declared["normalizedDepth"])).margin(.00001f));
    }
    juce::Array<juce::var> resolvedRows;
    for (const auto &row : resolved)
    {
        REQUIRE(row.source.isNotEmpty());
        REQUIRE(row.destination.isNotEmpty());
        REQUIRE(row.amount.isNotEmpty());
        REQUIRE(std::isfinite(row.normalizedDepth));
        REQUIRE(std::abs(row.normalizedDepth) > 0.f);
        REQUIRE_FALSE(row.muted);
        auto entry = object();
        property(entry, "source", row.source);
        property(entry, "destination", row.destination);
        property(entry, "amount", row.amount);
        property(entry, "normalizedDepth", row.normalizedDepth);
        resolvedRows.add(entry);
    }
    REQUIRE(bridge.activeGestureCount() == 0);
    synth.storage.rngGen.g.seed(0x5a117u);
    synth.storage.noiseRng.reseed(0x6b228u);
    std::srand(0x71a3);

    auto fixture = object();
    property(fixture, "name", profile.name);
    property(fixture, "category", juce::String(patch.category));
    property(fixture, "author", juce::String(patch.author));
    property(fixture, "comment", juce::String(patch.comment));
    property(fixture, "license", juce::String(patch.license));
    juce::Array<juce::var> tags;
    for (const auto &tag : patch.tags)
        tags.add(juce::String(tag.tag));
    property(fixture, "tags", juce::var(tags));
    property(fixture, "skinId", skinId);
    property(fixture, "basePatch", templatePath);
    property(fixture, "wavetable", wavetablePath);
    property(fixture, "wavetableSources", juce::var(wavetableSources));
    property(fixture, "wavetableGenerator", "scripts/reference-workspaces/generate_showcase_wavetables.py");
    property(fixture, "wavetableProvenance",
             skinNumber == 5
                 ? "Original analytic harmonic family plus an explicitly identified reused factory table; audible native oscillator sources, no reference-image samples"
                 : "Original analytic periodic harmonic families; audible native oscillator sources, no reference-image samples");
    property(fixture, "settings", juce::var(settings));
    property(fixture, "modulationRoutes", juce::var(routes));
    property(fixture, "resolvedRouteRows", juce::var(resolvedRows));
    property(fixture, "scope",
             "Actual engine patch derived from a checked-in template using native parameter "
             "and wavetable APIs. Not an assertion that every illustrated value is reproduced.");
    return fixture;
}
} // namespace

TEST_CASE("All ten reference workspace manifests validate",
          "[reference-workspaces][reference-manifest]")
{
    const auto manifests = workspaceManifests();
    REQUIRE(manifests.size() == 10);
    std::set<std::string> ids;
    std::set<std::string> names;
    std::set<std::string> styles;
    for (const auto &path : manifests)
    {
        const auto data = readManifest(path);
        const auto id = data["id"].toString();
        CAPTURE(id.toStdString());
        REQUIRE(ids.insert(id.toStdString()).second);
        REQUIRE(names.insert(data["name"].toString().toStdString()).second);
        REQUIRE(styles.insert(data["style"].toString().toStdString()).second);
        REQUIRE(static_cast<int>(data["width"]) == referenceWidth);
        REQUIRE(static_cast<int>(data["height"]) == referenceHeight);
        REQUIRE(data["controls"].isArray());
        REQUIRE(data["panels"].isArray());
        REQUIRE(data["adaptations"].isArray());

        const auto xmlFile = path.getSiblingFile("skin.xml");
        REQUIRE(xmlFile.existsAsFile());
        auto xml = juce::XmlDocument::parse(xmlFile);
        REQUIRE(xml != nullptr);
        auto *globals = xml->getChildByName("globals");
        REQUIRE(globals != nullptr);
        auto *entry = globals->getChildByName("reference-workspace");
        REQUIRE(entry != nullptr);
        REQUIRE(entry->getStringAttribute("layout") == "workspace.json");
    }
}

TEST_CASE("Reference controls preserve scene isolation and host gestures",
          "[reference-workspaces][reference-bindings]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    REQUIRE(processor.surge != nullptr);
    ParameterBridge bridge(processor);
    bridge.setScene(0);
    REQUIRE(bridge.valid("filter1.cutoff"));
    auto *adapter = adapterFor(processor, bridge, "filter1.cutoff");
    REQUIRE(adapter != nullptr);
    GestureProbe probe(*adapter);

    const float initialA = bridge.value("filter1.cutoff");
    bridge.setScene(1);
    const float initialB = bridge.value("filter1.cutoff");
    bridge.setScene(0);
    edit(bridge, "filter1.cutoff", 0.371f);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.371f).margin(0.001f));
    CHECK(probe.starts == 1);
    CHECK(probe.ends == 1);
    CHECK(probe.balance == 0);
    CHECK(probe.minimumBalance == 0);
    CHECK(probe.changes >= 1);
    CHECK_FALSE(adapter->inEditGesture.load());

    bridge.setScene(1);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(initialB).margin(0.00001f));
    bridge.setScene(0);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.371f).margin(0.001f));

    // A scene change while dragging must close the old scene's gesture, not send its
    // end event to a newly resolved parameter in the other scene.
    bridge.begin("filter1.cutoff");
    bridge.set("filter1.cutoff", 0.419f);
    bridge.setScene(1);
    CHECK(probe.starts == probe.ends);
    CHECK(probe.balance == 0);
    CHECK_FALSE(adapter->inEditGesture.load());
    bridge.setScene(0);
    edit(bridge, "filter1.cutoff", initialA);

    REQUIRE(bridge.parameter("osc1.pitch") != nullptr);
    REQUIRE(bridge.parameter("osc2.pitch") != nullptr);
    REQUIRE(bridge.parameter("osc3.pitch") != nullptr);
    CHECK(bridge.parameter("osc1.pitch") != bridge.parameter("osc2.pitch"));
    CHECK(bridge.parameter("osc2.pitch") != bridge.parameter("osc3.pitch"));
    CHECK(bridge.parameter("osc1.pitch") != bridge.parameter("osc3.pitch"));
    const float original2 = bridge.value("osc2.pitch");
    const float original3 = bridge.value("osc3.pitch");
    edit(bridge, "osc1.pitch", 0.613f);
    CHECK(bridge.value("osc2.pitch") == Catch::Approx(original2));
    CHECK(bridge.value("osc3.pitch") == Catch::Approx(original3));

    // Host-originated state is read directly from the stable existing adapter.
    adapter->setValueNotifyingHost(0.733f);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.733f).margin(0.001f));
    CHECK(bridge.display("filter1.cutoff").isNotEmpty());
    CHECK(bridge.label("filter1.cutoff").isNotEmpty());

    REQUIRE(processor.macrosById.size() == 8);
    for (int i = 0; i < 8; ++i)
    {
        const auto binding = "macro" + juce::String(i + 1);
        CAPTURE(binding.toStdString());
        REQUIRE(bridge.valid(binding));
        GestureProbe macroProbe(*processor.macrosById[static_cast<size_t>(i)]);
        edit(bridge, binding, 0.25f + 0.05f * static_cast<float>(i));
        CHECK(bridge.value(binding) == Catch::Approx(0.25f + 0.05f * i).margin(0.001f));
        CHECK(macroProbe.starts == 1);
        CHECK(macroProbe.ends == 1);
        CHECK(macroProbe.balance == 0);
    }

    CHECK_FALSE(bridge.valid("osc99.pitch"));
    CHECK_FALSE(bridge.valid("macro9"));
    CHECK_FALSE(bridge.valid("missing.binding"));
    edit(bridge, "filter1.cutoff", std::numeric_limits<float>::quiet_NaN());
    CHECK(std::isfinite(bridge.value("filter1.cutoff")));
    edit(bridge, "filter1.cutoff", -3.0f);
    CHECK(bridge.value("filter1.cutoff") >= 0.0f);
    edit(bridge, "filter1.cutoff", 3.0f);
    CHECK(bridge.value("filter1.cutoff") <= 1.0f);
}

TEST_CASE("Reference JSON rejects ambiguous encodings before native parsing",
          "[reference-workspaces][reference-manifest][reference-security]")
{
    const auto manifests = workspaceManifests();
    REQUIRE(!manifests.isEmpty());
    const auto original = manifests[0].loadFileAsString();
    const auto data = readManifest(manifests[0]);
    const auto encodedName = juce::JSON::toString(data["name"]);
    REQUIRE(original.contains(encodedName));
    const auto withName = [&original, &encodedName](const juce::String &encoded) {
        return original.replace(encodedName, encoded);
    };
    const std::vector<std::pair<juce::String, juce::String>> malformed = {
        {"escaped duplicate key", "{\"sch\\u0065ma\": 1," + original.substring(1)},
        {"trailing comma", original.upToLastOccurrenceOf("}", false, false) + ",}"},
        {"unknown escape", withName(R"("bad\q")")},
        {"lone high surrogate", withName(R"("\ud800")")},
        {"lone low surrogate", withName(R"("\udfff")")},
        {"embedded NUL", withName(R"("bad\u0000name")")},
        {"integer overflow",
         original.replace("\"schema\": 1", "\"schema\": 18446744073709551617")}};
    juce::TemporaryFile temporary(".json");
    for (const auto &[name, text] : malformed)
    {
        CAPTURE(name.toStdString());
        REQUIRE(text != original);
        REQUIRE(temporary.getFile().replaceWithText(text));
        juce::var loaded = data;
        juce::String error;
        CHECK_FALSE(Surge::GUI::Reference::loadManifest(temporary.getFile(), loaded, error));
        CHECK(loaded.isVoid());
        CHECK(error.isNotEmpty());
    }

    // A valid supplementary Unicode character is not the same as an unpaired
    // surrogate. Rejecting all escaped Unicode would silently break valid names.
    REQUIRE(temporary.getFile().replaceWithText(withName(R"("Orbit \ud83c\udf19")")));
    juce::var loaded;
    juce::String error;
    REQUIRE(Surge::GUI::Reference::loadManifest(temporary.getFile(), loaded, error));
    CHECK(error.isEmpty());
    CHECK(loaded["name"].toString().startsWith("Orbit "));
}

TEST_CASE("Reference menu indices round trip through native integer ranges",
          "[reference-workspaces][reference-bindings][reference-menus]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    REQUIRE(processor.surge != nullptr);
    ParameterBridge bridge(processor);
    std::set<juce::String> bindings;
    for (const auto &path : workspaceManifests())
    {
        const auto manifest = readManifest(path);
        for (const auto &control : *manifest["controls"].getArray())
            if (control["kind"].toString() == "menu")
            {
                bindings.insert(control["binding"].toString());
                // Source/FX tabs reuse stable menus, so all of their alternate
                // native ranges are part of menu coverage as well as defaults.
                if (const auto *alternates = control["bindings"].getDynamicObject())
                    for (const auto &entry : alternates->getProperties())
                        if (entry.value.isString())
                            bindings.insert(entry.value.toString());
            }
    }
    // Require meaningful menu families instead of a layout-dependent total.
    // Consolidating repeated controls into source tabs must not remove coverage,
    // and adding unrelated menus must not conceal a missing algorithm range.
    const auto requireMenu = [&bindings](const juce::String &binding) {
        CAPTURE(binding.toStdString());
        REQUIRE(bindings.count(binding) == 1);
    };
    for (int oscillator = 1; oscillator <= 3; ++oscillator)
        requireMenu("osc" + juce::String(oscillator) + ".type");
    for (int filter = 1; filter <= 2; ++filter)
    {
        requireMenu("filter" + juce::String(filter) + ".type");
        requireMenu("filter" + juce::String(filter) + ".subtype");
        requireMenu("env" + juce::String(filter) + ".mode");
    }
    for (int effect = 1; effect <= 8; ++effect)
        requireMenu("fx" + juce::String(effect) + ".type");
    for (int lfo = 1; lfo <= 4; ++lfo)
    {
        requireMenu("lfo" + juce::String(lfo) + ".shape");
        requireMenu("lfo" + juce::String(lfo) + ".trigmode");
    }
    for (const auto *binding : {"routing", "scene.mode", "scene.polymode", "scene.polylimit"})
        requireMenu(binding);

    // Exercise signed, positive-offset and boolean normalization independently
    // of whether the current layouts render these parameters as menus or values.
    // Merely checking octave's minimum would not test its choice conversion.
    for (int oscillator = 1; oscillator <= 3; ++oscillator)
    {
        const auto prefix = "osc" + juce::String(oscillator);
        const auto octave = prefix + ".octave";
        REQUIRE(bridge.parameter(octave) != nullptr);
        REQUIRE(bridge.parameter(octave)->val_min.i < 0);
        bindings.insert(octave);
        bindings.insert(prefix + ".keytrack");
        bindings.insert(prefix + ".mute");
    }
    REQUIRE(bridge.parameter("scene.polylimit")->val_min.i > 0);

    const auto roundTripChoices = [&bridge, &processor](const juce::String &binding) {
        CAPTURE(binding.toStdString());
        REQUIRE(bridge.valid(binding));
        REQUIRE(bridge.enabled(binding));
        auto *parameter = bridge.parameter(binding);
        REQUIRE(parameter != nullptr);
        auto *adapter = adapterFor(processor, bridge, binding);
        REQUIRE(adapter != nullptr);
        GestureProbe probe(*adapter);
        const auto original = bridge.value(binding);
        const auto choices = bridge.choices(binding);
        REQUIRE(!choices.isEmpty());
        for (int index = 0; index < choices.size(); ++index)
        {
            CAPTURE(index, choices[index].toStdString());
            REQUIRE(choices[index].isNotEmpty());
            const auto normalized = bridge.choiceValue(binding, index);
            REQUIRE(std::isfinite(normalized));
            REQUIRE(normalized >= 0.f);
            REQUIRE(normalized <= 1.f);
            edit(bridge, binding, normalized);
            CHECK(bridge.choiceIndex(binding) == index);
            if (parameter->valtype == vt_bool)
                CHECK(parameter->val.b == (index != 0));
            else
                CHECK(parameter->val.i == parameter->val_min.i + index);
            CHECK(bridge.activeGestureCount() == 0);
            CHECK_FALSE(adapter->inEditGesture.load());
        }
        edit(bridge, binding, original);
        CHECK(probe.starts == choices.size() + 1);
        CHECK(probe.ends == probe.starts);
        CHECK(probe.balance == 0);
        CHECK(probe.minimumBalance == 0);
    };

    for (int scene = 0; scene < n_scenes; ++scene)
    {
        CAPTURE(scene);
        bridge.setScene(scene);
        for (const auto &binding : bindings)
        {
            auto *parameter = bridge.parameter(binding);
            REQUIRE(parameter != nullptr);
            // Global/FX bindings target the same parameter in both scenes.
            if (scene == 0 || parameter->scene != 0)
                roundTripChoices(binding);
        }

        // Subtype counts are algorithm-dependent, unlike the declared parameter
        // storage range. Exercise every advertised subtype for every filter type.
        for (const auto *prefix : {"filter1", "filter2"})
        {
            const auto type = juce::String(prefix) + ".type";
            const auto subtype = juce::String(prefix) + ".subtype";
            const auto original = bridge.value(type);
            const auto types = bridge.choices(type);
            for (int index = 0; index < types.size(); ++index)
            {
                CAPTURE(type.toStdString(), index);
                edit(bridge, type, bridge.choiceValue(type, index));
                roundTripChoices(subtype);
            }
            edit(bridge, type, original);
        }
    }
}

TEST_CASE("Reference edits survive native processor state restore",
          "[reference-workspaces][reference-state]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    REQUIRE(processor.surge != nullptr);
    ParameterBridge bridge(processor);
    bridge.setScene(0);
    edit(bridge, "filter1.cutoff", 0.412f);
    edit(bridge, "macro1", 0.62f);
    edit(bridge, "osc2.pitch", 0.591f);
    edit(bridge, "env1.attack", 0.247f);
    edit(bridge, "lfo1.rate", 0.427f);
    bridge.setScene(1);
    edit(bridge, "filter1.cutoff", 0.691f);
    edit(bridge, "osc3.pitch", 0.381f);
    edit(bridge, "env2.release", 0.671f);
    for (int index = 0; index < 8; ++index)
        edit(bridge, "macro" + juce::String(index + 1), 0.12f + 0.1f * index);

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    REQUIRE(state.getSize() > 0);
    const auto expected = hostParameterValues(processor);
    REQUIRE(expected.size() > 100);
    edit(bridge, "filter1.cutoff", 0.17f);
    edit(bridge, "osc3.pitch", 0.52f);
    edit(bridge, "env2.release", 0.2f);
    bridge.setScene(0);
    edit(bridge, "filter1.cutoff", 0.83f);
    edit(bridge, "osc2.pitch", 0.4f);
    edit(bridge, "env1.attack", 0.7f);
    edit(bridge, "lfo1.rate", 0.1f);
    for (int index = 0; index < 8; ++index)
        edit(bridge, "macro" + juce::String(index + 1), 0.96f - 0.1f * index);

    // The engine has not started an audio callback in this test, so the processor's
    // existing idle-engine state restore path deserializes the queued state safely.
    processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    // Compare every host-visible value, including currently hidden parameters;
    // restoring only the handful of displayed controls is not sufficient.
    checkHostParameterValues(processor, expected);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.412f).margin(0.001f));
    CHECK(bridge.value("macro1") == Catch::Approx(0.12f).margin(0.001f));
    bridge.setScene(1);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.691f).margin(0.001f));
}

TEST_CASE("The native editor switches and reloads reference skins without changing the patch",
          "[reference-workspaces][reference-editor][reference-lifecycle]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto manifests = workspaceManifests();
    REQUIRE(manifests.size() == 10);
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    REQUIRE(processor.surge != nullptr);
    ParameterBridge bridge(processor);
    edit(bridge, "filter1.cutoff", 0.423f);
    edit(bridge, "macro1", 0.273f);
    bridge.setScene(1);
    edit(bridge, "filter1.cutoff", 0.672f);

    for (int lifecycle = 0; lifecycle < 2; ++lifecycle)
    {
        CAPTURE(lifecycle);
        // A real JUCE plug-in wrapper must notify the processor before deleting
        // its editor. Reproduce that host contract, including early test exits.
        const auto deleteAsHost = [&processor](juce::AudioProcessorEditor *editor) {
            if (editor != nullptr)
            {
                processor.editorBeingDeleted(editor);
                delete editor;
            }
        };
        std::unique_ptr<juce::AudioProcessorEditor, decltype(deleteAsHost)> owner(
            processor.createEditorIfNeeded(), deleteAsHost);
        auto *editor = dynamic_cast<SurgeSynthEditor *>(owner.get());
        REQUIRE(editor != nullptr);
        REQUIRE(processor.getActiveEditor() == editor);
        editor->addToDesktop(0);
        editor->setVisible(true);
        const auto stock = Surge::GUI::SkinDB::get()->getDefaultSkinEntry();

        // The first lifecycle covers every package in one live host editor; the
        // second proves reconstruction against the same processor and patch.
        const int count = lifecycle == 0 ? manifests.size() : 1;
        for (int index = 0; index < count; ++index)
        {
            const auto &path = manifests[index];
            const auto manifest = readManifest(path);
            CAPTURE(manifest["id"].toString().toStdString());
            const auto expected = hostParameterValues(processor);
            const bool dirty = processor.surge->storage.getPatch().isDirty;
            processor.surge->release_if_latched[0] = false;
            processor.surge->release_if_latched[1] = false;
            editor->selectSkin(skinEntry(path));
            INFO(editor->getReferenceWorkspaceError().toStdString());
            REQUIRE(editor->getReferenceWorkspaceError().isEmpty());
            auto *workspace = editor->getReferenceWorkspace();
            REQUIRE(workspace != nullptr);
            CHECK(workspace->currentId() == manifest["id"].toString());
            CHECK(workspace->isVisible());
            CHECK(workspace->getWidth() > 0);
            CHECK(workspace->getHeight() > 0);
            checkHostParameterValues(processor, expected);
            CHECK(processor.surge->storage.getPatch().isDirty == dirty);
            CHECK_FALSE(processor.surge->release_if_latched[0]);
            CHECK_FALSE(processor.surge->release_if_latched[1]);

            // The real full-editor action and return must retain the selected
            // native workspace, scene and all hidden host parameters.
            auto *full =
                dynamic_cast<juce::Button *>(findComponent(*workspace, "workspace.full-editor"));
            REQUIRE(full != nullptr);
            REQUIRE(static_cast<bool>(full->onClick));
            full->onClick();
            CHECK_FALSE(workspace->isVisible());
            editor->showReferenceWorkspace(true);
            CHECK(workspace->isVisible());
            checkHostParameterValues(processor, expected);
            CHECK_FALSE(processor.surge->release_if_latched[0]);
            CHECK_FALSE(processor.surge->release_if_latched[1]);

            if (index == 0 || index == count - 1)
            {
                // Explicit skin reload replaces the component. Never retain a
                // raw pointer to a control or bridge across this operation.
                juce::Component::SafePointer<Workspace> previous(workspace);
                editor->resetWindowFromSkin();
                CHECK(previous == nullptr);
                workspace = editor->getReferenceWorkspace();
                REQUIRE(workspace != nullptr);
                CHECK(workspace->currentId() == manifest["id"].toString());
                checkHostParameterValues(processor, expected);
                CHECK_FALSE(processor.surge->release_if_latched[0]);
                CHECK_FALSE(processor.surge->release_if_latched[1]);
            }

            if (index == 0)
            {
                // Put the real editor in scene B before the remaining theme
                // switches. This catches redundant scene-change actions that
                // would release latched voices despite no sound-design edit.
                workspace->selectScene(1);
                CHECK(workspace->bridge().getScene() == 1);
                CHECK(processor.surge->storage.getPatch().scene_active.val.i == 1);
            }
        }

        const auto expected = hostParameterValues(processor);
        juce::Component::SafePointer<Workspace> previous(editor->getReferenceWorkspace());
        editor->selectSkin(stock);
        CHECK(editor->getReferenceWorkspace() == nullptr);
        CHECK(previous == nullptr);
        CHECK(editor->getReferenceWorkspaceError().isEmpty());
        checkHostParameterValues(processor, expected);
        owner.reset();
        CHECK(processor.getActiveEditor() == nullptr);
        checkHostParameterValues(processor, expected);
    }
}

TEST_CASE("Performance workspace keyboard and XY controls release owned interactions",
          "[reference-workspaces][reference-performance][reference-lifecycle]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto manifests = workspaceManifests();
    juce::File performanceManifest;
    for (const auto &path : manifests)
        if (readManifest(path)["id"].toString() == "10-live-performance-deck")
            performanceManifest = path;
    REQUIRE(performanceManifest.existsAsFile());
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    REQUIRE(processor.surge != nullptr);
    auto workspace = std::make_unique<Workspace>(processor);
    juce::String error;
    REQUIRE(workspace->load(performanceManifest, error));
    workspace->setSize(referenceWidth, referenceHeight);
    workspace->addToDesktop(0);
    workspace->setVisible(true);
    auto &bridge = workspace->bridge();
    REQUIRE(processor.macrosById.size() == 8);

    // Screen-reader numeric values are normalized, whereas text must be parsed
    // in the parameter's displayed unit; "440 Hz" must never mean normalized 440.
    const auto manifest = readManifest(performanceManifest);
    auto *macroControl = findBindingControl(*workspace, manifest, "macro1");
    REQUIRE(macroControl != nullptr);
    auto *macroSlider = findSlider(*macroControl);
    REQUIRE(macroSlider != nullptr);
    auto *macroHandler = macroSlider->getAccessibilityHandler();
    REQUIRE(macroHandler != nullptr);
    auto *macroValue = macroHandler->getValueInterface();
    REQUIRE(macroValue != nullptr);
    CHECK_FALSE(macroValue->isReadOnly());
    macroValue->setValue(.42);
    CHECK(bridge.value("macro1") == Catch::Approx(.42f).margin(.00001f));
    macroValue->setValueAsString("0.37");
    CHECK(bridge.value("macro1") == Catch::Approx(.37f).margin(.00001f));
    for (const auto *invalid : {"not a number", "NaN", "1.1"})
    {
        macroValue->setValueAsString(invalid);
        CHECK(bridge.value("macro1") == Catch::Approx(.37f).margin(.00001f));
    }
    auto *cutoffControl = findBindingControl(*workspace, manifest, "filter1.cutoff");
    REQUIRE(cutoffControl != nullptr);
    auto *cutoffSlider = findSlider(*cutoffControl);
    REQUIRE(cutoffSlider != nullptr);
    auto *cutoffHandler = cutoffSlider->getAccessibilityHandler();
    REQUIRE(cutoffHandler != nullptr);
    auto *cutoffValue = cutoffHandler->getValueInterface();
    REQUIRE(cutoffValue != nullptr);
    cutoffValue->setValueAsString("440 Hz");
    REQUIRE(bridge.parameter("filter1.cutoff") != nullptr);
    CHECK(bridge.parameter("filter1.cutoff")->val.f == Catch::Approx(0.f).margin(.0001f));
    CHECK(bridge.value("filter1.cutoff") < .9f);
    CHECK(bridge.activeGestureCount() == 0);

    GestureProbe xProbe(*processor.macrosById[0]);
    GestureProbe yProbe(*processor.macrosById[1]);
    auto *xy = findComponent(*workspace, "workspace.macro-xy");
    REQUIRE(xy != nullptr);
    const auto pad = xy->getLocalBounds().toFloat().reduced(3);
    REQUIRE(pad.getWidth() > 0.f);
    REQUIRE(pad.getHeight() > 0.f);
    const auto quarter =
        pad.getTopLeft() + juce::Point<float>(pad.getWidth() * .25f, pad.getHeight() * .25f);
    xy->mouseDown(mouseEvent(*xy, quarter));
    CHECK(bridge.value("macro1") == Catch::Approx(.25f).margin(.00001f));
    CHECK(bridge.value("macro2") == Catch::Approx(.75f).margin(.00001f));
    CHECK(bridge.activeGestureCount() == 2);
    xy->mouseDrag(mouseEvent(*xy, {pad.getRight() + 100.f, pad.getY() - 100.f}));
    CHECK(bridge.value("macro1") == Catch::Approx(1.f));
    CHECK(bridge.value("macro2") == Catch::Approx(1.f));
    xy->mouseUp(mouseEvent(*xy, quarter));
    CHECK(bridge.activeGestureCount() == 0);
    CHECK(xProbe.starts == 1);
    CHECK(xProbe.ends == 1);
    CHECK(yProbe.starts == 1);
    CHECK(yProbe.ends == 1);

    REQUIRE(xy->keyPressed(juce::KeyPress(juce::KeyPress::leftKey)));
    REQUIRE(xy->keyPressed(
        juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier, 0)));
    CHECK(bridge.value("macro1") == Catch::Approx(.99f).margin(.00001f));
    CHECK(bridge.value("macro2") == Catch::Approx(.999f).margin(.00001f));
    CHECK(xProbe.balance == 0);
    CHECK(yProbe.balance == 0);

    // Resize and scene changes must cancel two-axis gestures, not merely hide
    // their display while the host remains in an automation-edit transaction.
    xy->mouseDown(mouseEvent(*xy, quarter));
    workspace->setSize(referenceWidth - 100, referenceHeight - 60);
    CHECK(bridge.activeGestureCount() == 0);
    CHECK(xProbe.balance == 0);
    CHECK(yProbe.balance == 0);
    xy->mouseDown(mouseEvent(*xy, quarter));
    workspace->selectScene(1);
    CHECK(bridge.activeGestureCount() == 0);
    CHECK(xProbe.balance == 0);
    CHECK(yProbe.balance == 0);

    auto *keyboard = dynamic_cast<juce::MidiKeyboardComponent *>(
        findComponent(*workspace, "workspace.performance-keyboard"));
    REQUIRE(keyboard != nullptr);
    REQUIRE(keyboard->getRangeStart() <= 60);
    REQUIRE(keyboard->getRangeEnd() >= 60);
    const auto key = keyboard->getRectangleForKey(60);
    const auto keyPoint = juce::Point<float>(key.getCentreX(), key.getBottom() - 3.f);
    REQUIRE(keyboard->getNoteAndVelocityAtPosition(keyPoint).note == 60);
    keyboard->mouseDown(mouseEvent(*keyboard, keyPoint));
    REQUIRE(processor.midiKeyboardState.isNoteOn(1, 60));
    // A host/external note is not owned by this keyboard and must be untouched.
    processor.midiKeyboardState.noteOn(2, 71, .6f);
    xy->mouseDown(mouseEvent(*xy, quarter));
    workspace->setVisible(false);
    CHECK_FALSE(processor.midiKeyboardState.isNoteOn(1, 60));
    CHECK(processor.midiKeyboardState.isNoteOn(2, 71));
    CHECK(bridge.activeGestureCount() == 0);
    CHECK(xProbe.balance == 0);
    CHECK(yProbe.balance == 0);
    workspace->setVisible(true);
    keyboard->mouseDown(mouseEvent(*keyboard, keyPoint));
    REQUIRE(processor.midiKeyboardState.isNoteOn(1, 60));
    xy->mouseDown(mouseEvent(*xy, quarter));
    juce::Component::SafePointer<juce::Component> oldKeyboard(keyboard);
    juce::Component::SafePointer<juce::Component> oldXY(xy);
    REQUIRE(workspace->load(performanceManifest, error));
    CHECK(oldKeyboard == nullptr);
    CHECK(oldXY == nullptr);
    CHECK_FALSE(processor.midiKeyboardState.isNoteOn(1, 60));
    CHECK(processor.midiKeyboardState.isNoteOn(2, 71));
    CHECK(bridge.activeGestureCount() == 0);
    CHECK(xProbe.balance == 0);
    CHECK(yProbe.balance == 0);

    keyboard = dynamic_cast<juce::MidiKeyboardComponent *>(
        findComponent(*workspace, "workspace.performance-keyboard"));
    REQUIRE(keyboard != nullptr);
    keyboard->mouseDown(mouseEvent(*keyboard, keyPoint));
    REQUIRE(processor.midiKeyboardState.isNoteOn(1, 60));
    workspace.reset();
    CHECK_FALSE(processor.midiKeyboardState.isNoteOn(1, 60));
    CHECK(processor.midiKeyboardState.isNoteOn(2, 71));
    CHECK(xProbe.minimumBalance == 0);
    CHECK(yProbe.minimumBalance == 0);
    processor.midiKeyboardState.noteOff(2, 71, 0.f);
}

TEST_CASE("Preset header shortcuts retain native browser actions and skin visibility",
          "[reference-workspaces][reference-browser][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    processor.surge->storage.getPatch().isDirty = true;
    const auto before = hostParameterValues(processor);
    const auto patchId = processor.surge->patchid;
    Workspace workspace(processor);
    juce::String error;
    const auto live = sourceRoot().getChildFile(
        "resources/data/skins/reference-10-live.surge-skin/workspace.json");
    REQUIRE(workspace.load(live, error));
    workspace.setSize(referenceWidth, referenceHeight);
    workspace.addToDesktop(0);
    workspace.setVisible(true);
    juce::StringArray actions;
    workspace.onAction = [&](const juce::String &action) { actions.add(action); };
    std::array<juce::Button *, 2> buttons{};
    const std::array<juce::String, 2> ids{"workspace.header.preset-search",
                                           "workspace.header.preset-list"};
    for (size_t index = 0; index < ids.size(); ++index)
    {
        CAPTURE(ids[index].toStdString());
        auto *button = dynamic_cast<juce::Button *>(findComponent(workspace, ids[index]));
        REQUIRE(button != nullptr);
        buttons[index] = button;
        CHECK(button->isShowing());
        CHECK(button->isEnabled());
        CHECK(button->getWantsKeyboardFocus());
        CHECK(button->getName().isNotEmpty());
        CHECK(button->getTitle().isNotEmpty());
        CHECK(button->getDescription().isNotEmpty());
        CHECK(button->getTooltip().isNotEmpty());
        auto *handler = button->getAccessibilityHandler();
        REQUIRE(handler != nullptr);
        CHECK(handler->getRole() == juce::AccessibilityRole::button);
        CHECK(handler->getActions().contains(juce::AccessibilityActionType::press));
        REQUIRE(button->getParentComponent() != nullptr);
        CHECK(button->getParentComponent()->getLocalBounds().contains(button->getBounds()));
        REQUIRE(static_cast<bool>(button->onClick));
        button->onClick();
        REQUIRE(actions.size() == static_cast<int>(index + 1));
        CHECK(actions[actions.size() - 1] == "browser");
        // The existing editor owns dirty-patch confirmation and preset loading.
        // A header shortcut only opens that guarded browser path.
        CHECK(processor.surge->storage.getPatch().isDirty);
        CHECK(processor.surge->patchid == patchId);
    }
    CHECK_FALSE(buttons[0]->getBounds().intersects(buttons[1]->getBounds()));
    CHECK(workspace.bridge().activeGestureCount() == 0);
    checkHostParameterValues(processor, before);

    // These controls belong to Live's second header row. Reusing the Workspace
    // for another skin must hide them instead of leaving invisible hit targets.
    REQUIRE(workspace.load(sourceRoot().getChildFile(
        "resources/data/skins/reference-07-aurora.surge-skin/workspace.json"), error));
    for (const auto &id : ids)
    {
        auto *button = findComponent(workspace, id);
        REQUIRE(button != nullptr);
        CHECK_FALSE(button->isVisible());
    }

    // Tokyo's native PRESETS tab occupies the header Browser button's former
    // rectangle. The redundant header target must be hidden while the actual
    // browser and its native tab/full-browser controls remain available.
    REQUIRE(workspace.load(sourceRoot().getChildFile(
        "resources/data/skins/reference-06-tokyo.surge-skin/workspace.json"), error));
    auto *headerBrowser = dynamic_cast<juce::Button *>(
        findComponent(workspace, "workspace.navigation.browser"));
    REQUIRE(headerBrowser != nullptr);
    CHECK_FALSE(headerBrowser->isVisible());
    for (const auto *id : {"workspace.browser", "workspace.browser.tab.0",
                           "workspace.browser.full"})
    {
        auto *component = findComponent(workspace, id);
        REQUIRE(component != nullptr);
        CHECK(component->isShowing());
    }
    REQUIRE(workspace.load(live, error));
    for (const auto &id : ids)
    {
        auto *button = findComponent(workspace, id);
        REQUIRE(button != nullptr);
        CHECK(button->isShowing());
    }
    headerBrowser = dynamic_cast<juce::Button *>(
        findComponent(workspace, "workspace.navigation.browser"));
    REQUIRE(headerBrowser != nullptr);
    CHECK(headerBrowser->isShowing());
    REQUIRE(static_cast<bool>(headerBrowser->onClick));
    CHECK(actions.size() == 2);
    headerBrowser->onClick();
    REQUIRE(actions.size() == 3);
    CHECK(actions[2] == "browser");
    CHECK(processor.surge->storage.getPatch().isDirty);
    checkHostParameterValues(processor, before);
}

TEST_CASE("Live keyboard range display agrees with native scene routing",
          "[reference-workspaces][reference-performance][reference-display-isolation]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    Workspace workspace(processor);
    juce::String error;
    REQUIRE(workspace.load(sourceRoot().getChildFile(
        "resources/data/skins/reference-10-live.surge-skin/workspace.json"), error));
    workspace.setSize(referenceWidth, referenceHeight);
    workspace.addToDesktop(0);
    workspace.setVisible(true);
    auto *keyboard = dynamic_cast<juce::MidiKeyboardComponent *>(
        findComponent(workspace, "workspace.performance-keyboard"));
    REQUIRE(keyboard != nullptr);
    CHECK(keyboard->getRangeStart() == 0);
    CHECK(keyboard->getRangeEnd() == 120);
    CHECK(keyboard->getKeyWidth() == Catch::Approx(8.f));
    CHECK(keyboard->getMidiChannel() == 1);
    CHECK(static_cast<bool>(keyboard->getProperties()["reference.keyboard-footer-labels"]));
    // Every displayed white key, including the extreme Cs, must still hit its
    // own native MIDI identity after fitting the full ten-octave keyboard.
    int whiteKeys = 0;
    for (int note = keyboard->getRangeStart(); note <= keyboard->getRangeEnd(); ++note)
    {
        if (juce::MidiMessage::isMidiNoteBlack(note))
            continue;
        ++whiteKeys;
        CAPTURE(note);
        const auto key = keyboard->getRectangleForKey(note);
        CHECK(keyboard->getLocalBounds().toFloat().contains(key));
        CHECK(keyboard->getNoteAndVelocityAtPosition(
                  {key.getCentreX(), key.getBottom() - 3.f}).note == note);
    }
    CHECK(whiteKeys == 71);
    auto &bridge = workspace.bridge();
    const auto hostInteger = [&](const juce::String &binding, int value) {
        auto *parameter = bridge.parameter(binding);
        auto *adapter = adapterFor(processor, bridge, binding);
        REQUIRE(parameter != nullptr);
        REQUIRE(adapter != nullptr);
        REQUIRE(value >= parameter->val_min.i);
        REQUIRE(value <= parameter->val_max.i);
        adapter->setValueNotifyingHost(
            Parameter::intScaledToFloat(value, parameter->val_max.i, parameter->val_min.i));
        REQUIRE(parameter->val.i == value);
    };
    struct RoutingCase
    {
        int mode, active, split;
    };
    const std::array<RoutingCase, 13> cases{{
        {sm_single, 0, 60}, {sm_single, 1, 60}, {sm_dual, 0, 60}, {sm_dual, 1, 60},
        {sm_split, 0, 0}, {sm_split, 0, 1}, {sm_split, 0, 60}, {sm_split, 1, 127},
        {sm_chsplit, 0, 0}, {sm_chsplit, 0, 7}, {sm_chsplit, 1, 8},
        {sm_chsplit, 0, 64}, {sm_chsplit, 1, 127}}};
    for (const auto &entry : cases)
    {
        CAPTURE(entry.mode, entry.active, entry.split);
        hostInteger("scene.mode", entry.mode);
        hostInteger("scene.active", entry.active);
        hostInteger("scene.splitpoint", entry.split);
        // Merely selecting the other editing scene must not turn its range
        // into the sounding scene. Scene-active is a separate native parameter.
        workspace.selectScene(1 - entry.active, false);
        const auto before = hostParameterValues(processor);
        const auto randomState = processor.surge->storage.rngGen.g;
        const bool dirty = processor.surge->storage.getPatch().isDirty;
        workspace.refresh();
        REQUIRE(workspace.createComponentSnapshot(workspace.getLocalBounds()).isValid());
        const auto &properties = keyboard->getProperties();
        CHECK(static_cast<int>(properties["reference.keyboard-mode"]) == entry.mode);
        CHECK(static_cast<int>(properties["reference.keyboard-active-scene"]) == entry.active);
        CHECK(static_cast<int>(properties["reference.keyboard-split-point"]) == entry.split);
        CHECK(static_cast<int>(properties["reference.keyboard-channel"]) ==
              keyboard->getMidiChannel());
        CHECK(static_cast<int>(properties["reference.keyboard-octave-label-count"]) == 11);
        juce::StringArray expectedLabels;
        for (int note = 0; note <= 120; note += 12)
            expectedLabels.add(juce::MidiMessage::getMidiNoteName(
                note, true, true, keyboard->getOctaveForMiddleC()));
        CHECK(properties["reference.keyboard-octave-labels"].toString() ==
              expectedLabels.joinIntoString(" "));
        CHECK(keyboard->getDescription().isNotEmpty());
        const auto *ranges = properties["reference.keyboard-ranges"].getArray();
        REQUIRE(ranges != nullptr);
        REQUIRE(!ranges->isEmpty());
        for (const auto &range : *ranges)
        {
            const auto *fields = range.getArray();
            REQUIRE(fields != nullptr);
            REQUIRE(fields->size() == 5);
            CHECK(static_cast<int>((*fields)[0]) >= 0);
            CHECK(static_cast<int>((*fields)[0]) <= 1);
            CHECK(static_cast<int>((*fields)[1]) >= 0);
            CHECK(static_cast<int>((*fields)[2]) <= 127);
            CHECK(static_cast<int>((*fields)[1]) <= static_cast<int>((*fields)[2]));
            CHECK(static_cast<int>((*fields)[3]) >= 1);
            CHECK(static_cast<int>((*fields)[4]) <= 16);
            CHECK(static_cast<int>((*fields)[3]) <= static_cast<int>((*fields)[4]));
        }
        // Use the native routing function as the oracle, including notes above
        // the visual keyboard and both sides of each key/channel boundary.
        // Channel Split describes all channels; other modes describe this
        // keyboard's current MIDI channel, which is one in the native surface.
        const int firstChannel = entry.mode == sm_chsplit ? 1 : keyboard->getMidiChannel();
        const int lastChannel = entry.mode == sm_chsplit ? 16 : firstChannel;
        for (int channel = firstChannel; channel <= lastChannel; ++channel)
            for (int note = 0; note < 128; ++note)
            {
                CAPTURE(channel, note);
                int displayedMask = 0;
                for (const auto &range : *ranges)
                {
                    const auto &fields = *range.getArray();
                    if (note >= static_cast<int>(fields[1]) &&
                        note <= static_cast<int>(fields[2]) &&
                        channel >= static_cast<int>(fields[3]) &&
                        channel <= static_cast<int>(fields[4]))
                        displayedMask |= 1 << static_cast<int>(fields[0]);
                }
                CHECK(displayedMask == processor.surge->calculateChannelMask(channel - 1, note));
            }
        CHECK(processor.surge->storage.rngGen.g == randomState);
        CHECK(processor.surge->storage.getPatch().isDirty == dirty);
        CHECK(bridge.activeGestureCount() == 0);
        checkHostParameterValues(processor, before);
    }
}

TEST_CASE("Compact value controls preserve real drag and typed-unit editing",
          "[reference-workspaces][reference-values][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    const auto manifests = workspaceManifests();
    REQUIRE(manifests.size() == 10);
    Workspace workspace(processor);
    juce::String error;
    const auto manifest = readManifest(manifests[0]);
    REQUIRE(workspace.load(manifests[0], error));
    workspace.setSize(referenceWidth, referenceHeight);
    workspace.addToDesktop(0);
    workspace.setVisible(true);
    ScopedModalCleanup modalCleanup;
    auto &bridge = workspace.bridge();

    // Obsidian's right-hand ADSR fields now follow native source tabs. Select
    // the actual filter envelope before testing its compact value entry path.
    const juce::String valueBinding = "env2.attack";
    bool selectedFilterEnvelope = false;
    for (const auto &panel : *manifest["panels"].getArray())
        if (auto *choices = panel["selection"]["choices"].getArray())
            for (const auto &choice : *choices)
                if (choice["source"].toString() == "envelope" &&
                    static_cast<int>(choice["index"]) == 2)
                    selectedFilterEnvelope = workspace.selectView(
                        panel["selection"]["id"].toString(), choice["id"].toString());
    REQUIRE(selectedFilterEnvelope);
    juce::String valueId;
    for (const auto &control : *manifest["controls"].getArray())
        if (control["kind"].toString() == "value")
            if (auto *component = findComponent(workspace, control["id"].toString()))
                if (component->getProperties()["reference.binding"].toString() == valueBinding)
                    valueId = control["id"].toString();
    REQUIRE(valueId.isNotEmpty());
    auto *slider = dynamic_cast<juce::Slider *>(findComponent(workspace, valueId));
    REQUIRE(slider != nullptr);
    REQUIRE(slider->isEnabled());
    REQUIRE(slider->getWantsKeyboardFocus());
    auto *adapter = adapterFor(processor, bridge, valueBinding);
    REQUIRE(adapter != nullptr);
    GestureProbe probe(*adapter);
    edit(bridge, valueBinding, .4f);
    workspace.refresh();
    const auto before = bridge.value(valueBinding);
    const auto start = slider->getLocalBounds().toFloat().getCentre();
    const auto down = mouseEvent(*slider, start);
    slider->mouseDown(down);
    slider->mouseDrag(down.withNewPosition(start.translated(0, -25.f)));
    slider->mouseUp(down.withNewPosition(start.translated(0, -25.f)));
    CHECK(std::abs(bridge.value(valueBinding) - before) > .01f);
    CHECK(bridge.activeGestureCount() == 0);
    CHECK(probe.starts == probe.ends);
    CHECK(probe.minimumBalance == 0);

    // Open the actual nonblocking callout via its keyboard entry point; this is
    // not a direct call to ParameterBridge::setText disguised as a UI test.
    REQUIRE(slider->keyPressed(juce::KeyPress(juce::KeyPress::returnKey)));
    auto *input =
        dynamic_cast<juce::TextEditor *>(findComponent(workspace, "workspace.value-entry.input"));
    auto *apply =
        dynamic_cast<juce::Button *>(findComponent(workspace, "workspace.value-entry.apply"));
    REQUIRE(input != nullptr);
    REQUIRE(apply != nullptr);
    REQUIRE(static_cast<bool>(apply->onClick));
    const auto beforeInvalid = bridge.value(valueBinding);
    input->setText("this is not a duration", false);
    apply->onClick();
    CHECK(bridge.value(valueBinding) == Catch::Approx(beforeInvalid));
    CHECK(input->getDescription().isNotEmpty());
    input->setText("250 ms", false);
    apply->onClick();
    REQUIRE(bridge.parameter(valueBinding) != nullptr);
    CHECK(bridge.parameter(valueBinding)->val.f == Catch::Approx(-2.f).margin(.0001f));
    CHECK(probe.starts == probe.ends);
    CHECK(probe.balance == 0);
    CHECK(bridge.activeGestureCount() == 0);
    CHECK_FALSE(adapter->inEditGesture.load());
}

TEST_CASE("Typed native values reject missing numbers and retain native units and names",
          "[reference-workspaces][reference-interactions][reference-typed-values]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    ParameterBridge bridge(processor);
    juce::String error;
    REQUIRE(bridge.enabled("env2.attack"));
    REQUIRE(bridge.setText("env2.attack", "250 ms", error));
    const auto expected = bridge.value("env2.attack");
    for (const auto *input : {"", "   ", "this is not a duration", "ms", "NaN", "inf", "-inf"})
    {
        CAPTURE(input);
        CHECK_FALSE(bridge.setText("env2.attack", input, error));
        CHECK(error.isNotEmpty());
        CHECK(bridge.value("env2.attack") == Catch::Approx(expected));
    }
    REQUIRE(bridge.setText("env2.attack", "0.5 s", error));
    CHECK(bridge.parameter("env2.attack")->val.f == Catch::Approx(-1.f).margin(.0001f));
    REQUIRE(bridge.setText("env2.attack", "0 ms", error));
    CHECK(bridge.parameter("env2.attack")->val.f ==
          Catch::Approx(bridge.parameter("env2.attack")->val_min.f));

    REQUIRE(bridge.setText("filter1.cutoff", "440 Hz", error));
    CHECK(bridge.parameter("filter1.cutoff")->val.f == Catch::Approx(0.f).margin(.0001f));
    REQUIRE(bridge.setText("filter1.cutoff", "A4", error));
    const auto noteValue = bridge.value("filter1.cutoff");
    for (const auto *input : {"garbage", "banana", "A bad note", "C#x"})
    {
        CAPTURE(input);
        CHECK_FALSE(bridge.setText("filter1.cutoff", input, error));
        CHECK(bridge.value("filter1.cutoff") == Catch::Approx(noteValue));
    }
    REQUIRE(bridge.setText("osc1.retrigger", "On", error));
    CHECK(bridge.value("osc1.retrigger") == Catch::Approx(1.f));
    REQUIRE(bridge.setText("osc1.retrigger", "Off", error));
    CHECK(bridge.value("osc1.retrigger") == Catch::Approx(0.f));
    const auto modes = bridge.choices("scene.mode");
    REQUIRE(!modes.isEmpty());
    REQUIRE(bridge.setText("scene.mode", modes[0], error));
    CHECK(bridge.choiceIndex("scene.mode") == 0);
    CHECK(bridge.activeGestureCount() == 0);
}

TEST_CASE("Compact blank captions retain native accessible parameter names",
          "[reference-workspaces][reference-accessibility]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    int blankCaptions = 0;
    for (const auto &file : workspaceManifests())
    {
        const auto manifest = readManifest(file);
        Workspace workspace(processor);
        juce::String error;
        REQUIRE(workspace.load(file, error));
        workspace.setSize(referenceWidth, referenceHeight);
        auto &bridge = workspace.bridge();
        for (const auto &control : *manifest["controls"].getArray())
        {
            auto *slider = dynamic_cast<juce::Slider *>(
                findComponent(workspace, control["id"].toString()));
            if (slider == nullptr)
                continue;
            CAPTURE(manifest["id"].toString().toStdString(),
                    control["id"].toString().toStdString());
            CHECK(slider->getName().isNotEmpty());
            CHECK(slider->getTitle().isNotEmpty());
            CHECK(slider->getDescription().isNotEmpty());
            if (control.hasProperty("label") && control["label"].toString().trim().isEmpty())
            {
                ++blankCaptions;
                const auto binding = slider->getProperties()["reference.binding"].toString();
                REQUIRE(bridge.label(binding).isNotEmpty());
                CHECK(slider->getName().startsWith(bridge.label(binding)));
            }
        }
    }
    // The seven Live header/oscillator readouts intentionally suppress face
    // captions. Their accessible names must survive that visual treatment.
    CHECK(blankCaptions >= 7);
}

TEST_CASE("Native preset scopes preserve catalog partitions and stable patch identities",
          "[reference-workspaces][reference-browser]")
{
    using Surge::GUI::Reference::BrowserFilter;
    using Surge::GUI::Reference::browserRows;
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    installBrowserTestCatalog(processor, testData.directory);
    const auto before = hostParameterValues(processor);
    const auto ids = [](const auto &rows) {
        std::vector<int> result;
        for (const auto &row : rows)
            if (!row.category)
                result.push_back(row.patchId);
        return result;
    };
    CHECK(ids(browserRows(processor, "Test", 100, BrowserFilter::all)) ==
          std::vector<int>{4, 2, 0, 5, 1, 3});
    CHECK(ids(browserRows(processor, "test", 100, BrowserFilter::factory)) ==
          std::vector<int>{0, 1});
    CHECK(ids(browserRows(processor, "Test", 100, BrowserFilter::user)) ==
          std::vector<int>{4, 5, 3});
    CHECK(ids(browserRows(processor, {}, 100, BrowserFilter::favorites)) ==
          std::vector<int>{4, 0});
    CHECK(ids(browserRows(processor, {}, 100, BrowserFilter::all, 0)) ==
          std::vector<int>{0, 1});
    CHECK(ids(browserRows(processor, {}, 100, BrowserFilter::all, 3)) ==
          std::vector<int>{4, 3});
    CHECK(ids(browserRows(processor, "Shared/Child", 100)) == std::vector<int>{4, 1});
    CHECK(ids(browserRows(processor, {}, 100, BrowserFilter::favorites, 0)) ==
          std::vector<int>{0});
    CHECK(browserRows(processor, "Test", 0).empty());
    CHECK(browserRows(processor, "No matching name", 100).empty());
    CHECK(ids(browserRows(processor, "Test", 1)) == std::vector<int>{4});

    const auto factory = browserRows(processor, {}, 100, BrowserFilter::factory);
    REQUIRE(factory.size() == 3);
    CHECK(factory.front().category);
    CHECK(factory.front().categoryId == 0);
    CHECK(factory.front().patchCount == 2);
    const auto user = browserRows(processor, {}, 100, BrowserFilter::user);
    std::vector<int> userFolders;
    for (const auto &row : user)
        if (row.category)
        {
            userFolders.push_back(row.categoryId);
            CHECK(row.patchCount == (row.categoryId == 3 ? 2 : 1));
        }
    CHECK(userFolders == std::vector<int>{3, 5});
    CHECK(processor.surge->patchid == 0);
    checkHostParameterValues(processor, before);
}

TEST_CASE("Native browser tabs and keyboard navigation retain the guarded load action",
          "[reference-workspaces][reference-browser][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    installBrowserTestCatalog(processor, testData.directory);
    processor.surge->storage.getPatch().isDirty = true;
    const auto before = hostParameterValues(processor);
    Workspace workspace(processor);
    juce::String error;
    const auto manifest = sourceRoot().getChildFile(
        "resources/data/skins/reference-07-aurora.surge-skin/workspace.json");
    REQUIRE(workspace.load(manifest, error));
    workspace.setSize(referenceWidth, referenceHeight);
    workspace.addToDesktop(0);
    workspace.setVisible(true);
    juce::StringArray actions;
    workspace.onAction = [&](const juce::String &action) { actions.add(action); };
    auto *browser = findComponent(workspace, "workspace.browser");
    auto *search = dynamic_cast<juce::TextEditor *>(
        findComponent(workspace, "workspace.browser.search"));
    auto *list = dynamic_cast<juce::ListBox *>(
        findComponent(workspace, "workspace.browser.list"));
    auto *back = dynamic_cast<juce::Button *>(
        findComponent(workspace, "workspace.browser.back"));
    auto *full = dynamic_cast<juce::Button *>(
        findComponent(workspace, "workspace.browser.full"));
    REQUIRE(browser != nullptr);
    REQUIRE(search != nullptr);
    REQUIRE(list != nullptr);
    REQUIRE(back != nullptr);
    REQUIRE(full != nullptr);
#if JUCE_MAJOR_VERSION >= 8
    auto *model = list->getListBoxModel();
#else
    auto *model = list->getModel();
#endif
    REQUIRE(model != nullptr);
    REQUIRE(static_cast<bool>(search->onTextChange));
    REQUIRE(static_cast<bool>(search->onReturnKey));
    const auto query = [&](const juce::String &value) {
        search->setText(value, false);
        search->onTextChange();
    };
    const auto tab = [&](int index) {
        auto *button = dynamic_cast<juce::Button *>(findComponent(
            workspace, "workspace.browser.tab." + juce::String(index)));
        REQUIRE(button != nullptr);
        REQUIRE(static_cast<bool>(button->onClick));
        CHECK(button->getName().isNotEmpty());
        CHECK(button->getTitle().isNotEmpty());
        CHECK(button->getWantsKeyboardFocus());
        CHECK(browser->getLocalBounds().contains(button->getBounds()));
        button->onClick();
        CHECK(button->getToggleState());
    };
    query("Test");
    const std::array<int, 4> expectedCounts{6, 2, 3, 2};
    for (int index = 0; index < 4; ++index)
    {
        tab(index);
        CHECK(model->getNumRows() == expectedCounts[index]);
    }
    tab(0);
    query({});
    const auto allRows = model->getNumRows();
    REQUIRE(model->getNameForRow(0) == "Category: Shared");
    CHECK_FALSE(back->isVisible());
    list->selectRow(0);
    REQUIRE(list->keyPressed(juce::KeyPress(juce::KeyPress::returnKey)));
    CHECK(back->isVisible());
    CHECK(model->getNumRows() == 2);
    CHECK(model->getNameForRow(0) == "Preset: Test Factory Root");
    CHECK(model->getNameForRow(1) == "Preset: Test Factory Child");
    REQUIRE(static_cast<bool>(back->onClick));
    back->onClick();
    CHECK_FALSE(back->isVisible());
    CHECK(model->getNumRows() == allRows);
    CHECK(actions.isEmpty());

    query("Test User Child");
    REQUIRE(model->getNumRows() == 1);
    search->onReturnKey();
    REQUIRE(actions.size() == 1);
    // The matching row is zero, but its native catalog identity is four. The
    // callback belongs to the existing editor dirty-patch confirmation path.
    CHECK(actions[0] == "load-preset:4");
    CHECK(processor.surge->patchid == 0);
    CHECK(processor.surge->storage.getPatch().isDirty);
    REQUIRE(static_cast<bool>(full->onClick));
    full->onClick();
    REQUIRE(actions.size() == 2);
    CHECK(actions[1] == "browser");
    for (auto *component : {static_cast<juce::Component *>(search),
                            static_cast<juce::Component *>(list),
                            static_cast<juce::Component *>(full)})
    {
        CHECK(component->getName().isNotEmpty());
        CHECK(component->getTitle().isNotEmpty());
        CHECK(component->getWantsKeyboardFocus());
        CHECK(browser->getLocalBounds().contains(component->getBounds()));
    }
    CHECK(workspace.bridge().activeGestureCount() == 0);
    checkHostParameterValues(processor, before);
}

TEST_CASE("Live display refresh preserves audible state and independent audio samples",
          "[reference-workspaces][reference-audio][reference-display-isolation]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor withUI, withoutUI;
    testData.attach(withUI, "with-ui");
    testData.attach(withoutUI, "without-ui");
    const auto patch =
        sourceRoot().getChildFile("resources/data/patches_factory/Templates/Init Wavetable.fxp");
    const auto table = sourceRoot().getChildFile("resources/data/wavetables/Generated/Formants.wt");
    REQUIRE(patch.existsAsFile());
    REQUIRE(table.existsAsFile());
    for (auto *processor : {&withUI, &withoutUI})
    {
        REQUIRE(processor->surge->loadPatchByPath(patch.getFullPathName().toRawUTF8(), -1,
                                                  "Reference isolation fixture"));
        auto &oscillator = processor->surge->storage.getPatch().scene[0].osc[0];
        processor->surge->storage.load_wt(table.getFullPathName().toStdString(), &oscillator.wt,
                                          &oscillator);
        REQUIRE(oscillator.wt.n_tables > 1);
        processor->setRateAndBufferSizeDetails(sampleRate, sampleBlock);
        processor->prepareToPlay(sampleRate, sampleBlock);
        processor->surge->storage.rngGen.g.seed(0x5a117u);
        processor->surge->storage.noiseRng.reseed(0x6b228u);
    }
    const auto manifests = workspaceManifests();
    REQUIRE(manifests.size() == 10);
    Workspace workspace(withUI);
    juce::String error;
    REQUIRE(workspace.load(manifests[0], error));
    workspace.setSize(referenceWidth, referenceHeight);
    workspace.addToDesktop(0);
    workspace.setVisible(true);
    const auto expected = hostParameterValues(withUI);
    checkHostParameterValues(withoutUI, expected);

    juce::AudioBuffer<float> uiAudio(6, sampleBlock), controlAudio(6, sampleBlock);
    float audiblePeak = 0.f, maximumDifference = 0.f;
    const auto started = juce::Time::getMillisecondCounterHiRes();
    for (int block = 0; block < 24; ++block)
    {
        juce::MidiBuffer uiMidi, controlMidi;
        if (block == 0)
        {
            const auto note = juce::MidiMessage::noteOn(1, 60, .8f);
            uiMidi.addEvent(note, 0);
            controlMidi.addEvent(note, 0);
        }
        uiAudio.clear();
        controlAudio.clear();
        withUI.processBlock(uiAudio, uiMidi);
        withoutUI.processBlock(controlAudio, controlMidi);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < sampleBlock; ++sample)
            {
                const auto a = uiAudio.getSample(channel, sample);
                const auto b = controlAudio.getSample(channel, sample);
                REQUIRE(std::isfinite(a));
                REQUIRE(std::isfinite(b));
                audiblePeak = std::max(audiblePeak, std::abs(a));
                maximumDifference = std::max(maximumDifference, std::abs(a - b));
            }
        const auto audibleRng = withUI.surge->storage.rngGen.g;
        workspace.refresh();
        // The preview has private model state; it must not advance the audible
        // oscillator/modulation RNG merely because a window was repainted.
        CHECK(withUI.surge->storage.rngGen.g == audibleRng);
        if (block == 8 || block == 16)
            REQUIRE(workspace.createComponentSnapshot(workspace.getLocalBounds()).isValid());
    }
    const auto elapsed = juce::Time::getMillisecondCounterHiRes() - started;
    CHECK(std::isfinite(elapsed));
    CHECK(elapsed >= 0.0);
    CHECK(audiblePeak > .0001f);
    CHECK(maximumDifference < .000001f);
    checkHostParameterValues(withUI, expected);
    CHECK(workspace.bridge().activeGestureCount() == 0);
    withUI.reset();
    withoutUI.reset();
    withUI.releaseResources();
    withoutUI.releaseResources();
}

TEST_CASE("Wavetable terrain snapshots own distinct real source frames without editing the synth",
          "[reference-workspaces][reference-display-isolation][reference-wavetable]")
{
    using Surge::GUI::Reference::Visuals;
    using Surge::GUI::Reference::WavetablePreview;
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    const auto patch =
        sourceRoot().getChildFile("resources/data/patches_factory/Templates/Init Wavetable.fxp");
    const auto file = sourceRoot().getChildFile("resources/data/wavetables/Generated/Formants.wt");
    REQUIRE(patch.existsAsFile());
    REQUIRE(file.existsAsFile());
    REQUIRE(processor.surge->loadPatchByPath(patch.getFullPathName().toRawUTF8(), -1,
                                             "Terrain isolation fixture"));
    auto &storage = processor.surge->storage;
    auto &oscillator = storage.getPatch().scene[0].osc[0];
    storage.load_wt(file.getFullPathName().toStdString(), &oscillator.wt, &oscillator);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleBlock);
    processor.prepareToPlay(sampleRate, sampleBlock);
    REQUIRE(oscillator.wt.n_tables > 1);
    ParameterBridge bridge(processor);
    edit(bridge, "osc1.param0", .23f);
    const auto state = hostParameterValues(processor);
    const auto rng = storage.rngGen.g;
    const auto first = Visuals::sample(processor, 0);
    const auto &terrain = first.wavetable[0];
    REQUIRE(terrain.available());
    REQUIRE(terrain.sourceFrames > 1);
    CHECK(terrain.frames <= WavetablePreview::maximumFrames);
    CHECK(terrain.sourceFrameSize == oscillator.wt.size);
    CHECK(terrain.position == Catch::Approx(.23f).margin(.0001f));
    CHECK_FALSE(terrain.sampleMode);
    REQUIRE(std::all_of(terrain.samples.begin(), terrain.samples.end(),
                        [](float value) { return std::isfinite(value); }));
    // Endpoints coincide with native table samples without resampling ambiguity.
    CHECK(terrain.samples.front() == oscillator.wt.TableF32WeakPointers[0][0][0]);
    CHECK(terrain.samples.back() ==
          oscillator.wt.TableF32WeakPointers[0][terrain.sourceFrames - 1][oscillator.wt.size - 1]);
    bool distinctFrames = false;
    for (int frame = 1; frame < terrain.frames; ++frame)
        for (int phase = 0; phase < WavetablePreview::pointsPerFrame; ++phase)
            distinctFrames |=
                std::abs(terrain.samples[frame * WavetablePreview::pointsPerFrame + phase] -
                         terrain.samples[phase]) > .001f;
    CHECK(distinctFrames); // A duplicated current waveform cannot satisfy this.
    CHECK(storage.rngGen.g == rng);
    checkHostParameterValues(processor, state);
    const auto repeated = Visuals::sample(processor, 0);
    CHECK(repeated.wavetable[0].samples == terrain.samples);

    edit(bridge, "osc1.param0", .79f);
    const auto moved = Visuals::sample(processor, 0);
    CHECK(moved.wavetable[0].position == Catch::Approx(.79f).margin(.0001f));
    CHECK(moved.wavetable[0].samples == terrain.samples);
    CHECK(terrain.position == Catch::Approx(.23f).margin(.0001f));

    // A loader holding the table mutex must produce an unavailable snapshot,
    // not block on the loader or read partially replaced source data. The bounded
    // worker timeout prevents a regression from hanging the test suite forever.
    std::promise<void> locked, release;
    auto releaseSignal = release.get_future();
    auto holder = std::async(std::launch::async, [&] {
        std::lock_guard<std::mutex> lock(storage.waveTableDataMutex);
        locked.set_value();
        releaseSignal.wait_for(std::chrono::seconds(2));
    });
    locked.get_future().wait();
    const auto busy = Visuals::sample(processor, 0);
    release.set_value();
    holder.get();
    CHECK_FALSE(busy.wavetable[0].available());
    CHECK(Visuals::sample(processor, 0).wavetable[0].available());

    // Replacing the native table cannot alter already-owned preview data.
    const auto saved = terrain.samples;
    const auto replacement =
        sourceRoot().getChildFile("resources/data/wavetables/Generated/Sine One Harmonic.wt");
    REQUIRE(replacement.existsAsFile());
    storage.load_wt(replacement.getFullPathName().toStdString(), &oscillator.wt, &oscillator);
    const auto replaced = Visuals::sample(processor, 0);
    REQUIRE(replaced.wavetable[0].available());
    CHECK(terrain.samples == saved);
    CHECK(replaced.wavetable[0].samples != saved);
    CHECK(storage.rngGen.g == rng);
    processor.releaseResources();
}

TEST_CASE("Native modulation row edits preserve stable targets, undo, and scene isolation",
          "[reference-workspaces][reference-modulation][reference-interactions]")
{
    using Surge::GUI::Reference::ModulationEditorModel;
    using Surge::GUI::Reference::modulationRows;
    using Surge::GUI::Reference::RouteRow;
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleBlock);
    processor.prepareToPlay(sampleRate, sampleBlock);
    configureShowcaseFixture(processor);
    auto &synth = *processor.surge;
    ParameterBridge bridge(processor);
    auto *targetA = bridge.parameter("filter1.cutoff");
    auto *globalTarget = bridge.parameter("fx1.param0");
    REQUIRE(targetA != nullptr);
    REQUIRE(globalTarget != nullptr);
    bridge.setScene(1);
    auto *targetB = bridge.parameter("filter1.cutoff");
    REQUIRE(targetB != nullptr);
    REQUIRE(targetB->id != targetA->id);
    REQUIRE(synth.setModDepth01(targetB->id, ms_ctrl1, 1, 0, -.31f));
    const auto parametersBefore = hostParameterValues(processor);
    const auto rowFor = [&](int scene, int parameterId, modsources source) -> RouteRow {
        const auto rows = modulationRows(processor, scene);
        auto found = std::find_if(rows.begin(), rows.end(), [&](const RouteRow &row) {
            return row.key.parameterId == parameterId && row.key.sourceId == source;
        });
        REQUIRE(found != rows.end());
        return *found;
    };
    const auto a = rowFor(0, targetA->id, ms_ctrl1);
    const auto b = rowFor(1, targetB->id, ms_ctrl1);
    CHECK(a.key.sourceScene == 0);
    CHECK(b.key.sourceScene == 1);
    CHECK(a.key != b.key);

    // Exercise the real undo manager with its actual editor owner, observing
    // JUCE's host deletion contract even if an assertion aborts this test.
    const auto deleteAsHost = [&processor](juce::AudioProcessorEditor *editor) {
        if (editor)
        {
            processor.editorBeingDeleted(editor);
            delete editor;
        }
    };
    std::unique_ptr<juce::AudioProcessorEditor, decltype(deleteAsHost)> editor(
        processor.createEditorIfNeeded(), deleteAsHost);
    REQUIRE(editor != nullptr);
    REQUIRE(processor.undoManager != nullptr);

    struct Listener : SurgeSynthesizer::ModulationAPIListener
    {
        SurgeSynthesizer &synth;
        int begins{0}, ends{0}, sets{0}, mutes{0}, clears{0};
        explicit Listener(SurgeSynthesizer &s) : synth(s) { synth.addModulationAPIListener(this); }
        ~Listener() override { synth.removeModulationAPIListener(this); }
        void modSet(long, modsources, int, int, float, bool) override { ++sets; }
        void modMuted(long, modsources, int, int, bool) override { ++mutes; }
        void modCleared(long, modsources, int, int) override { ++clears; }
        void modBeginEdit(long, modsources, int, int, float) override { ++begins; }
        void modEndEdit(long, modsources, int, int, float) override { ++ends; }
    } listener(synth);

    ModulationEditorModel model(processor);
    REQUIRE(model.begin(a));
    REQUIRE(model.setDepth(.61f));
    REQUIRE(model.setDepth(.42f));
    CHECK_FALSE(model.setDepth(std::numeric_limits<float>::quiet_NaN()));
    model.end();
    CHECK_FALSE(model.isEditing());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(.42f));
    CHECK(synth.getModDepth01(targetB->id, ms_ctrl1, 1, 0) == Catch::Approx(-.31f));
    CHECK(listener.begins == 1);
    CHECK(listener.ends == 1);
    REQUIRE(processor.undoManager->undo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(a.normalizedDepth));
    REQUIRE(processor.undoManager->redo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(.42f));

    REQUIRE(model.setMuted(rowFor(0, targetA->id, ms_ctrl1), true));
    REQUIRE(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    REQUIRE(model.begin(rowFor(0, targetA->id, ms_ctrl1)));
    REQUIRE(model.setDepth(0.f));
    CHECK(synth.getModRouting(targetA->id, ms_ctrl1, 0, 0) == nullptr);
    // Zero is native route removal, but the same drag can cross it and restore
    // that exact route without losing its mute state or editing Scene B.
    REQUIRE(model.setDepth(-.35f));
    model.end();
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(-.35f));
    CHECK(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    const auto negative = rowFor(0, targetA->id, ms_ctrl1);
    REQUIRE(model.remove(negative));
    CHECK_FALSE(model.begin(negative));
    REQUIRE(processor.undoManager->undo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(-.35f));
    CHECK(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    // These are discrete UI gestures even when executed within the old 200 ms
    // merge window: undo the crossed-zero drag, then mute, independently.
    REQUIRE(processor.undoManager->undo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(.42f));
    CHECK(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    REQUIRE(processor.undoManager->undo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(.42f));
    CHECK_FALSE(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    REQUIRE(processor.undoManager->redo());
    CHECK(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    REQUIRE(processor.undoManager->redo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(-.35f));
    CHECK(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    // Redo must transfer those same boundaries, not merge them back together.
    REQUIRE(processor.undoManager->undo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(.42f));
    CHECK(synth.isModulationMuted(targetA->id, ms_ctrl1, 0, 0));
    REQUIRE(processor.undoManager->redo());
    CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(-.35f));

    auto stale = rowFor(0, targetA->id, ms_ctrl1);
    ++stale.key.targetType;
    CHECK_FALSE(model.begin(stale));
    CHECK_FALSE(model.setMuted(stale, false));
    CHECK_FALSE(model.remove(stale));

    // A global macro route keeps its stored source scene when inspected from B.
    // Using the selected display scene instead would silently edit another key.
    const auto globalFromB = rowFor(1, globalTarget->id, ms_ctrl8);
    CHECK(globalFromB.key.sourceScene == 0);
    REQUIRE(model.begin(globalFromB));
    REQUIRE(model.setDepth(.19f));
    model.end();
    CHECK(synth.getModDepth01(globalTarget->id, ms_ctrl8, 0, 0) == Catch::Approx(.19f));

    {
        ModulationEditorModel unfinished(processor);
        REQUIRE(unfinished.begin(b));
        REQUIRE(unfinished.setDepth(-.22f));
        // Closing/rebuilding the editor ends a pending gesture through RAII.
    }

    // Exercise the actual row embedded in ClearSignal, not merely the adapter.
    // Selecting a row and using its accessible depth slider must edit the same
    // Scene B route while leaving the Scene A route and base parameter untouched.
    {
        const auto manifests = workspaceManifests();
        REQUIRE(manifests.size() == 10);
        Workspace workspace(processor);
        juce::String error;
        REQUIRE(workspace.load(manifests[7], error));
        workspace.setSize(referenceWidth, referenceHeight);
        workspace.addToDesktop(0);
        workspace.setVisible(true);
        workspace.selectScene(1);
        workspace.refresh();
        const auto key = b.key;
        const auto suffix = juce::String(key.parameterId) + "." + juce::String(key.sourceId) + "." +
                            juce::String(key.sourceScene) + "." + juce::String(key.sourceIndex);
        auto *row = dynamic_cast<Surge::GUI::Reference::ModulationRowInteraction *>(
            findComponent(workspace, "workspace.modulation.row." + suffix));
        REQUIRE(row != nullptr);
        row->mouseDown(mouseEvent(*row, {5.f, 5.f}));
        auto &slider = row->depthSlider();
        REQUIRE(slider.getAccessibilityHandler() != nullptr);
        REQUIRE(slider.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)));
        CHECK(synth.getModDepth01(targetB->id, ms_ctrl1, 1, 0) == Catch::Approx(-.21f));
        REQUIRE(slider.keyPressed(
            juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::shiftModifier, 0)));
        CHECK(synth.getModDepth01(targetB->id, ms_ctrl1, 1, 0) == Catch::Approx(-.211f));
        CHECK(synth.getModDepth01(targetA->id, ms_ctrl1, 0, 0) == Catch::Approx(-.35f));
        workspace.setVisible(false);
    }
    CHECK(listener.begins == listener.ends);
    CHECK(listener.sets > 0);
    CHECK(listener.mutes > 0);
    CHECK(listener.clears > 0);
    checkHostParameterValues(processor, parametersBefore);
    processor.releaseResources();
}

TEST_CASE("Native source and FX tabs preserve sound and rebind stable controls",
          "[reference-workspaces][reference-selection][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleBlock);
    processor.prepareToPlay(sampleRate, sampleBlock);
    configureShowcaseFixture(processor);
    int groupsTested = 0, mappingsTested = 0, slidersEdited = 0;
    for (const auto &file : workspaceManifests())
    {
        const auto manifest = readManifest(file);
        Workspace workspace(processor);
        juce::String error;
        REQUIRE(workspace.load(file, error));
        workspace.setSize(referenceWidth, referenceHeight);
        workspace.addToDesktop(0);
        workspace.setVisible(true);
        auto &bridge = workspace.bridge();
        const auto baseline = hostParameterValues(processor);
        const auto *panels = manifest["panels"].getArray();
        const auto *controls = manifest["controls"].getArray();
        REQUIRE(panels != nullptr);
        REQUIRE(controls != nullptr);
        for (const auto &panel : *panels)
        {
            const auto selection = panel["selection"];
            if (!selection.isObject())
                continue;
            ++groupsTested;
            const auto group = selection["id"].toString();
            const auto *choices = selection["choices"].getArray();
            REQUIRE(choices != nullptr);
            for (const auto &choice : *choices)
            {
                const auto choiceId = choice["id"].toString();
                CAPTURE(file.getFileName().toStdString(), group.toStdString(),
                        choiceId.toStdString());
                const auto beforeSelection = hostParameterValues(processor);
                REQUIRE(workspace.selectView(group, choiceId));
                CHECK(workspace.selectedView(group) == choiceId);
                CHECK(hostParameterValues(processor) == beforeSelection);
                bool editedThisChoice = false;
                for (const auto &control : *controls)
                {
                    if (control["selectionGroup"].toString() != group)
                        continue;
                    auto *component = findComponent(workspace, control["id"].toString());
                    REQUIRE(component != nullptr);
                    const auto mapped = control["bindings"][juce::Identifier(choiceId)];
                    const auto binding = mapped.isString() ? mapped.toString() : juce::String();
                    CHECK(component->getProperties()["reference.binding"].toString() == binding);
                    CHECK(component->isEnabled() == (binding.isNotEmpty() && bridge.enabled(binding)));
                    ++mappingsTested;
                    auto *slider = dynamic_cast<juce::Slider *>(component);
                    if (editedThisChoice || slider == nullptr || !component->isEnabled())
                        continue;
                    const auto *parameter = bridge.parameter(binding);
                    if (parameter == nullptr || parameter->valtype != vt_float)
                        continue;
                    const auto oldValue = bridge.value(binding);
                    const auto nextValue = oldValue < .5f ? .631f : .273f;
                    slider->setValue(nextValue, juce::sendNotificationSync);
                    CHECK(bridge.value(binding) == Catch::Approx(nextValue).margin(.001f));
                    CHECK(bridge.activeGestureCount() == 0);
                    edit(bridge, binding, oldValue);
                    checkHostParameterValues(processor, baseline);
                    const auto defaultChoice = selection["default"].toString();
                    if (defaultChoice != choiceId && slider->onDragStart)
                    {
                        slider->onDragStart();
                        REQUIRE(bridge.activeGestureCount() == 1);
                        REQUIRE(workspace.selectView(group, defaultChoice));
                        CHECK(bridge.activeGestureCount() == 0);
                        CHECK(findComponent(workspace, control["id"].toString()) == component);
                        REQUIRE(workspace.selectView(group, choiceId));
                    }
                    editedThisChoice = true;
                    ++slidersEdited;
                }
            }
            REQUIRE(workspace.selectView(group, selection["default"].toString()));
        }
        CHECK_FALSE(workspace.selectView("missing-group", "missing-choice"));
        checkHostParameterValues(processor, baseline);
        workspace.setVisible(false);
    }
    CHECK(groupsTested > 0);
    CHECK(mappingsTested > 0);
    CHECK(slidersEdited > 0);
    processor.releaseResources();
}

TEST_CASE("Segmented native menus retain real modes and balanced host gestures",
          "[reference-workspaces][reference-menus][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    struct SegmentCase
    {
        const char *control;
        const char *binding;
        std::vector<int> nativeValues;
    };
    for (const auto &style : {juce::String("04-bauhaus"), juce::String("05-space")})
    {
        Workspace workspace(processor);
        juce::String error;
        REQUIRE(workspace.load(sourceRoot().getChildFile(
            "resources/data/skins/reference-" + style + ".surge-skin/workspace.json"), error));
        workspace.setSize(referenceWidth, referenceHeight);
        workspace.addToDesktop(0);
        workspace.setVisible(true);
        auto &bridge = workspace.bridge();
        const std::vector<SegmentCase> cases = style.startsWith("04")
            ? std::vector<SegmentCase>{{"osc1-type", "osc1.type", {ot_wavetable, ot_classic, ot_FM2, -1}},
                                       {"routing-mode", "routing", {fc_serial1, -1, fc_dual1, fc_stereo}}}
            : std::vector<SegmentCase>{{"space-scene-mode", "scene.mode", {sm_single, sm_split, sm_dual}},
                                       {"space-poly-mode", "scene.polymode", {pm_poly, pm_mono, pm_mono_st}}};
        for (const auto &entry : cases)
        {
            CAPTURE(style.toStdString(), entry.control, entry.binding);
            auto *menu = dynamic_cast<juce::ComboBox *>(findComponent(workspace, entry.control));
            REQUIRE(menu != nullptr);
            auto *parameter = bridge.parameter(entry.binding);
            auto *adapter = adapterFor(processor, bridge, entry.binding);
            REQUIRE(parameter != nullptr);
            REQUIRE(adapter != nullptr);
            GestureProbe probe(*adapter);
            CHECK(menu->getNumItems() == bridge.choices(entry.binding).size());
            for (size_t index = 0; index < entry.nativeValues.size(); ++index)
            {
                CAPTURE(index);
                auto *button = dynamic_cast<juce::Button *>(findComponent(
                    workspace, juce::String(entry.control) + ".segment." + juce::String(index)));
                REQUIRE(button != nullptr);
                REQUIRE(static_cast<bool>(button->onClick));
                CHECK(button->getName().isNotEmpty());
                CHECK(button->getDescription().isNotEmpty());
                CHECK(menu->getLocalBounds().contains(button->getBounds()));
                const auto before = parameter->val.i;
                const auto starts = probe.starts;
                button->onClick();
                if (entry.nativeValues[index] < 0)
                {
                    CHECK_FALSE(button->isEnabled());
                    CHECK(button->getDescription().startsWith("Unavailable:"));
                    CHECK(parameter->val.i == before);
                    CHECK(probe.starts == starts);
                }
                else
                {
                    CHECK(button->isEnabled());
                    CHECK(button->getWantsKeyboardFocus());
                    CHECK(parameter->val.i == entry.nativeValues[index]);
                    CHECK(button->getToggleState());
                    CHECK(menu->getSelectedItemIndex() == parameter->val.i - parameter->val_min.i);
                }
                CHECK(probe.starts == probe.ends);
                CHECK(probe.balance == 0);
                CHECK(probe.minimumBalance == 0);
                CHECK(bridge.activeGestureCount() == 0);
                CHECK_FALSE(adapter->inEditGesture.load());
            }
            REQUIRE(probe.starts > 0);
            if (juce::String(entry.binding) == "routing")
            {
                // From Serial, right-arrow skips the unavailable reversed
                // topology and chooses the actual native parallel routing.
                auto *serial = dynamic_cast<juce::Button *>(
                    findComponent(workspace, "routing-mode.segment.0"));
                REQUIRE(serial != nullptr);
                serial->onClick();
                serial->grabKeyboardFocus();
                REQUIRE(static_cast<juce::Component *>(serial)->keyPressed(
                    juce::KeyPress(juce::KeyPress::rightKey)));
                CHECK(parameter->val.i == fc_dual1);
                CHECK(probe.starts == probe.ends);
                CHECK(bridge.activeGestureCount() == 0);
            }
        }
    }
}

TEST_CASE("Native distortion transfer previews follow model and drive without editing state",
          "[reference-workspaces][reference-visuals][reference-effects]")
{
    using Surge::GUI::Reference::distortionTransfer;
    juce::ScopedJuceInitialiser_GUI gui;
    IsolatedUserData testData;
    SurgeSynthProcessor processor;
    testData.attach(processor);
    ParameterBridge bridge(processor);
    auto &storage = processor.surge->storage;
    const auto select = [&](const juce::String &binding, int nativeValue) {
        CAPTURE(binding.toStdString(), nativeValue);
        auto *parameter = bridge.parameter(binding);
        REQUIRE(parameter != nullptr);
        const auto index = nativeValue - parameter->val_min.i;
        REQUIRE(index >= 0);
        REQUIRE(index < bridge.choices(binding).size());
        edit(bridge, binding, bridge.choiceValue(binding, index));
        processor.surge->processAudioThreadOpsWhenAudioEngineUnavailable();
        CHECK(parameter->val.i == nativeValue);
    };
    select("fx1.type", fxt_distortion);
    // ParameterBridge's param suffix is the native zero-based FX array index.
    const auto modelBinding = "fx1.param" + juce::String(DistortionEffect::dist_model);
    const auto driveBinding = "fx1.param" + juce::String(DistortionEffect::dist_drive);
    const auto setDrive = [&](float decibels) {
        auto *parameter = bridge.parameter(driveBinding);
        REQUIRE(parameter != nullptr);
        REQUIRE(parameter->valtype == vt_float);
        auto converted = *parameter;
        converted.val.f = decibels;
        edit(bridge, driveBinding, converted.get_value_f01());
        CHECK(parameter->val.f == Catch::Approx(decibels).margin(.0001f));
    };
    select(modelBinding, 0); // Native Soft model.
    setDrive(0.f);
    const auto parametersBefore = hostParameterValues(processor);
    const auto rngBefore = storage.rngGen.g;
    auto noiseBefore = storage.noiseRng;
    juce::MemoryBlock stateBefore, stateAfter;
    processor.getStateInformation(stateBefore);
    const auto soft = distortionTransfer(processor, 0);
    REQUIRE(soft.available);
    REQUIRE(soft.values.size() == 257);
    CHECK(soft.caption.contains("shaper only"));
    CHECK(soft.values.front() < 0.f);
    CHECK(soft.values.back() > 0.f);
    for (const auto value : soft.values)
        CHECK(std::isfinite(value));
    const auto repeated = distortionTransfer(processor, 0);
    CHECK(repeated.available);
    CHECK(repeated.values == soft.values);
    processor.getStateInformation(stateAfter);
    CHECK(stateAfter == stateBefore);
    CHECK(storage.rngGen.g == rngBefore);
    auto noiseAfter = storage.noiseRng;
    for (int sample = 0; sample < 16; ++sample)
        CHECK(noiseBefore.unifU32() == noiseAfter.unifU32());
    checkHostParameterValues(processor, parametersBefore);

    setDrive(12.f);
    const auto driven = distortionTransfer(processor, 0);
    REQUIRE(driven.available);
    CHECK(driven.values != soft.values);
    select(modelBinding, 1); // Native Hard model at the same drive.
    const auto hard = distortionTransfer(processor, 0);
    REQUIRE(hard.available);
    CHECK(hard.values != driven.values);
    for (int model = 0; model <= 6; ++model)
    {
        CAPTURE(model);
        select(modelBinding, model);
        const auto rng = storage.rngGen.g;
        const auto curve = distortionTransfer(processor, 0);
        REQUIRE(curve.available);
        REQUIRE(curve.values.size() == 257);
        for (const auto value : curve.values)
            CHECK(std::isfinite(value));
        CHECK(distortionTransfer(processor, 0).values == curve.values);
        CHECK(storage.rngGen.g == rng);
    }
    select(modelBinding, 7); // Fuzz owns a native shared random lookup table.
    const auto fuzzRng = storage.rngGen.g;
    std::srand(0x7a91);
    const auto expectedRandom = std::rand();
    std::srand(0x7a91);
    CHECK_FALSE(distortionTransfer(processor, 0).available);
    CHECK(std::rand() == expectedRandom);
    CHECK(storage.rngGen.g == fuzzRng);
    CHECK_FALSE(distortionTransfer(processor, -1).available);
    CHECK_FALSE(distortionTransfer(processor, n_fx_slots).available);
    select("fx1.type", fxt_chorus4);
    CHECK_FALSE(distortionTransfer(processor, 0).available);
    CHECK(bridge.activeGestureCount() == 0);
}

TEST_CASE("Host reset clears active effect filter and delay histories",
          "[reference-workspaces][reference-audio][reference-reset]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    // -1 keeps the complete eight-slot chain used by native captures. The
    // individual cases locate regressions in both stateful filter paths.
    for (const int activeEffect : std::array<int, 4>{fxt_off, fxt_distortion, fxt_chorus4, -1})
    {
        CAPTURE(activeEffect);
        IsolatedUserData testData;
        SurgeSynthProcessor processor;
        testData.attach(processor);
        processor.setRateAndBufferSizeDetails(sampleRate, sampleBlock);
        processor.prepareToPlay(sampleRate, sampleBlock);
        configureShowcaseFixture(processor);
        ParameterBridge bridge(processor);
        // Edits use native deferred effect initialization, before audio.
        for (int slot = 1; activeEffect >= 0 && slot <= n_fx_slots; ++slot)
        {
            const auto binding = "fx" + juce::String(slot) + ".type";
            const auto type = slot == 1 ? activeEffect : fxt_off;
            auto *parameter = bridge.parameter(binding);
            REQUIRE(parameter != nullptr);
            edit(bridge, binding, bridge.choiceValue(binding, type - parameter->val_min.i));
            processor.surge->processAudioThreadOpsWhenAudioEngineUnavailable();
        }
        float playingPeak = 0.f;
        for (int block = 0; block < showcaseAudioBlocks; ++block)
        {
            juce::MidiBuffer notes;
            if (block == 0)
                notes.addEvent(juce::MidiMessage::noteOn(1, 60, .65f), 0);
            playingPeak = std::max(playingPeak, processBlock(processor, notes));
        }
        REQUIRE(playingPeak > .0001f);
        const auto values = hostParameterValues(processor);
        processor.reset();
        CHECK(processor.surge->voices[0].empty());
        CHECK(processor.surge->voices[1].empty());
        float resetPeak = 0.f;
        for (int block = 0; block < 4; ++block)
            resetPeak = std::max(resetPeak, processBlock(processor));
        CAPTURE(resetPeak);
        CHECK(resetPeak < .000001f);
        checkHostParameterValues(processor, values);
        processor.releaseResources();
    }
}

TEST_CASE("Ten reference workspaces render native controls and export evidence",
          "[reference-workspaces][reference-render]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto manifests = workspaceManifests();
    REQUIRE(manifests.size() == 10);
    const auto outputSetting =
        juce::SystemStats::getEnvironmentVariable("SURGE_REFERENCE_SCREENSHOTS", "");
    const bool exportImages = outputSetting.isNotEmpty();
    auto outputDirectory = juce::File::getCurrentWorkingDirectory().getChildFile(outputSetting);
    if (exportImages)
        REQUIRE(outputDirectory.createDirectory().wasOk());

    juce::Array<juce::var> captures;
    // Checkpoint completed records so a later REQUIRE failure cannot orphan
    // already-written screenshots. A complete record is not a passing-test
    // declaration; workflow exit status and strict comparison gates still apply.
    const auto writeEvidence = [&] {
        if (exportImages)
        {
            auto report = object();
            property(report, "schema", 1);
            property(report, "captureMethod", "JUCE Component::createComponentSnapshot");
            property(report, "target", "surge-xt-tests [reference-render]");
            property(report, "generatedUtc", juce::Time::getCurrentTime().toISO8601(true));
            property(report, "sourceRevisionAtConfigure", SURGE_REFERENCE_SOURCE_REVISION);
            property(report, "sourceDirtyAtConfigure", static_cast<bool>(SURGE_REFERENCE_SOURCE_DIRTY));
            property(report, "sampleRate", sampleRate);
            property(report, "audioBlock", sampleBlock);
            property(report, "expectedCaptures", static_cast<int>(manifests.size()));
            property(report, "capturedCount", captures.size());
            property(report, "complete", captures.size() == static_cast<int>(manifests.size()));
            property(report, "captures", juce::var(captures));
            property(report, "scope",
                     "Native component renders and control-callback tests. Not OS desktop captures, "
                     "not a DAW certification, not a claim of 1:1 image fidelity.");
            REQUIRE(outputDirectory.getChildFile("capture-evidence.json")
                        .replaceWithText(juce::JSON::toString(report, true)));
        }
    };
    writeEvidence();
    for (const auto &path : manifests)
    {
        const auto manifest = readManifest(path);
        const auto id = manifest["id"].toString();
        CAPTURE(id.toStdString());
        REQUIRE(id.isNotEmpty());
        REQUIRE(juce::File::createLegalFileName(id) == id);
        REQUIRE(!id.containsChar('/'));
        REQUIRE(!id.containsChar('\\'));

        IsolatedUserData testData;
        SurgeSynthProcessor processor;
        testData.attach(processor);
        REQUIRE(processor.surge != nullptr);
        processor.setRateAndBufferSizeDetails(sampleRate, sampleBlock);
        processor.prepareToPlay(sampleRate, sampleBlock);
        const auto fixture = configureShowcaseFixture(processor, id);
        Workspace workspace(processor);
        juce::String error;
        const bool loaded = workspace.load(path, error);
        INFO("Workspace error: " << error.toStdString());
        REQUIRE(loaded);
        REQUIRE(error.isEmpty());
        REQUIRE(workspace.currentId() == id);
        workspace.setSize(referenceWidth, referenceHeight);
        workspace.addToDesktop(0);
        workspace.setVisible(true);
        workspace.selectScene(0);
        // The output views consume the real processor tap only while visible.
        // Feed a reproducible chord and enough real output to fill all 64 FFT
        // history rows. Refresh each four blocks so the bounded tap does not
        // overflow. No image or invented samples fill any output display.
        float audioPeak = 0.f;
        for (int block = 0; block < showcaseAudioBlocks; ++block)
        {
            juce::MidiBuffer notes;
            if (block == 0)
                for (const auto pitch : {48, 60, 67})
                    notes.addEvent(juce::MidiMessage::noteOn(1, pitch, 0.65f), 0);
            audioPeak = std::max(audioPeak, processBlock(processor, notes));
            if ((block + 1) % 4 == 0)
                workspace.refresh();
        }
        CHECK(audioPeak > 0.0001f);
        CHECK(audioPeak < 1.0f);
        const auto baselineAudioPeak = audioPeak;
        workspace.refresh();

        auto &bridge = workspace.bridge();
        int resolvedControls = 0;
        for (const auto &control : *manifest["controls"].getArray())
        {
            const auto binding = control["binding"].toString();
            const auto controlId = control["id"].toString();
            INFO("Control: " << controlId.toStdString() << ", binding: " << binding.toStdString());
            REQUIRE(bridge.valid(binding));
            REQUIRE(std::isfinite(bridge.value(binding)));
            auto *component = findComponent(workspace, controlId);
            REQUIRE(component != nullptr);
            CHECK(component->isVisible());
            CHECK(component->getWidth() > 0);
            CHECK(component->getHeight() > 0);
            if (control["kind"].toString() == "value" ||
                control["labelPosition"].toString() == "top")
            {
                // Numeric fields/top labels are new paint treatments of live
                // sliders, not inert text standing in for an editable control.
                auto *slider = findSlider(*component);
                REQUIRE(slider != nullptr);
                // JUCE excludes disabled controls from keyboard traversal.
                // Inactive algorithm slots must not become focus traps.
                CHECK(slider->getWantsKeyboardFocus() == bridge.enabled(binding));
                CHECK(slider->getName().isNotEmpty());
                CHECK(slider->getMinimum() == Catch::Approx(0.0));
                CHECK(slider->getMaximum() == Catch::Approx(1.0));
                if (bridge.enabled(binding))
                    CHECK(slider->getValue() == Catch::Approx(bridge.value(binding)).margin(.001f));
            }
            ++resolvedControls;
        }
        REQUIRE(resolvedControls > 0);

        // Snapshot the real component before modifying any parameter. It shows
        // the documented live-engine showcase fixture, not reference art.
        auto initial = workspace.createComponentSnapshot(workspace.getLocalBounds());
        REQUIRE(initial.isValid());
        REQUIRE(initial.getWidth() == referenceWidth);
        REQUIRE(initial.getHeight() == referenceHeight);
        if (exportImages)
            REQUIRE(writePng(initial, outputDirectory.getChildFile(id + ".png")));

        REQUIRE(bridge.valid("filter1.cutoff"));
        const float before = bridge.value("filter1.cutoff");
        const float requested = before > 0.5f ? 0.283f : 0.761f;
        auto *cutoffComponent = findBindingControl(workspace, manifest, "filter1.cutoff");
        REQUIRE(cutoffComponent != nullptr);
        auto *cutoffSlider = findSlider(*cutoffComponent);
        REQUIRE(cutoffSlider != nullptr);
        // Exercise a real control callback, not just the bridge. Native sliders use
        // normalized values; a UI adapter breaking that convention fails explicitly.
        REQUIRE(cutoffSlider->getMinimum() == Catch::Approx(0.0));
        REQUIRE(cutoffSlider->getMaximum() == Catch::Approx(1.0));
        cutoffSlider->setValue(requested, juce::sendNotificationSync);
        float editedAudioPeak = 0.f;
        for (int block = 0; block < 4; ++block)
            editedAudioPeak = std::max(editedAudioPeak, processBlock(processor));
        CHECK(editedAudioPeak < 1.0f);
        audioPeak = std::max(audioPeak, editedAudioPeak);
        workspace.refresh();
        const float after = bridge.value("filter1.cutoff");
        CHECK(after == Catch::Approx(requested).margin(0.001f));
        CHECK(std::abs(after - before) > 0.1f);
        auto edited = workspace.createComponentSnapshot(workspace.getLocalBounds());
        const int differences = changedPixels(initial, edited);
        // This proves only that the native display responds. It is deliberately NOT
        // described as a reference-fidelity score or a human visual approval.
        CHECK(differences > 0);
        if (exportImages)
            REQUIRE(writePng(edited, outputDirectory.getChildFile(id + "-edited.png")));

        auto *adapter = adapterFor(processor, bridge, "filter1.cutoff");
        REQUIRE(adapter != nullptr);
        adapter->setValueNotifyingHost(0.579f);
        float hostUpdatedAudioPeak = 0.f;
        for (int block = 0; block < 4; ++block)
            hostUpdatedAudioPeak = std::max(hostUpdatedAudioPeak, processBlock(processor));
        CHECK(hostUpdatedAudioPeak < 1.0f);
        audioPeak = std::max(audioPeak, hostUpdatedAudioPeak);
        workspace.refresh();
        CHECK(cutoffSlider->getValue() == Catch::Approx(0.579f).margin(0.001f));

        // Exercise layout scaling; snapshots remain native and preserve the selected
        // patch. A human comparison is still required for text clipping and usability.
        for (const double scale : {1.25, 1.5})
        {
            workspace.setSize(juce::roundToInt(referenceWidth * scale),
                              juce::roundToInt(referenceHeight * scale));
            workspace.refresh();
            auto scaled = workspace.createComponentSnapshot(workspace.getLocalBounds());
            REQUIRE(scaled.isValid());
            CHECK(scaled.getWidth() == juce::roundToInt(referenceWidth * scale));
            CHECK(scaled.getHeight() == juce::roundToInt(referenceHeight * scale));
        }

        workspace.setSize(referenceWidth, referenceHeight);
        edit(bridge, "filter1.cutoff", before);
        workspace.refresh();
        // Feed real MIDI through the existing engine. Finite silence is not enough:
        // the restored showcase patch must produce a nonzero main-output signal.
        // This remains an offline DSP check, not a device/DAW listening test.
        // Measure this stage independently: reusing the earlier maximum would
        // allow silence caused by a control edit or layout transition to pass.
        float restoredAudioPeak = 0.f;
        for (int block = 0; block < 4; ++block)
            restoredAudioPeak = std::max(restoredAudioPeak, processBlock(processor));
        CHECK(restoredAudioPeak > 0.0001f);
        CHECK(restoredAudioPeak < 1.0f);
        audioPeak = std::max(audioPeak, restoredAudioPeak);
        processor.reset();
        // reset() is a host panic, not note-off: stopSound() frees all voices
        // and suspends every FX to clear its tail. Exercise that contract with
        // the active showcase FX rather than loosening a silence threshold.
        CHECK(processor.surge->voices[0].empty());
        CHECK(processor.surge->voices[1].empty());
        float postResetPeak = 0.f;
        for (int block = 0; block < 4; ++block)
            postResetPeak = std::max(postResetPeak, processBlock(processor));
        CHECK(postResetPeak < 0.000001f);
        processor.releaseResources();

        auto capture = object();
        property(capture, "id", id);
        property(capture, "name", manifest["name"]);
        property(capture, "kind", "native JUCE Workspace component snapshot");
        property(capture, "manifest", path.getRelativePathFrom(sourceRoot()));
        property(capture, "image", id + ".png");
        property(capture, "editedImage", id + "-edited.png");
        property(capture, "width", referenceWidth);
        property(capture, "height", referenceHeight);
        property(capture, "patch", fixture["name"].toString() + "; no reference image loaded");
        property(capture, "patchFixture", fixture);
        property(capture, "audioFixture",
                 "Held MIDI channel 1 notes 48, 60 and 67, velocity 0.65; " +
                     juce::String(showcaseAudioBlocks) + " " + juce::String(sampleBlock) +
                     "-sample blocks at " + juce::String(sampleRate) +
                     " Hz before baseline capture, refreshing each four blocks; "
                     "output visuals read the real processor audio tap");
        property(capture, "audioBlockCount", showcaseAudioBlocks);
        property(capture, "audioBlockSize", sampleBlock);
        property(capture, "audioSampleRate", sampleRate);
        property(capture, "audioSeconds", showcaseAudioBlocks * sampleBlock / sampleRate);
        property(capture, "scene", "A");
        property(capture, "resolvedManifestControls", resolvedControls);
        property(capture, "editedBinding", "filter1.cutoff");
        property(capture, "normalizedBefore", before);
        property(capture, "normalizedAfter", after);
        property(capture, "changedPixelsAfterControlEdit", differences);
        property(capture, "hostUpdateNormalized", 0.579);
        property(capture, "offlineMainOutputPeak", audioPeak);
        property(capture, "baselineMainOutputPeak", baselineAudioPeak);
        property(capture, "editedMainOutputPeak", editedAudioPeak);
        property(capture, "hostUpdatedMainOutputPeak", hostUpdatedAudioPeak);
        property(capture, "restoredMainOutputPeak", restoredAudioPeak);
        property(capture, "postResetMainOutputPeak", postResetPeak);
        property(capture, "scalesRendered", juce::var(juce::Array<juce::var>{1.0, 1.25, 1.5}));
        property(capture, "fidelityAssessment", "Requires human comparison; no pixel-exact claim");
        captures.add(capture);
        writeEvidence();
    }

}
