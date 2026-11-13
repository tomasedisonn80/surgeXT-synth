/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthEditor.h"
#include "SurgeSynthProcessor.h"
#include "PatchDB.h"
#include "UserDefaults.h"
#include "gui/reference/ParameterBridge.h"
#include "gui/reference/WavetableSelection.h"
#include <algorithm>
#include <cstdlib>

namespace
{
using namespace Surge::GUI::Reference;

struct WavetableTestDirectory
{
    juce::File path{juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("surge-wavetable-selection-" + juce::Uuid().toString())};
    WavetableTestDirectory() { REQUIRE(path.createDirectory().wasOk()); }
    ~WavetableTestDirectory() { path.deleteRecursively(); }
};

struct WavetableFixture
{
    // Destruction order keeps the directory until all processor workers stop.
    WavetableTestDirectory directory;
    SurgeSynthProcessor processor;
    struct DeleteAsHost
    {
        SurgeSynthProcessor &processor;
        void operator()(juce::AudioProcessorEditor *editor) const
        {
            if (editor)
            {
                processor.editorBeingDeleted(editor);
                delete editor;
            }
        }
    };
    std::unique_ptr<juce::AudioProcessorEditor, DeleteAsHost> editor{nullptr, {processor}};

    explicit WavetableFixture(bool nativeEditor = false)
    {
        REQUIRE(processor.surge != nullptr);
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
        storage.datapath = sourceRoot() / "resources" / "data";
        storage.refresh_wtlist();
        loadInit();
        if (nativeEditor)
        {
            editor.reset(processor.createEditorIfNeeded());
            REQUIRE(dynamic_cast<SurgeSynthEditor *>(editor.get()) != nullptr);
            REQUIRE(processor.undoManager != nullptr);
        }
    }
    static fs::path sourceRoot() { return fs::path(SURGE_REFERENCE_SOURCE_DIR); }
    fs::path tablePath() const
    {
        return sourceRoot() / "resources" / "data" / "wavetables" / "Generated" / "Formants.wt";
    }
    void loadInit()
    {
        const auto path = sourceRoot() / "resources" / "data" / "patches_factory" / "Templates" /
                          "Init Wavetable.fxp";
        REQUIRE(processor.surge->loadPatchByPath(path_to_string(path).c_str(), -1,
                                                 "Wavetable selection fixture"));
        REQUIRE_FALSE(processor.surge->audio_processing_active.load());
        processor.surge->processAudioThreadOpsWhenAudioEngineUnavailable();
    }
    WavetableSelectionRequest request()
    {
        const auto state = snapshotWavetableSelection(processor, 0, 0);
        REQUIRE(state.has_value());
        REQUIRE(state->applicable);
        REQUIRE_FALSE(state->loadQueued);
        return {state->target, tablePath()};
    }
};

struct WavetableNotice
{
    SurgeSynthProcessor &processor;
    int count{0};
    float selectedId{-1};
    std::string value;
    explicit WavetableNotice(SurgeSynthProcessor &processor) : processor(processor)
    {
        processor.addParamChangeListener(
            "reference-wavetable-test",
            [this](const std::string &path, int values, float id, float, float,
                   const std::string &name) {
                if (path == "/wavetable/a/osc/1/id")
                {
                    CHECK(values == 1);
                    ++count;
                    selectedId = id;
                    value = name;
                }
            });
    }
    ~WavetableNotice() { processor.deleteParamChangeListener("reference-wavetable-test"); }
};
} // namespace

TEST_CASE("Reference wavetable selections queue real data with isolated native undo",
          "[reference-workspaces][reference-wavetables][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    WavetableFixture fixture(true);
    auto &processor = fixture.processor;
    auto &synth = *processor.surge;
    auto &source = synth.storage.getPatch().scene[0].osc[0];
    const auto request = fixture.request();
    const auto otherOscillator = snapshotWavetableSelection(processor, 0, 1);
    const auto otherScene = snapshotWavetableSelection(processor, 1, 0);
    REQUIRE(otherOscillator.has_value());
    REQUIRE(otherScene.has_value());
    const int id = resolveWavetableSelection(processor, request);
    REQUIRE(id >= 0);
    REQUIRE(id != request.target.tableId);
    WavetableNotice notice(processor);
    std::string announcement;
    REQUIRE(applyWavetableSelection(processor, request, {},
                                    [&](const auto &message) { announcement = message; }));
    CHECK(source.wt.queue_id == id);
    CHECK(source.wt.current_id == request.target.tableId);
    CHECK(synth.storage.getCurrentWavetableName(&source) == request.target.tableName);
    CHECK(processor.undoManager->canUndo());
    CHECK(notice.count == 1);
    CHECK(notice.selectedId == Catch::Approx(id));
    CHECK(notice.value == request.target.tableName);
    CHECK(announcement == "Loading wavetable " + synth.storage.wt_list[id].name);

    synth.processAudioThreadOpsWhenAudioEngineUnavailable();
    REQUIRE(source.wt.queue_id == -1);
    CHECK(source.wt.current_id == id);
    CHECK(source.wt.n_tables > 1);
    CHECK(synth.storage.getCurrentWavetableName(&source) == synth.storage.wt_list[id].name);
    REQUIRE(snapshotWavetableSelection(processor, 0, 1).has_value());
    REQUIRE(snapshotWavetableSelection(processor, 1, 0).has_value());
    CHECK(snapshotWavetableSelection(processor, 0, 1)->target == otherOscillator->target);
    CHECK(snapshotWavetableSelection(processor, 1, 0)->target == otherScene->target);
    CHECK_FALSE(applyWavetableSelection(processor, request));

    processor.undoManager->undo();
    synth.processAudioThreadOpsWhenAudioEngineUnavailable();
    CHECK(source.wt.current_id == request.target.tableId);
    CHECK(synth.storage.getCurrentWavetableName(&source) == request.target.tableName);
    CHECK(processor.undoManager->canRedo());
}

