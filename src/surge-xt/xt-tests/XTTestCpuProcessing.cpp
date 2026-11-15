/* Surge XT processor CPU-path regressions. GPL-3.0-or-later; see LICENSE. */
#include "catch2/catch_amalgamated.hpp"
#include "SurgeSynthProcessor.h"
#include "PatchDB.h"
#include "UserDefaults.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

namespace
{
constexpr int timelineFrames = 1024;
constexpr float guardValue = 12345.0f;

struct TestDirectory
{
    juce::File path{juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("surge-cpu-processing-" + juce::Uuid().toString())};
    TestDirectory() { REQUIRE(path.createDirectory().wasOk()); }
    ~TestDirectory() { path.deleteRecursively(); }
};

// Guarded host-owned storage; renderTimeline keeps the preceding callback alive
// while constructing the next buffer so the allocator cannot reuse its pointers.
struct GuardedBuffer
{
    std::array<std::vector<float>, 6> storage;
    std::array<float *, 6> pointers{};
    juce::AudioBuffer<float> audio;
    GuardedBuffer(int channels, int frames)
    {
        for (int channel = 0; channel < channels; ++channel)
        {
            storage[channel].assign(frames + 2, guardValue);
            pointers[channel] = storage[channel].data() + 1;
        }
        audio.setDataToReferTo(pointers.data(), channels, frames);
    }
    void checkGuards() const
    {
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        {
            INFO("channel " << channel);
            REQUIRE(storage[channel].front() == guardValue);
            REQUIRE(storage[channel].back() == guardValue);
        }
    }
};

struct ProcessingFixture
{
    juce::ScopedJuceInitialiser_GUI gui;
    TestDirectory directory;
    SurgeSynthProcessor processor;

    ProcessingFixture(bool sceneBuses, int inputChannels, bool latent, bool inputOscillator)
    {
        REQUIRE(processor.surge != nullptr);
        auto &synth = *processor.surge;
        auto &storage = synth.storage;
        storage.patchDB.reset();
        storage.userDataPath = fs::path(directory.path.getFullPathName().toStdString());
        storage.localAppDataPath = storage.userDataPath / "Application";
        storage.initializeUserDataPaths();
        storage.createUserDirectory();
        storage.userDefaultsProvider = std::make_unique<Surge::Storage::UserDefaultsProvider>(
            storage.userDataPath, "SurgeXT", Surge::Storage::defaultKeyToString,
            [&storage](const std::string &message, const std::string &title) {
                storage.reportError(message, title);
            });
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCIn, 0);
        storage.userDefaultsProvider->addOverride(Surge::Storage::StartOSCOut, 0);
        storage.oscStartIn = false;
        storage.oscStartOut = false;
        storage.patchDB = std::make_unique<Surge::PatchStorage::PatchDB>(&storage);
        storage.patchDBInitialized = false;

        auto layout = processor.getBusesLayout();
        layout.inputBuses.set(0, inputChannels == 0 ? juce::AudioChannelSet::disabled()
                                 : inputChannels == 1 ? juce::AudioChannelSet::mono()
                                                      : juce::AudioChannelSet::stereo());
        for (int bus = 1; bus <= 2; ++bus)
            layout.outputBuses.set(bus, sceneBuses ? juce::AudioChannelSet::stereo()
                                                  : juce::AudioChannelSet::disabled());
        REQUIRE(processor.setBusesLayout(layout));
        processor.setRateAndBufferSizeDetails(48000, 128);
        processor.prepareToPlay(48000, 128);
        processor.inputIsLatent = latent;

        auto &patch = storage.getPatch();
        patch.init_default_values();
        patch.scenemode.val.i = sm_dual;
        patch.fx_bypass.val.i = fxb_no_fx;
        for (auto &scene : patch.scene)
            scene.osc[0].type.val.i = inputOscillator ? ot_audioinput : ot_sine;
        patch.update_controls(true);
        for (auto &scene : patch.scene)
        {
            scene.drift.val.f = 0.0f;
            scene.osc[0].retrigger.val.b = true;
        }

        juce::AudioBuffer<float> warmup(sceneBuses ? 6 : 2, 128);
        warmup.clear();
        juce::MidiBuffer midi;
        processor.processBlock(warmup, midi);
        storage.rngGen.g.seed(1819);
        storage.noiseRng = sst::basic_blocks::dsp::RNG(1820);
        std::srand(1821);
        synth.time_data.ppqPos = 0;
    }

    ~ProcessingFixture() { processor.releaseResources(); }
};

