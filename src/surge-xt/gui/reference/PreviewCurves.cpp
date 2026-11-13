/*
 * Surge XT reference workspaces.
 * Copyright 2026, Surge XT contributors. GPL-3.0-or-later.
 */
#include "PreviewCurves.h"

#include "DistortionEffect.h"
#include "LFOModulationSource.h"
#include "SurgeStorage.h"
#include "SurgeSynthProcessor.h"
#include "sst/filters/FilterPlotter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Surge::GUI::Reference
{
namespace
{
constexpr int filterPoints{256};
constexpr int transferPoints{257};
constexpr int lfoPoints{512};
constexpr int maximumEnvelopeBlocks{4096};

PreviewCurve unavailable(const juce::String &caption) { return {{}, caption, false}; }

struct FilterKey
{
    const SurgeSynthProcessor *processor{nullptr};
    int type{-1}, subtype{-1};
    float cutoff{0}, resonance{0}, gain{0};

    bool operator==(const FilterKey &other) const
    {
        return processor == other.processor && type == other.type && subtype == other.subtype &&
               cutoff == other.cutoff && resonance == other.resonance && gain == other.gain;
    }
};

struct CachedFilter
{
    FilterKey key;
    PreviewCurve curve;
};

struct CachedTransfer
{
    const SurgeSynthProcessor *processor{nullptr};
    int model{-1};
    float driveDb{0};
    PreviewCurve curve;
};

// Each message thread owns only four bounded caches. No worker can retain a
// pointer into a destroyed processor, and no DSP state is shared with a voice.
struct FilterPreviewCache
{
    sst::filters::FilterPlotter plotter{13};
    std::array<CachedFilter, n_scenes * 2> filters;
};

bool populateLFOParameters(LFOStorage &lfo, std::array<pdata, n_scene_params> &parameters)
{
    // Explicit members avoid depending on Parameter's layout or undefined
    // pointer arithmetic across fields of LFOStorage.
    const std::array<const Parameter *, 13> source{
        &lfo.rate,     &lfo.shape,    &lfo.start_phase, &lfo.magnitude, &lfo.deform,
        &lfo.trigmode, &lfo.unipolar, &lfo.delay,       &lfo.hold,      &lfo.attack,
        &lfo.decay,    &lfo.sustain,  &lfo.release};
    for (const auto *parameter : source)
    {
        if (parameter->param_id_in_scene < 0 || parameter->param_id_in_scene >= n_scene_params)
            return false;
        parameters[parameter->param_id_in_scene] = parameter->val;
    }
    return true;
}

double envelopeStageSeconds(const Parameter &parameter, double tempoRatio)
{
    // Minimum values are zero-time stage sentinels in native attackFrom().
    if (parameter.val.f == parameter.val_min.f)
        return 0.;
    const auto seconds = std::exp2(static_cast<double>(parameter.val.f));
    return parameter.temposync ? seconds / tempoRatio : seconds;
}

} // namespace

PreviewCurve filterResponse(SurgeSynthProcessor &processor, int scene, int filterIndex)
{
    if (!processor.surge || scene < 0 || scene >= n_scenes || filterIndex < 0 || filterIndex >= 2)
        return unavailable("Filter preview unavailable");

    const auto &sceneStorage = processor.surge->storage.getPatch().scene[scene];
    const auto &filter = sceneStorage.filterunit[filterIndex];
    FilterKey key{&processor,          filter.type.val.i,      filter.subtype.val.i,
                  filter.cutoff.val.f, filter.resonance.val.f, sceneStorage.level_pfg.val.f};

    if (filterIndex == 1)
    {
        if (sceneStorage.f2_cutoff_is_offset.val.b)
            key.cutoff += sceneStorage.filterunit[0].cutoff.val.f;
        if (sceneStorage.f2_link_resonance.val.b)
            key.resonance = sceneStorage.filterunit[0].resonance.val.f;
    }

    if (key.type < 0 || key.type >= sst::filters::num_filter_types || key.subtype < 0 ||
        key.subtype >= std::max(1, sst::filters::fut_subcount[key.type]) ||
        !std::isfinite(key.cutoff) || !std::isfinite(key.resonance) || !std::isfinite(key.gain))
        return unavailable("Filter settings unavailable");

    const auto name = juce::String(filter.type.get_display());
    if (key.type == sst::filters::fut_SNH)
        return unavailable(name + " - time-varying; use native Filter Analysis");

    const auto type = static_cast<sst::filters::FilterType>(key.type);
    const auto subtype = static_cast<sst::filters::FilterSubType>(key.subtype);
    if (type != sst::filters::fut_none && !sst::filters::GetQFPtrFilterUnit(type, subtype))
        return unavailable(name + " - response unavailable");

    thread_local FilterPreviewCache cache;
    auto &entry = cache.filters[static_cast<std::size_t>(scene * 2 + filterIndex)];
    if (entry.key == key)
        return entry.curve;

    PreviewCurve result;
    result.caption = name;
    if (sst::filters::fut_subcount[key.type] > 1)
        result.caption += " / " + juce::String(filter.subtype.get_display());
    result.caption += " - base response, 96 kHz";

    if (type == sst::filters::fut_none)
    {
        result.values.assign(filterPoints, 0.f);
        result.caption = "Filter off - unity response";
        result.available = true;
    }
    else
    {
        sst::filters::FilterPlotParameters plotParameters;
        plotParameters.startFreqHz = filterPreviewMinimumHz;
        plotParameters.endFreqHz = filterPreviewMaximumHz;
        // This is the same drive conversion as the stock Filter Analysis.
        plotParameters.inputAmplitude *= std::exp2(key.gain / 18.f);
        if (!std::isfinite(plotParameters.inputAmplitude) || plotParameters.inputAmplitude <= 0.f)
            return unavailable(name + " - invalid input gain");

        const auto response = cache.plotter.plotFilterMagnitudeResponse(
            type, subtype, key.cutoff, key.resonance, plotParameters);
        const auto &frequencies = response.first;
        const auto &magnitudes = response.second;
        if (frequencies.size() < 2 || magnitudes.size() != frequencies.size())
            return unavailable(name + " - response unavailable");

        result.values.reserve(filterPoints);
        for (int point = 0; point < filterPoints; ++point)
        {
            const auto frequency =
                filterPreviewMinimumHz * std::pow(filterPreviewMaximumHz / filterPreviewMinimumHz,
                                                  static_cast<float>(point) / (filterPoints - 1));
            const auto next =
                std::lower_bound(frequencies.begin() + 1, frequencies.end(), frequency);
            const auto high = std::min<std::size_t>(
                static_cast<std::size_t>(next - frequencies.begin()), frequencies.size() - 1);
            const auto low = high - 1;
            const auto position = std::clamp(
                (frequency - frequencies[low]) / (frequencies[high] - frequencies[low]), 0.f, 1.f);
            // Nonfinite bins mean an unreliable measurement, not a flat curve.
            if (!std::isfinite(magnitudes[low]) || !std::isfinite(magnitudes[high]))
                return unavailable(name + " - response not finite");
            result.values.push_back(magnitudes[low] +
                                    position * (magnitudes[high] - magnitudes[low]));
        }
        result.available = true;
    }

    entry = {key, result};
    return result;
}

PreviewCurve distortionTransfer(SurgeSynthProcessor &processor, int fxSlot)
{
    if (!processor.surge || fxSlot < 0 || fxSlot >= n_fx_slots)
        return unavailable("Shaper transfer unavailable");

    auto &storage = processor.surge->storage;
    const auto &effect = storage.getPatch().fx[fxSlot];
    if (effect.type.val.i != fxt_distortion)
        return unavailable("Effect family illustration");

    const auto model = effect.p[DistortionEffect::dist_model].val.i;
    const auto &driveParameter = effect.p[DistortionEffect::dist_drive];
    const auto driveDb = driveParameter.get_extended(driveParameter.val.f);
    if (model < 0 || model >= n_fxws || !std::isfinite(driveDb) ||
        !std::isfinite(driveParameter.val.f) || driveParameter.val.f < driveParameter.val_min.f ||
        driveParameter.val.f > driveParameter.val_max.f)
        return unavailable("Shaper settings unavailable");

    using Shape = sst::waveshapers::WaveshaperType;
    const auto shape = FXWaveShapers[model];
    const auto name = juce::String(sst::waveshapers::wst_names[static_cast<int>(shape)]);
    // The native Fuzz table generator owns shared static random state. Calling
    // it for the first time from a preview can change the order in which later
    // audible lookup tables are generated. A family icon is honest and inert.
    if (shape == Shape::wst_fuzzsoft)
        return unavailable(name + " - effect family illustration");

    thread_local std::array<CachedTransfer, n_fx_slots> cache;
    auto &entry = cache[static_cast<std::size_t>(fxSlot)];
    if (entry.processor == &processor && entry.model == model && entry.driveDb == driveDb)
        return entry.curve;

    const auto drive = storage.db_to_linear(driveDb);
    if (!std::isfinite(drive) || drive <= 0.f)
        return unavailable(name + " - drive unavailable");

    const bool vectorShaper = shape >= Shape::wst_sine;
    const auto operation = sst::waveshapers::GetQuadWaveshaper(shape);
    if (vectorShaper && !operation)
        return unavailable(name + " - transfer unavailable");

    auto evaluate = [&](float input, bool initialize) {
        // All lanes and registers are private and initialized. Setting init for
        // each independent point evaluates the underlying ADAA transfer without
        // inventing an input history or borrowing an audible processor state.
        sst::waveshapers::QuadWaveshaperState state{};
        for (auto &value : state.R)
            value = SIMD_MM(setzero_ps)();
        state.init = initialize
                         ? SIMD_MM(cmpeq_ps)(SIMD_MM(setzero_ps)(), SIMD_MM(setzero_ps)())
                         : SIMD_MM(setzero_ps)();
        alignas(16) float result[4]{};
        SIMD_MM(store_ps)(result, operation(&state, SIMD_MM(set1_ps)(input),
                                           SIMD_MM(set1_ps)(drive)));
        return result[0];
    };
    // Match DistortionEffect's zero-input correction and DIGITAL drive path.
    const auto dcOffset = vectorShaper ? evaluate(0.f, false) : 0.f;
    if (!std::isfinite(dcOffset))
        return unavailable(name + " - transfer not finite");

    PreviewCurve result;
    result.caption = name + " transfer - shaper only";
    result.values.reserve(transferPoints);
    for (int point = 0; point < transferPoints; ++point)
    {
        const auto input = -1.f + 2.f * point / (transferPoints - 1);
        const auto output = vectorShaper
                                ? evaluate(shape == Shape::wst_digital ? input * drive : input,
                                           true) - dcOffset
                                : storage.lookup_waveshape(shape, input * drive);
        if (!std::isfinite(output))
            return unavailable(name + " - transfer not finite");
        result.values.push_back(output);
    }
    result.available = true;
    entry = {&processor, model, driveDb, result};
    return result;
}

PreviewCurve lfoCurve(SurgeSynthProcessor &processor, int scene, int lfoIndex)
{
    if (!processor.surge || scene < 0 || scene >= n_scenes || lfoIndex < 0 || lfoIndex >= n_lfos)
        return unavailable("LFO preview unavailable");

    auto &storage = processor.surge->storage;
    auto &patch = storage.getPatch();
    auto lfo = patch.scene[scene].lfo[lfoIndex];
    const auto shape = lfo.shape.val.i;
    if (shape < 0 || shape >= n_lfo_types)
        return unavailable("LFO shape unavailable");

    const juce::String name{lt_names[shape]};
    // Formula evaluation can execute user code. MSEG requires its own potentially
    // large segment/loop renderer. Neither belongs in a throttled mini-preview.
    if (shape == lt_formula || shape == lt_mseg)
        return unavailable(name + " - use native LFO editor");

    const auto sampleRate = storage.dsamplerate;
    if (!(sampleRate > 0.) || !std::isfinite(sampleRate))
        return unavailable(name + " - sample rate unavailable");

    for (const auto value : {lfo.rate.val.f, lfo.start_phase.val.f, lfo.magnitude.val.f,
                             lfo.deform.val.f, lfo.delay.val.f, lfo.hold.val.f, lfo.attack.val.f,
                             lfo.decay.val.f, lfo.sustain.val.f, lfo.release.val.f})
        if (!std::isfinite(value))
            return unavailable(name + " - settings unavailable");
    // In particular, native step-sequence start phase is not clamped by the
    // evaluator. Reject an invalid snapshot before its phase-wrap loops run.
    if (lfo.start_phase.val.f < lfo.start_phase.val_min.f ||
        lfo.start_phase.val.f > lfo.start_phase.val_max.f ||
        lfo.trigmode.val.i < lfo.trigmode.val_min.i || lfo.trigmode.val.i > lfo.trigmode.val_max.i)
        return unavailable(name + " - phase/trigger settings unavailable");

    // These copies, the seeded display RNG, and the local parameter array are
    // owned by this invocation. Native audible LFO instances remain untouched.
    auto sequence = patch.stepsequences[scene][lfoIndex];
    if (shape == lt_stepseq)
    {
        if (sequence.loop_start < 0 || sequence.loop_start >= n_stepseqsteps ||
            sequence.loop_end < 0 || sequence.loop_end >= n_stepseqsteps)
            return unavailable(name + " - loop settings unavailable");
        for (const auto value : sequence.steps)
            if (!std::isfinite(value))
                return unavailable(name + " - step data unavailable");
    }

    PreviewCurve result;
    result.caption = name;
    int blockCount = lfoPoints;
    int releaseBlock = std::numeric_limits<int>::max();

    if (shape == lt_envelope)
    {
        const double tempoRatio = storage.temposyncratio;
        if (!(tempoRatio > 0.) || !std::isfinite(tempoRatio))
            return unavailable(name + " - tempo unavailable");
        const auto gateSeconds = envelopeStageSeconds(lfo.delay, tempoRatio) +
                                 envelopeStageSeconds(lfo.attack, tempoRatio) +
                                 envelopeStageSeconds(lfo.hold, tempoRatio) +
                                 envelopeStageSeconds(lfo.decay, tempoRatio) + 0.25;
        const auto releaseSeconds = envelopeStageSeconds(lfo.release, tempoRatio);
        const auto requestedBlocks =
            std::ceil((gateSeconds + releaseSeconds) * sampleRate / BLOCK_SIZE);
        if (!std::isfinite(requestedBlocks))
            return unavailable(name + " - duration unavailable");
        blockCount =
            static_cast<int>(std::clamp(requestedBlocks, 1., double(maximumEnvelopeBlocks)));
        releaseBlock = static_cast<int>(std::min(std::ceil(gateSeconds * sampleRate / BLOCK_SIZE),
                                                 double(maximumEnvelopeBlocks)));
        result.caption += " - " + juce::String(blockCount * BLOCK_SIZE / sampleRate, 2) + " s";
        if (requestedBlocks > maximumEnvelopeBlocks)
            result.caption += " (time-limited preview)";
        if (lfo.delay.deactivated)
            result.caption += " (envelope bypassed)";
        if (lfo.release.val.f == lfo.release.val_max.f)
            result.caption += " (release held)";
    }
    else
    {
        // Only the private time axis is normalized. The native shape algorithm,
        // deform type, phase/shuffle, current amplitude and polarity are retained.
        // With deactivated rate the real frozen phase must be left intact.
        if (!lfo.rate.deactivated)
        {
            const auto periods =
                shape == lt_stepseq ? 16.f : (shape == lt_noise || shape == lt_snh ? 4.f : 1.f);
            lfo.rate.val.f =
                static_cast<float>(std::log2(sampleRate * periods / (lfoPoints * BLOCK_SIZE)));
            lfo.rate.temposync = false;
        }
        lfo.delay.deactivated = true;
        result.caption += lfo.rate.deactivated ? " - frozen shape, envelope excluded"
                                               : " - shape preview, envelope excluded";
        if (shape == lt_noise || shape == lt_snh)
            result.caption += " (seeded example)";
    }

    std::array<pdata, n_scene_params> parameters{};
    if (!populateLFOParameters(lfo, parameters))
        return unavailable(name + " - parameter mapping unavailable");

    LFOModulationSource preview;
    preview.assign(&storage, &lfo, parameters.data(), nullptr, &sequence, nullptr, nullptr, true);
    preview.setIsVoice(lfoIndex < n_lfos_voice);
    preview.attack();

    std::vector<float> blocks;
    blocks.reserve(static_cast<std::size_t>(blockCount));
    bool clipped{false};
    for (int block = 0; block < blockCount; ++block)
    {
        if (block == releaseBlock)
            preview.release();
        preview.process_block();
        const auto value = preview.get_output(0);
        if (!std::isfinite(value))
            return unavailable(name + " - preview not finite");
        clipped = clipped || value < -1.f || value > 1.f;
        blocks.push_back(std::clamp(value, -1.f, 1.f));
    }

    const auto outputPoints = std::min(blockCount, lfoPoints);
    result.values.reserve(static_cast<std::size_t>(outputPoints));
    for (int point = 0; point < outputPoints; ++point)
    {
        const auto index = static_cast<std::size_t>(point) * (blocks.size() - 1) /
                           static_cast<std::size_t>(std::max(1, outputPoints - 1));
        result.values.push_back(blocks[index]);
    }
    if (clipped)
        result.caption += " (display clipped at +/-1)";
    result.available = true;
    return result;
}

} // namespace Surge::GUI::Reference
