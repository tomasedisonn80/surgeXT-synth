/*
 * Surge XT modulation-processing mask differential regression tests.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * This executable is not linked into the plugin.
 */
#include "SurgeSynthesizer.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
struct Proxy : SurgeSynthesizer::PluginLayer
{
    void surgeParameterUpdated(const SurgeSynthesizer::ID &, float) override {}
    void surgeMacroUpdated(long, float) override {}
};

using Mask = std::array<std::array<bool, n_modsources>, n_scenes>;

Mask readMask(const SurgePatch &patch)
{
    Mask result{};
    for (int scene = 0; scene < n_scenes; ++scene)
        for (int source = 0; source < n_modsources; ++source)
            result[scene][source] = patch.scene[scene].modsource_doprocess[source];
    return result;
}

void writeMask(SurgePatch &patch, const Mask &mask)
{
    for (int scene = 0; scene < n_scenes; ++scene)
        for (int source = 0; source < n_modsources; ++source)
            patch.scene[scene].modsource_doprocess[source] = mask[scene][source];
}

// Independent oracle copied from prepareModsourceDoProcess at
// 0dcc076937974605095a9567dacd3f28a0b04fd8. Keep the original control flow here;
// simplifying it alongside the production implementation defeats this comparison.
// The only adaptations are the explicit synth parameter and std::vector spelling.
void referencePrepare(SurgeSynthesizer &synth, int scenemask)
{
    auto &storage = synth.storage;
    for (int scene = 0; scene < n_scenes; scene++)
    {
        bool anyFormula{false};
        for (auto &lfs : storage.getPatch().scene[scene].lfo)
        {
            if (lfs.shape.val.i == lt_formula)
            {
                anyFormula = true;
            }
        }
        if ((1 << scene) & scenemask)
        {
            for (int i = 0; i < n_modsources; i++)
            {
                bool setTo = false;
                if (i >= ms_lfo1 && i <= ms_lfo6)
                {
                    auto lf = &(storage.getPatch().scene[scene].lfo[i - ms_lfo1]);
                    if (lf->shape.val.i == lt_stepseq)
                    {
                        auto ss = &(storage.getPatch().stepsequences[scene][i - ms_lfo1]);
                        setTo = (ss->trigmask != 0);
                    }
                }
                storage.getPatch().scene[scene].modsource_doprocess[i] = setTo;

                if (i == ms_pitchbend || i == ms_aftertouch || i == ms_modwheel || i == ms_breath ||
                    i == ms_expression || i == ms_sustain || i == ms_lowest_key ||
                    i == ms_highest_key || i == ms_latest_key)
                {
                    storage.getPatch().scene[scene].modsource_doprocess[i] |= anyFormula;
                }
            }

            for (int j = 0; j < 3; j++)
            {
                std::vector<ModulationRouting> *modlist;

                switch (j)
                {
                case 0:
                    modlist = &storage.getPatch().modulation_global;
                    break;
                case 1:
                    modlist = &storage.getPatch().scene[scene].modulation_scene;
                    break;
                case 2:
                    modlist = &storage.getPatch().scene[scene].modulation_voice;
                    break;
                }

                int n = modlist->size();
                for (int i = 0; i < n; i++)
                {
                    int id = modlist->at(i).source_id;
                    assert((id > 0) && (id < n_modsources));
                    storage.getPatch().scene[scene].modsource_doprocess[id] = true;
                }
            }
        }
    }
}

class Suite
{
  public:
    explicit Suite(SurgeSynthesizer &synth) : synth(synth), patch(synth.storage.getPatch()) {}

    void resetInputs()
    {
        patch.modulation_global.clear();
        for (int scene = 0; scene < n_scenes; ++scene)
        {
            patch.scene[scene].modulation_scene.clear();
            patch.scene[scene].modulation_voice.clear();
            for (int lfo = 0; lfo < n_lfos; ++lfo)
            {
                patch.scene[scene].lfo[lfo].shape.val.i = lt_sine;
                patch.stepsequences[scene][lfo].trigmask = 0;
            }
        }
    }

    void seedFlags(unsigned int pattern)
    {
        for (int scene = 0; scene < n_scenes; ++scene)
            for (int source = 0; source < n_modsources; ++source)
            {
                const bool value = pattern == 0 ? false : pattern == 1 ? true :
                    ((source + scene + static_cast<int>(pattern)) & 1) != 0;
                patch.scene[scene].modsource_doprocess[source] = value;
            }
    }

