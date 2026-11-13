/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Released under GPL-3.0-or-later; see LICENSE in the repository root.
 */

#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthProcessor.h"
#include "AliasOscillator.h"
#include "DisplayPreviewScope.h"
#include "OscillatorCommonFunctions.h"
#include "gui/reference/PreviewCurves.h"
#include "gui/reference/Visuals.h"
#include "sst/filters/FilterConfiguration.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <thread>
#include <vector>

namespace
{
using Surge::GUI::Reference::filterResponse;
using Surge::GUI::Reference::lfoCurve;
using Surge::GUI::Reference::PreviewCurve;
using Surge::GUI::Reference::Visuals;

constexpr double previewTestRate = 48000.0;
constexpr int previewTestBlock = 512;

void preparePreviewProcessor(SurgeSynthProcessor &processor)
{
    REQUIRE(processor.surge != nullptr);
    processor.setRateAndBufferSizeDetails(previewTestRate, previewTestBlock);
    processor.prepareToPlay(previewTestRate, previewTestBlock);
}

void checkCurve(const PreviewCurve &curve, size_t maximum, bool bipolar = false)
{
    INFO("Preview caption: " << curve.caption.toStdString());
    REQUIRE(curve.available);
    REQUIRE(!curve.caption.isEmpty());
    REQUIRE(!curve.values.empty());
    REQUIRE(curve.values.size() <= maximum);
    for (const auto value : curve.values)
    {
        REQUIRE(std::isfinite(value));
        if (bipolar)
            REQUIRE(std::abs(value) <= 1.0f);
    }
}

void checkUnavailable(const PreviewCurve &curve)
{
    CHECK_FALSE(curve.available);
    CHECK(curve.values.empty());
    CHECK_FALSE(curve.caption.isEmpty());
}

float maximumDifference(const std::vector<float> &a, const std::vector<float> &b)
{
    REQUIRE(a.size() == b.size());
    float result = 0.f;
    for (size_t i = 0; i < a.size(); ++i)
        result = std::max(result, std::abs(a[i] - b[i]));
    return result;
}

float maximumMagnitude(const std::vector<float> &values)
{
    float result = 0.f;
    for (const auto value : values)
        result = std::max(result, std::abs(value));
    return result;
}

std::vector<float> parameterSnapshot(const SurgeSynthProcessor &processor)
{
    std::vector<float> result;
    for (const auto *parameter : processor.getParameters())
        result.push_back(parameter->getValue());
    return result;
}

void configureFilter(SurgeSynthProcessor &processor, int scene, int index,
                     sst::filters::FilterType type, float cutoff = 0.f, float resonance = .25f)
{
    auto &filter = processor.surge->storage.getPatch().scene[scene].filterunit[index];
    filter.type.val.i = type;
    filter.subtype.val.i = 0;
    filter.cutoff.val.f = cutoff;
    filter.resonance.val.f = resonance;
}

void configureLfo(SurgeSynthProcessor &processor, int scene, int index, lfo_type shape)
{
    auto &lfo = processor.surge->storage.getPatch().scene[scene].lfo[index];
    lfo.shape.val.i = shape;
    lfo.rate.val.f = 2.f;
    lfo.rate.deactivated = false;
    lfo.rate.temposync = false;
    lfo.start_phase.val.f = 0.f;
    lfo.magnitude.val.f = .8f;
    lfo.deform.val.f = 0.f;
    lfo.deform.deform_type = 0;
    lfo.trigmode.val.i = lm_keytrigger;
    lfo.unipolar.val.b = false;
    lfo.delay.val.f = lfo.delay.val_min.f;
    lfo.delay.deactivated = false;
    lfo.attack.val.f = -5.f;
    lfo.hold.val.f = lfo.hold.val_min.f;
    lfo.decay.val.f = -3.f;
    lfo.sustain.val.f = .5f;
    lfo.release.val.f = -4.f;
    for (auto *parameter : {&lfo.delay, &lfo.attack, &lfo.hold, &lfo.decay, &lfo.release})
        parameter->temposync = false;
}
} // namespace

