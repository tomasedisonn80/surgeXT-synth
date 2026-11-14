/*
 * Offline native-engine renderer for the Scoring 1000 factory collection.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * This executable is not linked into the audio plugin.
 */
#include "SurgeSynthesizer.h"
#include "version.h"
#include "tinyxml/tinyxml.h"

#include <algorithm>
#include <array>
#include <bit>
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
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fsys = std::filesystem;
struct Proxy : SurgeSynthesizer::PluginLayer
{
    void surgeParameterUpdated(const SurgeSynthesizer::ID &, float) override {}
    void surgeMacroUpdated(long, float) override {}
};
struct Errors : SurgeStorage::ErrorListener
{
    std::vector<std::string> messages;
    void onSurgeError(const std::string &msg, const std::string &title,
                      const SurgeStorage::ErrorType &) override
    {
        messages.push_back(title + ": " + msg);
    }
};

std::string quote(const std::string &s)
{
    std::ostringstream o;
    o << '"';
    for (const unsigned char c : s)
    {
        switch (c)
        {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if (c < 32)
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c)
                  << std::dec;
            else
                o << c;
        }
    }
    return o.str() + '"';
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> r;
    size_t p = 0;
    for (;;)
    {
        const auto e = s.find(delim, p);
        r.push_back(s.substr(p, e == std::string::npos ? e : e - p));
        if (e == std::string::npos) break;
        p = e + 1;
    }
    return r;
}

double number(const std::string &s)
{
    size_t end = 0;
    const auto d = std::stod(s, &end);
    if (end != s.size() || !std::isfinite(d))
        throw std::runtime_error("Invalid finite number: " + s);
    return d;
}

int integer(const std::string &s)
{
    const auto n = number(s);
    if (n != std::floor(n) || n < std::numeric_limits<int>::min() ||
        n > std::numeric_limits<int>::max())
        throw std::runtime_error("Invalid integer: " + s);
    return static_cast<int>(n);
}

uint32_t read32(const std::vector<unsigned char> &d, size_t p, bool big)
{
    if (p + 4 > d.size()) throw std::runtime_error("Truncated FXP header");
    if (big) return (uint32_t(d[p]) << 24) | (uint32_t(d[p+1]) << 16) |
                    (uint32_t(d[p+2]) << 8) | uint32_t(d[p+3]);
    return uint32_t(d[p]) | (uint32_t(d[p+1]) << 8) |
           (uint32_t(d[p+2]) << 16) | (uint32_t(d[p+3]) << 24);
}

std::string validateFile(const std::string &path)
{
    const auto size = fsys::file_size(path);
    if (size < 92 || size > 64 * 1024 * 1024)
        throw std::runtime_error("Invalid FXP file size");
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open patch: " + path);
    std::vector<unsigned char> d((std::istreambuf_iterator<char>(f)), {});
    if (d.size() < 92 || d.size() > 64 * 1024 * 1024)
        throw std::runtime_error("Invalid FXP file size");
    const auto tag = [&](size_t p) { return std::string(d.begin()+p, d.begin()+p+4); };
    if (tag(0) != "CcnK" || tag(8) != "FPCh" || tag(16) != "cjs3" || tag(60) != "sub3")
        throw std::runtime_error("Invalid Surge FXP magic");
    if (read32(d,56,true) != d.size()-60)
        throw std::runtime_error("FXP chunk size does not equal file length");
    const auto xmlSize = read32(d,64,false);
    uint64_t payloadSize = 32 + uint64_t(xmlSize);
    for (size_t p=68;p<92;p+=4) payloadSize += read32(d,p,false);
    if (xmlSize == 0 || xmlSize > d.size()-92 || payloadSize > d.size()-60)
        throw std::runtime_error("Truncated XML or wavetable payload");
    std::string xml(d.begin()+92,d.begin()+92+xmlSize);
    TiXmlDocument doc;
    doc.Parse(xml.c_str());
    const auto root = doc.FirstChildElement("patch");
    const auto meta = root ? root->FirstChildElement("meta") : nullptr;
    const auto name = meta ? meta->Attribute("name") : nullptr;
    if (doc.Error() || !root || !root->FirstChildElement("parameters") || !name || !*name)
        throw std::runtime_error("Invalid patch XML or missing metadata");
    return name;
}

