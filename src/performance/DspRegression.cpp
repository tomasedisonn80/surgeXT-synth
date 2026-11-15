/*
 * Surge XT finite DSP compatibility fixtures for CPU optimizations.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Build this identical harness against baseline and candidate engines and compare
 * the complete PCM files. This executable is never linked into a plugin.
 */
#include "SurgeSynthesizer.h"
#include "ConditionerEffect.h"
#include "GraphicEQ11BandEffect.h"
#include "ParametricEQ3BandEffect.h"
#include "sst/plugininfra/cpufeatures.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fsys = std::filesystem;
constexpr int blocks = 1536;
constexpr int releaseBlock = 1024;
constexpr unsigned int seed = 1801;
constexpr int slotIndex = fxslot_global1;
constexpr std::array<int, 3> rates{44100, 48000, 96000};

struct Proxy : SurgeSynthesizer::PluginLayer
{
    void surgeParameterUpdated(const SurgeSynthesizer::ID &, float) override {}
    void surgeMacroUpdated(long, float) override {}
};

struct Errors : SurgeStorage::ErrorListener
{
    std::vector<std::string> messages;
    void onSurgeError(const std::string &message, const std::string &title,
                      const SurgeStorage::ErrorType &) override
    {
        messages.push_back(title + ": " + message);
    }
    void check() const
    {
        if (!messages.empty())
            throw std::runtime_error(messages.front());
    }
};

struct Fixture
{
    std::string name;
    int effect = fxt_off;
    int lowcutSlope = -1;
    int initialRateIndex = 0;
    int controlPhaseOffset = 0;
};

// Effect initialization can run on a host thread before the audio callback has
// established its floating-point environment. Restore that environment even if
// initialization throws, so the subsequent processing has the ordinary mode.
struct ScopedRoundingMode
{
    int previous = std::fegetround();
    explicit ScopedRoundingMode(int mode)
    {
        if (previous == -1 || std::fesetround(mode) != 0)
            throw std::runtime_error("Cannot establish regression rounding mode");
    }
    ~ScopedRoundingMode() { std::fesetround(previous); }
};

void reseed(SurgeSynthesizer &synth)
{
    synth.storage.rngGen.g.seed(seed);
    synth.storage.noiseRng = sst::basic_blocks::dsp::RNG(seed + 1);
    std::srand(seed + 2);
}

void loadEffect(SurgeSynthesizer &synth, int effect)
{
    auto &patch = synth.storage.getPatch();
    auto &type = patch.fx[slotIndex].type;
    synth.setParameter01(synth.idForParameter(&type),
                         float(effect - type.val_min.i) /
                             float(type.val_max.i - type.val_min.i),
                         false);
    synth.process();
    if (!synth.fx[slotIndex] || patch.fx[slotIndex].type.val.i != effect)
        throw std::runtime_error("Regression effect did not load");
}

void configureCoefficients(FxStorage &slot, int effect, float phase)
{
    if (effect == fxt_conditioner)
    {
        slot.p[ConditionerEffect::cond_bass].val.f = 6.f * phase;
        slot.p[ConditionerEffect::cond_treble].val.f = -4.f * phase;
        slot.p[ConditionerEffect::cond_hpwidth].val.f = -24.f + 24.f * phase;
    }
    else if (effect == fxt_eq)
    {
        for (int band = 0; band < 3; ++band)
        {
            slot.p[3 * band].val.f = (band == 1 ? -6.f : 5.f) * phase;
            slot.p[3 * band + 1].val.f = -24.f + band * 24.f + phase * 12.f;
            slot.p[3 * band + 2].val.f = 0.5f + 1.5f * phase;
        }
    }
    else if (effect == fxt_geq11)
    {
        for (int band = 0; band < 11; ++band)
            slot.p[band].val.f = (band % 2 == 0 ? 5.f : -4.f) * phase;
    }
}