    void check(const std::string &label, int scenemask)
    {
        const auto before = readMask(patch);
        referencePrepare(synth, scenemask);
        const auto expected = readMask(patch);
        writeMask(patch, before);
        synth.prepareModsourceDoProcess(scenemask);
        const auto actual = readMask(patch);
        ++cases;
        for (int scene = 0; scene < n_scenes; ++scene)
            for (int source = 0; source < n_modsources; ++source)
            {
                ++flags;
                if (actual[scene][source] != expected[scene][source])
                    throw std::runtime_error(label + ": case=" + std::to_string(cases) +
                        " scenemask=" + std::to_string(scenemask) +
                        " scene=" + std::to_string(scene) +
                        " source=" + std::to_string(source) +
                        " expected=" + std::to_string(expected[scene][source]) +
                        " actual=" + std::to_string(actual[scene][source]));
            }
    }

    void allSceneMasks(const std::string &label)
    {
        for (int scenemask = 0; scenemask < (1 << n_scenes); ++scenemask)
        {
            seedFlags(static_cast<unsigned int>(cases % 4));
            check(label, scenemask);
        }
    }

    std::vector<ModulationRouting> &routes(int kind, int scene)
    {
        if (kind == 0)
            return patch.modulation_global;
        if (kind == 1)
            return patch.scene[scene].modulation_scene;
        return patch.scene[scene].modulation_voice;
    }

    ModulationRouting route(int source, int kind, int scene, int sourceScene,
                            bool muted, float depth)
    {
        ModulationRouting result{};
        result.source_id = source;
        result.destination_id = kind == 0 ? patch.volume.id :
            patch.scene[scene].osc[0].pitch.param_id_in_scene;
        result.source_scene = sourceScene;
        result.muted = muted;
        result.depth = depth;
        return result;
    }

    void emptyAndInactiveScenes()
    {
        resetInputs();
        for (unsigned int pattern = 0; pattern < 4; ++pattern)
            for (int scenemask = 0; scenemask < (1 << n_scenes); ++scenemask)
            {
                seedFlags(pattern);
                check("empty routes/inactive sentinel flags", scenemask);
            }
    }

    void shapesAndTriggers()
    {
        for (int scene = 0; scene < n_scenes; ++scene)
            for (int lfo = 0; lfo < n_lfos; ++lfo)
                for (int shape = 0; shape < n_lfo_types; ++shape)
                    for (const auto trigger : {UINT64_C(0), UINT64_C(1), UINT64_C(1) << 63})
                    {
                        resetInputs();
                        patch.scene[scene].lfo[lfo].shape.val.i = shape;
                        patch.stepsequences[scene][lfo].trigmask = trigger;
                        allSceneMasks("shape/position/trigger matrix");
                    }

        for (int scene = 0; scene < n_scenes; ++scene)
            for (unsigned int combination = 0; combination < (1u << n_lfos_voice); ++combination)
            {
                resetInputs();
                for (int lfo = 0; lfo < n_lfos_voice; ++lfo)
                {
                    patch.scene[scene].lfo[lfo].shape.val.i = lt_stepseq;
                    patch.stepsequences[scene][lfo].trigmask =
                        (combination & (1u << lfo)) != 0 ? UINT64_C(1) << 32 : 0;
                }
                allSceneMasks("all six step-trigger combinations");
            }
    }

    void formulaSubsets()
    {
        for (int scene = 0; scene < n_scenes; ++scene)
            for (unsigned int combination = 0; combination < (1u << n_lfos); ++combination)
            {
                resetInputs();
                for (int lfo = 0; lfo < n_lfos; ++lfo)
                    patch.scene[scene].lfo[lfo].shape.val.i =
                        (combination & (1u << lfo)) != 0 ? lt_formula : lt_sine;
                allSceneMasks("all twelve formula subsets");
            }
    }

