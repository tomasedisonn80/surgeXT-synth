/*
 * Surge XT reference workspaces.
 * Copyright 2026, Surge XT contributors. GPL-3.0-or-later.
 */
#include "Visuals.h"

#include "Oscillator.h"
#include "AliasOscillator.h"
#include "DisplayPreviewScope.h"
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

std::vector<float> sampleOscillator(SurgeStorage &storage, OscillatorStorage &oscillatorStorage,
                                    juce::String &status)
{
    std::vector<float> result;
    status = "Preview unavailable";
    if (!(storage.dsamplerate > 0.0) || !std::isfinite(storage.dsamplerate))
    {
        status = juce::String::fromUTF8("Preview unavailable — sample rate");
        return result;
    }
    const auto type = oscillatorStorage.type.val.i;
    if (type < 0 || type >= n_osc_types)
        return result;
    if (type == ot_twist)
    {
        // Plaits uses a global engine RNG. Do not perturb audible instances or
        // attempt to save/reseed it while the audio callback can be running.
        status = juce::String::fromUTF8("Twist preview unavailable — use native oscillator editor");
        return result;
    }
    if (type == ot_audioinput)
    {
        status = juce::String::fromUTF8("Audio Input — no source preview");
        return result;
    }
    if (type == ot_alias)
    {
        const auto mode = oscillatorStorage.p[AliasOscillator::ao_wave].val.i;
        if (mode >= AliasOscillator::aow_mem_alias && mode <= AliasOscillator::aow_audiobuffer)
        {
            status = "Alias live-memory preview unavailable";
            return result;
        }
    }
    // Includes construction: Alias's small RNG otherwise consumes the shared C
    // generator before init(is_display) gets a chance to seed its private state.
    Surge::Oscillator::DisplayPreviewScope previewScope;

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
        {
            status = juce::String::fromUTF8("Preview updating — wavetable busy");
            return result;
        }

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
        {
            status = juce::String::fromUTF8("Preview updating — wavetable busy");
            return result;
        }
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
            {
                status = juce::String::fromUTF8("Preview updating — wavetable busy");
                return {}; // no partial or misleading waveform during a table replacement
            }

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

    status = "Native oscillator preview";
    return result;
}

WavetablePreview sampleWavetable(SurgeStorage &storage, const OscillatorStorage &oscillator)
{
    WavetablePreview result;
    const auto type = oscillator.type.val.i;
    if (type != ot_wavetable && type != ot_window)
        return result;

    // Allocate before locking. The lock protects only a bounded, read-only copy;
    // no oscillator, graphic object or shared random generator is used here.
    std::vector<float> samples(WavetablePreview::maximumFrames * WavetablePreview::pointsPerFrame);
    std::string tableName;
    tableName.reserve(4096);
    {
        std::unique_lock<std::mutex> lock(storage.waveTableDataMutex, std::try_to_lock);
        if (!lock.owns_lock())
            return result;
        if (oscillator.type.val.i != type)
            return result;
        const auto &table = oscillator.wt;
        if (!table.everBuilt || table.size < 1 || table.size > max_wtable_size ||
            table.n_tables < 1 || table.n_tables > max_subtables || table.TableF32Data == nullptr)
            return result;

        result.sampleMode = (table.flags & wtf_is_sample) != 0;
        // Ignore only the silent padding appended by the loader. Keep actual
        // source silence: it is valid data and must remain visible.
        result.sourceFrames = table.data_n_tables > 0
                                  ? std::min(table.data_n_tables, static_cast<int>(table.n_tables))
                                  : static_cast<int>(table.n_tables);
        result.sourceFrameSize = table.size;
        if (oscillator.wavetable_display_name.size() < 4096)
            tableName.assign(oscillator.wavetable_display_name);
        result.frames = result.sourceFrames == 1 ? 1 : WavetablePreview::maximumFrames;
        result.position = juce::jlimit(0.f, 1.f, finiteSample(oscillator.p[0].val.f));

        for (int frame = 0; frame < result.frames; ++frame)
        {
            const float sourcePosition = static_cast<float>(frame) * (result.sourceFrames - 1) /
                                         static_cast<float>(std::max(1, result.frames - 1));
            const int lowerFrame = static_cast<int>(sourcePosition);
            const int upperFrame = std::min(result.sourceFrames - 1, lowerFrame + 1);
            const auto *lower = table.TableF32WeakPointers[0][lowerFrame];
            const auto *upper = table.TableF32WeakPointers[0][upperFrame];
            if (lower == nullptr || upper == nullptr)
                return {};
            const float blend = sourcePosition - lowerFrame;
            for (int point = 0; point < WavetablePreview::pointsPerFrame; ++point)
            {
                const float phase = static_cast<float>(point) * (table.size - 1) /
                                    (WavetablePreview::pointsPerFrame - 1);
                const auto left = static_cast<int>(phase);
                const auto right = std::min(table.size - 1, left + 1);
                const float fraction = phase - left;
                const float a = finiteSample(lower[left]) * (1.f - fraction) +
                                finiteSample(lower[right]) * fraction;
                const float b = finiteSample(upper[left]) * (1.f - fraction) +
                                finiteSample(upper[right]) * fraction;
                samples[frame * WavetablePreview::pointsPerFrame + point] =
                    finiteSample(a * (1.f - blend) + b * blend);
            }
        }
    }
    samples.resize(result.frames * WavetablePreview::pointsPerFrame);
    result.name = juce::String::fromUTF8(tableName.c_str());
    result.samples = std::move(samples);
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
    // Do not read audio-thread-owned vu_peak floats here. Workspace obtains
    // channel peaks from its independent SPSC output tap on the message thread.

    for (int oscillator = 0; oscillator < n_oscs; ++oscillator)
    {
        data.oscillator[static_cast<std::size_t>(oscillator)] =
            sampleOscillator(synth.storage, synth.storage.getPatch().scene[scene].osc[oscillator],
                             data.oscillatorStatus[static_cast<std::size_t>(oscillator)]);
        data.wavetable[static_cast<std::size_t>(oscillator)] =
            sampleWavetable(synth.storage, synth.storage.getPatch().scene[scene].osc[oscillator]);
    }

    return data;
}

