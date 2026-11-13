/*
 * Surge XT reference workspaces.
 * Copyright 2026, Surge XT contributors. GPL-3.0-or-later.
 */
#include "Visuals.h"

#include "Oscillator.h"
#include "SurgeSynthProcessor.h"
#include "SurgeStorage.h"
#include "sst/filters/HalfRateFilter.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>

namespace Surge::GUI::Reference
{
namespace
{
constexpr int previewPoints = 512;
constexpr int averagingWindow = 4;
constexpr float displayPitch = 90.15f - 48.f;

float finiteSample(float sample) { return std::isfinite(sample) ? sample : 0.f; }

// spawn_osc constructs in caller-owned, aligned storage. Do NOT use delete: the
// virtual destructor must run, but the backing buffer is owned by this stack.
struct DisplayOscillatorDeleter
{
    void operator()(::Oscillator *oscillator) const
    {
        if (oscillator)
            oscillator->~Oscillator();
    }
};

std::vector<float> sampleOscillator(SurgeStorage &storage, OscillatorStorage &oscillatorStorage)
{
    std::vector<float> result;
    if (!(storage.dsamplerate > 0.0) || !std::isfinite(storage.dsamplerate))
        return result;

    alignas(16) unsigned char memory[oscillator_buffer_size]{};
    pdata parameters[n_scene_params]{};

    const auto pitchId = oscillatorStorage.pitch.param_id_in_scene;
    if (pitchId < 0 || pitchId >= n_scene_params)
        return result;

    parameters[pitchId].f = 0.f;
    for (const auto &parameter : oscillatorStorage.p)
    {
        if (parameter.param_id_in_scene < 0 || parameter.param_id_in_scene >= n_scene_params)
            return result;
        parameters[parameter.param_id_in_scene] = parameter.val;
    }

    std::unique_ptr<::Oscillator, DisplayOscillatorDeleter> oscillator;
    // Follow OscillatorWaveformDisplay's per-block synchronization. Never hold a
    // wavetable lock while painting or allocating graphics. try_lock also lets a
    // busy loader keep priority over a nonessential display refresh.
    {
        std::unique_lock<std::mutex> lock(storage.waveTableDataMutex, std::try_to_lock);
        if (!lock.owns_lock())
            return result;

        oscillator.reset(spawn_osc(oscillatorStorage.type.val.i, &storage, &oscillatorStorage,
                                   parameters, parameters, memory));
        if (!oscillator || !oscillator->allow_display())
            return result;
    }

    float pitch = displayPitch + 12.f * std::log2(storage.dsamplerate / 44100.0);
    // Match the original waveform display's tuning-aware pitch selection.
    if (!storage.isStandardTuning)
    {
        const auto target = storage.note_to_pitch_ignoring_tuning(pitch);
        for (int note = 0; note < 128; ++note)
        {
            const auto lower = storage.note_to_pitch(note);
            const auto upper = storage.note_to_pitch(note + 1);
            if (lower < target && upper > target && upper > lower)
            {
                pitch = note + static_cast<float>((target - lower) / (upper - lower));
                break;
            }
        }
    }

    {
        std::unique_lock<std::mutex> lock(storage.waveTableDataMutex, std::try_to_lock);
        if (!lock.owns_lock())
            return result;
        // Display mode resets phase and keeps preview-only delay lines private.
        // Disable initial drift to make repeated UI renders reproducible.
        oscillator->init(pitch, true, false);
    }

    alignas(16) float buffer[2][BLOCK_SIZE_OS]{};
    sst::filters::HalfRate::HalfRateFilter halfRate(6, true);
    halfRate.load_coefficients();
    halfRate.reset();
    int blockPosition = BLOCK_SIZE;
    result.reserve(previewPoints);

    for (int point = 0; point < previewPoints; ++point)
    {
        if (blockPosition >= BLOCK_SIZE)
        {
            std::unique_lock<std::mutex> lock(storage.waveTableDataMutex, std::try_to_lock);
            if (!lock.owns_lock())
                return {}; // no partial or misleading waveform during a table replacement

            oscillator->process_block(pitch, 0.f, false, false, 0.f);
            std::memcpy(buffer[0], oscillator->output, sizeof(buffer[0]));
            std::memcpy(buffer[1], oscillator->output, sizeof(buffer[1]));
            halfRate.process_block_D2(buffer[0], buffer[1], BLOCK_SIZE_OS);
            blockPosition = 0;
        }

        float sum = 0.f;
        for (int j = 0; j < averagingWindow; ++j)
            sum += finiteSample(buffer[0][blockPosition++]);

        result.push_back(finiteSample(sum / averagingWindow));
    }

    return result;
}

juce::Path makeTrace(const std::vector<float> &samples, juce::Rectangle<float> bounds,
                     float verticalScale)
{
    juce::Path path;
    if (samples.empty())
        return path;

    const auto count = samples.size();
    const float denominator = static_cast<float>(std::max<std::size_t>(count - 1, 1));
    for (std::size_t i = 0; i < count; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * static_cast<float>(i) / denominator;
        const float sample = juce::jlimit(-1.35f, 1.35f, finiteSample(samples[i]));
        const float y = bounds.getCentreY() - sample * bounds.getHeight() * verticalScale;
        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }
    return path;
}
} // namespace

VisualData Visuals::sample(SurgeSynthProcessor &processor, int scene)
{
    VisualData data;
    if (!processor.surge || scene < 0 || scene >= n_scenes)
        return data;

    auto &synth = *processor.surge;
    data.left = std::max(0.f, finiteSample(synth.vu_peak[0]));
    data.right = std::max(0.f, finiteSample(synth.vu_peak[1]));

    for (int oscillator = 0; oscillator < n_oscs; ++oscillator)
        data.oscillator[static_cast<std::size_t>(oscillator)] =
            sampleOscillator(synth.storage, synth.storage.getPatch().scene[scene].osc[oscillator]);

    return data;
}

void Visuals::grid(juce::Graphics &graphics, juce::Rectangle<float> bounds, juce::Colour colour)
{
    if (bounds.isEmpty())
        return;

    juce::Graphics::ScopedSaveState saved(graphics);
    graphics.reduceClipRegion(bounds.toNearestInt());
    graphics.setColour(colour.withMultipliedAlpha(0.12f));
    constexpr int columns = 16;
    constexpr int rows = 8;
    for (int column = 0; column <= columns; ++column)
    {
        const float x = bounds.getX() + bounds.getWidth() * column / columns;
        graphics.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
    }
    for (int row = 0; row <= rows; ++row)
    {
        const float y = bounds.getY() + bounds.getHeight() * row / rows;
        graphics.drawHorizontalLine(juce::roundToInt(y), bounds.getX(), bounds.getRight());
    }
    graphics.setColour(colour.withMultipliedAlpha(0.25f));
    graphics.drawHorizontalLine(juce::roundToInt(bounds.getCentreY()), bounds.getX(),
                                bounds.getRight());
}

void Visuals::waveform(juce::Graphics &graphics, juce::Rectangle<float> bounds,
                       const std::vector<float> &samples, juce::Colour primary,
                       juce::Colour secondary, bool terrain)
{
    if (bounds.isEmpty())
        return;

    juce::Graphics::ScopedSaveState saved(graphics);
    graphics.reduceClipRegion(bounds.toNearestInt());

    if (samples.empty())
    {
        // Empty means no sample is available, not an invented animated waveform.
        graphics.setColour(primary.withMultipliedAlpha(0.3f));
        graphics.drawLine(bounds.getX(), bounds.getCentreY(), bounds.getRight(),
                          bounds.getCentreY(), 1.f);
        return;
    }

    if (!terrain)
    {
        auto path = makeTrace(samples, bounds, 0.34f);
        auto fill = path;
        fill.lineTo(bounds.getRight(), bounds.getBottom());
        fill.lineTo(bounds.getX(), bounds.getBottom());
        fill.closeSubPath();
        graphics.setGradientFill(juce::ColourGradient(
            primary.withMultipliedAlpha(0.32f), bounds.getCentreX(), bounds.getY(),
            secondary.withMultipliedAlpha(0.015f), bounds.getCentreX(), bounds.getBottom(), false));
        graphics.fillPath(fill);

        graphics.setColour(primary.withMultipliedAlpha(0.08f));
        graphics.strokePath(path, juce::PathStrokeType(7.f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        graphics.setColour(primary.withMultipliedAlpha(0.18f));
        graphics.strokePath(path, juce::PathStrokeType(3.3f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        graphics.setGradientFill(juce::ColourGradient(primary, bounds.getX(), bounds.getCentreY(),
                                                      secondary, bounds.getRight(),
                                                      bounds.getCentreY(), false));
        graphics.strokePath(path, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        return;
    }

    // A perspective projection of the current oscillator waveform. Each layer is
    // deliberately the same data: no fabricated frame sweep or spectrum bins.
    constexpr int layerCount = 24;
    for (int layer = 0; layer < layerCount; ++layer)
    {
        const float t = static_cast<float>(layer) / (layerCount - 1);
        const float scale = 0.69f + 0.31f * t;
        const float height = bounds.getHeight() * (0.52f + 0.18f * t);
        const float centreY = bounds.getY() + bounds.getHeight() * (0.28f + 0.39f * t);
        const float left = bounds.getCentreX() - bounds.getWidth() * scale * 0.5f;
        auto layerBounds = juce::Rectangle<float>(left, centreY - height * 0.5f,
                                                  bounds.getWidth() * scale, height);
        auto path = makeTrace(samples, layerBounds, 0.32f);
        auto colour = primary.interpolatedWith(secondary, t);

        graphics.setColour(colour.withMultipliedAlpha(0.055f + 0.055f * t));
        graphics.strokePath(path, juce::PathStrokeType(4.f));
        graphics.setColour(colour.withMultipliedAlpha(0.30f + 0.62f * t));
        graphics.strokePath(path, juce::PathStrokeType(layer == layerCount - 1 ? 1.8f : 0.9f));
    }
}

} // namespace Surge::GUI::Reference
