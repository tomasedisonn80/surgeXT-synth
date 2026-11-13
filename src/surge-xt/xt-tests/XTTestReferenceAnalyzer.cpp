/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthProcessor.h"
#include "PatchDB.h"
#include "UserDefaults.h"
#include "gui/reference/Workspace.h"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
using Surge::GUI::Reference::Workspace;

struct AnalyzerDirectory
{
    juce::File path{juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("surge-analyzer-test-" + juce::Uuid().toString())};
    AnalyzerDirectory() { REQUIRE(path.createDirectory().wasOk()); }
    ~AnalyzerDirectory() { path.deleteRecursively(); }
};

struct AnalyzerFixture
{
    AnalyzerDirectory directory;
    SurgeSynthProcessor processor;
    std::unique_ptr<Workspace> workspace;
    AnalyzerFixture()
    {
        auto &storage = processor.surge->storage;
        storage.patchDB.reset();
        storage.userDataPath = fs::path(directory.path.getFullPathName().toStdString());
        storage.localAppDataPath = storage.userDataPath / "Application";
        storage.initializeUserDataPaths();
        storage.createUserDirectory();
        storage.userDefaultsProvider = std::make_unique<Surge::Storage::UserDefaultsProvider>(
            storage.userDataPath, "SurgeXT", Surge::Storage::defaultKeyToString,
            [&storage](const std::string &message, const std::string &title) {
                storage.reportError(message, title);
            });
        storage.userDefaultsProvider->addOverride(Surge::Storage::DefaultSkin, "");
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCIn, 0);
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCOut, 0);
        storage.patchDB = std::make_unique<Surge::PatchStorage::PatchDB>(&storage);
        storage.patchDBInitialized = false;
        storage.datapath = fs::path(SURGE_REFERENCE_SOURCE_DIR) / "resources" / "data";
        processor.prepareToPlay(48000, 512);
        workspace = std::make_unique<Workspace>(processor);
        workspace->setSize(1586, 992);
        workspace->addToDesktop(0);
        workspace->setVisible(true);
    }
    juce::var load(const juce::String &skin)
    {
        const auto file = juce::File(juce::String(SURGE_REFERENCE_SOURCE_DIR))
                              .getChildFile("resources/data/skins/reference-" + skin +
                                            ".surge-skin/workspace.json");
        juce::String error;
        REQUIRE(workspace->load(file, error));
        workspace->refresh();
        REQUIRE(processor.referenceAudio.subscribed());
        return juce::JSON::parse(file);
    }
};

juce::Component *findAnalyzerComponent(juce::Component &component, const juce::String &id)
{
    if (component.getComponentID() == id)
        return &component;
    for (auto *child : component.getChildren())
        if (auto *found = findAnalyzerComponent(*child, id))
            return found;
    return nullptr;
}

struct AnalyzerParameterNotices : juce::AudioProcessorParameter::Listener
{
    SurgeSynthProcessor &processor;
    int values{0}, gestures{0};
    std::vector<float> snapshot;
    explicit AnalyzerParameterNotices(SurgeSynthProcessor &processor) : processor(processor)
    {
        for (auto *parameter : processor.getParameters())
        {
            snapshot.push_back(parameter->getValue());
            parameter->addListener(this);
        }
    }
    ~AnalyzerParameterNotices() override
    {
        for (auto *parameter : processor.getParameters())
            parameter->removeListener(this);
    }
    void parameterValueChanged(int, float) override { ++values; }
    void parameterGestureChanged(int, bool) override { ++gestures; }
    void verify() const
    {
        CHECK(values == 0);
        CHECK(gestures == 0);
        REQUIRE(processor.getParameters().size() == static_cast<int>(snapshot.size()));
        for (int i = 0; i < processor.getParameters().size(); ++i)
            CHECK(processor.getParameters()[i]->getValue() == snapshot[i]);
    }
};

juce::Rectangle<int> changedBounds(const juce::Image &before, const juce::Image &after,
                                   juce::Rectangle<int> region)
{
    juce::Rectangle<int> changed;
    for (int y = region.getY(); y < region.getBottom(); ++y)
        for (int x = region.getX(); x < region.getRight(); ++x)
            if (before.getPixelAt(x, y) != after.getPixelAt(x, y))
                changed = changed.getUnion({x, y, 1, 1});
    return changed;
}
} // namespace

