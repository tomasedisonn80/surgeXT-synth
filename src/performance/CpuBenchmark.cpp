/*
 * Surge XT deterministic offline CPU benchmark.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * This tool is deliberately separate from the plugin and the scoring QA renderer.
 */
#include "SurgeSynthesizer.h"
#include "ConditionerEffect.h"
#include "GraphicEQ11BandEffect.h"
#include "ParametricEQ3BandEffect.h"
#include "version.h"
#include "sst/plugininfra/cpufeatures.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
namespace fsys = std::filesystem;
constexpr unsigned int seed = 1701;
constexpr int warmupBlocks = 128;
constexpr double tempo = 120.0;

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

std::string quote(const std::string &s)
{
    std::ostringstream out;
    out << '"';
    for (const unsigned char c : s)
    {
        switch (c)
        {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 32)
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c)
                    << std::dec;
            else
                out << c;
        }
    }
    return out.str() + '"';
}

double number(const std::string &s)
{
    std::size_t end = 0;
    const double value = std::stod(s, &end);
    if (end != s.size() || !std::isfinite(value))
        throw std::runtime_error("Expected a finite number: " + s);
    return value;
}

int boundedInteger(const std::string &s, int lower, int upper)
{
    const double value = number(s);
    if (value != std::floor(value) || value < lower || value > upper)
        throw std::runtime_error("Integer outside " + std::to_string(lower) + ".." +
                                 std::to_string(upper) + ": " + s);
    return static_cast<int>(value);
}

struct Options
{
    std::string data, output, pcmDir, scenario = "all", patch;
    int sampleRate = 48000, repetitions = 5;
    double seconds = 2.0, tailSeconds = 1.0;
};

Options parse(int argc, char **argv)
{
    std::map<std::string, std::string> args;
    const std::vector<std::string> known = {"--data", "--output", "--pcm-dir", "--scenario",
                                          "--patch", "--sample-rate", "--repetitions",
                                          "--seconds", "--tail-seconds"};
    for (int i = 1; i < argc; ++i)
    {
        const std::string key = argv[i];
        if (std::find(known.begin(), known.end(), key) == known.end())
            throw std::runtime_error("Unknown option: " + key);
        if (i + 1 == argc || args.contains(key) || std::string(argv[i + 1]).starts_with("--"))
            throw std::runtime_error("Missing value or duplicate option: " + key);
        args.emplace(key, argv[++i]);
    }
    const auto get = [&](const std::string &key, const std::string &fallback) {
        const auto it = args.find(key);
        return it == args.end() ? fallback : it->second;
    };
    Options result;
    result.data = get("--data", "");
    result.output = get("--output", "");
    result.pcmDir = get("--pcm-dir", "");
    result.scenario = get("--scenario", "all");
    result.patch = get("--patch", "");
    result.sampleRate = boundedInteger(get("--sample-rate", "48000"), 12000, 192000);
    result.repetitions = boundedInteger(get("--repetitions", "5"), 1, 30);
    result.seconds = number(get("--seconds", "2"));
    result.tailSeconds = number(get("--tail-seconds", "1"));
    if (result.seconds <= 0 || result.seconds > 30 || result.tailSeconds <= 0 ||
        result.tailSeconds > 30)
        throw std::runtime_error("Held and tail durations must be greater than zero and <= 30 s");
    if (result.output.empty())
        throw std::runtime_error("--output JSON is required (engine diagnostics use stdout)");
    if (!result.patch.empty() && result.scenario != "all" && result.scenario != "patch")
        throw std::runtime_error("--patch accepts only --scenario patch (or its default)");
    if (!result.data.empty() && !fsys::is_directory(result.data))
        throw std::runtime_error("Data directory does not exist: " + result.data);
    if (!result.patch.empty() && !fsys::is_regular_file(result.patch))
        throw std::runtime_error("Patch does not exist: " + result.patch);
    return result;
}

struct Scenario
{
    std::string name;
    int voices;
    bool lowcut = false, formula = false, formulaRouted = false;
    int effect = fxt_off;
};