double parameterValue(const Parameter &p, const pdata &v)
{
    if (p.valtype == vt_float) return v.f;
    if (p.valtype == vt_int) return v.i;
    return v.b ? 1 : 0;
}

void dumpParameters(SurgeSynthesizer &s, const std::string &path)
{
    std::ofstream o(path);
    if (!o) throw std::runtime_error("Cannot write parameter dump: " + path);
    o << std::setprecision(10) << "{\"name\":" << quote(s.storage.getPatch().name)
      << ",\"engine\":" << quote(Surge::Build::FullVersionStr) << ",\"parameters\":[";
    bool first = true;
    for (const auto p : s.storage.getPatch().param_ptr)
    {
        if (!first) o << ',';
        first = false;
        o << "{\"name\":" << quote(p->get_storage_name()) << ",\"id\":" << p->id
          << ",\"type\":" << p->valtype << ",\"control_type\":" << p->ctrltype
          << ",\"value\":" << parameterValue(*p,p->val)
          << ",\"min\":" << parameterValue(*p,p->val_min)
          << ",\"max\":" << parameterValue(*p,p->val_max)
          << ",\"default\":" << parameterValue(*p,p->val_default) << '}';
    }
    o << "]}\n";
    if (!o) throw std::runtime_error("Failed writing parameter dump");
}

struct Scenario
{
    std::string patch, out, notes = "60", macros;
    int sampleRate = 48000, velocity = 96, wheel = 0, pressure = 0, expression = 127;
    double seconds = 6, hold = 3, tempo = 120;
    uint32_t seed = 1000;
};

void checkScenario(const Scenario &c)
{
    if (c.patch.empty() || c.out.empty()) throw std::runtime_error("Patch/output paths are required");
    if (c.sampleRate < 8000 || c.sampleRate > 192000 || c.seconds <= 0 || c.seconds > 120 ||
        c.hold < 0 || c.hold > c.seconds || c.tempo < 10 || c.tempo > 400 ||
        c.velocity < 1 || c.velocity > 127 || c.wheel < 0 || c.wheel > 127 ||
        c.pressure < 0 || c.pressure > 127 || c.expression < 0 || c.expression > 127)
        throw std::runtime_error("Scenario exceeds documented argument bounds");
    if (std::llround(c.seconds * c.sampleRate) < 1)
        throw std::runtime_error("Scenario duration must produce at least one audio frame");
}

std::vector<int> noteList(const std::string &s)
{
    std::vector<int> r;
    for (const auto &v : split(s,','))
    {
        int n = integer(v);
        if (n < 0 || n > 127) throw std::runtime_error("MIDI note outside 0..127");
        if (std::find(r.begin(),r.end(),n) != r.end())
            throw std::runtime_error("Duplicate MIDI note in chord");
        r.push_back(n);
    }
    if (r.empty() || r.size() > 64) throw std::runtime_error("Chord must contain 1..64 notes");
    return r;
}

void ensureParent(const std::string &path)
{
    const auto p = fsys::path(path).parent_path();
    if (!p.empty()) fsys::create_directories(p);
}