void deactivateBands(FxStorage &slot, int effect, bool deactivated)
{
    if (effect == fxt_conditioner)
    {
        for (auto parameter : {ConditionerEffect::cond_bass, ConditionerEffect::cond_treble,
                               ConditionerEffect::cond_hpwidth})
            slot.p[parameter].deactivated = deactivated;
    }
    else if (effect == fxt_eq)
        for (int band = 0; band < 3; ++band)
            slot.p[3 * band].deactivated = deactivated;
    else if (effect == fxt_geq11)
        for (int band = 0; band < 11; ++band)
            slot.p[band].deactivated = deactivated;
}

std::vector<float> render(const Fixture &fixture)
{
    Proxy proxy;
    Errors errors;
    std::srand(seed + 2);
    auto synth = std::make_unique<SurgeSynthesizer>(
        &proxy, SurgeStorage::skipPatchLoadDataPathSentinel);
    synth->storage.addErrorListener(&errors);
    auto guard = sst::plugininfra::cpufeatures::FPUStateGuard();
    synth->setSamplerate(rates[fixture.initialRateIndex]);
    synth->time_data.tempo = 120;
    synth->time_data.ppqPos = 0;
    reseed(*synth);
    auto &patch = synth->storage.getPatch();
    patch.polylimit.val.i = 32;
    patch.fx_bypass.val.i = fxb_all_fx;
    patch.fx_disable.val.i = 0;
    if (fixture.effect != fxt_off)
    {
        loadEffect(*synth, fixture.effect);
        configureCoefficients(patch.fx[slotIndex], fixture.effect, 0.75f);
        deactivateBands(patch.fx[slotIndex], fixture.effect, false);
    }
    else
    {
        patch.scenemode.val.i = sm_dual;
        for (int scene = 0; scene < n_scenes; ++scene)
        {
            patch.scene[scene].lowcut.deactivated = false;
            patch.scene[scene].lowcut.deform_type = fixture.lowcutSlope;
            patch.scene[scene].lowcut.val.f = -24.f + scene * 12.f;
        }
    }
    for (int block = 0; block < 128; ++block)
        synth->process();
    reseed(*synth);
    synth->time_data.ppqPos = 0;
    for (int note = 0; note < 8; ++note)
        synth->playNote(0, 48 + note, 96, 0);
    errors.check();

    std::vector<float> pcm(std::size_t(blocks) * BLOCK_SIZE * 2);
    for (int block = 0; block < blocks; ++block)
    {
        if (block == 512 + fixture.controlPhaseOffset ||
            block == 768 + fixture.controlPhaseOffset)
        {
            const int offset = block == 512 + fixture.controlPhaseOffset ? 1 : 2;
            synth->setSamplerate(rates[(fixture.initialRateIndex + offset) % rates.size()]);
        }
        if (block == 640 + fixture.controlPhaseOffset)
        {
            // A host may reset processing without changing the numeric rate.
            // Exercise the reset notification at each EQ control-phase offset.
            synth->setSamplerate(synth->storage.samplerate);
        }
        if (block == 704 + fixture.controlPhaseOffset)
        {
            // The standalone FX wrapper calls storage directly, without the
            // synthesizer's effect reset notifications.
            ScopedRoundingMode outsideCallback(FE_UPWARD);
            synth->storage.setSamplerate(synth->storage.samplerate);
        }
        if (block == 832 + fixture.controlPhaseOffset)
        {
            // Rebuild the lookup tables without changing the rate or its inverse.
            // This is a separate dependency of cached filter coefficients.
            ScopedRoundingMode outsideCallback(FE_DOWNWARD);
            synth->storage.retuneTo12TETScaleC261Mapping();
        }
        if (fixture.effect != fxt_off)
        {
            auto &slot = patch.fx[slotIndex];
            // Every-block parameter changes exercise the effects' existing slow
            // coefficient-update cadence rather than bypassing it from the test.
            if (block >= 96 && block < 256)
                configureCoefficients(slot, fixture.effect,
                                      0.2f + 0.8f * float((block - 96) % 37) / 36.f);
            // Change targets while bands are disabled, then allow several slow
            // updates before re-enabling with those exact same target values.
            if (block >= 272 && block < 288)
                configureCoefficients(slot, fixture.effect,
                                      0.25f + 0.5f * float(block - 272) / 15.f);
            if (block == 256 || block == 320)
            {
                const bool disable = block == 256;
                deactivateBands(slot, fixture.effect, disable);
            }
            if (block == 352 || block == 368)
                patch.fx_disable.val.i = block == 352 ? (1 << slotIndex) : 0;
            if (block == 360)
                configureCoefficients(slot, fixture.effect, 0.4f);
            if (block == 384)
                synth->fx[slotIndex]->suspend();
            if (block == 448)
            {
                ScopedRoundingMode outsideCallback(FE_UPWARD);
                synth->fx[slotIndex]->init();
            }
            if (block >= 896 && block < 928)
            {
                // Hold each sign across more than one slow control update.
                slot.p[0].val.f = block < 912 ? 0.f : -0.f;
            }
        }
        else
        {
            for (int scene = 0; scene < n_scenes; ++scene)
            {
                auto &cutoff = patch.scene[scene].lowcut;
                if (block >= 96 && block < 256)
                    cutoff.val.f = -36.f + scene * 12.f + float((block - 96) % 49);
                if (block >= 272 && block < 288)
                    cutoff.val.f = -24.f + scene * 12.f + float(block - 272);
                if (block == 256 || block == 320)
                    cutoff.deactivated = block == 256;
                if (block >= 896 && block < 928)
                    cutoff.val.f = block < 912 ? 0.f : -0.f;
            }
        }
        if (block == releaseBlock)
            for (int note = 0; note < 8; ++note)
                synth->releaseNote(0, 48 + note, 0);
        // The FX wrapper and ARM64 callbacks may preserve the host's rounding
        // mode. Change only that mode while coefficient inputs remain fixed.
        if (block >= 944 && block < 1008)
        {
            ScopedRoundingMode callbackMode(block < 976 ? FE_UPWARD : FE_DOWNWARD);
            synth->process();
        }
        else
        {
            synth->process();
        }
        for (int frame = 0; frame < BLOCK_SIZE; ++frame)
            for (int channel = 0; channel < 2; ++channel)
            {
                const float sample = synth->output[channel][frame];
                if (!std::isfinite(sample))
                    throw std::runtime_error("Nonfinite output in " + fixture.name);
                pcm[(std::size_t(block) * BLOCK_SIZE + frame) * 2 + channel] = sample;
            }
        synth->time_data.ppqPos += double(BLOCK_SIZE) / synth->storage.samplerate * 2.0;
    }
    errors.check();
    return pcm;
}

