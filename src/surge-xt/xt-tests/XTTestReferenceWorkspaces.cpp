/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Released under GPL-3.0-or-later; see LICENSE in the repository root.
 */

#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthProcessor.h"
#include "gui/reference/Manifest.h"
#include "gui/reference/ParameterBridge.h"
#include "gui/reference/Workspace.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace
{
using Surge::GUI::Reference::ParameterBridge;
using Surge::GUI::Reference::Workspace;

constexpr int referenceWidth = 1586;
constexpr int referenceHeight = 992;
constexpr int sampleBlock = 512;
constexpr double sampleRate = 48000.0;

juce::File sourceRoot() { return juce::File(juce::String(SURGE_REFERENCE_SOURCE_DIR)); }

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

void processBlock(SurgeSynthProcessor &processor, juce::MidiBuffer midi = {})
{
    juce::AudioBuffer<float> audio(6, sampleBlock);
    audio.clear();
    processor.processBlock(audio, midi);
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            REQUIRE(std::isfinite(audio.getSample(channel, sample)));
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
    SurgeSynthProcessor processor;
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

TEST_CASE("Reference edits survive native processor state restore",
          "[reference-workspaces][reference-state]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    SurgeSynthProcessor processor;
    REQUIRE(processor.surge != nullptr);
    ParameterBridge bridge(processor);
    bridge.setScene(0);
    edit(bridge, "filter1.cutoff", 0.412f);
    edit(bridge, "macro1", 0.62f);
    bridge.setScene(1);
    edit(bridge, "filter1.cutoff", 0.691f);

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    REQUIRE(state.getSize() > 0);
    edit(bridge, "filter1.cutoff", 0.17f);
    bridge.setScene(0);
    edit(bridge, "filter1.cutoff", 0.83f);
    edit(bridge, "macro1", 0.06f);

    // The engine has not started an audio callback in this test, so the processor's
    // existing idle-engine state restore path deserializes the queued state safely.
    processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.412f).margin(0.001f));
    CHECK(bridge.value("macro1") == Catch::Approx(0.62f).margin(0.001f));
    bridge.setScene(1);
    CHECK(bridge.value("filter1.cutoff") == Catch::Approx(0.691f).margin(0.001f));
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

        SurgeSynthProcessor processor;
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
        workspace.selectScene(0);
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
            ++resolvedControls;
        }
        REQUIRE(resolvedControls > 0);

        // Snapshot the real component before modifying any parameter. No concept PNG is
        // loaded by this harness, and no preset numbers are fabricated to resemble the art.
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
        // Feed real MIDI through the existing engine and check finite audio. This does
        // not claim a device/DAW listening test or a measured spectrum screenshot.
        juce::MidiBuffer note;
        note.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        processBlock(processor, note);
        processBlock(processor);
        processor.reset();
        processBlock(processor);
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
        property(capture, "scene", "A");
        property(capture, "resolvedManifestControls", resolvedControls);
        property(capture, "editedBinding", "filter1.cutoff");
        property(capture, "normalizedBefore", before);
        property(capture, "normalizedAfter", after);
        property(capture, "changedPixelsAfterControlEdit", differences);
        property(capture, "hostUpdateNormalized", 0.579);
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