TEST_CASE("Reference analyzer tabs preserve audio parameters and queued stereo capture",
          "[reference-workspaces][reference-analyzer][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    AnalyzerFixture fixture;
    for (const auto *skin : {"01-obsidian", "02-ivory", "08-clear", "10-live"})
    {
        INFO(skin);
        const auto manifest = fixture.load(skin);
        AnalyzerParameterNotices notices(fixture.processor);
        int panelsTested = 0;
        for (const auto &panel : *manifest["panels"].getArray())
        {
            const auto *modes = panel["analyzerModes"].getArray();
            if (modes == nullptr)
                continue;
            ++panelsTested;
            const auto &rect = panel["analyzerModesRect"];
            std::vector<juce::Button *> tabs;
            for (int i = 0; i < modes->size(); ++i)
            {
                const auto mode = modes->getReference(i).toString();
                auto *button = dynamic_cast<juce::Button *>(findAnalyzerComponent(
                    *fixture.workspace, "workspace.analyzer." + panel["id"].toString() + "." + mode));
                REQUIRE(button != nullptr);
                REQUIRE(button->onClick);
                CHECK(button->getWantsKeyboardFocus());
                REQUIRE(button->getAccessibilityHandler() != nullptr);
                CHECK(button->getToggleState() == (mode == panel["analyzerDefault"].toString()));
                const auto width = (static_cast<float>(rect[2]) - 4.f * (modes->size() - 1)) /
                                   modes->size();
                CHECK(std::abs(button->getX() - (static_cast<float>(rect[0]) + i * (width + 4))) <= 1);
                CHECK(std::abs(button->getWidth() - width) <= 1);
                CHECK(button->getY() == static_cast<int>(rect[1]));
                CHECK(button->getHeight() == static_cast<int>(rect[3]));
                tabs.push_back(button);
            }
            const std::array<float, 4> left{.1f, .2f, .3f, .4f}, right{-.4f, -.3f, -.2f, -.1f};
            REQUIRE(fixture.processor.referenceAudio.push(left.data(), right.data(), left.size()) == left.size());
            for (int mode = 0; mode < static_cast<int>(tabs.size()); ++mode)
            {
                tabs[mode]->onClick();
                for (int check = 0; check < static_cast<int>(tabs.size()); ++check)
                    CHECK(tabs[check]->getToggleState() == (check == mode));
                REQUIRE(static_cast<juce::Component *>(tabs[mode])->keyPressed(
                    juce::KeyPress(juce::KeyPress::rightKey)));
                CHECK(tabs[(mode + 1) % tabs.size()]->getToggleState());
                CHECK(tabs[(mode + 1) % tabs.size()]->hasKeyboardFocus(false));
            }
            const auto [capturedLeft, capturedRight] = fixture.processor.referenceAudio.popall();
            CHECK(capturedLeft == std::vector<float>(left.begin(), left.end()));
            CHECK(capturedRight == std::vector<float>(right.begin(), right.end()));
        }
        CHECK(panelsTested == 1);
        notices.verify();
        fixture.load(skin); // A skin rebuild restores its declared default, not a stale mode.
        for (const auto &panel : *manifest["panels"].getArray())
            if (panel.hasProperty("analyzerModes"))
            {
                auto *button = dynamic_cast<juce::Button *>(findAnalyzerComponent(
                    *fixture.workspace, "workspace.analyzer." + panel["id"].toString() + "." +
                                            panel["analyzerDefault"].toString()));
                REQUIRE(button != nullptr);
                CHECK(button->getToggleState());
            }
    }
}