TEST_CASE("Reference filter previews follow native algorithms and reuse bounded caches",
          "[reference-workspaces][reference-previews]")
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SurgeSynthProcessor processor;
    preparePreviewProcessor(processor);
    auto &storage = processor.surge->storage;
    auto &scene = storage.getPatch().scene[0];

    std::vector<float> lowpass, highpass;
    for (const auto type : {sst::filters::fut_none, sst::filters::fut_lp12, sst::filters::fut_hp12,
                            sst::filters::fut_bp12, sst::filters::fut_lpmoog})
    {
        CAPTURE(static_cast<int>(type));
        configureFilter(processor, 0, 0, type);
        const auto before = parameterSnapshot(processor);
        const auto rngBefore = storage.rngGen.g;
        const auto dirtyBefore = storage.getPatch().isDirty.load();
        auto curve = filterResponse(processor, 0, 0);
        checkCurve(curve, 256);
        REQUIRE(curve.values.size() == 256);
        CHECK(parameterSnapshot(processor) == before);
        CHECK(storage.rngGen.g == rngBefore);
        CHECK(storage.getPatch().isDirty.load() == dirtyBefore);
        const auto repeated = filterResponse(processor, 0, 0);
        CHECK(repeated.values == curve.values);
        CHECK(repeated.caption == curve.caption);
        curve.values[0] += 10.f;
        CHECK(filterResponse(processor, 0, 0).values == repeated.values);
        if (type == sst::filters::fut_none)
            CHECK(maximumMagnitude(repeated.values) == 0.f);
        if (type == sst::filters::fut_lp12)
            lowpass = repeated.values;
        if (type == sst::filters::fut_hp12)
            highpass = repeated.values;
    }
    REQUIRE(maximumDifference(lowpass, highpass) > 1.f);

    configureFilter(processor, 0, 0, sst::filters::fut_lp12, -24.f, .2f);
    const auto lower = filterResponse(processor, 0, 0);
    scene.filterunit[0].cutoff.val.f = 24.f;
    const auto higher = filterResponse(processor, 0, 0);
    checkCurve(lower, 256);
    checkCurve(higher, 256);
    CHECK(maximumDifference(lower.values, higher.values) > 1.f);
    scene.filterunit[0].resonance.val.f = .8f;
    const auto resonant = filterResponse(processor, 0, 0);
    checkCurve(resonant, 256);
    CHECK(maximumDifference(higher.values, resonant.values) > .1f);
}

TEST_CASE("Reference filter previews respect scene and filter two links and reject invalid data",
          "[reference-workspaces][reference-previews]")
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SurgeSynthProcessor processor;
    preparePreviewProcessor(processor);
    auto &scene = processor.surge->storage.getPatch().scene[0];
    configureFilter(processor, 0, 0, sst::filters::fut_lp12, -12.f, .6f);
    configureFilter(processor, 0, 1, sst::filters::fut_lp12, 12.f, .1f);
    scene.f2_cutoff_is_offset.val.b = true;
    scene.f2_link_resonance.val.b = true;
    const auto linked = filterResponse(processor, 0, 1);
    checkCurve(linked, 256);
    scene.f2_cutoff_is_offset.val.b = false;
    scene.f2_link_resonance.val.b = false;
    scene.filterunit[1].cutoff.val.f = 0.f;
    scene.filterunit[1].resonance.val.f = .6f;
    const auto explicitValues = filterResponse(processor, 0, 1);
    checkCurve(explicitValues, 256);
    CHECK(linked.values == explicitValues.values);

    configureFilter(processor, 1, 0, sst::filters::fut_hp12, 24.f, .3f);
    const auto sceneA = filterResponse(processor, 0, 0);
    const auto sceneB = filterResponse(processor, 1, 0);
    checkCurve(sceneA, 256);
    checkCurve(sceneB, 256);
    CHECK(maximumDifference(sceneA.values, sceneB.values) > 1.f);
    CHECK(filterResponse(processor, 0, 0).values == sceneA.values);

    for (const auto [sceneIndex, filterIndex] :
         {std::pair{-1, 0}, std::pair{n_scenes, 0}, std::pair{0, -1}, std::pair{0, 2}})
        checkUnavailable(filterResponse(processor, sceneIndex, filterIndex));
    configureFilter(processor, 0, 0, sst::filters::fut_SNH);
    checkUnavailable(filterResponse(processor, 0, 0));
    scene.filterunit[0].type.val.i = sst::filters::num_filter_types;
    checkUnavailable(filterResponse(processor, 0, 0));
    configureFilter(processor, 0, 0, sst::filters::fut_lp12);
    scene.filterunit[0].subtype.val.i = -1;
    checkUnavailable(filterResponse(processor, 0, 0));
    scene.filterunit[0].subtype.val.i = 0;
    scene.filterunit[0].cutoff.val.f = std::numeric_limits<float>::quiet_NaN();
    checkUnavailable(filterResponse(processor, 0, 0));
    scene.filterunit[0].cutoff.val.f = 0.f;
}

