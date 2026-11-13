/*
 * Surge XT - native reference-workspace MIDI regressions.
 * Released under GPL-3.0-or-later; see LICENSE in the repository root.
 */
#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthProcessor.h"
#include "PatchDB.h"
#include "UserDefaults.h"
#include "ModulationSource.h"
#include "gui/reference/Workspace.h"

#include <array>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace
{
using Workspace = Surge::GUI::Reference::Workspace;
using MidiRecord = SurgeSynthProcessor::midiR;

struct MidiTestUserData
{
    MidiTestUserData()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("surge-reference-midi-" + juce::Uuid().toString()))
    {
        REQUIRE(directory.createDirectory().wasOk());
    }
    ~MidiTestUserData() { directory.deleteRecursively(); }
    void attach(SurgeSynthProcessor &processor)
    {
        auto &storage = processor.surge->storage;
        storage.patchDB.reset();
        storage.userDataPath = fs::path(directory.getFullPathName().toStdString());
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
        storage.userDefaultsProvider->addOverride(Surge::Storage::DefaultSkin, "");
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCIn, 0);
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCOut, 0);
    }
    juce::File directory;
};

juce::Component *midiComponent(juce::Component &parent, const juce::String &id)
{
    if (parent.getComponentID() == id)
        return &parent;
    for (auto *child : parent.getChildren())
        if (auto *found = midiComponent(*child, id))
            return found;
    return nullptr;
}

struct MidiHarness
{
    MidiHarness()
    {
        REQUIRE(processor.surge != nullptr);
        data.attach(processor);
        processor.prepareToPlay(48000.0, 512);
        block(); // Complete ordinary native initialization before the tested input.
    }
    ~MidiHarness()
    {
        workspace.reset();
        processor.releaseResources();
    }
    void block(juce::MidiBuffer midi = {})
    {
        juce::AudioBuffer<float> audio(6, 512);
        audio.clear();
        processor.processBlock(audio, midi);
        for (int channel = 0; channel < 2; ++channel)
            REQUIRE(std::isfinite(audio.getMagnitude(channel, 0, audio.getNumSamples())));
    }
    void load(const juce::String &package = "reference-10-live")
    {
        if (!workspace)
            workspace = std::make_unique<Workspace>(processor);
        juce::String error;
        const auto manifest =
            juce::File(juce::String(SURGE_REFERENCE_SOURCE_DIR))
                .getChildFile("resources/data/skins/" + package + ".surge-skin/workspace.json");
        const bool loaded = workspace->load(manifest, error);
        INFO(error.toStdString());
        REQUIRE(loaded);
        REQUIRE(error.isEmpty());
        workspace->setSize(1586, 992);
        workspace->addToDesktop(0);
        workspace->setVisible(true);
        workspace->refresh();
    }
    juce::Slider &lane(const char *name)
    {
        REQUIRE(workspace != nullptr);
        auto *slider = dynamic_cast<juce::Slider *>(
            midiComponent(*workspace, "workspace.midi." + juce::String(name)));
        REQUIRE(slider != nullptr);
        return *slider;
    }
    int applied(MidiRecord::Type type) const
    {
        const auto &synth = *processor.surge;
        if (type == MidiRecord::PITCHWHEEL)
            return synth.pitchbendMIDIVal.load();
        if (type == MidiRecord::MODWHEEL)
            return synth.modwheelCC.load();
        if (type == MidiRecord::EXPRESSION)
            return synth.expressionCC.load();
        if (type == MidiRecord::AFTERTOUCH)
            return synth.channelAftertouchMIDIVal.load();
        return synth.sustainpedalCC.load();
    }
    float target(int source, int scene = 0) const
    {
        auto *controller = dynamic_cast<ControllerModulationSource *>(
            processor.surge->storage.getPatch().scene[scene].modsources[source]);
        REQUIRE(controller != nullptr);
        return controller->get_target01(0);
    }
    void host(int pitch, int mod, int expression, int pressure)
    {
        juce::MidiBuffer input;
        input.addEvent(juce::MidiMessage::pitchWheel(1, pitch + 8192), 0);
        input.addEvent(juce::MidiMessage::controllerEvent(1, 1, mod), 1);
        input.addEvent(juce::MidiMessage::controllerEvent(1, 11, expression), 2);
        input.addEvent(juce::MidiMessage::channelPressureChange(1, pressure), 3);
        block(input);
        if (workspace)
            workspace->refresh();
    }
    void fillQueue(int modValue = 17)
    {
        int inserted = 0;
        for (; inserted < 8192; ++inserted)
            if (!processor.midiFromGUI.push(MidiRecord(MidiRecord::MODWHEEL, modValue)))
                break;
        REQUIRE(inserted > 0);
        REQUIRE(inserted < 8192);
    }

