/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "WorkspaceData.h"
#include "SurgeSynthProcessor.h"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace Surge::GUI::Reference
{
namespace
{
constexpr size_t routeLimit = 128;
constexpr int browserLimit = 100;

bool validIndex(int index, size_t size) { return index >= 0 && static_cast<size_t>(index) < size; }

bool sourceIsBipolar(const SurgeSynthesizer &synth, modsources source, int scene)
{
    // The general engine helper uses scene_active. This workspace can inspect the
    // other scene, so LFO polarity must be read from the actual route's source scene.
    if (isLFO(source))
    {
        const auto &lfo = synth.storage.getPatch().scene[scene].lfo[source - ms_lfo1];
        return !lfo.unipolar.val.i && lfo.shape.val.i != lt_envelope;
    }
    return synth.isBipolarModulation(source);
}
} // namespace

std::vector<RouteRow> modulationRows(SurgeSynthProcessor &processor, int scene)
{
    std::vector<RouteRow> result;
    if (!processor.surge)
        return result;

    scene = std::clamp(scene, 0, n_scenes - 1);
    auto &synth = *processor.surge;
    auto &storage = synth.storage;
    std::unique_lock<std::recursive_mutex> lock(storage.modRoutingMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return result;

    auto &patch = storage.getPatch();
    result.reserve(16);
    const auto append = [&](const std::vector<ModulationRouting> &routes, int idBase, bool global) {
        for (const auto &route : routes)
        {
            if (result.size() >= routeLimit)
                break;
            if (route.source_id <= ms_original || route.source_id >= n_modsources ||
                !std::isfinite(route.depth))
                continue;

            const auto source = static_cast<modsources>(route.source_id);
            const bool distinct = isModulatorDistinctPerScene(source);
            const int sourceScene = global && distinct ? route.source_scene : scene;
            if (sourceScene < 0 || sourceScene >= n_scenes ||
                (global && distinct && sourceScene != scene))
                continue;
            if (isLFO(source) && (route.source_index < 0 || route.source_index >= max_lfo_indices))
                continue;

            const auto destination = static_cast<int64_t>(route.destination_id) + idBase;
            if (destination < 0 || static_cast<size_t>(destination) >= patch.param_ptr.size())
                continue;
            auto *parameter = patch.param_ptr[static_cast<size_t>(destination)];
            if (parameter == nullptr)
                continue;

            char destinationName[1024]{};
            SurgeSynthesizer::ID parameterId;
            if (!synth.fromSynthSideId(static_cast<int>(destination), parameterId))
                continue;
            if (parameter->ctrlgroup == cg_FX)
                synth.getParameterNameExtendedByFXGroup(parameterId, destinationName);
            else
                synth.getParameterName(parameterId, destinationName);

            char amount[TXT_SIZE]{};
            ModulationDisplayInfoWindowStrings displayInfo;
            const float extendedDepth =
                parameter->extend_range ? parameter->get_extended(route.depth) : route.depth;
            if (!std::isfinite(extendedDepth))
                continue;
            parameter->get_display_of_modulation_depth(amount, extendedDepth,
                                                       sourceIsBipolar(synth, source, sourceScene),
                                                       Parameter::InfoWindow, &displayInfo);

            const float normalized = parameter->get_modulation_f01(route.depth);
            RouteRow row;
            row.source = juce::String(ModulatorName::modulatorNameWithIndex(
                                          &storage, sourceScene, route.source_id,
                                          route.source_index, false, global && distinct))
                             .substring(0, 160);
            row.destination = juce::String(destinationName).substring(0, 200);
            row.amount = juce::String(amount).substring(0, 100);
            row.normalizedDepth =
                std::isfinite(normalized) ? std::clamp(normalized, -1.f, 1.f) : 0.f;
            row.muted = route.muted;
            result.push_back(std::move(row));
        }
    };

    // Show the selected scene first, then shared/global targets relevant to it.
    append(patch.scene[scene].modulation_voice, patch.scene_start[scene], false);
    append(patch.scene[scene].modulation_scene, patch.scene_start[scene], false);
    append(patch.modulation_global, 0, true);
    return result;
}

std::vector<BrowserRow> browserRows(SurgeSynthProcessor &processor, const juce::String &query,
                                    int maxRows)
{
    std::vector<BrowserRow> result;
    maxRows = std::clamp(maxRows, 0, browserLimit);
    if (!processor.surge || maxRows == 0)
        return result;

    // refreshPatchlist and native browser operations are message-thread work;
    // callers must obey the same ownership rather than reading from audio/background threads.
    auto &storage = processor.surge->storage;
    const auto needle = query.trim().substring(0, 256);
    result.reserve(static_cast<size_t>(maxRows));
    const auto &order = storage.patchOrdering;
    const size_t patchCount = order.empty() ? storage.patch_list.size() : order.size();
    const auto patchAt = [&](size_t i) { return order.empty() ? static_cast<int>(i) : order[i]; };
    const auto full = [&] { return result.size() >= static_cast<size_t>(maxRows); };
    const auto addPatch = [&](int id) {
        if (validIndex(id, storage.patch_list.size()) && !full())
            result.push_back(
                {juce::String(storage.patch_list[id].name).substring(0, 200), id, false});
    };

    if (needle.isNotEmpty())
    {
        for (size_t i = 0; i < patchCount; ++i)
        {
            if (full())
                break;
            const int id = patchAt(i);
            if (!validIndex(id, storage.patch_list.size()))
                continue;
            const auto &patch = storage.patch_list[id];
            const juce::String name(patch.name);
            const juce::String category =
                validIndex(patch.category, storage.patch_category.size())
                    ? juce::String(storage.patch_category[patch.category].name)
                    : juce::String();
            if (name.containsIgnoreCase(needle) || category.containsIgnoreCase(needle))
                addPatch(id);
        }
        return result;
    }

    int currentCategory = -1;
    const int currentId = processor.surge->patchid;
    if (validIndex(currentId, storage.patch_list.size()))
        currentCategory = storage.patch_list[currentId].category;
    if (!validIndex(currentCategory, storage.patch_category.size()))
    {
        const auto &currentCategoryName = storage.getPatch().category;
        for (size_t i = 0; i < storage.patch_category.size(); ++i)
            if (storage.patch_category[i].name == currentCategoryName)
            {
                currentCategory = static_cast<int>(i);
                break;
            }
    }
    const auto expandCategory = [&](int categoryId) {
        if (!validIndex(categoryId, storage.patch_category.size()) || full())
            return;
        result.push_back(
            {juce::String(storage.patch_category[categoryId].name).substring(0, 200), -1, true});
        int added = 0;
        for (size_t i = 0; i < patchCount && !full() && added < 5; ++i)
        {
            const int id = patchAt(i);
            if (validIndex(id, storage.patch_list.size()) &&
                storage.patch_list[id].category == categoryId)
            {
                addPatch(id);
                ++added;
            }
        }
    };

    if (validIndex(currentCategory, storage.patch_category.size()))
        expandCategory(currentCategory);

    const auto &categoryOrder = storage.patchCategoryOrdering;
    const size_t categoryCount =
        categoryOrder.empty() ? storage.patch_category.size() : categoryOrder.size();
    for (size_t i = 0; i < categoryCount; ++i)
    {
        if (full())
            break;
        const int categoryId = categoryOrder.empty() ? static_cast<int>(i) : categoryOrder[i];
        if (!validIndex(categoryId, storage.patch_category.size()))
            continue;
        if (categoryId == currentCategory)
            continue;
        const auto &category = storage.patch_category[categoryId];
        if (!category.isRoot || category.numberOfPatchesInCategoryAndChildren == 0)
            continue;
        if (result.empty())
            expandCategory(categoryId);
        else
            result.push_back({juce::String(category.name).substring(0, 200), -1, true});
    }

    // Catalogs with no category metadata still expose real installed patches.
    if (result.empty())
        for (size_t i = 0; i < patchCount; ++i)
        {
            if (full())
                break;
            addPatch(patchAt(i));
        }
    return result;
}
} // namespace Surge::GUI::Reference