TEST_CASE("Reference LFO previews distinguish native shapes without changing patch or Surge RNG",
          "[reference-workspaces][reference-previews]")
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SurgeSynthProcessor processor;
    preparePreviewProcessor(processor);
    auto &storage = processor.surge->storage;
    auto &lfo = storage.getPatch().scene[0].lfo[0];
    auto &sequence = storage.getPatch().stepsequences[0][0];
    sequence.loop_start = 0;
    sequence.loop_end = n_stepseqsteps - 1;
    for (int step = 0; step < n_stepseqsteps; ++step)
        sequence.steps[step] = (step % 4) * .5f - .75f;
    std::array<std::vector<float>, 8> shapes;
    for (const auto shape : {lt_sine, lt_tri, lt_square, lt_ramp, lt_noise, lt_snh, lt_stepseq})
    {
        CAPTURE(static_cast<int>(shape));
        configureLfo(processor, 0, 0, shape);
        const auto before = parameterSnapshot(processor);
        const auto rngBefore = storage.rngGen.g;
        const auto dirtyBefore = storage.getPatch().isDirty.load();
        const std::vector<float> stepsBefore(std::begin(sequence.steps), std::end(sequence.steps));
        const auto curve = lfoCurve(processor, 0, 0);
        checkCurve(curve, 512, true);
        REQUIRE(curve.values.size() == 512);
        CHECK(maximumMagnitude(curve.values) > .05f);
        CHECK(curve.caption.contains("envelope excluded"));
        CHECK(lfoCurve(processor, 0, 0).values == curve.values);
        CHECK(parameterSnapshot(processor) == before);
        CHECK(storage.rngGen.g == rngBefore);
        CHECK(storage.getPatch().isDirty.load() == dirtyBefore);
        CHECK(std::equal(std::begin(sequence.steps), std::end(sequence.steps),
                         std::begin(stepsBefore)));
        shapes[static_cast<size_t>(shape)] = curve.values;
    }
    CHECK(maximumDifference(shapes[lt_sine], shapes[lt_square]) > .2f);
    CHECK(maximumDifference(shapes[lt_sine], shapes[lt_ramp]) > .2f);
    CHECK(maximumDifference(shapes[lt_sine], shapes[lt_stepseq]) > .2f);
    configureLfo(processor, 0, 0, lt_sine);
    lfo.magnitude.val.f = .2f;
    const auto quiet = lfoCurve(processor, 0, 0);
    checkCurve(quiet, 512, true);
    CHECK(maximumMagnitude(quiet.values) < maximumMagnitude(shapes[lt_sine]) * .4f);
    lfo.unipolar.val.b = true;
    const auto unipolar = lfoCurve(processor, 0, 0);
    checkCurve(unipolar, 512, true);
    CHECK(*std::min_element(unipolar.values.begin(), unipolar.values.end()) >= -0.00001f);
    lfo.rate.deactivated = true;
    const auto frozen = lfoCurve(processor, 0, 0);
    checkCurve(frozen, 512, true);
    CHECK(frozen.caption.contains("frozen"));
}