TEST_CASE("Reference vectorscope renders paired stereo polarity and clears invalid or dropped data",
          "[reference-workspaces][reference-analyzer][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    AnalyzerFixture fixture;
    const auto manifest = fixture.load("02-ivory");
    juce::Rectangle<int> plot;
    for (const auto &panel : *manifest["panels"].getArray())
        if (panel["kind"].toString() == "analyzer")
        {
            const auto &rect = panel["plot"];
            plot = {static_cast<int>(rect[0]), static_cast<int>(rect[1]),
                    static_cast<int>(rect[2]), static_cast<int>(rect[3])};
        }
    REQUIRE_FALSE(plot.isEmpty());
    auto *vectorTab = dynamic_cast<juce::Button *>(findAnalyzerComponent(
        *fixture.workspace, "workspace.analyzer.ivory.analyzer.vectorscope"));
    REQUIRE(vectorTab != nullptr);
    vectorTab->onClick();
    std::array<float, 512> zero{}, left{}, right{};
    const auto capture = [&](const auto &l, const auto &r) {
        REQUIRE(fixture.processor.referenceAudio.push(l.data(), r.data(), l.size()) == l.size());
        fixture.workspace->refresh();
        return fixture.workspace->createComponentSnapshot(plot);
    };
    const auto blank = capture(zero, zero);
    for (size_t i = 0; i < left.size(); ++i)
        left[i] = .6f * std::sin(juce::MathConstants<float>::twoPi * i / 64.f);
    const auto mono = capture(left, left);
    for (size_t i = 0; i < left.size(); ++i)
        right[i] = -left[i];
    const auto antiPhase = capture(left, right);
    const auto side = std::min(plot.getWidth() - 16, plot.getHeight() - 32);
    const auto region = juce::Rectangle<int>(plot.getWidth() / 2 - 46, 8 + side / 2 - 46, 92, 92);
    const auto monoChanges = changedBounds(blank, mono, region);
    const auto stereoChanges = changedBounds(blank, antiPhase, region);
    CHECK(monoChanges.getHeight() > 40);
    CHECK(monoChanges.getWidth() < 8);
    CHECK(stereoChanges.getWidth() > 40);
    CHECK(stereoChanges.getHeight() < 8);

    left.fill(std::numeric_limits<float>::quiet_NaN());
    right.fill(std::numeric_limits<float>::infinity());
    const auto invalid = capture(left, right);
    CHECK(changedBounds(blank, invalid, region).isEmpty());
    std::vector<float> full(8192, .5f);
    REQUIRE(fixture.processor.referenceAudio.push(full.data(), full.data(), full.size()) == full.size());
    REQUIRE(fixture.processor.referenceAudio.push(zero.data(), zero.data(), zero.size()) == 0);
    fixture.workspace->refresh();
    const auto overflow = fixture.workspace->createComponentSnapshot(plot);
    CHECK(changedBounds(blank, overflow, region).getWidth() < 8);
    CHECK(changedBounds(blank, overflow, region).getHeight() < 8);
}

TEST_CASE("Reference analyzer output is independent of display refresh batch size",
          "[reference-workspaces][reference-analyzer][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    AnalyzerFixture fixture;
    const auto manifest = fixture.load("01-obsidian");
    juce::Rectangle<int> plot;
    for (const auto &panel : *manifest["panels"].getArray())
        if (panel["kind"].toString() == "analyzer")
        {
            const auto &rect = panel["plot"];
            plot = {static_cast<int>(rect[0]), static_cast<int>(rect[1]),
                    static_cast<int>(rect[2]), static_cast<int>(rect[3])};
        }
    REQUIRE_FALSE(plot.isEmpty());
    std::array<juce::Button *, 2> tabs{};
    int tabIndex = 0;
    for (const auto *mode : {"waterfall", "scope"})
    {
        auto *tab = dynamic_cast<juce::Button *>(findAnalyzerComponent(
            *fixture.workspace, "workspace.analyzer.panel.analyzer." + juce::String(mode)));
        REQUIRE(tab != nullptr);
        REQUIRE(tab->onClick);
        tabs[tabIndex++] = tab;
    }

    // Eight different native audio windows reveal discarded intermediate FFTs.
    // The non-divisor batch also checks scope samples retained across refreshes.
    std::array<float, 16384> left{}, right{};
    for (size_t i = 0; i < left.size(); ++i)
    {
        const auto window = i / 2048;
        const auto frequency = 187.5 * (1 + window);
        const auto angle = juce::MathConstants<double>::twoPi * frequency * i / 48000.0;
        left[i] = .55f * static_cast<float>(std::sin(angle));
        right[i] = .35f * static_cast<float>(std::sin(angle + .7));
    }
    const auto capture = [&](size_t batch) {
        fixture.workspace->setVisible(false);
        fixture.workspace->setVisible(true);
        fixture.workspace->refresh();
        for (size_t first = 0; first < left.size(); first += batch)
        {
            const auto count = std::min(batch, left.size() - first);
            REQUIRE(fixture.processor.referenceAudio.push(left.data() + first,
                                                          right.data() + first, count) == count);
            fixture.workspace->refresh();
        }
        std::array<juce::Image, 2> images;
        for (size_t mode = 0; mode < tabs.size(); ++mode)
        {
            tabs[mode]->onClick();
            images[mode] = fixture.workspace->createComponentSnapshot(plot);
        }
        return images;
    };
    const auto reference = capture(512);
    for (const auto batch : {size_t{333}, size_t{8192}})
    {
        INFO("Audio frames per display refresh: " << batch);
        const auto candidate = capture(batch);
        for (size_t mode = 0; mode < tabs.size(); ++mode)
        {
            INFO(tabs[mode]->getButtonText());
            CHECK(changedBounds(reference[mode], candidate[mode],
                                reference[mode].getBounds()).isEmpty());
        }
    }
}
