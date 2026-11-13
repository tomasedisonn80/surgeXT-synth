/*
 * Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE.
 * Message-thread adapter between the alternative editor and existing parameters.
 */
#ifndef SURGE_REFERENCE_PARAMETER_BRIDGE_H
#define SURGE_REFERENCE_PARAMETER_BRIDGE_H

#include <juce_core/juce_core.h>
#include <functional>
#include <unordered_map>

class Parameter;
struct SurgeSynthProcessor;
namespace juce
{
class RangedAudioParameter;
}

namespace Surge::GUI::Reference
{
class ParameterBridge
{
  public:
    explicit ParameterBridge(SurgeSynthProcessor &);
    ~ParameterBridge();
    ParameterBridge(const ParameterBridge &) = delete;
    ParameterBridge &operator=(const ParameterBridge &) = delete;

    void setScene(int);
    int getScene() const { return scene; }
    bool valid(const juce::String &) const;
    bool enabled(const juce::String &) const;
    float value(const juce::String &) const;
    juce::String display(const juce::String &) const;
    juce::String label(const juce::String &) const;
    void begin(const juce::String &);
    void set(const juce::String &, float normalized);
    void end(const juce::String &);
    void endAll();
    void reset(const juce::String &);
    juce::StringArray choices(const juce::String &) const;
    float choiceValue(const juce::String &, int choiceIndex) const;
    int choiceIndex(const juce::String &) const;
    bool setText(const juce::String &, const juce::String &, juce::String &error);

    Parameter *parameter(const juce::String &) const;
    SurgeSynthProcessor &processor() { return owner; }
    const SurgeSynthProcessor &processor() const { return owner; }
    size_t activeGestureCount() const { return gestures.size(); }

    // Optional replacement hooks for an editor-owned undo policy. Without a hook,
    // the processor's existing undo manager captures the pre-edit state.
    std::function<void(Parameter *)> onBeginParameter;
    std::function<void(int)> onBeginMacro;

  private:
    struct Gesture
    {
        juce::RangedAudioParameter *adapter{};
        Parameter *parameter{};
        int macro{-1};
    };
    SurgeSynthProcessor &owner;
    int scene{0};
    std::unordered_map<std::string, Gesture> gestures;

    int macroIndex(const juce::String &) const;
    juce::RangedAudioParameter *adapter(const juce::String &) const;
    int choiceCount(const Parameter *) const;
    void finish(const Gesture &);
    void captureUndo(const Gesture &);
};
} // namespace Surge::GUI::Reference
#endif
