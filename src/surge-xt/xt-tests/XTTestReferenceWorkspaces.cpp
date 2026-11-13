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

#include <algorithm>
#include <cmath>
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
                bindings.insert(control["binding"].toString());
    }
    REQUIRE(bindings.size() >= 30);

    // These ranges expose bugs hidden by zero-based menus: octave is signed and
    // polyphony starts above zero. Boolean controls use a third normalization path.
    REQUIRE(bridge.parameter("osc1.octave")->val_min.i < 0);
    REQUIRE(bridge.parameter("scene.polylimit")->val_min.i > 0);
    bindings.insert("osc1.keytrack");
    bindings.insert("osc1.mute");

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
        std::unique_ptr<juce::AudioProcessorEditor> owner(processor.createEditorIfNeeded());
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

    juce::String valueId;
    for (const auto &control : *manifest["controls"].getArray())
        if (control["kind"].toString() == "value" && control["binding"].toString() == "env1.attack")
            valueId = control["id"].toString();
    REQUIRE(valueId.isNotEmpty());
    auto *slider = dynamic_cast<juce::Slider *>(findComponent(workspace, valueId));
    REQUIRE(slider != nullptr);
    REQUIRE(slider->isEnabled());
    REQUIRE(slider->getWantsKeyboardFocus());
    auto *adapter = adapterFor(processor, bridge, "env1.attack");
    REQUIRE(adapter != nullptr);
    GestureProbe probe(*adapter);
    edit(bridge, "env1.attack", .4f);
    workspace.refresh();
    const auto before = bridge.value("env1.attack");
    const auto start = slider->getLocalBounds().toFloat().getCentre();
    const auto down = mouseEvent(*slider, start);
    slider->mouseDown(down);
    slider->mouseDrag(down.withNewPosition(start.translated(0, -25.f)));
    slider->mouseUp(down.withNewPosition(start.translated(0, -25.f)));
    CHECK(std::abs(bridge.value("env1.attack") - before) > .01f);
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
    const auto beforeInvalid = bridge.value("env1.attack");
    input->setText("this is not a duration", false);
    apply->onClick();
    CHECK(bridge.value("env1.attack") == Catch::Approx(beforeInvalid));
    CHECK(input->getDescription().isNotEmpty());
    input->setText("250 ms", false);
    apply->onClick();
    REQUIRE(bridge.parameter("env1.attack") != nullptr);
    CHECK(bridge.parameter("env1.attack")->val.f == Catch::Approx(-2.f).margin(.0001f));
    CHECK(probe.starts == probe.ends);
    CHECK(probe.balance == 0);
    CHECK(bridge.activeGestureCount() == 0);
    CHECK_FALSE(adapter->inEditGesture.load());
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
        sourceRoot().getChildFile("resources/data/patches_factory/Templates/Init Sine.fxp");
    REQUIRE(patch.existsAsFile());
    for (auto *processor : {&withUI, &withoutUI})
    {
        REQUIRE(processor->surge->loadPatchByPath(patch.getFullPathName().toRawUTF8(), -1,
                                                  "Reference isolation fixture"));
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
        // Feed a reproducible held note; no external image or invented samples
        // are used to fill an oscilloscope, spectrum or output meter.
        juce::MidiBuffer note;
        note.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        float audioPeak = processBlock(processor, note);
        for (int block = 0; block < 4; ++block)
            audioPeak = std::max(audioPeak, processBlock(processor));
        CHECK(audioPeak > 0.0001f);
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
                CHECK(slider->getWantsKeyboardFocus());
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
        // the startup patch with the documented live-note fixture, not reference art.
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
        for (int block = 0; block < 4; ++block)
            processBlock(processor);
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
        // the restored startup patch must produce a nonzero main-output signal.
        // This remains an offline DSP check, not a device/DAW listening test.
        for (int block = 0; block < 4; ++block)
            audioPeak = std::max(audioPeak, processBlock(processor));
        CHECK(audioPeak > 0.0001f);
        processor.reset();
        CHECK(processBlock(processor) < 0.000001f);
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
        property(capture, "patch", "Processor startup patch; no reference image loaded");
        property(capture, "audioFixture",
                 "Held MIDI channel 1 note 60, velocity 0.8; five 512-sample blocks at 48 kHz "
                 "before baseline capture; output visuals read the real processor audio tap");
        property(capture, "scene", "A");
        property(capture, "resolvedManifestControls", resolvedControls);
        property(capture, "editedBinding", "filter1.cutoff");
        property(capture, "normalizedBefore", before);
        property(capture, "normalizedAfter", after);
        property(capture, "changedPixelsAfterControlEdit", differences);
        property(capture, "hostUpdateNormalized", 0.579);
        property(capture, "offlineMainOutputPeak", audioPeak);
        property(capture, "scalesRendered", juce::var(juce::Array<juce::var>{1.0, 1.25, 1.5}));
        property(capture, "fidelityAssessment", "Requires human comparison; no pixel-exact claim");
        captures.add(capture);
    }

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
        property(report, "captures", juce::var(captures));
        property(report, "scope",
                 "Native component renders and control-callback tests. Not OS desktop captures, "
                 "not a DAW certification, not a claim of 1:1 image fidelity.");
        REQUIRE(outputDirectory.getChildFile("capture-evidence.json")
                    .replaceWithText(juce::JSON::toString(report, true)));
    }
}
