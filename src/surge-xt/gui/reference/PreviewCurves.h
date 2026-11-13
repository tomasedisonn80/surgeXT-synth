/*
 * Surge XT reference workspaces.
 * Copyright 2026, Surge XT contributors. GPL-3.0-or-later.
 */
#ifndef SURGE_SRC_SURGE_XT_GUI_REFERENCE_PREVIEWCURVES_H
#define SURGE_SRC_SURGE_XT_GUI_REFERENCE_PREVIEWCURVES_H

#include "juce_core/juce_core.h"

#include <vector>

class SurgeSynthProcessor;

namespace Surge::GUI::Reference
{

inline constexpr float filterPreviewMinimumHz{13.57f};
inline constexpr float filterPreviewMaximumHz{25087.f};
inline constexpr float filterPreviewMinimumDb{-42.f};
inline constexpr float filterPreviewMaximumDb{18.f};

struct PreviewCurve
{
    std::vector<float> values;
    juce::String caption;
    bool available{false};
};

/** All indices are zero-based. Call on the message thread, on changes or at a
    restrained refresh rate, never from an audio callback or paint().

    Filter values are dB, with a uniformly logarithmic frequency axis from
    filterPreviewMinimumHz to filterPreviewMaximumHz. The curve is a base-parameter
    response of an independent instance of the selected filter/subtype at 96 kHz,
    as in the native Filter Analysis, not a live/voice-modulated measurement.
    Display-only states are private; filter 2 offset/link and pre-filter gain are
    respected. Work is capped at one 8192-sample sweep and two fixed-size FFTs on a
    cache miss, then reduced to 256 points. Unchanged settings reuse the cache.
*/
PreviewCurve filterResponse(SurgeSynthProcessor &, int scene, int filterIndex);

/** Native Distortion shaper transfer, at the current base drive, for a zero-based
    FX storage slot. The 257 input samples are uniformly spaced from -1 to +1;
    output values retain their actual amplitude (the renderer may display-clip).
    This is the shaper alone, excluding pre/post EQ, feedback, oversampling and
    output gain. Legacy models use the same lookup as DistortionEffect; newer
    supported models use private, freshly initialized SIMD state per point.
    The Fuzz model is deliberately unavailable: its native shared random lookup
    table must not be initialized by changing a UI view. Other effects also
    return unavailable. A bounded per-slot cache avoids repeat DSP work.
    Call from the message-thread refresh, never from audio or paint().
*/
PreviewCurve distortionTransfer(SurgeSynthProcessor &, int fxSlot);

/** Values are in [-1, 1] (extended amplitude is display-clipped). Periodic shapes
    use Surge's own private, deterministic display LFO, with current deformation,
    phase, amplitude and polarity. Their x axis is normalized shape/sequence time,
    not elapsed wall-clock time; the amplitude envelope is deliberately excluded
    and the caption states this. A deactivated rate retains its frozen phase.

    Envelope shape uses the actual DAHDSR settings and an explicit note-release
    event after decay plus 0.25 s of sustain, up to 4096 control blocks; its caption
    reports the elapsed duration and whether the simulation is truncated.
    Formula and MSEG require their dedicated native editors and return unavailable,
    never an invented sine. Every call returns at most 512 points and processes at
    most 4096 LFO control blocks, without advancing the audible LFO or shared RNG.
*/
PreviewCurve lfoCurve(SurgeSynthProcessor &, int scene, int lfoIndex);

} // namespace Surge::GUI::Reference

#endif