std::string render(SurgeSynthesizer &s, Errors &errors, const Scenario &c,
                   const std::string &roundtripDir, const std::string &dumpPath)
{
    checkScenario(c);
    const auto notes = noteList(c.notes);
    const auto expected = validateFile(c.patch);
    std::vector<std::pair<int,float>> macros;
    if (!c.macros.empty() && c.macros != "-")
        for (const auto &m : split(c.macros,','))
        {
            const auto a = split(m,'=');
            if (a.size()!=2) throw std::runtime_error("Macro syntax is index=value");
            const int i = integer(a[0]);
            const auto v = number(a[1]);
            if (i<0 || i>=n_customcontrollers || v<0 || v>1)
                throw std::runtime_error("Macro index/value outside 0..7/0..1");
            macros.emplace_back(i,static_cast<float>(v));
        }
    errors.messages.clear();
    s.allNotesOff();
    if (s.storage.samplerate != c.sampleRate) s.setSamplerate(c.sampleRate);
    s.storage.rngGen.g.seed(c.seed);
    s.storage.noiseRng = sst::basic_blocks::dsp::RNG(c.seed + 1);
    // OscillatorDriftUnisonCharacter uses the process C RNG. This standalone
    // renderer is single-threaded; never reseed the process RNG in the plugin.
    std::srand(c.seed + 2);
    // false restores the serialized global gain and bypass state instead of retaining the host's.
    if (!s.loadPatchByPath(c.patch.c_str(),-1,expected.c_str(),false))
        throw std::runtime_error("Native loadPatchByPath failed");
    if (!errors.messages.empty()) throw std::runtime_error(errors.messages.front());
    if (s.storage.getPatch().name != expected) throw std::runtime_error("Native patch identity mismatch");
    if (!dumpPath.empty()) dumpParameters(s,dumpPath);
    std::string roundtrip;
    if (!roundtripDir.empty())
    {
        const auto p = fsys::path(roundtripDir) / fsys::path(c.patch).filename();
        ensureParent(p.string());
        s.savePatchToPath(fs::path(p.string()),false);
        if (validateFile(p.string()) != expected)
            throw std::runtime_error("Native roundtrip identity mismatch");
        if (!s.loadPatchByPath(p.string().c_str(),-1,expected.c_str(),false))
            throw std::runtime_error("Native roundtrip reload failed");
        roundtrip = p.string();
    }
    s.channelController(0,64,0);
    s.channelController(0,1,c.wheel);
    s.channelController(0,11,c.expression);
    s.channelAftertouch(0,c.pressure);
    s.pitchBend(0,0);
    for (const auto &[i,v] : macros) s.setMacroParameter01(i,v);
    s.time_data.tempo = c.tempo;
    s.time_data.ppqPos = 0;
    // Allow controller smoothing to settle without keys held; fixed preroll is part of the fixture.
    for (int i=0;i<128;++i) s.process();
    s.time_data.ppqPos = 0;
    s.storage.rngGen.g.seed(c.seed);
    s.storage.noiseRng = sst::basic_blocks::dsp::RNG(c.seed + 1);
    std::srand(c.seed + 2);
    for (int n : notes) s.playNote(0,n,c.velocity,0);

    ensureParent(c.out);
    std::ofstream audio(c.out,std::ios::binary);
    if (!audio) throw std::runtime_error("Cannot create render output");
    static_assert(std::endian::native == std::endian::little,
                  "The raw PCM format requires a little-endian target");
    const auto frames = static_cast<int64_t>(std::llround(c.seconds*c.sampleRate));
    const auto releaseFrame = static_cast<int64_t>(std::llround(c.hold*c.sampleRate));
    int64_t rendered=0, nonfinite=0;
    bool released=false;
    double peak=0, energy=0, sum=0;
    std::array<float,BLOCK_SIZE*2> interleaved{};
    while (rendered<frames)
    {
        if (!released && rendered>=releaseFrame)
        {
            for (int n : notes) s.releaseNote(0,n,0);
            released=true;
        }
        s.process();
        const int count=static_cast<int>(std::min<int64_t>(BLOCK_SIZE,frames-rendered));
        for (int i=0;i<count;++i)
            for (int ch=0;ch<2;++ch)
            {
                const float v=s.output[ch][i];
                interleaved[2*i+ch]=v;
                if (!std::isfinite(v)) ++nonfinite;
                else {peak=std::max(peak,std::abs(double(v)));energy+=double(v)*v;sum+=v;}
            }
        audio.write(reinterpret_cast<const char *>(interleaved.data()),count*2*sizeof(float));
        rendered+=count;
        s.time_data.ppqPos += (double(BLOCK_SIZE)/c.sampleRate)*(c.tempo/60.0);
    }
    audio.close();
    s.allNotesOff();
    if (!audio) throw std::runtime_error("Failed writing render output");
    if (!errors.messages.empty()) throw std::runtime_error(errors.messages.front());
    std::ostringstream r;
    r << std::setprecision(12)
      << "{\"ok\":" << (nonfinite==0?"true":"false") << ",\"patch\":" << quote(c.patch)
      << ",\"name\":" << quote(expected) << ",\"out\":" << quote(c.out)
      << ",\"engine\":" << quote(Surge::Build::FullVersionStr)
      << ",\"git\":" << quote(Surge::Build::GitHash)
      << ",\"frames\":" << frames << ",\"sample_rate\":" << c.sampleRate
      << ",\"channels\":2,\"format\":\"f32le-interleaved\",\"block_size\":" << BLOCK_SIZE
      << ",\"seconds\":" << c.seconds << ",\"hold\":" << c.hold
      << ",\"notes\":" << quote(c.notes) << ",\"velocity\":" << c.velocity
      << ",\"tempo\":" << c.tempo << ",\"macros\":" << quote(c.macros)
      << ",\"modwheel\":" << c.wheel << ",\"pressure\":" << c.pressure
      << ",\"expression\":" << c.expression << ",\"seed\":" << c.seed
      << ",\"preroll_blocks\":128,\"restores_global_gain\":true"
      << ",\"nonfinite\":" << nonfinite << ",\"peak\":" << peak
      << ",\"rms\":" << std::sqrt(energy/(frames*2)) << ",\"mean\":" << sum/(frames*2)
      << ",\"roundtrip\":" << quote(roundtrip) << '}';
    return r.str();
}

