/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Released under GPL-3.0-or-later; see LICENSE in the repository root.
 */

#include "catch2/catch_amalgamated.hpp"

#if HAS_CLAP_JUCE_EXTENSIONS

#include "SurgeStorage.h"
#include "EmbeddedFactoryContent.h"
#include "gui/SkinSupport.h"
#include <clap/clap.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>

namespace sst::surge_xt::preset_discovery
{
bool declarePresetLocations(const SurgeStorage &, const clap_preset_discovery_indexer &);
}

namespace
{
struct DiscoveryTree
{
    fs::path root;
    DiscoveryTree()
    {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int n = 0; n < 100; ++n)
        {
            auto candidate = fs::temp_directory_path() /
                             ("surge-preset-discovery-" + std::to_string(stamp) + "-" +
                              std::to_string(n));
            if (fs::create_directory(candidate))
            {
                root = candidate;
                return;
            }
        }
        throw std::runtime_error("Cannot reserve a preset discovery fixture directory");
    }
    ~DiscoveryTree()
    {
        std::error_code ignored;
        fs::remove_all(root, ignored);
    }
};

struct CapturedLocation
{
    fs::path path;
    uint32_t flags;
    uint32_t kind;
};

struct RecordingIndexer
{
    clap_preset_discovery_indexer indexer{};
    std::vector<CapturedLocation> locations;
    bool reject{false};

    RecordingIndexer()
    {
        indexer.clap_version = CLAP_VERSION;
        indexer.indexer_data = this;
        indexer.declare_location = [](const clap_preset_discovery_indexer *self,
                                      const clap_preset_discovery_location *location) {
            auto &state = *static_cast<RecordingIndexer *>(self->indexer_data);
            if (!location || !location->location || !location->name)
                return false;
            state.locations.push_back(
                {string_to_path(location->location), location->flags, location->kind});
            return !state.reject;
        };
    }

    bool contains(const fs::path &path, uint32_t flags) const
    {
        return std::any_of(locations.begin(), locations.end(), [&](const auto &entry) {
            return entry.path == path && entry.flags == flags &&
                   entry.kind == CLAP_PRESET_DISCOVERY_LOCATION_FILE;
        });
    }
};
} // namespace