std::vector<Scenario> scenarios(const Options &options)
{
    if (!options.patch.empty())
        return {{"patch", 8}};
    const std::vector<Scenario> all = {{"idle", 0}, {"init-1", 1}, {"init-8", 8},
                                      {"init-16", 16}, {"lowcut-steep", 8, true},
                                      {"unused-formula", 16, false, true},
                                      {"active-formula", 16, false, true, true},
                                      {"conditioner", 8, false, false, false, fxt_conditioner},
                                      {"eq3", 8, false, false, false, fxt_eq},
                                      {"eq11", 8, false, false, false, fxt_geq11}};
    if (options.scenario == "all")
        return all;
    for (const auto &scenario : all)
        if (scenario.name == options.scenario)
            return {scenario};
    throw std::runtime_error("Unknown scenario: " + options.scenario);
}

void reseed(SurgeSynthesizer &synth)
{
    synth.storage.rngGen.g.seed(seed);
    synth.storage.noiseRng = sst::basic_blocks::dsp::RNG(seed + 1);
    // Only this single-threaded offline process changes the process-wide C RNG.
    std::srand(seed + 2);
}

void configure(SurgeSynthesizer &synth, const Options &options, const Scenario &scenario)
{
    reseed(synth);
    if (!options.patch.empty())
    {
        const std::string name = fsys::path(options.patch).stem().string();
        if (!synth.loadPatchByPath(options.patch.c_str(), -1, name.c_str(), false))
            throw std::runtime_error("Cannot load fixture patch: " + options.patch);
    }
    else
    {
        auto &patch = synth.storage.getPatch();
        patch.polylimit.val.i = 32;
        if (scenario.effect != fxt_off)
        {
            auto &slot = patch.fx[fxslot_global1];
            auto &type = slot.type;
            const float normalized = float(scenario.effect - type.val_min.i) /
                                     float(type.val_max.i - type.val_min.i);
            synth.setParameter01(synth.idForParameter(&type), normalized, false);
            synth.process(); // Load the real effect and its native defaults before timing.
            if (!synth.fx[fxslot_global1] || slot.type.val.i != scenario.effect)
                throw std::runtime_error("Effect fixture did not load");
            if (scenario.effect == fxt_conditioner)
            {
                slot.p[ConditionerEffect::cond_bass].val.f = 6.0f;
                slot.p[ConditionerEffect::cond_treble].val.f = -4.0f;
                slot.p[ConditionerEffect::cond_hpwidth].val.f = -12.0f;
                slot.p[ConditionerEffect::cond_hpwidth].deactivated = false;
            }
            else if (scenario.effect == fxt_eq)
            {
                slot.p[ParametricEQ3BandEffect::eq3_gain1].val.f = 6.0f;
                slot.p[ParametricEQ3BandEffect::eq3_freq1].val.f = -12.0f;
                slot.p[ParametricEQ3BandEffect::eq3_bw1].val.f = 1.0f;
                slot.p[ParametricEQ3BandEffect::eq3_gain2].val.f = -4.0f;
                slot.p[ParametricEQ3BandEffect::eq3_gain3].val.f = 3.0f;
            }
            else
            {
                for (int band = 0; band < 11; ++band)
                    slot.p[band].val.f = band % 2 == 0 ? 3.0f : -2.0f;
            }
        }
        if (scenario.lowcut)
        {
            patch.scenemode.val.i = sm_dual;
            for (auto &scene : patch.scene)
            {
                scene.lowcut.deactivated = false;
                scene.lowcut.val.f = -12.0f;
                scene.lowcut.deform_type = 3;
            }
        }
        if (scenario.formula)
        {
#if HAS_LUA
            // Formula LFO2..6 are present in both cases; only the active scenario routes them.
            // The script reads macros and expression so formula input setup remains observable.
            for (int i = 1; i < n_lfos_voice; ++i)
            {
                patch.scene[0].lfo[i].shape.val.i = lt_formula;
                patch.formulamods[0][i].setFormula(R"LUA(
function process(state)
    state.output = (state.phase * 2 - 1) * 0.2 + state.macros[1] * 0.1 + state.cc_expr * 0.01
    return state
end
)LUA");
                if (scenario.formulaRouted &&
                    !synth.setModDepth01(patch.scene[0].osc[0].pitch.id,
                                         static_cast<modsources>(ms_lfo1 + i), 0, 0, 0.03f))
                    throw std::runtime_error("Could not route formula fixture");
            }
            synth.setMacroParameter01(0, 0.25f);
            synth.channelController(0, 11, 64);
#else
            throw std::runtime_error("Formula scenarios require a build with Lua enabled");
#endif
        }
    }
    synth.time_data.tempo = tempo;
    synth.time_data.ppqPos = 0;
    for (int i = 0; i < warmupBlocks; ++i)
        synth.process();
    reseed(synth);
    synth.time_data.ppqPos = 0;
}

