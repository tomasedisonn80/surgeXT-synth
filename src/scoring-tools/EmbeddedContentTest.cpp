/*
 * Surge XT - a free and open source hybrid synthesizer
 * Copyright 2026, various authors, as described in the GitHub transaction log.
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 */

#include "EmbeddedFactoryContent.h"
#include "SurgeStorage.h"
#include "zstd.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace
{
void require(bool condition, const std::string &description)
{
    if (!condition)
        throw std::runtime_error(description);
}

std::vector<char> readFile(const fs::path &path)
{
    std::ifstream file(path, std::ios::binary);
    require(static_cast<bool>(file), "Cannot open test file: " + path_to_string(path));
    return std::vector<char>((std::istreambuf_iterator<char>(file)), {});
}

void writeFile(const fs::path &path, const std::vector<char> &bytes)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    file.close();
    require(static_cast<bool>(file), "Cannot write test file: " + path_to_string(path));
}

struct TemporaryDirectory
{
    fs::path path;
    TemporaryDirectory()
    {
        for (unsigned int attempt = 0; attempt < 100; ++attempt)
        {
            const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto candidate = fs::temp_directory_path() /
                             ("surge-embedded-test-" + std::to_string(now) + "-" +
                              std::to_string(attempt));
            if (fs::create_directory(candidate))
            {
                path = std::move(candidate);
                return;
            }
        }
        throw std::runtime_error("Cannot reserve isolated test directory.");
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
};

std::vector<char> compress(const std::vector<char> &payload)
{
    std::vector<char> result(ZSTD_compressBound(payload.size()));
    const auto size =
        ZSTD_compress(result.data(), result.size(), payload.data(), payload.size(), 1);
    require(!ZSTD_isError(size), "Cannot encode negative test fixture.");
    result.resize(size);
    return result;
}

std::set<fs::path> expectedFiles(const fs::path &dataRoot)
{
    std::set<fs::path> files;
    for (const auto &file : fs::recursive_directory_iterator(dataRoot / "patches_factory" /
                                                            "Scoring 1000"))
    {
        if (file.is_regular_file())
            files.insert(file.path().lexically_relative(dataRoot));
    }
    require(files.size() == 1000, "Expected exactly 1000 source scoring presets.");
    size_t skins = 0;
    for (const auto &skin : fs::directory_iterator(dataRoot / "skins"))
    {
        const auto name = path_to_string(skin.path().filename());
        if (skin.is_directory() && name.rfind("reference-", 0) == 0)
        {
            ++skins;
            for (const auto &file : fs::recursive_directory_iterator(skin.path()))
                if (file.is_regular_file())
                    files.insert(file.path().lexically_relative(dataRoot));
        }
    }
    require(skins == 10, "Expected exactly ten source reference skins.");
    return files;
}

size_t countScoring(const SurgeStorage &storage, const fs::path &root)
{
    size_t count = 0;
    const auto bank = root / "patches_factory" / "Scoring 1000";
    for (const auto &patch : storage.patch_list)
    {
        const auto relative = patch.path.lexically_relative(bank);
        if (!relative.empty() && *relative.begin() != "..")
        {
            require(storage.patch_category[patch.category].isFactory,
                    "Embedded patches must remain factory patches.");
            ++count;
        }
    }
    return count;
}

void pass(const std::string &name) { std::cout << "PASS " << name << '\n'; }
} // namespace

