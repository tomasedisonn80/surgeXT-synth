/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_WAVETABLE_SELECTION_H
#define SURGE_REFERENCE_WAVETABLE_SELECTION_H

#include "SurgeSynthProcessor.h"
#include "UndoManager.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace Surge::GUI::Reference
{
/** A popup belongs to the source that opened it, not just its oscillator number.
    The engine's replacement token changes on table load, paste and undo; the
    monotonic patch generation also catches same-patch XML-only reloads which
    leave that table buffer unchanged. */
struct WavetableSelectionTarget
{
    int scene{-1}, oscillator{-1}, algorithm{-1}, patchId{-1}, tableId{-1};
    std::uint64_t patchGeneration{}, replacementToken{};
    std::string patchName, patchCategory, tableName;

    bool operator==(const WavetableSelectionTarget &) const = default;
};

struct WavetableSelectionRequest
{
    WavetableSelectionTarget target;
    // A rescan can move catalog indices while a popup is open. Resolve the
    // selected file again immediately before submitting the native load.
    fs::path path;
};

struct WavetableSelectionSnapshot
{
    WavetableSelectionTarget target;
    bool applicable{false}, loadQueued{false};
};

/** Message-thread refresh only. Never block the loader or acquire this lock in
    paint. This follows getCurrentWavetableName's exact display-name precedence
    without calling that locking helper while already holding its mutex. */
inline std::optional<WavetableSelectionSnapshot>
snapshotWavetableSelection(SurgeSynthProcessor &processor, int scene, int oscillator)
{
    if (!processor.surge || scene < 0 || scene >= n_scenes || oscillator < 0 ||
        oscillator >= n_oscs || processor.surge->halt_engine)
        return std::nullopt;
    auto &synth = *processor.surge;
    auto &storage = synth.storage;
    std::unique_lock<std::mutex> lock(storage.waveTableDataMutex, std::try_to_lock);
    if (!lock.owns_lock() || synth.halt_engine)
        return std::nullopt;

    auto &patch = storage.getPatch();
    const auto &source = patch.scene[scene].osc[oscillator];
    WavetableSelectionSnapshot result;
    auto &target = result.target;
    target.patchGeneration = storage.patchLoadGeneration.load(std::memory_order_acquire);
    target.scene = scene;
    target.oscillator = oscillator;
    target.algorithm = source.type.val.i;
    target.patchId = synth.patchid;
    target.patchName = patch.name;
    target.patchCategory = patch.category;
    target.tableId = source.wt.current_id;
    target.replacementToken = storage.wtGenPublishToken[scene * n_oscs + oscillator];
    if (!source.wavetable_display_name.empty())
        target.tableName = source.wavetable_display_name;
    else if (target.tableId >= 0 && target.tableId < static_cast<int>(storage.wt_list.size()))
        target.tableName = storage.wt_list[target.tableId].name;
    else
        target.tableName = source.wt.flags & wtf_is_sample ? "(Patch Sample)" : "(Patch Wavetable)";
    result.applicable = uses_wavetabledata(target.algorithm);
    result.loadQueued = source.wt.queue_id >= 0 || !source.wt.queue_filename.empty() ||
                        source.wt.queue_reslice;
    if (synth.halt_engine ||
        storage.patchLoadGeneration.load(std::memory_order_acquire) != target.patchGeneration)
        return std::nullopt;
    return result;
}

/** Resolve both the source identity and catalog path; a stale request is a
    no-op. Kept independent of the popup so native tests can deliver a delayed
    request after a scene, algorithm, patch or catalog change. */
inline int resolveWavetableSelection(SurgeSynthProcessor &processor,
                                     const WavetableSelectionRequest &request)
{
    if (request.path.empty())
        return -1;
    const auto state = snapshotWavetableSelection(processor, request.target.scene,
                                                 request.target.oscillator);
    if (!state || !state->applicable || state->loadQueued || state->target != request.target)
        return -1;
    const auto &catalog = processor.surge->storage.wt_list;
    const auto found = std::find_if(catalog.begin(), catalog.end(),
                                   [&request](const auto &entry) { return entry.path == request.path; });
    return found == catalog.end() ? -1 : static_cast<int>(std::distance(catalog.begin(), found));
}

using WavetableScriptLoader = std::function<void(int, int, int, const fs::path &)>;
using WavetableAnnouncement = std::function<void(const std::string &)>;

/** Use the existing native undo/load/listener path. A normal wavetable is only
    queued here: the engine owns all actual file loading and table replacement.
    Script metadata/generation must go through the native editor's script API;
    a workspace without that editor disables script choices instead of treating
    a .wtscript as ordinary audio data. */
inline bool applyWavetableSelection(SurgeSynthProcessor &processor,
                                    const WavetableSelectionRequest &request,
                                    const WavetableScriptLoader &loadScript = {},
                                    const WavetableAnnouncement &announce = {})
{
    const int id = resolveWavetableSelection(processor, request);
    if (id < 0)
        return false;
    auto &storage = processor.surge->storage;
    const auto entry = storage.wt_list[id];
    const bool script = entry.path.extension() == ".wtscript";
    if (script && !loadScript)
        return false;
    const auto scene = request.target.scene;
    const auto oscillator = request.target.oscillator;
    if (processor.undoManager)
        processor.undoManager->pushWavetable(scene, oscillator);

    if (script)
        loadScript(scene, oscillator, id, entry.path);
    else
        storage.getPatch().scene[scene].osc[oscillator].wt.queue_id = id;

    if (announce)
        announce("Loading wavetable " + entry.name);
    // Match the native selector: notify the selected catalog ID together with
    // the current (not optimistically substituted) name. Script metadata may
    // already have updated that name, and a normal queued table may have loaded
    // in the meantime. Keep the prior real name if the loader holds its mutex.
    const auto current = snapshotWavetableSelection(processor, scene, oscillator);
    processor.paramChangeToListeners(nullptr, true, processor.SCT_WAVETABLE,
                                     static_cast<float>(scene), static_cast<float>(oscillator),
                                     static_cast<float>(id),
                                     current ? current->target.tableName : request.target.tableName);
    return true;
}
} // namespace Surge::GUI::Reference
#endif