TEST_CASE("CLAP advertises the embedded bank and preserves unrelated preset locations",
          "[embedded][preset-discovery]")
{
    DiscoveryTree tree;
    const auto installed = tree.root / "installed";
    const auto cached = tree.root / "cache";
    const auto users = tree.root / "user-patches";
    SurgeStorage::SurgeStorageConfig config;
    config.suppliedDataPath = installed.u8string();
    config.scanWavetableAndPatches = false;
    config.createUserDirectory = false;
    auto storage = std::make_unique<SurgeStorage>(config);
    storage->datapath = installed;
    storage->userPatchesPath = users;
    storage->embeddedFactoryDataPath = cached;
    fs::create_directories(cached / "patches_factory/Scoring 1000");

    SECTION("plugin-only installation declares the cached bank")
    {
        RecordingIndexer recorder;
        REQUIRE(sst::surge_xt::preset_discovery::declarePresetLocations(*storage, recorder.indexer));
        REQUIRE(recorder.locations.size() == 1);
        REQUIRE(recorder.contains(cached / "patches_factory",
                                  CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
    }

    SECTION("stale installed bank is excluded while other factory and user content remains")
    {
        fs::create_directories(installed / "patches_factory/Legacy Collection");
        fs::create_directories(installed / "patches_factory/sCoRiNg 1000");
        fs::create_directories(installed / "patches_3rdparty");
        fs::create_directories(users);
        const auto loosePreset = installed / "patches_factory/Loose.FXP";
        std::ofstream(loosePreset, std::ios::binary).put('x');
        RecordingIndexer recorder;
        REQUIRE(sst::surge_xt::preset_discovery::declarePresetLocations(*storage, recorder.indexer));
        REQUIRE(recorder.locations.size() == 5);
        REQUIRE(recorder.contains(cached / "patches_factory",
                                  CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
        REQUIRE(recorder.contains(installed / "patches_factory/Legacy Collection",
                                  CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
        REQUIRE(recorder.contains(loosePreset, CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
        REQUIRE(recorder.contains(installed / "patches_3rdparty",
                                  CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
        REQUIRE(recorder.contains(users, CLAP_PRESET_DISCOVERY_IS_USER_CONTENT));
        REQUIRE_FALSE(recorder.contains(installed / "patches_factory",
                                        CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
        REQUIRE_FALSE(recorder.contains(installed / "patches_factory/sCoRiNg 1000",
                                        CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
    }

    SECTION("matching full installation is declared once")
    {
        fs::create_directories(installed / "patches_factory/Scoring 1000");
        storage->embeddedFactoryDataPath = installed;
        RecordingIndexer recorder;
        REQUIRE(sst::surge_xt::preset_discovery::declarePresetLocations(*storage, recorder.indexer));
        REQUIRE(recorder.locations.size() == 1);
        REQUIRE(recorder.contains(installed / "patches_factory",
                                  CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT));
    }

    SECTION("host rejection stops further location callbacks")
    {
        fs::create_directories(users);
        RecordingIndexer recorder;
        recorder.reject = true;
        REQUIRE_FALSE(sst::surge_xt::preset_discovery::declarePresetLocations(*storage, recorder.indexer));
        REQUIRE(recorder.locations.size() == 1);
    }
}

TEST_CASE("Native skin discovery preserves ten embedded workspaces and personal skins",
          "[embedded][skin-discovery]")
{
    DiscoveryTree tree;
    const auto installed = tree.root / "installed";
    const auto userSkins = tree.root / "user-skins";
    const auto archive = Surge::Storage::getSurgeCommonBinaryResource("scoring-factory.sxtz");
    const auto identity = Surge::Storage::getSurgeCommonBinaryResource("scoring-factory.id");
    REQUIRE(archive.has_value());
    REQUIRE(identity.has_value());
    std::string id(identity->begin(), identity->end());
    while (!id.empty() && (id.back() == '\n' || id.back() == '\r'))
        id.pop_back();
    const auto content = Surge::Storage::materializeEmbeddedFactoryContent(
        installed, tree.root / "application-data", *archive, id);
    REQUIRE(content.error.empty());
    REQUIRE_FALSE(content.reusedInstalled);
    REQUIRE_FALSE(content.dataPath.empty());

    SurgeStorage::SurgeStorageConfig config;
    config.suppliedDataPath = installed.u8string();
    config.scanWavetableAndPatches = false;
    config.createUserDirectory = false;
    auto storage = std::make_unique<SurgeStorage>(config);
    storage->datapath = installed;
    storage->embeddedFactoryDataPath = content.dataPath;
    storage->userSkinsPath = userSkins;

    const auto writeSkin = [](const fs::path &directory, const std::string &displayName) {
        fs::create_directories(directory);
        std::ofstream xml(directory / "skin.xml", std::ios::binary);
        xml << "<surge-skin name=\"" << displayName
            << "\" category=\"Fixture\" version=\"2\"><globals/></surge-skin>\n";
        xml.close();
        REQUIRE(xml.good());
    };
    writeSkin(installed / "skins/ReFeReNcE-01-Obsidian.surge-skin", "Stale Obsidian");
    writeSkin(installed / "skins/Legacy.surge-skin", "Legacy One");
    writeSkin(installed / "skins/nested/Legacy.surge-skin", "Legacy Two");
    writeSkin(userSkins / "reference-01-obsidian.surge-skin", "Personal Obsidian");
    writeSkin(userSkins / "Personal.surge-skin", "Personal Other");

    auto *database = Surge::GUI::SkinDB::get();
    database->rescanForSkins(storage.get());
    const std::set<std::string> expectedNames{
        "01 Obsidian Precision", "02 Ivory Atelier", "03 Spectral Observatory",
        "04 Bauhaus Modular", "05 Space-Age 1977", "06 Tokyo Industrial", "07 Liquid Aurora",
        "08 ClearSignal Accessible", "09 Signal-Flow Cartographer", "10 Live Performance Deck"};
    std::set<std::string> actualNames;
    std::set<std::string> retainedFactoryNames;
    std::set<std::string> retainedUserNames;
    for (const auto &entry : database->getAvailableSkins())
    {
        REQUIRE(entry.displayName != "Stale Obsidian");
        if (entry.category == "Reference Workspaces")
        {
            REQUIRE(entry.rootType == Surge::GUI::FACTORY);
            REQUIRE(entry.parseable);
            REQUIRE(string_to_path(entry.root).lexically_normal() ==
                    (content.dataPath / "skins").lexically_normal() / "");
            REQUIRE(actualNames.insert(entry.displayName).second);
            const auto package = string_to_path(entry.root + entry.name);
            REQUIRE(fs::is_regular_file(package / "skin.xml"));
            REQUIRE(fs::is_regular_file(package / "workspace.json"));
        }
        else if (entry.rootType == Surge::GUI::FACTORY)
            retainedFactoryNames.insert(entry.displayName);
        else if (entry.rootType == Surge::GUI::USER)
            retainedUserNames.insert(entry.displayName);
    }
    REQUIRE(actualNames == expectedNames);
    REQUIRE(retainedFactoryNames == std::set<std::string>{"Legacy One", "Legacy Two"});
    REQUIRE(retainedUserNames == std::set<std::string>{"Personal Obsidian", "Personal Other"});
    REQUIRE(database->getDefaultSkinEntry().rootType == Surge::GUI::MEMORY);

    const auto temporaryDefault = installed / "skins/default.surge-skin";
    writeSkin(temporaryDefault, "Fixture Default");
    database->rescanForSkins(storage.get());
    REQUIRE(database->getDefaultSkinEntry().rootType == Surge::GUI::FACTORY);
    fs::remove_all(temporaryDefault);
    database->rescanForSkins(storage.get());
    REQUIRE(database->getDefaultSkinEntry().rootType == Surge::GUI::MEMORY);
}

#endif