struct Evidence
{
    std::string hash;
    double peak = 0;
};

Evidence save(const fsys::path &path, const std::vector<float> &pcm)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t) && std::numeric_limits<float>::is_iec559);
    std::ofstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("Cannot open PCM output");
    std::uint64_t hash = UINT64_C(14695981039346656037);
    Evidence evidence;
    for (const float sample : pcm)
    {
        evidence.peak = std::max(evidence.peak, std::abs(double(sample)));
        const auto bits = std::bit_cast<std::uint32_t>(sample);
        std::array<char, 4> bytes{};
        for (unsigned int byte = 0; byte < 4; ++byte)
        {
            const auto value = (bits >> (8 * byte)) & 0xffu;
            bytes[byte] = static_cast<char>(value);
            hash ^= value;
            hash *= UINT64_C(1099511628211);
        }
        stream.write(bytes.data(), bytes.size());
    }
    stream.close();
    if (!stream)
        throw std::runtime_error("Failed writing PCM");
    if (evidence.peak < 1e-6)
        throw std::runtime_error("Unexpectedly silent fixture");
    std::ostringstream encoded;
    encoded << std::hex << std::setw(16) << std::setfill('0') << hash;
    evidence.hash = encoded.str();
    return evidence;
}
} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "surge-dsp-regression --output-dir DIR\n"
                     "Writes finite engine fixtures and a JSON manifest. Compare complete\n"
                     "PCM bytes between identical harnesses linked to baseline and candidate.\n"
                     "The output directory must be absent or empty.\n";
        return 0;
    }
    try
    {
        if (argc != 3 || std::string(argv[1]) != "--output-dir" || !*argv[2])
            throw std::runtime_error("Expected --output-dir DIR");
        const fsys::path directory(argv[2]);
        // Baseline and candidate evidence must not silently replace one another,
        // and an output path must never overwrite files in a source checkout.
        if (fsys::exists(directory) &&
            (!fsys::is_directory(directory) || !fsys::is_empty(directory)))
            throw std::runtime_error("Output directory must be absent or empty");
        fsys::create_directories(directory);
        std::vector<Fixture> fixtures;
        for (int rate = 0; rate < int(rates.size()); ++rate)
        {
            const std::string suffix = "-" + std::to_string(rates[rate]);
            fixtures.push_back({"conditioner" + suffix, fxt_conditioner, -1, rate});
            for (int phase = 0; phase < 8; ++phase)
            {
                const auto phaseSuffix = suffix + "-phase" + std::to_string(phase);
                fixtures.push_back({"eq3" + phaseSuffix, fxt_eq, -1, rate, phase});
                fixtures.push_back({"eq11" + phaseSuffix, fxt_geq11, -1, rate, phase});
            }
            for (int slope = 0; slope < 4; ++slope)
                fixtures.push_back({"lowcut-" + std::to_string(slope) + suffix, fxt_off,
                                    slope, rate});
        }
        std::ofstream manifest(directory / "manifest.json");
        if (!manifest)
            throw std::runtime_error("Cannot open manifest");
        manifest << std::setprecision(12)
                 << "{\"schema\":1,\"tool\":\"surge-dsp-regression\",\"seed\":" << seed
                 << ",\"block_size\":" << BLOCK_SIZE << ",\"blocks\":" << blocks
                 << ",\"release_block\":" << releaseBlock
                 << ",\"fpu_guard\":true"
                 << ",\"effect_init_rounding\":\"FE_UPWARD, then restore callback mode\""
                 << ",\"table_reset_rounding\":\"direct rate: FE_UPWARD; standard tuning: FE_DOWNWARD; restore callback mode\""
                 << ",\"processing_rounding\":\"blocks [944,976): FE_UPWARD; [976,1008): FE_DOWNWARD; otherwise callback mode\""
                 << ",\"format\":\"f32le-interleaved\",\"channels\":2"
                 << ",\"hash_algorithm\":\"fnv1a64-f32le\",\"fixtures\":[";
        for (std::size_t index = 0; index < fixtures.size(); ++index)
        {
            const auto &fixture = fixtures[index];
            const auto pcm = render(fixture);
            const auto evidence = save(directory / (fixture.name + ".f32"), pcm);
            manifest << (index ? "," : "") << "{\"name\":\"" << fixture.name
                     << "\",\"file\":\"" << fixture.name << ".f32\",\"frames\":"
                     << pcm.size() / 2 << ",\"rate_change_blocks\":["
                     << 512 + fixture.controlPhaseOffset << ','
                     << 768 + fixture.controlPhaseOffset << "],\"sample_rates\":[";
            for (int offset = 0; offset < 3; ++offset)
                manifest << (offset ? "," : "")
                         << rates[(fixture.initialRateIndex + offset) % rates.size()];
            manifest << "],\"peak\":" << evidence.peak << ",\"pcm_hash\":\""
                     << evidence.hash << "\",\"same_rate_reset_blocks\":["
                     << 640 + fixture.controlPhaseOffset
                     << "],\"outside_callback_init_blocks\":[";
            if (fixture.effect != fxt_off)
                manifest << 448;
            manifest << "],\"direct_storage_rate_reset_blocks\":["
                     << 704 + fixture.controlPhaseOffset
                     << "],\"standard_tuning_table_reset_blocks\":["
                     << 832 + fixture.controlPhaseOffset
                     << "],\"processing_rounding_ranges\":[[944,976],[976,1008]]}";
            std::cerr << fixture.name << ": " << evidence.hash << '\n';
        }
        manifest << "],\"ok\":true}\n";
        manifest.close();
        if (!manifest)
            throw std::runtime_error("Failed writing manifest");
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "surge-dsp-regression: " << error.what() << '\n';
        return 1;
    }
}