Scenario fromRow(const std::vector<std::string> &v, const Scenario &defaults)
{
    if (v.size()<8 || v.size()>12) throw std::runtime_error("Batch row requires 8..12 TSV fields");
    auto c=defaults;
    c.patch=v[0];c.out=v[1];c.sampleRate=integer(v[2]);c.seconds=number(v[3]);
    c.hold=number(v[4]);c.notes=v[5];c.velocity=integer(v[6]);c.tempo=number(v[7]);
    if (v.size()>8)c.macros=v[8];
    if (v.size()>9 && !v[9].empty())c.wheel=integer(v[9]);
    if (v.size()>10 && !v[10].empty())c.pressure=integer(v[10]);
    if (v.size()>11 && !v[11].empty())c.expression=integer(v[11]);
    return c;
}
}

int main(int argc,char **argv)
{
    try
    {
        std::map<std::string,std::string> args;
        for (int i=1;i<argc;++i)
        {
            const std::string key=argv[i];
            if (key=="--help")
            {
                std::cout << "surge-scoring-render --patch FILE --out F32 [--sample-rate 48000] "
                    "[--seconds 6 --hold 3 --notes 60,64,67 --velocity 96 --tempo 120] "
                    "[--macro 0=1,2=0.5 --modwheel 0 --pressure 0 --expression 127 --seed 1000]\n"
                    "  --batch FILE.tsv --report FILE.jsonl (see src/scoring-tools/README.md)\n"
                    "  --dump-parameters FILE.json [--patch FILE]; --roundtrip-dir DIR\n"
                    "  --data DIR (default resources/data); --discover FILE.json\n"
                    "  --fresh-engine 1 resets all retained DSP state for each batch row\n";
                return 0;
            }
            if (!key.starts_with("--") || i+1>=argc) throw std::runtime_error("Expected --option value");
            if (args.contains(key)) throw std::runtime_error("Duplicate option: "+key);
            args[key]=argv[++i];
        }
        const std::vector<std::string> accepted={"--patch","--out","--sample-rate","--seconds","--hold",
          "--notes","--velocity","--tempo","--macro","--modwheel","--pressure","--expression",
          "--seed","--batch","--report","--dump-parameters","--roundtrip-dir","--data","--discover","--fresh-engine"};
        for (const auto &[k,v]:args)
            if (std::find(accepted.begin(),accepted.end(),k)==accepted.end())
                throw std::runtime_error("Unknown option: "+k);
        auto value=[&](const std::string &k,const std::string &d){return args.contains(k)?args.at(k):d;};
        Scenario defaults;
        defaults.patch=value("--patch","");defaults.out=value("--out","");
        defaults.sampleRate=integer(value("--sample-rate","48000"));
        if (defaults.sampleRate < 8000 || defaults.sampleRate > 192000)
            throw std::runtime_error("Sample rate must be between 8000 and 192000 Hz");
        defaults.seconds=number(value("--seconds","6"));defaults.hold=number(value("--hold","3"));
        defaults.notes=value("--notes","60");defaults.velocity=integer(value("--velocity","96"));
        defaults.tempo=number(value("--tempo","120"));defaults.macros=value("--macro","");
        defaults.wheel=integer(value("--modwheel","0"));defaults.pressure=integer(value("--pressure","0"));
        defaults.expression=integer(value("--expression","127"));
        const auto seed=integer(value("--seed","1000"));
        if (seed<0) throw std::runtime_error("Seed must be nonnegative");
        defaults.seed=static_cast<uint32_t>(seed);
        Errors errors;
        Proxy proxy;
        auto synth=std::make_unique<SurgeSynthesizer>(&proxy,value("--data","resources/data"));
        synth->setSamplerate(defaults.sampleRate);
        synth->storage.addErrorListener(&errors);
        if (args.contains("--discover"))
        {
            std::ofstream o(args.at("--discover"));
            o << '[';
            bool first=true;
            for (const auto &p:synth->storage.patch_list)
            {
                if (!first)o << ',';
                first=false;
                o << "{\"name\":" << quote(p.name) << ",\"path\":" << quote(p.path.string()) << '}';
            }
            o << "]\n";
            if (!o)throw std::runtime_error("Cannot write factory discovery report");
        }
        std::ofstream report;
        if (args.contains("--report"))
        {
            ensureParent(args.at("--report"));report.open(args.at("--report"));
            if (!report)throw std::runtime_error("Cannot open JSONL report");
        }
        std::ostream &result=report.is_open()?report:std::cout;
        int failures=0;
        const auto run=[&](const Scenario &c)
        {
            try
            {
                if (integer(value("--fresh-engine", "0")) == 1)
                {
                    // A separate instance eliminates retained FX/LFO phases between tests.
                    // The library remains unchanged; this is offline test isolation only.
                    synth.reset();
                    std::srand(c.seed + 2);
                    synth=std::make_unique<SurgeSynthesizer>(&proxy,value("--data","resources/data"));
                    synth->setSamplerate(c.sampleRate);
                    synth->storage.addErrorListener(&errors);
                }
                const auto r=render(*synth,errors,c,value("--roundtrip-dir",""),
                                    value("--dump-parameters",""));
                result << r << '\n';result.flush();
                if (r.starts_with("{\"ok\":false"))++failures;
            }
            catch (const std::exception &e)
            {
                ++failures;
                result << "{\"ok\":false,\"patch\":" << quote(c.patch)
                       << ",\"out\":" << quote(c.out) << ",\"error\":" << quote(e.what()) << "}\n";
                result.flush();
            }
        };
        if (args.contains("--batch"))
        {
            if (args.contains("--dump-parameters"))
                throw std::runtime_error("--dump-parameters is single-patch only");
            std::ifstream batch(args.at("--batch"));
            if (!batch)throw std::runtime_error("Cannot open batch TSV");
            std::string line;
            int lineNumber=0;
            while (std::getline(batch,line))
            {
                ++lineNumber;
                if (!line.empty() && line.back()=='\r')line.pop_back();
                if (line.empty() || line[0]=='#')continue;
                try{run(fromRow(split(line,'\t'),defaults));}
                catch(const std::exception &e)
                {
                    ++failures;result << "{\"ok\":false,\"line\":" << lineNumber
                      << ",\"error\":" << quote(e.what()) << "}\n";
                }
            }
        }
        else if (!defaults.out.empty())run(defaults);
        else if (args.contains("--dump-parameters"))
        {
            if (!defaults.patch.empty())
            {
                const auto name=validateFile(defaults.patch);
                if (!synth->loadPatchByPath(defaults.patch.c_str(),-1,name.c_str(),false))
                    throw std::runtime_error("Native patch load failed");
            }
            dumpParameters(*synth,args.at("--dump-parameters"));
        }
        else if (!args.contains("--discover"))throw std::runtime_error("No render, dump or discovery requested");
        synth->storage.removeErrorListener(&errors);
        if (!result)throw std::runtime_error("Failed writing JSONL report");
        return failures?1:0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "surge-scoring-render: " << e.what() << '\n';
        return 2;
    }
}
