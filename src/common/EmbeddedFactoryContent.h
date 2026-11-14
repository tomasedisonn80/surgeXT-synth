/*
 * Surge XT - a free and open source hybrid synthesizer
 * Copyright 2026, various authors, as described in the GitHub transaction log.
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 */

#ifndef SURGE_SRC_COMMON_EMBEDDEDFACTORYCONTENT_H
#define SURGE_SRC_COMMON_EMBEDDEDFACTORYCONTENT_H

#include "filesystem/import.h"
#include <span>
#include <string>

namespace Surge::Storage
{
struct EmbeddedFactoryContentResult
{
    fs::path dataPath;
    std::string error;
    bool reusedInstalled{false};
};

// Call during non-realtime initialization only. The zstd resource is a bounded,
// versioned container of factory patches and reference skins. A complete matching
// installation is reused; otherwise only the application's private, content-keyed
// cache is written. Explicit user data paths do not pass through this function.
// The byte-oriented interface also permits testing without a synth or GUI.
EmbeddedFactoryContentResult materializeEmbeddedFactoryContent(
    const fs::path &installedDataRoot, const fs::path &applicationDataRoot,
    std::span<const char> compressedArchive, const std::string &contentId);
} // namespace Surge::Storage

#endif
