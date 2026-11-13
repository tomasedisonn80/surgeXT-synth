/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_MODULATION_INTERACTION_H
#define SURGE_REFERENCE_MODULATION_INTERACTION_H

#include "WorkspaceData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace Surge::GUI::Reference
{
/** Native input over a painted route row.

    The workspace retains its reference-specific table typography and amount
    artwork. This transparent component supplies selection, a real accessible
    normalized depth slider, and a native mute/remove context menu. Place the
    component over one row and call setAmountBounds() with its local amount cell.

    onChanged may rebuild the surrounding table; it is always called last.
    Keep row components alive while model.isEditing(), including a zero crossing.
    End all interactions before changing scene/patch or destroying the model.
*/
class ModulationRowInteraction : public juce::Component, private juce::AsyncUpdater
{
  public:
    ModulationRowInteraction(ModulationEditorModel &model, const RouteRow &row, juce::Colour accent)
        : model(model), row(row), accent(accent)
    {
        setWantsKeyboardFocus(true);
        setInterceptsMouseClicks(true, true);
        depth.setComponentID("workspace.modulation.depth");
        depth.setSliderStyle(juce::Slider::LinearHorizontal);
        depth.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        depth.setRange(-1.0, 1.0, .001);
        depth.setMouseDragSensitivity(240);
        depth.setWantsKeyboardFocus(true);
        depth.colour = accent;
        depth.requestMenu = [this] { showMenu(); };
        depth.requestSelection = [this] { select(); };
        depth.cancelEdit = [this] { cancelGesture(); };
        depth.onDragStart = [this] {
            cancelGesture();
            dragging = this->model.begin(this->row);
            select();
        };
        depth.onValueChange = [this] {
            if (syncing)
                return;
            const bool oneShot = !dragging;
            if (oneShot && !this->model.begin(this->row))
            {
                syncValue();
                return;
            }
            const bool edited = this->model.setDepth(static_cast<float>(depth.getValue()));
            if (oneShot)
                this->model.end();
            if (!edited)
            {
                cancelGesture();
                syncValue();
                return;
            }
            this->row.normalizedDepth = static_cast<float>(depth.getValue());
            changed();
        };
        depth.onDragEnd = [this] {
            cancelGesture();
            changed();
        };
        addAndMakeVisible(depth);
        setRow(row);
    }
    ~ModulationRowInteraction() override
    {
        cancelPendingUpdate();
        cancelGesture();
    }

    void setRow(const RouteRow &updated)
    {
        if (row.key != updated.key)
            cancelGesture();
        row = updated;
        const auto suffix =
            juce::String(row.key.parameterId) + "." + juce::String(row.key.sourceId) + "." +
            juce::String(row.key.sourceScene) + "." + juce::String(row.key.sourceIndex);
        setComponentID("workspace.modulation.row." + suffix);
        depth.setComponentID("workspace.modulation.depth." + suffix);
        const auto description = row.source + juce::String::fromUTF8(" → ") + row.destination;
        setName(description);
        setTitle(description);
        setDescription(description + ". " + row.amount + (row.muted ? ". Muted." : ". Active.") +
                       " Return focuses depth. M toggles mute. Delete removes this route. "
                       "Right-click opens route actions.");
        depth.setName("Modulation depth: " + description);
        depth.setTitle(depth.getName());
        depth.setDescription("Normalized depth from -1 to +1; native amount " + row.amount +
                             ". Arrow keys adjust. Shift makes fine adjustments.");
        depth.setTooltip(
            description + "\n" + row.amount +
            "\nDrag to change depth. Shift for fine adjustment. Right-click for actions.");
        if (!dragging)
            syncValue();
        repaint();
    }
    const RouteRow &route() const { return row; }
    void setSelected(bool value)
    {
        selected = value;
        repaint();
    }
    void setAmountBounds(juce::Rectangle<int> bounds) { depth.setBounds(bounds); }
    juce::Slider &depthSlider() { return depth; }
    void cancelGesture()
    {
        dragging = false;
        if (model.isEditing(row.key))
            model.end();
    }
    void visibilityChanged() override
    {
        if (!isShowing())
            cancelGesture();
    }
    void focusLost(FocusChangeType) override
    {
        if (!hasKeyboardFocus(true))
            cancelGesture();
    }
    void paint(juce::Graphics &g) override
    {
        if (selected)
        {
            g.setColour(accent.withAlpha(.065f));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 3);
        }
        if (hasKeyboardFocus(false))
        {
            g.setColour(accent);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 3, 1.3f);
        }
    }
    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.mods.isPopupMenu())
        {
            showMenu();
            return;
        }
        grabKeyboardFocus();
        select();
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        const auto code = key.getKeyCode();
        if (code == juce::KeyPress::returnKey)
        {
            depth.grabKeyboardFocus();
            return true;
        }
        if (code == juce::KeyPress::escapeKey)
        {
            cancelGesture();
            return true;
        }
        if (code == juce::KeyPress::F10Key && key.getModifiers().isShiftDown())
        {
            showMenu();
            return true;
        }
        if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
        {
            cancelGesture();
            if (model.remove(row))
                changed();
            return true;
        }
        if (!key.getModifiers().isCommandDown() &&
            (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M'))
        {
            cancelGesture();
            if (model.setMuted(row, !row.muted))
                changed();
            return true;
        }
        return false;
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<juce::AccessibilityHandler>(
            *this, juce::AccessibilityRole::row,
            juce::AccessibilityActions{}.addAction(juce::AccessibilityActionType::focus, [this] {
                grabKeyboardFocus();
                select();
            }));
    }

    std::function<void(const RouteRow &)> onSelected;
    std::function<void()> onChanged;

  private:
    class DepthSlider : public juce::Slider
    {
      public:
        juce::Colour colour;
        std::function<void()> requestMenu, requestSelection, cancelEdit;
        // Table values are painted by the workspace from the real route. The
        // slider only contributes a focus/hover affordance over that amount cell.
        void paint(juce::Graphics &g) override
        {
            if (hasKeyboardFocus(true) || isMouseOverOrDragging())
            {
                g.setColour(colour.withAlpha(hasKeyboardFocus(true) ? .9f : .5f));
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 2, 1);
            }
        }
        juce::String getTextFromValue(double value) override { return juce::String(value, 3); }
        void mouseDown(const juce::MouseEvent &event) override
        {
            if (event.mods.isPopupMenu())
            {
                if (requestMenu)
                    requestMenu();
                return;
            }
            juce::Slider::mouseDown(event);
        }
        bool keyPressed(const juce::KeyPress &key) override
        {
            if (key.getKeyCode() == juce::KeyPress::escapeKey)
            {
                if (cancelEdit)
                    cancelEdit();
                return true;
            }
            const auto code = key.getKeyCode();
            if (code == juce::KeyPress::leftKey || code == juce::KeyPress::downKey ||
                code == juce::KeyPress::rightKey || code == juce::KeyPress::upKey)
            {
                const auto delta = key.getModifiers().isShiftDown() ? .001 : .01;
                const auto direction =
                    code == juce::KeyPress::leftKey || code == juce::KeyPress::downKey ? -1 : 1;
                setValue(getValue() + direction * delta, juce::sendNotificationSync);
                return true;
            }
            return juce::Slider::keyPressed(key);
        }
        void focusGained(FocusChangeType cause) override
        {
            juce::Slider::focusGained(cause);
            if (requestSelection)
                requestSelection();
        }
        void focusLost(FocusChangeType cause) override
        {
            juce::Slider::focusLost(cause);
            if (cancelEdit)
                cancelEdit();
        }
    };
    void syncValue()
    {
        syncing = true;
        depth.setValue(row.normalizedDepth, juce::dontSendNotification);
        syncing = false;
    }
    void select()
    {
        const auto callback = onSelected;
        const auto snapshot = row;
        if (callback)
            callback(snapshot);
    }
    void changed()
    {
        // A route deletion can rebuild this table. Leave the Slider/PopupMenu
        // callback stack first; JUCE must finish its own value-change handling.
        triggerAsyncUpdate();
    }
    void handleAsyncUpdate() override
    {
        const auto callback = onChanged;
        if (callback)
            callback();
    }
    void showMenu()
    {
        cancelGesture();
        juce::PopupMenu menu;
        menu.addSectionHeader(row.source + juce::String::fromUTF8(" → ") + row.destination);
        menu.addItem(1, row.muted ? "Unmute modulation" : "Mute modulation");
        menu.addItem(2, "Remove modulation");
        juce::Component::SafePointer<ModulationRowInteraction> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safe](int id) {
            if (!safe)
                return;
            if (id == 1)
            {
                if (safe->model.setMuted(safe->row, !safe->row.muted))
                    safe->changed();
            }
            else if (id == 2)
            {
                if (safe->model.remove(safe->row))
                    safe->changed();
            }
        });
    }
    ModulationEditorModel &model;
    RouteRow row;
    juce::Colour accent;
    DepthSlider depth;
    bool dragging{false}, syncing{false}, selected{false};
};
} // namespace Surge::GUI::Reference
#endif
