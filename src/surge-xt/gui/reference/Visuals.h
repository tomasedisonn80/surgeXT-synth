/*
 * Surge XT reference workspaces.
 * Copyright 2026, Surge XT contributors. GPL-3.0-or-later.
 */
#ifndef SURGE_SRC_SURGE_XT_GUI_REFERENCE_VISUALS_H
#define SURGE_SRC_SURGE_XT_GUI_REFERENCE_VISUALS_H

#include "juce_gui_basics/juce_gui_basics.h"

#include <array>
#include <vector>

class SurgeSynthProcessor;

namespace Surge::GUI::Reference
{

/** A bounded, owned snapshot of the source wavetable, before oscillator warping.
    Rows interpolate actual source frames; columns interpolate their phase samples.
    These are neither frequency bins nor captures of currently sounding voices.
    A busy table loader produces an empty snapshot and never stalls the audio thread.
*/
struct WavetablePreview
{
    static constexpr int pointsPerFrame = 256;
    static constexpr int maximumFrames = 48;
    std::vector<float> samples;
    juce::String name;
    int frames{0}, sourceFrames{0}, sourceFrameSize{0};
    float position{0.f};
    bool sampleMode{false};

    bool available() const
    {
        return frames > 0 && frames <= maximumFrames &&
               samples.size() == static_cast<size_t>(frames * pointsPerFrame);
    }
};

/** Message-thread display data, independent of the audible oscillator instances.

    `oscillator` contains previews produced by the actual oscillator algorithms at
    a fixed display pitch, as in the stock Surge waveform display. These are not
    captures of the currently sounding voices, nor frequency-domain spectra.

    `output` is filled separately by Workspace from its independent processor
    tap. The stock oscilloscope owns a different single-consumer ring, which this
    display never reads. An empty output vector must not be replaced by invented
    activity. `left`/`right` are filled separately with channel peaks from that
    same tap, in linear amplitude; sample() never reads audio-owned VU floats.
    The FFT/spectrogram uses the actual tap, not these oscillator previews.
*/
struct VisualData
{
    std::array<std::vector<float>, 3> oscillator;
    std::array<juce::String, 3> oscillatorStatus;
    std::array<WavetablePreview, 3> wavetable;
    std::vector<float> output;
    float left{0.f};
    float right{0.f};
};

class Visuals
{
  public:
    /** Call on the message thread, at a restrained rate or after parameter changes.
        This method does not process the audible synth, change its parameters, or
        subscribe to or consume the stock oscilloscope's output buffer.
    */
    static VisualData sample(SurgeSynthProcessor &, int scene);

    /** Draw a current-waveform preview. `terrain` is a perspective stack of the
        SAME waveform, not a claim to be a spectrogram or a sweep of WT frames.
        Both modes use only supplied samples and no clock/random animation.
    */
    static void waveform(juce::Graphics &, juce::Rectangle<float>, const std::vector<float> &,
                         juce::Colour primary, juce::Colour secondary, bool terrain = false);

    enum class WavetableView
    {
        perspective,
        ribbons,
        harmonicRibbons
    };
    /** Render actual copied source frames. Perspective is a lit sample mesh;
        ribbons uses a slanted phase/frame plane in large views and a nearly
        frontal miniature. Harmonic ribbons use crisp unlit traces around zero.
        Filled strips connect adjacent real frames. The perspective floor is
        an unlabelled sample-range plane, not frequency or measured output.
        Colour, glow and projection are cosmetic; ordinates come from samples.
        The highlighted row follows the actual unmodulated wavetable position.
    */
    static void wavetable(juce::Graphics &, juce::Rectangle<float>, const WavetablePreview &,
                          juce::Colour primary, juce::Colour secondary,
                          WavetableView = WavetableView::perspective);

    static void grid(juce::Graphics &, juce::Rectangle<float>, juce::Colour);
};

} // namespace Surge::GUI::Reference

#endif