float inputSample(int channel, int frame)
{
    // Distinct, exactly reproducible channels below the engine's input clip threshold.
    return float(((frame * (channel == 0 ? 5 : 11)) % 101) - 50) / 256.0f;
}

struct Capture
{
    std::vector<std::vector<float>> channels;
    std::array<std::array<float, BLOCK_SIZE>, 2> lastEngineInput{};
};

Capture renderTimeline(const std::vector<int> &sizes, bool sceneBuses, int inputChannels = 0,
                       bool latent = false, bool extraOutputs = true, bool zeroCallbacks = false)
{
    ProcessingFixture fixture(sceneBuses, inputChannels, latent, inputChannels != 0);
    fixture.processor.surge->activateExtraOutputs = extraOutputs;
    const int channels = sceneBuses ? 6 : 2;
    Capture result;
    result.channels.resize(channels);
    for (auto &channel : result.channels)
        channel.reserve(timelineFrames);

    struct Event
    {
        int sample;
        juce::MidiMessage midi;
    };
    const std::array<Event, 8> events{{
        {0, juce::MidiMessage::noteOn(1, 60, 0.75f)},
        {1, juce::MidiMessage::controllerEvent(1, 1, 37)},
        {31, juce::MidiMessage::noteOn(1, 67, 0.5f)},
        {32, juce::MidiMessage::pitchWheel(1, 9000)},
        {63, juce::MidiMessage::noteOff(1, 60)},
        {127, juce::MidiMessage::noteOn(1, 64, 0.6f)},
        {511, juce::MidiMessage::noteOff(1, 67)},
        {767, juce::MidiMessage::noteOff(1, 64)},
    }};

    int first = 0;
    std::size_t callback = 0;
    std::unique_ptr<GuardedBuffer> previousBuffer;
    while (first < timelineFrames)
    {
        if (zeroCallbacks)
        {
            GuardedBuffer empty(channels, 0);
            juce::MidiBuffer midi;
            fixture.processor.processBlock(empty.audio, midi);
            empty.checkGuards();
        }
        const int count = std::min(sizes[callback++ % sizes.size()], timelineFrames - first);
        auto currentBuffer = std::make_unique<GuardedBuffer>(channels, count);
        auto &buffer = *currentBuffer;
        for (int channel = 0; channel < inputChannels; ++channel)
            for (int sample = 0; sample < count; ++sample)
                buffer.audio.setSample(channel, sample, inputSample(channel, first + sample));
        juce::MidiBuffer midi;
        for (const auto &event : events)
            if (event.sample >= first && event.sample < first + count)
                midi.addEvent(event.midi, event.sample - first);
        fixture.processor.processBlock(buffer.audio, midi);
        buffer.checkGuards();
        for (int channel = 0; channel < channels; ++channel)
        {
            const float *samples = buffer.audio.getReadPointer(channel);
            for (int sample = 0; sample < count; ++sample)
            {
                REQUIRE(std::isfinite(samples[sample]));
                REQUIRE(samples[sample] != guardValue);
            }
            result.channels[channel].insert(result.channels[channel].end(), samples, samples + count);
        }
        first += count;
        previousBuffer = std::move(currentBuffer);
    }
    for (int channel = 0; channel < 2; ++channel)
        std::copy_n(fixture.processor.surge->input[channel], BLOCK_SIZE,
                    result.lastEngineInput[channel].begin());
    return result;
}

void requireSameAudio(const Capture &expected, const Capture &actual)
{
    REQUIRE(actual.channels.size() == expected.channels.size());
    for (std::size_t channel = 0; channel < expected.channels.size(); ++channel)
    {
        INFO("output channel " << channel);
        REQUIRE(actual.channels[channel] == expected.channels[channel]);
    }
}
} // namespace

TEST_CASE("Processor output and MIDI timing survive host callback segmentation",
          "[cpu-processing][reference-workspaces]")
{
    for (bool scenes : {false, true})
    {
        INFO("scene buses " << scenes);
        const auto reference = renderTimeline({128}, scenes);
        REQUIRE(std::any_of(reference.channels[0].begin(), reference.channels[0].end(),
                            [](float value) { return std::abs(value) > 0.0001f; }));
        for (const auto &sizes : {std::vector<int>{1}, std::vector<int>{32},
                                  std::vector<int>{64},
                                  std::vector<int>{3, 31, 1, 17, 32, 65, 64, 128}})
            requireSameAudio(reference, renderTimeline(sizes, scenes, 0, false, true, true));
    }
}