    void routingMatrix()
    {
        for (int kind = 0; kind < 3; ++kind)
            for (int scene = 0; scene < n_scenes; ++scene)
                for (int source = 1; source < n_modsources; ++source)
                    for (int sourceScene = 0; sourceScene < n_scenes; ++sourceScene)
                        for (const bool muted : {false, true})
                            for (const float depth : {0.0f, 0.375f, -0.25f})
                            {
                                resetInputs();
                                routes(kind, scene).push_back(
                                    route(source, kind, scene, sourceScene, muted, depth));
                                allSceneMasks("routing source/list/scene/mute/depth matrix");
                            }

        resetInputs();
        for (int scene = 0; scene < n_scenes; ++scene)
            for (int kind = 0; kind < 3; ++kind)
                for (int source = 1; source < n_modsources; ++source)
                {
                    const auto item = route(source, kind, scene, 1 - scene, true, 0.0f);
                    routes(kind, scene).push_back(item);
                    routes(kind, scene).push_back(item);
                }
        allSceneMasks("dense duplicate muted zero-depth routes");

        // Explicit invariants supplement the copied oracle. Global source_scene
        // does not restrict which active scene gets its processing flag.
        resetInputs();
        patch.modulation_global.push_back(route(ms_lfo2, 0, 0, 1, true, 0.0f));
        seedFlags(0);
        check("muted zero-depth global route remains enabled", 3);
        requireFlag(0, ms_lfo2, true);
        requireFlag(1, ms_lfo2, true);
        patch.modulation_global.clear();
        check("route removal clears old active flags", 3);
        requireFlag(0, ms_lfo2, false);
        requireFlag(1, ms_lfo2, false);
    }

    void evolvingCases()
    {
        constexpr unsigned int randomSeed = 0x53555247u;
        std::minstd_rand random(randomSeed);
        resetInputs();
        seedFlags(2);
        for (unsigned int iteration = 0; iteration < 12000; ++iteration)
        {
            const int scene = static_cast<int>(random() % n_scenes);
            const int lfo = static_cast<int>(random() % n_lfos);
            const int kind = static_cast<int>(random() % 3);
            auto &list = routes(kind, scene);
            switch (random() % 7)
            {
            case 0:
                patch.scene[scene].lfo[lfo].shape.val.i = static_cast<int>(random() % n_lfo_types);
                break;
            case 1:
                patch.stepsequences[scene][lfo].trigmask =
                    random() % 2 == 0 ? 0 : UINT64_C(1) << (random() % 64);
                break;
            case 2:
                if (list.size() < 64)
                    list.push_back(route(1 + static_cast<int>(random() % (n_modsources - 1)),
                        kind, scene, static_cast<int>(random() % n_scenes),
                        random() % 2 != 0, random() % 2 != 0 ? 0.25f : 0.0f));
                break;
            case 3:
                if (!list.empty())
                    list.erase(list.begin() + static_cast<std::ptrdiff_t>(random() % list.size()));
                break;
            case 4:
                if (!list.empty())
                {
                    auto &item = list[random() % list.size()];
                    item.muted = !item.muted;
                    item.depth = item.depth == 0 ? -0.25f : 0.0f;
                }
                break;
            case 5:
                if (!list.empty())
                {
                    auto &item = list[random() % list.size()];
                    item.source_id = 1 + static_cast<int>(random() % (n_modsources - 1));
                    item.source_scene = static_cast<int>(random() % n_scenes);
                }
                break;
            case 6:
                if (iteration % 23 == 0)
                    list.clear();
                break;
            }
            // Intentionally retain flags from the preceding call: inactive
            // scenes and scene activation must also work across live edits.
            check("evolving routing/LFO sequence seed=0x53555247 iteration=" +
                      std::to_string(iteration), static_cast<int>(random() % (1u << n_scenes)));
        }
    }

    void requireFlag(int scene, int source, bool expected) const
    {
        if (patch.scene[scene].modsource_doprocess[source] != expected)
            throw std::runtime_error("Explicit route invariant failed: scene=" +
                std::to_string(scene) + " source=" + std::to_string(source));
    }

    std::uint64_t cases{0}, flags{0};

  private:
    SurgeSynthesizer &synth;
    SurgePatch &patch;
};
} // namespace

int main()
{
    try
    {
        Proxy proxy;
        auto synth = std::make_unique<SurgeSynthesizer>(
            &proxy, SurgeStorage::skipPatchLoadDataPathSentinel);
        Suite suite(*synth);
        suite.emptyAndInactiveScenes();
        suite.shapesAndTriggers();
        suite.formulaSubsets();
        suite.routingMatrix();
        suite.evolvingCases();
        std::cout << "Modulation mask differential tests passed: " << suite.cases
                  << " cases, " << suite.flags << " individual flag comparisons.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "surge-modulation-mask-test: " << error.what() << '\n';
        return 1;
    }
}