    juce::ScopedJuceInitialiser_GUI gui;
    MidiTestUserData data;
    SurgeSynthProcessor processor;
    std::unique_ptr<Workspace> workspace;
};

juce::MouseEvent midiMouse(juce::Component &component, juce::Point<float> position)
{
    const auto now = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(), position,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),
            1.f, 0.f, 0.f, 0.f, 0.f, &component, &component, now, position, now, 1, false};
}

struct MidiModalCleanup
{
    MidiModalCleanup()
    {
        REQUIRE(juce::ModalComponentManager::getInstance()->getNumModalComponents() == 0);
    }
    ~MidiModalCleanup()
    {
        if (auto *manager = juce::ModalComponentManager::getInstanceWithoutCreating())
            manager->cancelAllModalComponents();
        juce::ModalComponentManager::deleteInstance();
    }
};
} // namespace

TEST_CASE("Reference MIDI queue delivers native controller targets and bounds",
          "[reference-workspaces][reference-midi]")
{
    MidiHarness h;
    using M = MidiRecord;
    const std::array<std::pair<M::Type, int>, 5> inputs{{
        {M::PITCHWHEEL, -4096}, {M::MODWHEEL, 51}, {M::EXPRESSION, 83},
        {M::AFTERTOUCH, 102}, {M::SUSPEDAL, 127}}};
    for (const auto &[type, value] : inputs)
    {
        const auto before = h.applied(type);
        REQUIRE(h.processor.queueMidiController(type, value));
        CHECK(h.applied(type) == before); // GUI enqueue must not write the engine.
    }
    h.block();
    for (const auto &[type, value] : inputs)
        CHECK(h.applied(type) == value);
    CHECK(h.processor.surge->storage.pitch_bend == Catch::Approx(-.5f));
    for (int scene = 0; scene < 2; ++scene)
    {
        CHECK(h.target(ms_modwheel, scene) == Catch::Approx(51.f / 127));
        CHECK(h.target(ms_expression, scene) == Catch::Approx(83.f / 127));
        CHECK(h.target(ms_aftertouch, scene) == Catch::Approx(102.f / 127));
    }

    for (const auto &[type, ignored] : inputs)
    {
        CAPTURE(static_cast<int>(type), ignored);
        REQUIRE(h.processor.queueMidiController(type, std::numeric_limits<int>::max()));
        h.block();
        CHECK(h.applied(type) == (type == M::PITCHWHEEL ? 8191 : 127));
        REQUIRE(h.processor.queueMidiController(type, std::numeric_limits<int>::min()));
        h.block();
        CHECK(h.applied(type) == (type == M::PITCHWHEEL ? -8192 : 0));
    }
    const int ready = h.processor.midiFromGUI.af.getNumReady();
    CHECK_FALSE(h.processor.queueMidiController(M::NOTE, 60));
    CHECK(h.processor.midiFromGUI.af.getNumReady() == ready);
}

TEST_CASE("Reference spring releases survive queue saturation without cancelling newer gestures",
          "[reference-workspaces][reference-midi][reference-lifecycle]")
{
    MidiHarness h;
    for (const auto type : {MidiRecord::PITCHWHEEL, MidiRecord::AFTERTOUCH})
    {
        CAPTURE(static_cast<int>(type));
        REQUIRE(h.processor.queueMidiController(type, 63));
        h.block();
        REQUIRE(h.applied(type) == 63);
        h.fillQueue();
        REQUIRE(h.processor.queueMidiController(type, 0));
        CHECK_FALSE(h.processor.queueMidiController(type, 32));
        h.block();
        CHECK(h.applied(type) == 0);

        // A producer obtains a free queue position after an emergency release:
        // the later accepted motion must win over that older fallback.
        h.fillQueue();
        REQUIRE(h.processor.queueMidiController(type, 0));
        MidiRecord discarded;
        REQUIRE(h.processor.midiFromGUI.pop(discarded));
        REQUIRE(h.processor.queueMidiController(type, 91));
        h.block();
        CHECK(h.applied(type) == 91);

        // Retain an older accepted packet to model delayed producer delivery.
        // Reintroducing it after the emergency release must not revive a spring.
        REQUIRE(h.processor.queueMidiController(type, 47));
        MidiRecord delayed;
        REQUIRE(h.processor.midiFromGUI.pop(delayed));
        REQUIRE(delayed.type == type);
        h.fillQueue();
        REQUIRE(h.processor.queueMidiController(type, 0));
        h.block();
        REQUIRE(h.applied(type) == 0);
        REQUIRE(h.processor.midiFromGUI.push(delayed));
        h.block();
        CHECK(h.applied(type) == 0);
    }
}