TEST_CASE("Reference LFO envelope previews are bounded and unsupported data stays unavailable",
          "[reference-workspaces][reference-previews]")
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SurgeSynthProcessor processor;
    preparePreviewProcessor(processor);
    auto &storage = processor.surge->storage;
    auto &lfo = storage.getPatch().scene[0].lfo[0];
    configureLfo(processor, 0, 0, lt_envelope);
    const auto envelope = lfoCurve(processor, 0, 0);
    checkCurve(envelope, 512, true);
    CHECK(maximumMagnitude(envelope.values) > .1f);
    CHECK_FALSE(envelope.caption.contains("envelope excluded"));
    lfo.attack.val.f = lfo.attack.val_max.f;
    const auto longEnvelope = lfoCurve(processor, 0, 0);
    checkCurve(longEnvelope, 512, true);
    CHECK(longEnvelope.caption.contains("time-limited"));

    for (const auto shape : {lt_formula, lt_mseg})
    {
        configureLfo(processor, 0, 0, shape);
        const auto curve = lfoCurve(processor, 0, 0);
        checkUnavailable(curve);
        CHECK(curve.caption.contains("native LFO editor"));
    }
    for (const auto [sceneIndex, lfoIndex] :
         {std::pair{-1, 0}, std::pair{n_scenes, 0}, std::pair{0, -1}, std::pair{0, n_lfos}})
        checkUnavailable(lfoCurve(processor, sceneIndex, lfoIndex));
    configureLfo(processor, 0, 0, lt_sine);
    lfo.shape.val.i = n_lfo_types;
    checkUnavailable(lfoCurve(processor, 0, 0));
    lfo.shape.val.i = lt_sine;
    lfo.magnitude.val.f = std::numeric_limits<float>::infinity();
    checkUnavailable(lfoCurve(processor, 0, 0));
    configureLfo(processor, 0, 0, lt_stepseq);
    auto &sequence = storage.getPatch().stepsequences[0][0];
    sequence.loop_start = -1;
    checkUnavailable(lfoCurve(processor, 0, 0));
    sequence.loop_start = 0;
    sequence.loop_end = n_stepseqsteps - 1;
    sequence.steps[3] = std::numeric_limits<float>::quiet_NaN();
    checkUnavailable(lfoCurve(processor, 0, 0));
    sequence.steps[3] = 0.f;
    configureLfo(processor, 0, 0, lt_sine);
}

TEST_CASE("Reference oscillator previews are finite isolated views with honest empty states",
          "[reference-workspaces][reference-previews]")
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SurgeSynthProcessor processor;
    preparePreviewProcessor(processor);
    auto &storage = processor.surge->storage;
    auto &patch = storage.getPatch();
    for (const auto [index, type] :
         {std::pair{0, ot_classic}, std::pair{1, ot_sine}, std::pair{2, ot_audioinput}})
    {
        patch.scene[0].osc[index].type.val.i = type;
        patch.update_controls(false, &patch.scene[0].osc[index]);
    }
    const auto before = parameterSnapshot(processor);
    const auto rngBefore = storage.rngGen.g;
    const auto dirtyBefore = patch.isDirty.load();
    const auto preview = Visuals::sample(processor, 0);
    for (int oscillator = 0; oscillator < 2; ++oscillator)
    {
        REQUIRE(preview.oscillator[oscillator].size() == 512);
        for (const auto sample : preview.oscillator[oscillator])
            REQUIRE(std::isfinite(sample));
        CHECK(maximumMagnitude(preview.oscillator[oscillator]) > .001f);
    }
    CHECK(maximumDifference(preview.oscillator[0], preview.oscillator[1]) > .01f);
    CHECK(preview.oscillator[2].empty()); // Audio Input has no fictional source waveform.
    CHECK(preview.oscillatorStatus[2].contains("Audio Input"));
    CHECK(preview.output.empty()); // Only the independent output tap supplies live audio.
    CHECK(parameterSnapshot(processor) == before);
    CHECK(storage.rngGen.g == rngBefore);
    CHECK(patch.isDirty.load() == dirtyBefore);

    for (const auto scene : {-1, n_scenes})
    {
        const auto absent = Visuals::sample(processor, scene);
        for (const auto &oscillator : absent.oscillator)
            CHECK(oscillator.empty());
        CHECK(absent.output.empty());
        CHECK(absent.left == 0.f);
        CHECK(absent.right == 0.f);
    }
    processor.surge->vu_peak[0] = std::numeric_limits<float>::quiet_NaN();
    processor.surge->vu_peak[1] = -1.f;
    const auto sanitized = Visuals::sample(processor, 0);
    CHECK(sanitized.left == 0.f);
    CHECK(sanitized.right == 0.f);
}

