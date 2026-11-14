/*
 * Surge XT - a free and open source hybrid synthesizer
 * Copyright 2026, various authors, as described in the GitHub transaction log.
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 */

#include "EmbeddedFactoryContent.h"
#include "zstd.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Surge::Storage
{
namespace
{
constexpr size_t maxContainerBytes = 64 * 1024 * 1024;
constexpr size_t maxEntryCount = 1200;
constexpr std::string_view patchPrefix = "patches_factory/Scoring 1000/";
constexpr std::string_view skinPrefix = "skins/reference-";

struct Entry
{
    std::string name;
    std::span<const char> bytes;
};

uint32_t read32(std::span<const char> data, size_t &position)
{
    if (position > data.size() || data.size() - position < 4)
        throw std::runtime_error("Truncated embedded factory container header.");
    uint32_t value = 0;
    for (unsigned int n = 0; n < 4; ++n)
        value |= static_cast<uint32_t>(static_cast<unsigned char>(data[position++])) << (8 * n);
    return value;
}

std::string lowerAscii(std::string text)
{
    for (auto &ch : text)
        if (ch >= 'A' && ch <= 'Z')
            ch = static_cast<char>(ch + ('a' - 'A'));
    return text;
}

bool safeRelativeName(const std::string &name)
{
    if (name.empty() || name.size() > 1024 || name.front() == '/' || name.back() == '/')
        return false;
    for (const auto ch : name)
        if (static_cast<unsigned char>(ch) < 32 || static_cast<unsigned char>(ch) >= 127 ||
            ch == '\\' || ch == ':')
            return false;
    size_t begin = 0;
    while (begin < name.size())
    {
        const auto end = name.find('/', begin);
        const auto component = name.substr(begin, end == std::string::npos ? end : end - begin);
        if (component.empty() || component == "." || component == ".." ||
            component.back() == '.' || component.back() == ' ')
            return false;
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return true;
}

std::vector<Entry> parseContainer(std::span<const char> data)
{
    if (data.size() < 12 || std::memcmp(data.data(), "SXTEMB01", 8) != 0)
        throw std::runtime_error("Unsupported embedded factory container.");
    size_t position = 8;
    const auto count = read32(data, position);
    if (count < 1020 || count > maxEntryCount)
        throw std::runtime_error("Unexpected embedded factory entry count.");

    std::vector<Entry> entries;
    entries.reserve(count);
    std::set<std::string> names;
    std::map<std::string, size_t> categories;
    std::map<std::string, std::set<std::string>> skins;
    size_t patchCount = 0;
    for (uint32_t n = 0; n < count; ++n)
    {
        const auto nameSize = read32(data, position);
        const auto fileSize = read32(data, position);
        if (nameSize == 0 || nameSize > 1024 || nameSize > data.size() - position)
            throw std::runtime_error("Invalid embedded factory path length.");
        std::string name(data.data() + position, nameSize);
        position += nameSize;
        if (!safeRelativeName(name) || !names.insert(lowerAscii(name)).second)
            throw std::runtime_error("Unsafe or duplicate embedded factory path.");
        if (fileSize > data.size() - position)
            throw std::runtime_error("Truncated embedded factory file.");

        if (name.compare(0, patchPrefix.size(), patchPrefix) == 0)
        {
            const auto categoryEnd = name.find('/', patchPrefix.size());
            if (categoryEnd == std::string::npos || categoryEnd == patchPrefix.size() ||
                name.find('/', categoryEnd + 1) != std::string::npos ||
                name.size() < 4 || name.substr(name.size() - 4) != ".fxp")
                throw std::runtime_error("Invalid embedded scoring preset path.");
            ++categories[name.substr(patchPrefix.size(), categoryEnd - patchPrefix.size())];
            ++patchCount;
        }
        else if (name.compare(0, skinPrefix.size(), skinPrefix) == 0)
        {
            const auto skinEnd = name.find('/', skinPrefix.size());
            if (skinEnd == std::string::npos)
                throw std::runtime_error("Invalid embedded reference skin path.");
            const auto package = name.substr(6, skinEnd - 6);
            constexpr std::string_view ending = ".surge-skin";
            if (package.size() <= ending.size() ||
                package.compare(package.size() - ending.size(), ending.size(), ending) != 0)
                throw std::runtime_error("Invalid embedded reference skin package.");
            skins[package].insert(name.substr(skinEnd + 1));
        }
        else
            throw std::runtime_error("Unexpected embedded factory content path.");

        entries.push_back({std::move(name), data.subspan(position, fileSize)});
        position += fileSize;
    }
    if (position != data.size() || patchCount != 1000 || categories.size() != 10 ||
        skins.size() != 10)
        throw std::runtime_error("Incomplete embedded factory content.");
    for (const auto &[name, size] : categories)
        if (size != 100)
            throw std::runtime_error("Incorrect embedded scoring category size.");
    for (const auto &[name, files] : skins)
        if (files.count("skin.xml") != 1 || files.count("workspace.json") != 1)
            throw std::runtime_error("Incomplete embedded reference skin package.");
    return entries;
}

bool fileMatches(const fs::path &file, std::span<const char> expected)
{
    std::error_code ec;
    if (!fs::is_regular_file(file, ec) || ec || fs::file_size(file, ec) != expected.size() || ec)
        return false;
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        return false;
    std::array<char, 64 * 1024> block{};
    size_t position = 0;
    while (position < expected.size())
    {
        const auto size = std::min(block.size(), expected.size() - position);
        stream.read(block.data(), static_cast<std::streamsize>(size));
        if (stream.gcount() != static_cast<std::streamsize>(size) ||
            std::memcmp(block.data(), expected.data() + position, size) != 0)
            return false;
        position += size;
    }
    return true;
}

// Only the application-data ancestor may be a user-selected symlink. Everything
// created below its canonical path is private cache state and must be a real
// directory/file, never a link to user content elsewhere.
void requireRealDirectory(const fs::path &directory)
{
    std::error_code ec;
    auto status = fs::symlink_status(directory, ec);
    if (ec && ec != std::errc::no_such_file_or_directory)
        throw fs::filesystem_error("Cannot inspect factory cache directory", directory, ec);
    if (fs::is_symlink(status))
        throw std::runtime_error("A factory cache directory is a symbolic link.");
    if (!fs::exists(status))
    {
        fs::create_directory(directory, ec);
        if (ec)
            throw fs::filesystem_error("Cannot create factory cache directory", directory, ec);
        status = fs::symlink_status(directory);
    }
    if (!fs::is_directory(status))
        throw std::runtime_error("A factory cache directory is not a directory.");
}

void requireRealParents(const fs::path &root, const fs::path &relative)
{
    auto directory = root;
    for (const auto &component : relative.parent_path())
    {
        directory /= component;
        requireRealDirectory(directory);
    }
    std::error_code ec;
    const auto status = fs::symlink_status(root / relative, ec);
    if (ec && ec != std::errc::no_such_file_or_directory)
        throw fs::filesystem_error("Cannot inspect factory cache file", root / relative, ec);
    if (fs::is_symlink(status) || (fs::exists(status) && !fs::is_regular_file(status)))
        throw std::runtime_error("A factory cache file is not a regular file.");
}

void verifyCacheInventory(const fs::path &root, const std::vector<Entry> &entries)
{
    std::set<fs::path> expectedFiles, expectedDirectories;
    for (const auto &entry : entries)
    {
        const auto relative = fs::path(entry.name);
        expectedFiles.insert(relative);
        for (auto parent = relative.parent_path(); !parent.empty(); parent = parent.parent_path())
            expectedDirectories.insert(parent);
    }
    // Do not follow or later expose an unexpected link through patch/skin
    // discovery, including links that are not one of the expected filenames.
    for (const auto &item : fs::recursive_directory_iterator(root))
    {
        const auto relative = item.path().lexically_relative(root);
        const auto status = item.symlink_status();
        if (fs::is_symlink(status))
            throw std::runtime_error("The factory cache contains an unexpected symbolic link.");
        if (fs::is_directory(status))
        {
            if (expectedDirectories.count(relative) == 0)
                throw std::runtime_error("The factory cache contains an unexpected directory.");
        }
        else if (!fs::is_regular_file(status) ||
                 (expectedFiles.count(relative) == 0 && relative.filename() != ".DS_Store"))
            throw std::runtime_error("The factory cache contains an unexpected file.");
    }
}

bool contentMatches(const fs::path &root, const std::vector<Entry> &entries)
{
    if (root.empty())
        return false;
    for (const auto &entry : entries)
        if (!fileMatches(root / fs::path(entry.name), entry.bytes))
            return false;

    // An older or edited installation may contain every expected patch plus
    // additional FXPs. Do not expose these as a second/expanded Scoring 1000.
    std::set<std::string> expectedPatches;
    for (const auto &entry : entries)
        if (entry.name.compare(0, patchPrefix.size(), patchPrefix) == 0)
            expectedPatches.insert(entry.name);
    std::error_code ec;
    fs::recursive_directory_iterator it(root / fs::path(patchPrefix), ec), end;
    if (ec)
        return false;
    size_t count = 0;
    while (it != end)
    {
        const auto &path = it->path();
        if (lowerAscii(path.extension().u8string()) == ".fxp")
        {
            const auto relative = path.lexically_relative(root).generic_u8string();
            if (expectedPatches.count(relative) != 1)
                return false;
            ++count;
        }
        it.increment(ec);
        if (ec)
            return false;
    }
    return count == 1000;
}

struct StagingDirectory
{
    fs::path path;
    ~StagingDirectory()
    {
        if (!path.empty())
        {
            std::error_code ignored;
            fs::remove_all(path, ignored);
        }
    }
};

fs::path newStagingDirectory(const fs::path &parent)
{
    static std::atomic<uint64_t> counter{0};
    // create_directory is the exclusive operation: even equal clock values in
    // separate processes cannot cause a shared staging directory.
    for (unsigned int attempt = 0; attempt < 128; ++attempt)
    {
        const auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto candidate = parent / (".staging-" + std::to_string(ticks) + "-" +
                                   std::to_string(counter.fetch_add(1)));
        std::error_code ec;
        if (fs::create_directory(candidate, ec))
            return candidate;
        if (ec && ec != std::errc::file_exists)
            throw fs::filesystem_error("Cannot create factory staging directory", candidate, ec);
    }
    throw std::runtime_error("Cannot reserve a factory staging directory.");
}

void replaceFile(const fs::path &source, const fs::path &destination)
{
#if defined(_WIN32)
    // std::filesystem::rename cannot replace an existing file on Windows.
    // Both paths are on the cache filesystem, so this replacement is atomic.
    if (!MoveFileExW(source.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING))
        throw fs::filesystem_error("Cannot publish factory cache file", source, destination,
                                   std::error_code(GetLastError(), std::system_category()));
#else
    fs::rename(source, destination);
#endif
}
} // namespace

EmbeddedFactoryContentResult materializeEmbeddedFactoryContent(
    const fs::path &installedDataRoot, const fs::path &applicationDataRoot,
    std::span<const char> compressedArchive, const std::string &contentId)
{
    EmbeddedFactoryContentResult result;
    try
    {
        static std::mutex initializationMutex;
        const std::lock_guard<std::mutex> lock(initializationMutex);
        if (contentId.size() != 64 ||
            !std::all_of(contentId.begin(), contentId.end(), [](const char ch) {
                return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
            }))
            throw std::runtime_error("Invalid embedded factory content identifier.");
        if (compressedArchive.empty() || compressedArchive.size() > maxContainerBytes)
            throw std::runtime_error("Invalid embedded factory archive size.");
        const auto expandedSize =
            ZSTD_getFrameContentSize(compressedArchive.data(), compressedArchive.size());
        if (expandedSize == ZSTD_CONTENTSIZE_ERROR || expandedSize == ZSTD_CONTENTSIZE_UNKNOWN ||
            expandedSize < 12 || expandedSize > maxContainerBytes)
            throw std::runtime_error("Invalid or oversized embedded factory zstd frame.");
        if (ZSTD_findFrameCompressedSize(compressedArchive.data(), compressedArchive.size()) !=
            compressedArchive.size())
            throw std::runtime_error("Unexpected trailing embedded factory archive data.");
        std::vector<char> container(static_cast<size_t>(expandedSize));
        const auto decoded = ZSTD_decompress(container.data(), container.size(),
                                             compressedArchive.data(), compressedArchive.size());
        if (ZSTD_isError(decoded) || decoded != container.size())
            throw std::runtime_error("Cannot decompress embedded factory content.");
        const auto entries = parseContainer(container);

        if (contentMatches(installedDataRoot, entries))
        {
            result.dataPath = installedDataRoot;
            result.reusedInstalled = true;
            return result;
        }
        if (applicationDataRoot.empty())
            throw std::runtime_error("No application data directory is available for factory content.");

        fs::create_directories(applicationDataRoot);
        const auto parent = fs::canonical(applicationDataRoot) / "FactoryContent";
        requireRealDirectory(parent);
        const auto cache = parent / contentId;
        requireRealDirectory(cache);
        for (const auto &entry : entries)
            requireRealParents(cache, fs::path(entry.name));
        verifyCacheInventory(cache, entries);

        if (!contentMatches(cache, entries))
        {
            StagingDirectory staging{newStagingDirectory(parent)};
            size_t index = 0;
            for (const auto &entry : entries)
            {
                const auto relative = fs::path(entry.name);
                const auto destination = cache / relative;
                if (!fileMatches(destination, entry.bytes))
                {
                    const auto temporary = staging.path / std::to_string(index);
                    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
                    if (!file)
                        throw std::runtime_error("Cannot open a factory cache staging file.");
                    file.write(entry.bytes.data(), static_cast<std::streamsize>(entry.bytes.size()));
                    file.close();
                    if (!file)
                        throw std::runtime_error("Cannot write complete factory content (check free disk space).");
                    requireRealParents(cache, relative);
                    replaceFile(temporary, destination);
                }
                ++index;
            }
            if (!contentMatches(cache, entries))
                throw std::runtime_error("Factory cache verification failed after materialization.");
        }
        result.dataPath = cache;
    }
    catch (const std::exception &error)
    {
        result.dataPath.clear();
        result.error = error.what();
    }
    return result;
}
} // namespace Surge::Storage