int main(int argc, char **argv)
{
    try
    {
        const auto archive =
            Surge::Storage::getSurgeCommonBinaryResource("scoring-factory.sxtz");
        const auto idBytes = Surge::Storage::getSurgeCommonBinaryResource("scoring-factory.id");
        require(archive.has_value() && idBytes.has_value(), "CMakeRC embedded resources are missing.");
        std::string id(idBytes->begin(), idBytes->end());
        while (!id.empty() && (id.back() == '\n' || id.back() == '\r'))
            id.pop_back();

        // This explicit smoke mode exercises the real ordinary startup path,
        // including its normal per-user application cache. It is separate from
        // the isolated default suite and must not inherit a data-root override.
        if (argc == 2 && std::string(argv[1]) == "--implicit-discovery")
        {
            require(std::getenv("SURGE_DATA_HOME") == nullptr &&
                        std::getenv("PIPELINE_OVERRIDE_DATA_HOME") == nullptr,
                    "Implicit discovery test requires no data-root environment override.");
            SurgeStorage::SurgeStorageConfig config;
            config.createUserDirectory = false;
            auto storage = std::make_unique<SurgeStorage>(config);
            require(!storage->embeddedFactoryDataPath.empty(),
                    "Ordinary startup did not resolve the compiled factory content.");
            require(countScoring(*storage, storage->embeddedFactoryDataPath) == 1000,
                    "Ordinary startup did not discover exactly 1000 built-in factory presets.");
            size_t skins = 0;
            for (const auto &entry :
                 fs::directory_iterator(storage->embeddedFactoryDataPath / "skins"))
                if (entry.is_directory() &&
                    entry.path().filename().u8string().rfind("reference-", 0) == 0 &&
                    fs::is_regular_file(entry.path() / "skin.xml") &&
                    fs::is_regular_file(entry.path() / "workspace.json"))
                    ++skins;
            require(skins == 10, "Ordinary startup did not resolve ten complete reference skins.");
            pass("ordinary implicit startup discovers 1000 factory presets and ten reference skins");
            std::cout << "Factory content: " << path_to_string(storage->embeddedFactoryDataPath)
                      << '\n';
            return 0;
        }

        // This mode permits a test driver to launch competing independent
        // processes against one cache without shell commands in this executable.
        if (argc == 4 && std::string(argv[1]) == "--materialize-only")
        {
            const auto result = Surge::Storage::materializeEmbeddedFactoryContent(
                string_to_path(argv[2]), string_to_path(argv[3]), *archive, id);
            require(result.error.empty() && !result.dataPath.empty(), result.error);
            std::cout << path_to_string(result.dataPath) << '\n';
            return 0;
        }

        require(argc == 2, "Usage: surge-embedded-content-test <resources/data>");
        const auto dataRoot = fs::canonical(string_to_path(argv[1]));
        const auto files = expectedFiles(dataRoot);
        pass("CMakeRC retrieval and source fixture identity");
        TemporaryDirectory temporary;
        const auto absent = temporary.path / "absent-installation";
        const auto app = temporary.path / "application-data";

        const auto installed = Surge::Storage::materializeEmbeddedFactoryContent(
            dataRoot, temporary.path / "unused-cache", *archive, id);
        require(installed.error.empty() && installed.reusedInstalled && installed.dataPath == dataRoot,
                "Exact installed content must be reused.");
        require(!fs::exists(temporary.path / "unused-cache"), "Installed reuse wrote a cache.");
        pass("complete installed content reuse without cache writes");

        const auto fresh =
            Surge::Storage::materializeEmbeddedFactoryContent(absent, app, *archive, id);
        require(fresh.error.empty() && !fresh.reusedInstalled && !fresh.dataPath.empty(), fresh.error);
        require(!fs::exists(absent), "Fallback must not write into the installation path.");
        size_t actualFiles = 0;
        for (const auto &file : fs::recursive_directory_iterator(fresh.dataPath))
            if (file.is_regular_file())
                ++actualFiles;
        require(actualFiles == files.size(), "Embedded cache has extra or missing files.");
        for (const auto &path : files)
            require(readFile(dataRoot / path) == readFile(fresh.dataPath / path),
                    "Materialized bytes differ: " + path_to_string(path));
        pass("no-install fallback reproduces every preset and reference skin byte");

        const auto partial = temporary.path / "old-installation";
        const auto oldFile = partial / *files.begin();
        fs::create_directories(oldFile.parent_path());
        const std::vector<char> oldBytes{'o', 'l', 'd'};
        writeFile(oldFile, oldBytes);
        const auto upgraded =
            Surge::Storage::materializeEmbeddedFactoryContent(partial, app, *archive, id);
        require(upgraded.error.empty() && upgraded.dataPath == fresh.dataPath &&
                    !upgraded.reusedInstalled && readFile(oldFile) == oldBytes,
                "Partial/old installed data must use the cache without changing installed files.");
        pass("partial installed content is supplemented without overwriting installation files");

        const auto victim = fresh.dataPath / *files.begin();
        const auto original = readFile(victim);
        const auto stamp = fs::last_write_time(victim);
        const auto repeat =
            Surge::Storage::materializeEmbeddedFactoryContent(absent, app, *archive, id);
        require(repeat.error.empty() && repeat.dataPath == fresh.dataPath &&
                    fs::last_write_time(victim) == stamp,
                "Unchanged cache should retain its content-keyed path and files.");
        writeFile(victim, {'b', 'a', 'd'});
        auto repair = Surge::Storage::materializeEmbeddedFactoryContent(absent, app, *archive, id);
        require(repair.error.empty() && readFile(victim) == original,
                "A damaged cached file was not repaired.");
        fs::remove(victim);
        repair = Surge::Storage::materializeEmbeddedFactoryContent(absent, app, *archive, id);
        require(repair.error.empty() && readFile(victim) == original,
                "A missing cached file was not repaired.");
        pass("stable reuse and missing/damaged cache repair");

        const auto outside = temporary.path / "outside-user-file";
        const std::vector<char> sentinel{'u', 's', 'e', 'r'};
        writeFile(outside, sentinel);
        fs::remove(victim);
        std::error_code symlinkError;
        fs::create_symlink(outside, victim, symlinkError);
        if (!symlinkError)
        {
            const auto rejected =
                Surge::Storage::materializeEmbeddedFactoryContent(absent, app, *archive, id);
            require(!rejected.error.empty() && rejected.dataPath.empty() &&
                        readFile(outside) == sentinel && fs::is_symlink(victim),
                    "Cache symlink must fail closed without changing its target.");
            fs::remove(victim);
            pass("cache file symlink rejected without touching user data");
        }
        else
            std::cout << "SKIP cache symlink fixture: " << symlinkError.message() << '\n';
        repair = Surge::Storage::materializeEmbeddedFactoryContent(absent, app, *archive, id);
        require(repair.error.empty(), repair.error);

        const auto expandedSize = ZSTD_getFrameContentSize(archive->data(), archive->size());
        require(expandedSize < 64 * 1024 * 1024, "Unexpected resource size in negative tests.");
        std::vector<char> raw(static_cast<size_t>(expandedSize));
        require(ZSTD_decompress(raw.data(), raw.size(), archive->data(), archive->size()) == raw.size(),
                "Cannot decode resource for negative fixtures.");
        unsigned int rejectedCount = 0;
        auto reject = [&](const std::vector<char> &bytes, const std::string &testId,
                          const std::string &name) {
            const auto target = temporary.path / ("rejected-" + std::to_string(rejectedCount++));
            const auto result =
                Surge::Storage::materializeEmbeddedFactoryContent(absent, target, bytes, testId);
            require(!result.error.empty() && result.dataPath.empty() && !fs::exists(target),
                    "Invalid archive accepted or wrote content: " + name);
        };
        auto truncated = *archive;
        truncated.resize(truncated.size() / 2);
        reject(truncated, id, "truncated zstd");
        auto trailing = *archive;
        trailing.push_back('x');
        reject(trailing, id, "trailing frame data");
        auto badMagic = raw;
        badMagic[0] = '?';
        reject(compress(badMagic), id, "container version");
        auto badPath = raw;
        require(badPath.size() > 23, "Fixture has no first path.");
        badPath[20] = '.';
        badPath[21] = '.';
        badPath[22] = '/';
        reject(compress(badPath), id, "path traversal");
        auto badLength = raw;
        for (size_t n = 12; n < 16; ++n)
            badLength[n] = static_cast<char>(0xff);
        reject(compress(badLength), id, "unbounded path length");
        reject(*archive, "../invalid-id", "cache identifier");
        pass("truncation, trailing data, version, path traversal, length and identifier rejection");

        std::vector<std::future<Surge::Storage::EmbeddedFactoryContentResult>> concurrent;
        for (int n = 0; n < 4; ++n)
            concurrent.push_back(std::async(std::launch::async, [&]() {
                return Surge::Storage::materializeEmbeddedFactoryContent(
                    absent, temporary.path / "concurrent-cache", *archive, id);
            }));
        fs::path concurrentPath;
        for (auto &task : concurrent)
        {
            const auto result = task.get();
            require(result.error.empty() && !result.dataPath.empty(), result.error);
            if (concurrentPath.empty())
                concurrentPath = result.dataPath;
            require(result.dataPath == concurrentPath, "Concurrent callers selected different caches.");
        }
        pass("four concurrent initializations share complete content");

        SurgeStorage::SurgeStorageConfig config;
        config.suppliedDataPath = path_to_string(dataRoot);
        config.createUserDirectory = false;
        auto storage = std::make_unique<SurgeStorage>(config);
        require(storage->embeddedFactoryDataPath.empty(), "Explicit data root enabled fallback.");
        require(countScoring(*storage, dataRoot) == 1000, "Explicit installed discovery is incomplete.");
        std::set<fs::path> otherBefore;
        const auto installedBank = dataRoot / "patches_factory" / "Scoring 1000";
        for (const auto &patch : storage->patch_list)
        {
            const auto relative = patch.path.lexically_relative(installedBank);
            if (relative.empty() || *relative.begin() == "..")
                otherBefore.insert(patch.path);
        }
        storage->embeddedFactoryDataPath = fresh.dataPath;
        storage->refresh_patchlist();
        require(countScoring(*storage, fresh.dataPath) == 1000 && countScoring(*storage, dataRoot) == 0,
                "Fallback discovery must expose exactly one factory bank.");
        std::set<fs::path> after;
        for (const auto &patch : storage->patch_list)
            after.insert(patch.path);
        for (const auto &path : otherBefore)
            require(after.count(path) != 0, "Fallback hid an unrelated installed/user preset.");
        pass("native factory discovery deduplicates the bank and preserves unrelated presets");

        config.suppliedDataPath = path_to_string(absent);
        auto missing = std::make_unique<SurgeStorage>(config);
        require(missing->embeddedFactoryDataPath.empty() && countScoring(*missing, fresh.dataPath) == 0,
                "An explicit missing installer fixture was silently rescued by embedded content.");
        pass("explicit missing data root retains installer-audit semantics");

        const auto mixedRoot = temporary.path / "case-variant-installation";
        const auto oldPreset = mixedRoot / "patches_factory" / "sCoRiNg 1000" / "01 Keys" /
                               "Old Factory Copy.fxp";
        const auto legacyPreset = mixedRoot / "patches_factory" / "Legacy" / "Keep Factory.fxp";
        const auto userRoot = temporary.path / "user-data";
        const auto userPreset = userRoot / "Patches" / "Personal" / "Keep User.fxp";
        for (const auto &path : {oldPreset, legacyPreset, userPreset})
        {
            fs::create_directories(path.parent_path());
            writeFile(path, original);
        }
        config.suppliedDataPath = path_to_string(mixedRoot);
        auto mixedStorage = std::make_unique<SurgeStorage>(config);
        mixedStorage->userDataPath = userRoot;
        mixedStorage->initializeUserDataPaths();
        mixedStorage->embeddedFactoryDataPath = fresh.dataPath;
        mixedStorage->refresh_patchlist();
        std::set<fs::path> mixedPaths;
        for (const auto &patch : mixedStorage->patch_list)
            mixedPaths.insert(patch.path);
        require(countScoring(*mixedStorage, fresh.dataPath) == 1000 &&
                    mixedPaths.count(oldPreset) == 0 && mixedPaths.count(legacyPreset) == 1 &&
                    mixedPaths.count(userPreset) == 1,
                "Case-variant stale bank suppression hid unrelated factory/user content.");
        pass("case-variant stale bank suppression preserves legacy factory and user presets");
        std::cout << "All embedded factory content regression checks passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