TEST_CASE("Reference display scopes isolate drift RNG and preserve the original audible algorithm",
          "[reference-workspaces][reference-previews]")
{
    using Surge::Oscillator::DisplayPreviewScope;
    CHECK_FALSE(DisplayPreviewScope::active());
    std::array<float, 24> original{}, wrapped{};
    std::srand(391);
    sst::basic_blocks::dsp::DriftLFO originalDrift;
    originalDrift.init(true);
    for (auto &value : original)
        value = originalDrift.next();
    std::srand(391);
    Surge::Oscillator::DriftLFO wrappedDrift;
    wrappedDrift.init(true);
    for (auto &value : wrapped)
        value = wrappedDrift.next();
    CHECK(wrapped == original); // Same RNG calls and samples on the audible/default path.
    CHECK(wrappedDrift.d == originalDrift.d);
    CHECK(wrappedDrift.d2 == originalDrift.d2);

    std::srand(9301);
    const auto expectedNext = std::rand();
    std::srand(9301);
    {
        DisplayPreviewScope outer;
        REQUIRE(DisplayPreviewScope::active());
        {
            DisplayPreviewScope inner;
            REQUIRE(DisplayPreviewScope::active());
            Surge::Oscillator::DriftLFO displayDrift;
            displayDrift.init(true);
            for (int i = 0; i < 24; ++i)
                CHECK(displayDrift.next() == 0.f);
            AliasOscillator::UInt8RNG aliasRng;
            CHECK(aliasRng.a == 73);
        }
        REQUIRE(DisplayPreviewScope::active());
        bool anotherThreadHasPreview = true;
        std::thread independent([&] { anotherThreadHasPreview = DisplayPreviewScope::active(); });
        independent.join();
        CHECK_FALSE(anotherThreadHasPreview);
    }
    CHECK_FALSE(DisplayPreviewScope::active());
    CHECK(std::rand() == expectedNext);
}

TEST_CASE("Reference oscillator sampling does not consume shared random sequences",
          "[reference-workspaces][reference-previews]")
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SurgeSynthProcessor processor;
    preparePreviewProcessor(processor);
    auto &storage = processor.surge->storage;
    auto &patch = storage.getPatch();
    for (const auto [index, type] :
         {std::pair{0, ot_classic}, std::pair{1, ot_sine}, std::pair{2, ot_alias}})
    {
        patch.scene[0].osc[index].type.val.i = type;
        patch.update_controls(false, &patch.scene[0].osc[index]);
    }
    auto &alias = patch.scene[0].osc[2];
    alias.p[AliasOscillator::ao_wave].val.i = AliasOscillator::aow_noise;
    const auto surgeRngBefore = storage.rngGen.g;
    auto noiseRngBefore = storage.noiseRng;
    const auto before = parameterSnapshot(processor);
    std::srand(1701);
    const auto expectedNext = std::rand();
    std::srand(1701);
    const auto preview = Visuals::sample(processor, 0);
    const auto repeated = Visuals::sample(processor, 0);
    CHECK(std::rand() == expectedNext);
    CHECK(storage.rngGen.g == surgeRngBefore);
    auto noiseRngAfter = storage.noiseRng;
    for (int i = 0; i < 8; ++i)
        CHECK(noiseRngBefore.unifU32() == noiseRngAfter.unifU32());
    CHECK(parameterSnapshot(processor) == before);
    for (int oscillator = 0; oscillator < n_oscs; ++oscillator)
    {
        REQUIRE(preview.oscillator[oscillator].size() == 512);
        CHECK(preview.oscillator[oscillator] == repeated.oscillator[oscillator]);
    }
    for (const auto mode : {AliasOscillator::aow_mem_alias, AliasOscillator::aow_mem_oscdata,
                            AliasOscillator::aow_mem_scenedata, AliasOscillator::aow_mem_dawextra,
                            AliasOscillator::aow_mem_stepseqdata, AliasOscillator::aow_audiobuffer})
    {
        CAPTURE(static_cast<int>(mode));
        alias.p[AliasOscillator::ao_wave].val.i = mode;
        const auto unavailable = Visuals::sample(processor, 0);
        CHECK(unavailable.oscillator[2].empty());
        CHECK(unavailable.oscillatorStatus[2].contains("unavailable"));
    }
    alias.type.val.i = ot_twist;
    const auto twist = Visuals::sample(processor, 0);
    CHECK(twist.oscillator[2].empty());
    CHECK(twist.oscillatorStatus[2].contains("Twist preview unavailable"));
    alias.type.val.i = ot_alias;
    alias.p[AliasOscillator::ao_wave].val.i = AliasOscillator::aow_sine;
}
