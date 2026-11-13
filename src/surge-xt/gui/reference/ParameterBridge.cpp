/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "ParameterBridge.h"
#include "SurgeSynthProcessor.h"
#include "UndoManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Surge::GUI::Reference
{
namespace
{
// Strictly parse a complete numeric suffix; String::getIntValue alone would
// silently accept malformed names such as osc1garbage.
int indexAfter(const juce::String &name, const juce::String &prefix, int limit,
               bool zeroBased = false)
{
    if (!name.startsWith(prefix))
        return -1;
    auto digits = name.substring(prefix.length());
    if (digits.isEmpty() || digits.length() > 3 || !digits.containsOnly("0123456789"))
        return -1;
    const int result = digits.getIntValue() - (zeroBased ? 0 : 1);
    return result >= 0 && result < limit ? result : -1;
}

float safeNormalized(float value)
{
    return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f;
}
} // namespace

ParameterBridge::ParameterBridge(SurgeSynthProcessor &processor) : owner(processor) {}
ParameterBridge::~ParameterBridge() { endAll(); }

void ParameterBridge::setScene(int nextScene)
{
    nextScene = std::clamp(nextScene, 0, n_scenes - 1);
    if (scene == nextScene)
        return;
    // End the OLD scene's targets before any control can resolve to the new one.
    endAll();
    scene = nextScene;
}

int ParameterBridge::macroIndex(const juce::String &binding) const
{
    return indexAfter(binding, "macro", n_customcontrollers);
}

Parameter *ParameterBridge::parameter(const juce::String &binding) const
{
    if (!owner.surge)
        return nullptr;
    auto &patch = owner.surge->storage.getPatch();
    auto &sc = patch.scene[scene];

    if (binding == "master.volume")
        return &patch.volume;
    if (binding == "scene.active")
        return &patch.scene_active;
    if (binding == "scene.mode")
        return &patch.scenemode;
    if (binding == "scene.polylimit")
        return &patch.polylimit;
    if (binding == "scene.splitpoint")
        return &patch.splitpoint;
    if (binding == "scene.volume")
        return &sc.volume;
    if (binding == "scene.pitch")
        return &sc.pitch;
    if (binding == "scene.octave")
        return &sc.octave;
    if (binding == "scene.pan")
        return &sc.pan;
    if (binding == "scene.width")
        return &sc.width;
    if (binding == "scene.portamento")
        return &sc.portamento;
    if (binding == "scene.polymode")
        return &sc.polymode;
    if (binding == "scene.drift")
        return &sc.drift;
    if (binding == "scene.fmdepth")
        return &sc.fm_depth;
    if (binding == "scene.fmmode")
        return &sc.fm_switch;
    if (binding == "routing")
        return &sc.filterblock_configuration;
    if (binding == "filter.drive")
        return &sc.wsunit.drive;
    if (binding == "filter.waveshaper")
        return &sc.wsunit.type;
    if (binding == "filter.feedback")
        return &sc.feedback;
    if (binding == "filter.balance")
        return &sc.filter_balance;
    if (binding == "filter.lowcut")
        return &sc.lowcut;
    if (binding == "amp.level")
        return &sc.vca_level;
    if (binding == "amp.velocity")
        return &sc.vca_velsense;
    if (binding == "noise.level")
        return &sc.level_noise;
    if (binding == "noise.color")
        return &sc.noise_colour;
    if (binding == "noise.mute")
        return &sc.mute_noise;
    if (binding == "noise.solo")
        return &sc.solo_noise;
    if (binding == "ring12.level")
        return &sc.level_ring_12;
    if (binding == "ring23.level")
        return &sc.level_ring_23;
    if (binding == "ring12.mute")
        return &sc.mute_ring_12;
    if (binding == "ring23.mute")
        return &sc.mute_ring_23;
    // There is intentionally no scene.mix: a crossfade is not a parameter morph.

    const auto group = binding.upToFirstOccurrenceOf(".", false, false);
    const auto field = binding.fromFirstOccurrenceOf(".", false, false);
    if (field.isEmpty())
        return nullptr;

    if (int i = indexAfter(group, "osc", n_oscs); i >= 0)
    {
        auto &osc = sc.osc[i];
        if (field == "type")
            return &osc.type;
        if (field == "pitch")
            return &osc.pitch;
        if (field == "octave")
            return &osc.octave;
        if (field == "keytrack")
            return &osc.keytrack;
        if (field == "retrigger")
            return &osc.retrigger;
        if (field == "level")
            return std::array<Parameter *, n_oscs>{&sc.level_o1, &sc.level_o2, &sc.level_o3}[i];
        if (field == "mute")
            return std::array<Parameter *, n_oscs>{&sc.mute_o1, &sc.mute_o2, &sc.mute_o3}[i];
        if (field == "solo")
            return std::array<Parameter *, n_oscs>{&sc.solo_o1, &sc.solo_o2, &sc.solo_o3}[i];
        if (field == "route")
            return std::array<Parameter *, n_oscs>{&sc.route_o1, &sc.route_o2, &sc.route_o3}[i];
        if (int p = indexAfter(field, "param", n_osc_params, true); p >= 0)
            return &osc.p[p];
    }
    if (int i = indexAfter(group, "filter", 2); i >= 0)
    {
        auto &filter = sc.filterunit[i];
        if (field == "type")
            return &filter.type;
        if (field == "subtype")
            return &filter.subtype;
        if (field == "cutoff")
            return &filter.cutoff;
        if (field == "resonance")
            return &filter.resonance;
        if (field == "keytrack")
            return &filter.keytrack;
        if (field == "envmod")
            return &filter.envmod;
    }
    if (int i = indexAfter(group, "env", 2); i >= 0)
    {
        auto &env = sc.adsr[i];
        if (field == "attack")
            return &env.a;
        if (field == "decay")
            return &env.d;
        if (field == "sustain")
            return &env.s;
        if (field == "release")
            return &env.r;
        if (field == "mode")
            return &env.mode;
        if (field == "attackshape")
            return &env.a_s;
        if (field == "decayshape")
            return &env.d_s;
        if (field == "releaseshape")
            return &env.r_s;
    }
    if (int i = indexAfter(group, "lfo", n_lfos); i >= 0)
    {
        auto &lfo = sc.lfo[i];
        if (field == "rate")
            return &lfo.rate;
        if (field == "shape")
            return &lfo.shape;
        if (field == "phase")
            return &lfo.start_phase;
        if (field == "magnitude")
            return &lfo.magnitude;
        if (field == "deform")
            return &lfo.deform;
        if (field == "trigmode")
            return &lfo.trigmode;
        if (field == "unipolar")
            return &lfo.unipolar;
        if (field == "delay")
            return &lfo.delay;
        if (field == "attack")
            return &lfo.attack;
        if (field == "hold")
            return &lfo.hold;
        if (field == "decay")
            return &lfo.decay;
        if (field == "sustain")
            return &lfo.sustain;
        if (field == "release")
            return &lfo.release;
    }
    if (int i = indexAfter(group, "fx", n_fx_slots); i >= 0)
    {
        if (field == "type")
            return &patch.fx[i].type;
        if (field == "return")
            return &patch.fx[i].return_level;
        if (int p = indexAfter(field, "param", n_fx_params, true); p >= 0)
            return &patch.fx[i].p[p];
    }
    return nullptr;
}

juce::RangedAudioParameter *ParameterBridge::adapter(const juce::String &binding) const
{
    if (const int i = macroIndex(binding); i >= 0)
        return i < static_cast<int>(owner.macrosById.size()) ? owner.macrosById[i] : nullptr;
    if (auto *p = parameter(binding))
    {
        auto found = owner.paramsByID.find(owner.surge->idForParameter(p));
        if (found != owner.paramsByID.end())
            return found->second;
    }
    return nullptr;
}

bool ParameterBridge::valid(const juce::String &binding) const
{
    return adapter(binding) != nullptr;
}

bool ParameterBridge::enabled(const juce::String &binding) const
{
    if (!valid(binding))
        return false;
    if (auto *p = parameter(binding))
        return p->ctrltype != ct_none && !p->appears_deactivated();
    return true;
}

float ParameterBridge::value(const juce::String &binding) const
{
    if (auto *a = adapter(binding))
        return safeNormalized(a->getValue());
    return 0.f;
}

juce::String ParameterBridge::display(const juce::String &binding) const
{
    if (auto *p = parameter(binding))
        return p->ctrltype == ct_none ? juce::String("—") : juce::String(p->get_display());
    if (macroIndex(binding) >= 0 && valid(binding))
        return juce::String(value(binding), 2);
    return "Unavailable";
}

juce::String ParameterBridge::label(const juce::String &binding) const
{
    if (auto *p = parameter(binding))
        return p->ctrltype == ct_none ? juce::String("Unused") : juce::String(p->get_name());
    if (auto *a = adapter(binding))
        return a->getName(128);
    return binding;
}

void ParameterBridge::captureUndo(const Gesture &gesture)
{
    if (gesture.parameter)
    {
        auto *p = gesture.parameter;
        if (onBeginParameter)
            onBeginParameter(p);
        else if (owner.undoManager)
        {
            if (p->ctrltype == ct_osctype)
                owner.undoManager->pushOscillator(p->scene - 1, p->ctrlgroup_entry);
            else if (p->ctrltype == ct_fxtype)
                owner.undoManager->pushFX(p->ctrlgroup_entry);
            else if (p->ctrltype == ct_lfotype)
                owner.undoManager->pushFullLFO(p->scene - 1, p->ctrlgroup_entry);
            else
                owner.undoManager->pushParameterChange(p->id, p, p->val);
        }
    }
    else if (gesture.macro >= 0)
    {
        if (onBeginMacro)
            onBeginMacro(gesture.macro);
        else if (owner.undoManager)
            owner.undoManager->pushMacroChange(gesture.macro, gesture.adapter->getValue());
    }
}

void ParameterBridge::begin(const juce::String &binding)
{
    const auto key = binding.toStdString();
    if (gestures.find(key) != gestures.end() || !enabled(binding))
        return;
    auto *a = adapter(binding);
    if (!a)
        return;
    // Do not start a second host gesture through an alias to an active target.
    for (const auto &entry : gestures)
        if (entry.second.adapter == a)
            return;
    Gesture gesture{a, parameter(binding), macroIndex(binding)};
    if (auto *p = dynamic_cast<SurgeParamToJuceParamAdapter *>(a))
    {
        if (p->inEditGesture.exchange(true))
            return;
    }
    captureUndo(gesture);
    a->beginChangeGesture();
    gestures.emplace(key, gesture);
}

void ParameterBridge::set(const juce::String &binding, float normalized)
{
    // Reject non-finite events rather than unexpectedly moving a control to 0.
    if (!std::isfinite(normalized))
        return;
    // A host automation or algorithm switch can deactivate a target while the
    // mouse is still down. End its gesture rather than writing an unused slot.
    if (!enabled(binding))
    {
        end(binding);
        return;
    }
    const auto key = binding.toStdString();
    const bool oneShot = gestures.find(key) == gestures.end();
    if (oneShot)
        begin(binding);
    const auto found = gestures.find(key);
    if (found == gestures.end())
        return;
    auto &gesture = found->second;
    normalized = safeNormalized(normalized);
    if (gesture.parameter)
    {
        auto &synth = *owner.surge;
        const auto id = synth.idForParameter(gesture.parameter);
        // Surge's JUCE adapter deliberately suppresses its setter during a UI
        // gesture. Mirror the stock editor: change the engine, then notify host.
        if (synth.setParameter01(id, normalized, false))
            synth.refresh_editor = true;
        synth.queueForRefresh(gesture.parameter->id);
        synth.processAudioThreadOpsWhenAudioEngineUnavailable();
        gesture.adapter->setValueNotifyingHost(synth.getParameter01(id));
    }
    else
    {
        gesture.adapter->setValueNotifyingHost(normalized);
    }
    if (oneShot)
        end(binding);
}

void ParameterBridge::finish(const Gesture &gesture)
{
    if (auto *p = dynamic_cast<SurgeParamToJuceParamAdapter *>(gesture.adapter))
        p->inEditGesture = false;
    gesture.adapter->endChangeGesture();
    if (gesture.parameter)
        owner.paramChangeToListeners(gesture.parameter);
    else if (gesture.macro >= 0)
        owner.paramChangeToListeners(nullptr, true, owner.SCT_MACRO,
                                     static_cast<float>(gesture.macro),
                                     gesture.adapter->getValue());
}

void ParameterBridge::end(const juce::String &binding)
{
    auto found = gestures.find(binding.toStdString());
    if (found == gestures.end())
        return;
    const auto gesture = found->second;
    gestures.erase(found);
    finish(gesture);
}

void ParameterBridge::endAll()
{
    auto pending = std::move(gestures);
    gestures.clear();
    for (const auto &entry : pending)
        finish(entry.second);
}

void ParameterBridge::reset(const juce::String &binding)
{
    if (auto *p = parameter(binding))
        set(binding, p->get_default_value_f01());
    else if (macroIndex(binding) >= 0)
        set(binding, 0.f);
}

int ParameterBridge::choiceCount(const Parameter *p) const
{
    if (!p || p->ctrltype == ct_none)
        return 0;
    if (p->valtype == vt_bool)
        return 2;
    if (p->valtype != vt_int)
        return 0;
    int maximum = p->val_max.i;
    if (p->ctrltype == ct_filtersubtype && p->scene >= 1 && p->scene <= n_scenes &&
        p->ctrlgroup_entry >= 0 && p->ctrlgroup_entry < 2)
    {
        const auto type = owner.surge->storage.getPatch()
                              .scene[p->scene - 1]
                              .filterunit[p->ctrlgroup_entry]
                              .type.val.i;
        if (type >= 0 && type < sst::filters::num_filter_types)
        {
            auto count = sst::filters::fut_subcount[type];
            if (type == sst::filters::FilterType::fut_cytomic_svf)
                count = sst::filters::FilterSubType::st_cytomic_allpass + 1;
            maximum = std::min(maximum, std::max(0, count - 1));
        }
    }
    const auto count = static_cast<int64_t>(maximum) - p->val_min.i + 1;
    return count > 0 && count <= 1024 ? static_cast<int>(count) : 0;
}

float ParameterBridge::choiceValue(const juce::String &binding, int choice) const
{
    auto *p = parameter(binding);
    const auto count = choiceCount(p);
    if (choice < 0 || choice >= count)
        return value(binding);
    if (p->valtype == vt_bool)
        return static_cast<float>(choice);
    if (p->val_max.i == p->val_min.i)
        return 0.f;
    // Surge's legacy integer normalization is 0.005 + 0.99 * x, not x.
    return safeNormalized(
        Parameter::intScaledToFloat(p->val_min.i + choice, p->val_max.i, p->val_min.i));
}

int ParameterBridge::choiceIndex(const juce::String &binding) const
{
    auto *p = parameter(binding);
    const auto count = choiceCount(p);
    if (count == 0)
        return -1;
    return std::clamp(p->valtype == vt_bool ? static_cast<int>(p->val.b) : p->val.i - p->val_min.i,
                      0, count - 1);
}

juce::StringArray ParameterBridge::choices(const juce::String &binding) const
{
    juce::StringArray result;
    auto *p = parameter(binding);
    const int count = choiceCount(p);
    for (int i = 0; i < count; ++i)
        result.add(juce::String(p->get_display(true, choiceValue(binding, i))));
    return result;
}

bool ParameterBridge::setText(const juce::String &binding, const juce::String &text,
                              juce::String &error)
{
    error.clear();
    if (!enabled(binding))
    {
        error = "This parameter is unavailable in the current algorithm.";
        return false;
    }
    if (auto *p = parameter(binding))
    {
        pdata parsed{};
        std::string reason;
        if (!p->set_value_from_string_onto(text.toStdString(), parsed, reason))
        {
            error = juce::String(reason);
            return false;
        }
        float result{};
        if (p->valtype == vt_int)
            result = p->val_min.i == p->val_max.i
                         ? 0.f
                         : Parameter::intScaledToFloat(parsed.i, p->val_max.i, p->val_min.i);
        else if (p->valtype == vt_bool)
            result = parsed.b ? 1.f : 0.f;
        else
            result = p->value_to_normalized(parsed.f);
        if (!std::isfinite(result))
        {
            error = "Enter a finite value within this parameter's range.";
            return false;
        }
        set(binding, result);
        return true;
    }
    try
    {
        auto input = text.trim().toStdString();
        size_t end{};
        const auto parsed = std::stof(input, &end);
        if (end != input.size() || !std::isfinite(parsed) || parsed < 0.f || parsed > 1.f)
            throw std::invalid_argument("range");
        set(binding, parsed);
        return true;
    }
    catch (const std::exception &)
    {
        error = "Enter a macro value from 0 to 1.";
        return false;
    }
}
} // namespace Surge::GUI::Reference