std::string checksum(const std::vector<float> &pcm)
{
    // FNV-1a over canonical little-endian IEEE float bytes, not a cryptographic identity.
    static_assert(sizeof(float) == sizeof(std::uint32_t) && std::numeric_limits<float>::is_iec559);
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (const float sample : pcm)
    {
        const auto bits = std::bit_cast<std::uint32_t>(sample);
        for (unsigned int byte = 0; byte < 4; ++byte)
        {
            hash ^= (bits >> (8 * byte)) & 0xffu;
            hash *= UINT64_C(1099511628211);
        }
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

void ensureParent(const fsys::path &path)
{
    if (!path.parent_path().empty())
        fsys::create_directories(path.parent_path());
}

bool sameFile(const fsys::path &left, const fsys::path &right)
{
    if (fsys::weakly_canonical(left) == fsys::weakly_canonical(right))
        return true;
    return fsys::exists(left) && fsys::exists(right) && fsys::equivalent(left, right);
}

void validateOutputPaths(const Options &options, const std::vector<Scenario> &fixtures)
{
    // Validate before opening any output: an accidental alias must never truncate
    // the input patch or turn a PCM file into the JSON report (including hard links).
    if (!options.patch.empty() && sameFile(options.output, options.patch))
        throw std::runtime_error("Report path aliases the input patch");
    if (!options.pcmDir.empty())
    {
        for (const auto &fixture : fixtures)
        {
            const auto pcm = fsys::path(options.pcmDir) / (fixture.name + ".f32");
            if (sameFile(options.output, pcm))
                throw std::runtime_error("Report path aliases a PCM output");
            if (!options.patch.empty() && sameFile(options.patch, pcm))
                throw std::runtime_error("PCM output aliases the input patch");
        }
    }
}

void writePcm(const fsys::path &path, const std::vector<float> &pcm)
{
    ensureParent(path);
    std::ofstream stream(path, std::ios::binary);
    for (const float sample : pcm)
    {
        const auto bits = std::bit_cast<std::uint32_t>(sample);
        const std::array<char, 4> bytes{static_cast<char>(bits), static_cast<char>(bits >> 8),
                                       static_cast<char>(bits >> 16), static_cast<char>(bits >> 24)};
        stream.write(bytes.data(), bytes.size());
    }
    stream.close();
    if (!stream)
        throw std::runtime_error("Cannot write PCM: " + path.string());
}

struct Run
{
    double heldSeconds = 0, tailSeconds = 0, peak = 0, rms = 0;
    std::string hash;
};

Run run(const Options &options, const Scenario &scenario, int heldBlocks, int tailBlocks,
        std::vector<float> &pcm)
{
    Proxy proxy;
    Errors errors;
    std::srand(seed + 2);
    const auto data = options.data.empty()
                          ? (options.patch.empty() ? SurgeStorage::skipPatchLoadDataPathSentinel
                                                   : std::string("resources/data"))
                          : options.data;
    auto synth = std::make_unique<SurgeSynthesizer>(&proxy, data);
    synth->storage.addErrorListener(&errors);
    synth->setSamplerate(options.sampleRate);
    if (synth->storage.samplerate != options.sampleRate)
        throw std::runtime_error("Engine rejected the requested sample rate");
    // Same FTZ/DAZ handling as both VST and CLAP callbacks. Guard destruction restores state.
    auto fpuGuard = sst::plugininfra::cpufeatures::FPUStateGuard();
    configure(*synth, options, scenario);
    errors.check();
    for (int i = 0; i < scenario.voices; ++i)
        synth->playNote(0, 48 + i, 96, 0);
    errors.check();

    const double ppqStep = double(BLOCK_SIZE) / options.sampleRate * (tempo / 60.0);
    const auto renderWindow = [&](int firstBlock, int blocks) {
        const auto start = std::chrono::steady_clock::now();
        for (int block = 0; block < blocks; ++block)
        {
            synth->process();
            auto *out = pcm.data() + std::size_t(firstBlock + block) * BLOCK_SIZE * 2;
            for (int frame = 0; frame < BLOCK_SIZE; ++frame)
            {
                out[frame * 2] = synth->output[0][frame];
                out[frame * 2 + 1] = synth->output[1][frame];
            }
            synth->time_data.ppqPos += ppqStep;
        }
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    };
    Run result;
    result.heldSeconds = renderWindow(0, heldBlocks);
    for (int i = 0; i < scenario.voices; ++i)
        synth->releaseNote(0, 48 + i, 0);
    result.tailSeconds = renderWindow(heldBlocks, tailBlocks);
    errors.check();
    double energy = 0;
    for (const float sample : pcm)
    {
        if (!std::isfinite(sample))
            throw std::runtime_error("Nonfinite audio in scenario " + scenario.name);
        result.peak = std::max(result.peak, std::abs(double(sample)));
        energy += double(sample) * sample;
    }
    result.rms = std::sqrt(energy / pcm.size());
    result.hash = checksum(pcm);
    return result;
}

double median(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    const auto n = values.size();
    return n % 2 ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2;
}

void writeSummary(std::ostream &out, const std::vector<double> &times, double audioSeconds)
{
    out << "{\"wall_seconds\":[";
    for (std::size_t i = 0; i < times.size(); ++i)
        out << (i ? "," : "") << times[i];
    out << "],\"median_seconds\":" << median(times)
        << ",\"min_seconds\":" << *std::min_element(times.begin(), times.end())
        << ",\"max_seconds\":" << *std::max_element(times.begin(), times.end())
        << ",\"audio_seconds\":" << audioSeconds
        << ",\"median_realtime_percent\":" << 100.0 * median(times) / audioSeconds << '}';
}
} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "surge-cpu-bench --output JSON [--data DIR] [--pcm-dir DIR]\n"
                     "  [--scenario all|idle|init-1|init-8|init-16|lowcut-steep|unused-formula|active-formula|conditioner|eq3|eq11]\n"
                     "  [--patch FILE] [--sample-rate 48000] [--repetitions 5]\n"
                     "  [--seconds 2] [--tail-seconds 1]\n"
                     "Durations round up to complete engine blocks. PCM is interleaved stereo f32le.\n"
                     "Wall timing includes process(), bounded PCM copies, and transport advancement;\n"
                     "it excludes engine setup, note events, file I/O, hashing, and statistics.\n";
        return 0;
    }
    try
    {
        const auto options = parse(argc, argv);
        const auto fixtures = scenarios(options);
        validateOutputPaths(options, fixtures);
        const int heldBlocks = static_cast<int>(std::ceil(options.seconds * options.sampleRate / BLOCK_SIZE));
        const int tailBlocks = static_cast<int>(std::ceil(options.tailSeconds * options.sampleRate / BLOCK_SIZE));
        const auto frames = std::size_t(heldBlocks + tailBlocks) * BLOCK_SIZE;
        const double heldAudioSeconds = double(heldBlocks) * BLOCK_SIZE / options.sampleRate;
        const double tailAudioSeconds = double(tailBlocks) * BLOCK_SIZE / options.sampleRate;
        ensureParent(options.output);
        std::ofstream report(options.output);
        if (!report)
            throw std::runtime_error("Cannot open report: " + options.output);
        report << std::setprecision(12)
               << "{\"schema\":1,\"tool\":\"surge-cpu-bench\",\"git\":" << quote(Surge::Build::GitHash)
               << ",\"engine\":" << quote(Surge::Build::FullVersionStr)
               << ",\"compiler\":" << quote(SURGE_BENCH_COMPILER)
               << ",\"configuration\":" << quote(SURGE_BENCH_CONFIGURATION)
               << ",\"lua_requested\":" << quote(SURGE_BENCH_LUA_IMPL)
               << ",\"has_lua\":" << (HAS_LUA ? "true" : "false")
               << ",\"lto_requested\":" << (SURGE_BENCH_LTO_REQUESTED ? "true" : "false")
               << ",\"sample_rate\":" << options.sampleRate << ",\"block_size\":" << BLOCK_SIZE
               << ",\"seed\":" << seed << ",\"warmup_blocks\":" << warmupBlocks
               << ",\"repetitions\":" << options.repetitions
               << ",\"format\":\"f32le-interleaved\",\"channels\":2,\"frames\":" << frames
               << ",\"pcm_hash_algorithm\":\"fnv1a64-f32le\",\"fpu_guard\":true"
               << ",\"timing\":\"steady_clock wall time; engine + PCM copy + transport; excludes setup/events/I-O/hash\""
               << ",\"patch\":" << quote(options.patch) << ",\"scenarios\":[";
        bool deterministic = true;
        for (std::size_t index = 0; index < fixtures.size(); ++index)
        {
            const auto &fixture = fixtures[index];
            std::vector<float> pcm(frames * 2);
            std::vector<Run> runs;
            std::vector<double> heldTimes, tailTimes;
            std::string firstHash;
            bool same = true;
            for (int repetition = 0; repetition < options.repetitions; ++repetition)
            {
                auto result = run(options, fixture, heldBlocks, tailBlocks, pcm);
                if (repetition == 0)
                {
                    firstHash = result.hash;
                    if (!options.pcmDir.empty())
                        writePcm(fsys::path(options.pcmDir) / (fixture.name + ".f32"), pcm);
                }
                same = same && result.hash == firstHash;
                heldTimes.push_back(result.heldSeconds);
                tailTimes.push_back(result.tailSeconds);
                runs.push_back(std::move(result));
            }
            deterministic = deterministic && same;
            report << (index ? "," : "") << "{\"name\":" << quote(fixture.name)
                   << ",\"requested_notes\":" << fixture.voices
                   << ",\"deterministic\":" << (same ? "true" : "false")
                   << ",\"held\":";
            writeSummary(report, heldTimes, heldAudioSeconds);
            report << ",\"tail\":";
            writeSummary(report, tailTimes, tailAudioSeconds);
            report << ",\"runs\":[";
            for (std::size_t i = 0; i < runs.size(); ++i)
                report << (i ? "," : "") << "{\"pcm_hash\":" << quote(runs[i].hash)
                       << ",\"peak\":" << runs[i].peak << ",\"rms\":" << runs[i].rms << '}';
            report << "]}";
            report.flush();
            std::cerr << fixture.name << ": held " << 100.0 * median(heldTimes) / heldAudioSeconds
                      << "% realtime, repeat PCM " << (same ? "identical" : "DIFFERS") << '\n';
        }
        report << "],\"ok\":" << (deterministic ? "true" : "false") << "}\n";
        report.close();
        if (!report)
            throw std::runtime_error("Failed writing benchmark report");
        return deterministic ? 0 : 2;
    }
    catch (const std::exception &error)
    {
        std::cerr << "surge-cpu-bench: " << error.what() << '\n';
        return 1;
    }
}