TEST_CASE("Live MIDI lanes reflect incoming host input and preserve idle state across views",
          "[reference-workspaces][reference-midi][reference-accessibility]")
{
    MidiHarness h;
    h.host(2048, 64, 77, 82);
    h.load();
    const std::array<const char *, 4> names{"pitch", "mod", "expression", "aftertouch"};
    const std::array<int, 4> values{2048, 64, 77, 82};
    const std::array<int, 4> centres{38, 76, 114, 152};
    for (size_t i = 0; i < names.size(); ++i)
    {
        CAPTURE(names[i]);
        auto &lane = h.lane(names[i]);
        CHECK(lane.isShowing());
        CHECK(lane.isEnabled());
        CHECK(lane.getWantsKeyboardFocus());
        CHECK(lane.getName().isNotEmpty());
        CHECK(lane.getTitle().isNotEmpty());
        CHECK(lane.getDescription().isNotEmpty());
        CHECK(lane.getTooltip().isNotEmpty());
        CHECK(lane.getMinimum() == (i == 0 ? -8192 : 0));
        CHECK(lane.getMaximum() == (i == 0 ? 8191 : 127));
        CHECK(lane.getInterval() == 1);
        CHECK(lane.getValue() == values[i]);
        const auto bounds = h.workspace->getLocalArea(&lane, lane.getLocalBounds());
        CHECK(bounds.getCentreX() == centres[i]);
        CHECK(bounds.getY() == 831);
        CHECK(bounds.getHeight() == 128);
        REQUIRE(lane.getParentComponent() != nullptr);
        CHECK(lane.getParentComponent()->getLocalBounds().contains(lane.getBounds()));
        auto *handler = lane.getAccessibilityHandler();
        REQUIRE(handler != nullptr);
        CHECK(handler->getRole() == juce::AccessibilityRole::slider);
        auto *value = handler->getValueInterface();
        REQUIRE(value != nullptr);
        CHECK_FALSE(value->isReadOnly());
        CHECK(value->getCurrentValue() == values[i]);
        CHECK(value->getCurrentValueAsString() == juce::String(values[i]));
    }

    auto &storage = h.processor.surge->storage;
    storage.getPatch().isDirty = false;
    std::vector<float> parameters;
    for (auto *parameter : h.processor.getParameters())
        parameters.push_back(parameter->getValue());
    const auto rng = storage.rngGen.g;
    auto noiseBefore = storage.noiseRng;
    h.workspace->refresh();
    REQUIRE(h.workspace->createComponentSnapshot(h.workspace->getLocalBounds()).isValid());
    h.load("reference-02-ivory");
    h.load();
    CHECK(h.processor.midiFromGUI.af.getNumReady() == 0);
    CHECK_FALSE(storage.getPatch().isDirty);
    CHECK(storage.rngGen.g == rng);
    auto noiseAfter = storage.noiseRng;
    for (int i = 0; i < 16; ++i)
        CHECK(noiseBefore.unifU32() == noiseAfter.unifU32());
    REQUIRE(static_cast<size_t>(h.processor.getParameters().size()) == parameters.size());
    for (size_t i = 0; i < parameters.size(); ++i)
        CHECK(h.processor.getParameters()[static_cast<int>(i)]->getValue() ==
              Catch::Approx(parameters[i]).margin(.000001f));
    for (size_t i = 0; i < names.size(); ++i)
        CHECK(h.lane(names[i]).getValue() == values[i]);
    h.host(-1024, 5, 109, 23);
    CHECK(h.lane("pitch").getValue() == -1024);
    CHECK(h.lane("mod").getValue() == 5);
    CHECK(h.lane("expression").getValue() == 109);
    CHECK(h.lane("aftertouch").getValue() == 23);
}

