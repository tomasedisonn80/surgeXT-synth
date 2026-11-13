/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_WORKSPACE_H
#define SURGE_REFERENCE_WORKSPACE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

class SurgeSynthProcessor;

namespace Surge::GUI::Reference
{
class ParameterBridge;
struct WavetableSelectionRequest;

/** A live native alternative editor. Layouts contain no audio or executable code. */
class Workspace : public juce::Component, private juce::Timer
{
  public:
    explicit Workspace(SurgeSynthProcessor &);
    ~Workspace() override;
    bool load(const juce::File &manifest, juce::String &error);
    void refresh();
    ParameterBridge &bridge();
    juce::String currentId() const;
    /** Select an existing panel view without editing processor parameters.
        Invalid group/choice IDs return false and preserve the current view. */
    bool selectView(const juce::String &groupId, const juce::String &choiceId);
    juce::String selectedView(const juce::String &groupId) const;
    /** notify=false synchronises the native/host scene without requesting another scene change. */
    void selectScene(int scene, bool notify = true);
    void paint(juce::Graphics &) override;
    void resized() override;
    void visibilityChanged() override;
    bool keyPressed(const juce::KeyPress &) override;

    std::function<void()> onFullEditor;
    std::function<void()> onChooseSkin;
    std::function<void(const juce::String &)> onAction;
    /** The native editor supplies script generation and accessible announcements.
        Without it, ordinary catalog tables still use the processor's queue. */
    std::function<bool(const WavetableSelectionRequest &)> onSelectWavetable;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Workspace)
};
} // namespace Surge::GUI::Reference
#endif