void Visuals::grid(juce::Graphics &graphics, juce::Rectangle<float> bounds, juce::Colour colour)
{
    if (bounds.isEmpty())
        return;

    juce::Graphics::ScopedSaveState saved(graphics);
    graphics.reduceClipRegion(bounds.toNearestInt());
    // Callers already supply a subdued grid colour. Multiplying by .12 here
    // made a .10-alpha grid effectively invisible in the native captures.
    graphics.setColour(colour.withMultipliedAlpha(0.65f));
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
    graphics.setColour(colour);
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

void Visuals::wavetable(juce::Graphics &g, juce::Rectangle<float> bounds,
                        const WavetablePreview &data, juce::Colour primary, juce::Colour secondary,
                        WavetableView view)
{
    if (bounds.isEmpty() || !data.available())
        return;
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(bounds.toNearestInt());
    const bool harmonic = view == WavetableView::harmonicRibbons;
    const bool ribbons = view != WavetableView::perspective;
    const bool miniature = bounds.getWidth() < 300 || bounds.getHeight() < 100;
    const auto sampleAt = [&](float frame, int phase) {
        const float selected = juce::jlimit(0.f, 1.f, frame) * (data.frames - 1);
        const int lo = static_cast<int>(selected), hi = std::min(data.frames - 1, lo + 1);
        const auto p = juce::jlimit(0, WavetablePreview::pointsPerFrame - 1, phase);
        const float mix = selected - lo;
        return juce::jlimit(
            -1.35f, 1.35f,
            finiteSample(data.samples[lo * WavetablePreview::pointsPerFrame + p]) * (1.f - mix) +
                finiteSample(data.samples[hi * WavetablePreview::pointsPerFrame + p]) * mix);
    };
    const auto projectSample = [&](float frame, int phase, float amplitude) -> juce::Point<float> {
        const float u = static_cast<float>(phase) / (WavetablePreview::pointsPerFrame - 1);
        if (ribbons)
        {
            // A fixed linear projection of the source amplitudes. In particular,
            // silence is not normalized into a visible ornamental wave.
            const float depth = frame - .5f;
            const auto inset = harmonic ? 0.f : .012f;
            return {bounds.getX() + bounds.getWidth() * (inset + (1.f - 2.f * inset) * u),
                    bounds.getCentreY() +
                        bounds.getHeight() * (depth * (harmonic ? .025f : .13f) -
                                              amplitude * (harmonic ? .44f : .36f + .04f * frame))};
        }
        const float depth = 1.f - frame;
        return {bounds.getX() +
                    bounds.getWidth() * (.025f + .13f * depth + u * (.95f - .26f * depth)),
                bounds.getY() +
                    bounds.getHeight() *
                        (miniature ? .40f + .37f * frame - amplitude * (.19f + .07f * frame)
                                   : .30f + .38f * frame - amplitude * (.27f + .05f * frame))};
    };
    const auto project = [&](float frame, int phase) {
        return projectSample(frame, phase, sampleAt(frame, phase));
    };

    // Reuse these bounded coordinates for surfaces and both wire directions.
    // The source snapshot is immutable; all allocation stays on the UI thread.
    std::vector<juce::Point<float>> vertices(data.samples.size());
    for (int frame = 0; frame < data.frames; ++frame)
        for (int phase = 0; phase < WavetablePreview::pointsPerFrame; ++phase)
            vertices[frame * WavetablePreview::pointsPerFrame + phase] =
                project(static_cast<float>(frame) / std::max(1, data.frames - 1), phase);
    const auto vertex = [&](int frame, int phase) {
        return vertices[frame * WavetablePreview::pointsPerFrame + phase];
    };
    const auto trace = [&](float frame) {
        juce::Path path;
        for (int phase = 0; phase < WavetablePreview::pointsPerFrame; ++phase)
        {
            const auto p = project(frame, phase);
            if (phase == 0)
                path.startNewSubPath(p);
            else
                path.lineTo(p);
        }
        return path;
    };
    const auto gradient = [&](float alpha) {
        if (harmonic)
        {
            auto paint = juce::ColourGradient(primary.withMultipliedAlpha(alpha),
                                              bounds.getTopLeft(),
                                              primary.withMultipliedAlpha(alpha),
                                              bounds.getTopRight(), false);
            paint.addColour(.24f,
                            primary.interpolatedWith(juce::Colours::white, .32f)
                                .withMultipliedAlpha(alpha));
            paint.addColour(.46f, secondary.withMultipliedAlpha(alpha));
            paint.addColour(.61f, primary.withMultipliedAlpha(alpha));
            paint.addColour(.78f, secondary.withMultipliedAlpha(alpha));
            return paint;
        }
        auto paint = juce::ColourGradient(secondary.withMultipliedAlpha(alpha), bounds.getTopLeft(),
                                          primary.withMultipliedAlpha(alpha),
                                          bounds.getBottomRight(), false);
        if (ribbons && !harmonic)
        {
            paint.addColour(.35, primary.withMultipliedAlpha(alpha));
            paint.addColour(
                .63,
                secondary.interpolatedWith(juce::Colours::white, .16f).withMultipliedAlpha(alpha));
            paint.addColour(.9, secondary.withMultipliedAlpha(alpha));
        }
        return paint;
    };

    if (!miniature && !ribbons && data.frames > 1)
    {
        // This is the lower sample-range plane in the same phase/frame
        // projection. It deliberately has no invented numerical scale.
        g.setGradientFill(gradient(.035f));
        juce::Path floor;
        floor.startNewSubPath(projectSample(0.f, 0, -1.f));
        floor.lineTo(projectSample(0.f, WavetablePreview::pointsPerFrame - 1, -1.f));
        floor.lineTo(projectSample(1.f, WavetablePreview::pointsPerFrame - 1, -1.f));
        floor.lineTo(projectSample(1.f, 0, -1.f));
        floor.closeSubPath();
        g.fillPath(floor);
        g.setGradientFill(gradient(.18f));
        for (int phase = 0; phase < WavetablePreview::pointsPerFrame; phase += 16)
            g.drawLine({projectSample(0.f, phase, -1.f), projectSample(1.f, phase, -1.f)}, .6f);
        for (int row = 0; row <= 10; ++row)
            g.drawLine({projectSample(row / 10.f, 0, -1.f),
                        projectSample(row / 10.f, WavetablePreview::pointsPerFrame - 1, -1.f)},
                       .6f);
    }
    else if (harmonic)
    {
        g.setColour(primary.interpolatedWith(juce::Colours::white, .55f).withAlpha(.58f));
        g.drawHorizontalLine(juce::roundToInt(bounds.getCentreY()), bounds.getX(),
                             bounds.getRight());
    }

    // Each translucent strip joins two adjacent source frames point for point;
    // no synthesized waveform, absolute-amplitude transform or texture is added.
    // Miniature plots retain sparse wires instead of muddy overlapping surfaces.
    if (!miniature && data.frames > 1)
        for (int frame = 0; frame + 1 < data.frames; ++frame)
        {
            juce::Path strip;
            strip.startNewSubPath(vertex(frame, 0));
            for (int phase = 1; phase < WavetablePreview::pointsPerFrame; ++phase)
                strip.lineTo(vertex(frame, phase));
            for (int phase = WavetablePreview::pointsPerFrame - 1; phase >= 0; --phase)
                strip.lineTo(vertex(frame + 1, phase));
            strip.closeSubPath();
            const auto depth = static_cast<float>(frame) / std::max(1, data.frames - 1);
            g.setGradientFill(gradient(harmonic ? .025f : ribbons ? .055f : .13f + .08f * depth));
            g.fillPath(strip);
        }

    // Connect actual adjacent frames into a phase-aligned mesh. These are
    // sample-domain connections, never frequency bins or output measurements.
    if (!ribbons && data.frames > 1)
    {
        g.setGradientFill(gradient(miniature ? .11f : .34f));
        for (int phase = 0; phase < WavetablePreview::pointsPerFrame; phase += miniature ? 24 : 8)
        {
            juce::Path seam;
            for (int frame = 0; frame < data.frames; ++frame)
            {
                const auto p = vertex(frame, phase);
                if (frame == 0)
                    seam.startNewSubPath(p);
                else
                    seam.lineTo(p);
            }
            g.strokePath(seam, juce::PathStrokeType(.65f));
        }
    }
    for (int frame = 0; frame < data.frames; ++frame)
    {
        if (miniature && (frame % 2 != 0) && frame != data.frames - 1)
            continue;
        const float t = static_cast<float>(frame) / std::max(1, data.frames - 1);
        juce::Path path;
        path.startNewSubPath(vertex(frame, 0));
        for (int phase = 1; phase < WavetablePreview::pointsPerFrame; ++phase)
            path.lineTo(vertex(frame, phase));
        if (ribbons && !harmonic && frame % 4 == 0)
        {
            auto fill = path;
            fill.lineTo(bounds.getRight(), bounds.getCentreY());
            fill.lineTo(bounds.getX(), bounds.getCentreY());
            fill.closeSubPath();
            g.setGradientFill(gradient(miniature ? .009f : .014f));
            g.fillPath(fill);
        }
        if (!harmonic)
        {
            g.setGradientFill(gradient(miniature ? .02f : ribbons ? .04f : .035f));
            g.strokePath(path, juce::PathStrokeType(miniature ? 3.f : ribbons ? 6.f : 4.5f));
        }
        g.setGradientFill(gradient(harmonic ? .84f : ribbons ? .37f + .45f * t
                                                           : .45f + .48f * t));
        g.strokePath(path, juce::PathStrokeType(miniature ? .7f : harmonic ? 1.f : .9f));
    }
    // Sample-mode morph is not a playback-head position. Do not imply that the
    // source table's static morph setting tracks sample playback.
    if (!data.sampleMode)
    {
        const auto selected = trace(data.position);
        if (!harmonic)
        {
            g.setGradientFill(gradient(.11f));
            g.strokePath(selected, juce::PathStrokeType(miniature ? 4.f : 9.f));
            g.setGradientFill(gradient(.35f));
            g.strokePath(selected, juce::PathStrokeType(miniature ? 2.4f : 4.f));
        }
        auto highlight = gradient(.95f);
        highlight.addColour(.5, primary.interpolatedWith(juce::Colours::white, .55f));
        g.setGradientFill(highlight);
        g.strokePath(selected, juce::PathStrokeType(miniature ? 1.3f : 1.7f));
        if (!miniature && !ribbons)
        {
            const auto point = project(data.position, WavetablePreview::pointsPerFrame / 2);
            const auto foot = projectSample(data.position, WavetablePreview::pointsPerFrame / 2,
                                             -1.f);
            const auto marker = primary.interpolatedWith(juce::Colours::white, .66f);
            g.setColour(marker.withAlpha(.16f));
            g.drawLine({point, foot}, 7.f);
            g.setColour(marker.withAlpha(.75f));
            g.drawLine({point, foot}, 1.3f);
            g.setColour(primary.withAlpha(.23f));
            g.fillEllipse(point.x - 10, point.y - 10, 20, 20);
            g.setColour(marker);
            g.fillEllipse(point.x - 4, point.y - 4, 8, 8);
            g.setColour(juce::Colours::white.withAlpha(.85f));
            g.drawEllipse(point.x - 5, point.y - 5, 10, 10, 1.2f);
        }
    }
}

} // namespace Surge::GUI::Reference