TEST_CASE("Deferred reference wavetable requests retain catalog and patch identity",
          "[reference-workspaces][reference-wavetables][reference-interactions]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    WavetableFixture fixture;
    auto &processor = fixture.processor;
    auto &storage = processor.surge->storage;
    const auto request = fixture.request();
    REQUIRE(resolveWavetableSelection(processor, request) >= 0);

    SECTION("A rescan resolves the chosen path, and a removed path becomes a no-op")
    {
        const int original = resolveWavetableSelection(processor, request);
        const auto originalCatalog = storage.wt_list;
        const auto other = std::find_if(storage.wt_list.begin(), storage.wt_list.end(),
                                        [&](const auto &entry) {
                                            const auto id = &entry - storage.wt_list.data();
                                            return id != original && id != request.target.tableId;
                                        });
        REQUIRE(other != storage.wt_list.end());
        const int moved = static_cast<int>(std::distance(storage.wt_list.begin(), other));
        std::swap(storage.wt_list[original], storage.wt_list[moved]);
        CHECK(resolveWavetableSelection(processor, request) == moved);
        storage.wt_list.erase(storage.wt_list.begin() + moved);
        CHECK_FALSE(applyWavetableSelection(processor, request));
        CHECK(storage.getPatch().scene[0].osc[0].wt.queue_id == -1);
        storage.wt_list = originalCatalog;
    }
    SECTION("A different oscillator algorithm cannot receive the delayed choice")
    {
        ParameterBridge bridge(processor);
        const auto binding = juce::String("osc1.type");
        const auto *parameter = bridge.parameter(binding);
        REQUIRE(parameter != nullptr);
        bridge.begin(binding);
        bridge.set(binding, bridge.choiceValue(binding, ot_classic - parameter->val_min.i));
        bridge.end(binding);
        processor.surge->processAudioThreadOpsWhenAudioEngineUnavailable();
        CHECK_FALSE(applyWavetableSelection(processor, request));
        CHECK(storage.getPatch().scene[0].osc[0].wt.queue_id == -1);
    }
    SECTION("An empty installed catalog cannot queue an invented wavetable")
    {
        storage.wt_list.clear();
        storage.wtOrdering.clear();
        CHECK_FALSE(applyWavetableSelection(processor, request));
        CHECK(storage.getPatch().scene[0].osc[0].wt.queue_id == -1);
    }
    SECTION("Reloading the exact same XML patch invalidates a pending selection")
    {
        void *xml{};
        const auto size = storage.getPatch().save_xml(&xml);
        std::unique_ptr<void, decltype(&std::free)> release(xml, &std::free);
        REQUIRE(size > 0);
        REQUIRE(xml != nullptr);
        storage.getPatch().load_xml(xml, static_cast<int>(size), false);
        const auto reloaded = snapshotWavetableSelection(processor, 0, 0);
        REQUIRE(reloaded.has_value());
        CHECK(reloaded->target.patchId == request.target.patchId);
        CHECK(reloaded->target.patchName == request.target.patchName);
        CHECK(reloaded->target.algorithm == request.target.algorithm);
        CHECK(reloaded->target.replacementToken == request.target.replacementToken);
        CHECK(reloaded->target.patchGeneration > request.target.patchGeneration);
        CHECK_FALSE(applyWavetableSelection(processor, request));
        CHECK(storage.getPatch().scene[0].osc[0].wt.queue_id == -1);
    }
    SECTION("A native binary patch reload also changes the generation")
    {
        fixture.loadInit();
        CHECK(storage.patchLoadGeneration.load() > request.target.patchGeneration);
        CHECK_FALSE(applyWavetableSelection(processor, request));
    }
#if HAS_LUA
    SECTION("A script delegates to the native loader instead of the normal table queue")
    {
        const auto script = std::find_if(storage.wt_list.begin(), storage.wt_list.end(),
                                         [](const auto &entry) {
                                             return entry.path.extension() == ".wtscript";
                                         });
        REQUIRE(script != storage.wt_list.end());
        const WavetableSelectionRequest scripted{request.target, script->path};
        REQUIRE_FALSE(applyWavetableSelection(processor, scripted));
        int calls = 0;
        REQUIRE(applyWavetableSelection(
            processor, scripted, [&](int scene, int oscillator, int id, const fs::path &path) {
                ++calls;
                CHECK(scene == 0);
                CHECK(oscillator == 0);
                CHECK(storage.wt_list[id].path == script->path);
                CHECK(path == script->path);
            }));
        CHECK(calls == 1);
        CHECK(storage.getPatch().scene[0].osc[0].wt.queue_id == -1);
    }
#endif
}