TEST_CASE("Inactive extra outputs are cleared without changing the main mix",
          "[cpu-processing][reference-workspaces]")
{
    const auto enabled = renderTimeline({1, 31, 32, 64, 128}, true);
    const auto disabled = renderTimeline({1, 31, 32, 64, 128}, true, 0, false, false);
    REQUIRE(disabled.channels[0] == enabled.channels[0]);
    REQUIRE(disabled.channels[1] == enabled.channels[1]);
    for (int channel = 2; channel < 6; ++channel)
        REQUIRE(std::all_of(disabled.channels[channel].begin(), disabled.channels[channel].end(),
                            [](float value) { return value == 0.0f; }));
}

TEST_CASE("Aliased host input is captured before output writes in both latency modes",
          "[cpu-processing][reference-workspaces]")
{
    for (int inputChannels : {1, 2})
    {
        INFO("input channels " << inputChannels);
        for (bool latent : {false, true})
        {
            INFO("latent input " << latent);
            const auto reference = renderTimeline({128}, true, inputChannels, latent);
            REQUIRE(std::any_of(reference.channels[0].begin(), reference.channels[0].end(),
                                [](float value) { return std::abs(value) > 0.0001f; }));
            requireSameAudio(reference, renderTimeline({32, 64, 128}, true, inputChannels, latent));
            if (latent)
            {
                requireSameAudio(reference, renderTimeline({1, 7, 31, 65}, true, inputChannels,
                                                           true, true, true));
                // A host can reveal an irregular callback size only after prepareToPlay.
                // Starting with one sample must select the same compensated input path
                // as explicitly enabling it, without reading beyond the aliased buffer.
                requireSameAudio(reference, renderTimeline({1, 7, 31, 65}, true, inputChannels,
                                                           false, true, true));
            }
            const int inputStart = timelineFrames - (latent ? 2 : 1) * BLOCK_SIZE;
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < BLOCK_SIZE; ++sample)
                    REQUIRE(reference.lastEngineInput[channel][sample] ==
                            inputSample(inputChannels == 1 ? 0 : channel, inputStart + sample));
        }
    }
}

TEST_CASE("Zero-length callbacks consume MIDI without advancing audio transport",
          "[cpu-processing][reference-workspaces]")
{
    ProcessingFixture fixture(true, 0, false, false);
    GuardedBuffer empty(6, 0);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.75f), 0);
    const auto position = fixture.processor.surge->time_data.ppqPos;
    fixture.processor.processBlock(empty.audio, midi);
    empty.checkGuards();
    REQUIRE(fixture.processor.surge->time_data.ppqPos == position);
    for (const auto &voices : fixture.processor.surge->voices)
    {
        REQUIRE(voices.size() == 1);
        REQUIRE(voices.front()->state.gate);
    }
    GuardedBuffer next(6, 1);
    juce::MidiBuffer noMidi;
    fixture.processor.processBlock(next.audio, noMidi);
    next.checkGuards();
    REQUIRE(fixture.processor.surge->time_data.ppqPos > position);
}

TEST_CASE("Host bypass and reset preserve output clearing after cached pointer processing",
          "[cpu-processing][reference-workspaces]")
{
    ProcessingFixture fixture(true, 0, false, false);
    GuardedBuffer initial(6, 128);
    juce::MidiBuffer note;
    note.addEvent(juce::MidiMessage::noteOn(1, 60, 0.75f), 0);
    fixture.processor.processBlock(initial.audio, note);
    initial.checkGuards();
    REQUIRE(initial.audio.getMagnitude(0, 128) > 0.0001f);

    auto *bypass = fixture.processor.getBypassParameter();
    REQUIRE(bypass != nullptr);
    bypass->setValue(1.0f);
    for (int callback = 0; callback < 9; ++callback)
    {
        GuardedBuffer buffer(6, 64);
        juce::MidiBuffer midi;
        fixture.processor.processBlock(buffer.audio, midi);
        buffer.checkGuards();
        if (callback == 8)
            REQUIRE(buffer.audio.getMagnitude(0, 64) == 0.0f);
    }

    bypass->setValue(0.0f);
    GuardedBuffer resumed(6, 128);
    juce::MidiBuffer anotherNote;
    anotherNote.addEvent(juce::MidiMessage::noteOn(1, 67, 0.75f), 0);
    fixture.processor.processBlock(resumed.audio, anotherNote);
    resumed.checkGuards();
    fixture.processor.reset();
    for (const auto &voices : fixture.processor.surge->voices)
        REQUIRE(voices.empty());
    GuardedBuffer reset(6, 128);
    juce::MidiBuffer noMidi;
    fixture.processor.processBlock(reset.audio, noMidi);
    reset.checkGuards();
    REQUIRE(reset.audio.getMagnitude(0, 128) < 1e-6f);
}