TEST_CASE("Live MIDI keyboard pointer and typed entry deliver real values and release owned springs",
          "[reference-workspaces][reference-midi][reference-interactions]")
{
    MidiHarness h;
    h.load();
    MidiModalCleanup modalCleanup;
    auto &pitch = h.lane("pitch");
    REQUIRE(pitch.keyPressed(juce::KeyPress(juce::KeyPress::upKey)));
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 0);
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 256);
    REQUIRE(pitch.keyStateChanged(false));
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 0);
    REQUIRE(pitch.keyPressed(
        juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier, 0)));
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 16);
    pitch.focusLost(juce::Component::focusChangedDirectly);
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 0);

    auto *pitchValue = pitch.getAccessibilityHandler()->getValueInterface();
    REQUIRE(pitchValue != nullptr);
    pitchValue->setValueAsString("-4096");
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == -4096);
    pitchValue->setValueAsString("-8193");
    pitchValue->setValueAsString("8192");
    CHECK(h.processor.midiFromGUI.af.getNumReady() == 0);
    REQUIRE(pitch.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey)));
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 0);

    auto &mod = h.lane("mod");
    REQUIRE(mod.keyPressed(juce::KeyPress(juce::KeyPress::upKey)));
    h.block();
    CHECK(h.applied(MidiRecord::MODWHEEL) == 1);
    REQUIRE(mod.keyStateChanged(false));
    h.block();
    CHECK(h.applied(MidiRecord::MODWHEEL) == 1);

    auto &touch = h.lane("aftertouch");
    const auto start = juce::Point<float>(touch.getWidth() * .5f, 70.f);
    const auto down = midiMouse(touch, start);
    touch.mouseDown(down);
    touch.mouseDrag(down.withNewPosition({start.x, 25.f}));
    h.block();
    CHECK(h.applied(MidiRecord::AFTERTOUCH) > 0);
    touch.mouseUp(down.withNewPosition({start.x, 25.f}));
    h.block();
    CHECK(h.applied(MidiRecord::AFTERTOUCH) == 0);

    // Exercise the actual callout and apply callback, including a malformed
    // signed token which std::from_chars would otherwise partly accept.
    REQUIRE(mod.keyPressed(juce::KeyPress(juce::KeyPress::returnKey)));
    auto *input = dynamic_cast<juce::TextEditor *>(
        midiComponent(*h.workspace, "workspace.value-entry.input"));
    auto *apply = dynamic_cast<juce::Button *>(
        midiComponent(*h.workspace, "workspace.value-entry.apply"));
    REQUIRE(input != nullptr);
    REQUIRE(apply != nullptr);
    REQUIRE(static_cast<bool>(apply->onClick));
    for (const auto *bad : {"", "1.5", "+-1", "++1", "--1", "128", "-1",
                            "NaN", "63tail", "99999999999999999999"})
    {
        CAPTURE(bad);
        input->setText(bad, false);
        apply->onClick();
        CHECK(input->getDescription().isNotEmpty());
        CHECK(h.processor.midiFromGUI.af.getNumReady() == 0);
        CHECK(h.applied(MidiRecord::MODWHEEL) == 1);
    }
    input->setText("+87", false);
    apply->onClick();
    CHECK(h.applied(MidiRecord::MODWHEEL) == 1);
    h.block();
    h.workspace->refresh();
    CHECK(h.applied(MidiRecord::MODWHEEL) == 87);
    CHECK(h.target(ms_modwheel) == Catch::Approx(87.f / 127));

    auto *expression = h.lane("expression").getAccessibilityHandler()->getValueInterface();
    REQUIRE(expression != nullptr);
    expression->setValueAsString("109");
    h.block();
    h.workspace->refresh();
    CHECK(h.applied(MidiRecord::EXPRESSION) == 109);
    CHECK(h.target(ms_expression) == Catch::Approx(109.f / 127));
    expression->setValueAsString("+-1");
    expression->setValue(std::numeric_limits<double>::quiet_NaN());
    CHECK(h.processor.midiFromGUI.af.getNumReady() == 0);

    // Switching skin ends an owned spring but keeps latched controller values.
    REQUIRE(h.lane("pitch").keyPressed(juce::KeyPress(juce::KeyPress::rightKey)));
    h.block();
    REQUIRE(h.applied(MidiRecord::PITCHWHEEL) != 0);
    h.fillQueue(87); // The real view-change release must survive saturation.
    h.load("reference-02-ivory");
    h.block();
    CHECK(h.applied(MidiRecord::PITCHWHEEL) == 0);
    CHECK(h.applied(MidiRecord::MODWHEEL) == 87);
    CHECK(h.applied(MidiRecord::EXPRESSION) == 109);
    h.load();
    REQUIRE(h.lane("aftertouch").keyPressed(juce::KeyPress(juce::KeyPress::upKey)));
    h.block();
    REQUIRE(h.applied(MidiRecord::AFTERTOUCH) != 0);
    h.fillQueue(87); // Destruction must deliver the owned Touch release too.
    h.workspace.reset();
    h.block();
    CHECK(h.applied(MidiRecord::AFTERTOUCH) == 0);
}
