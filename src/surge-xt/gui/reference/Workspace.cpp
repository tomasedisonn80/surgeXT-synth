/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "Workspace.h"
#include "Manifest.h"
#include "ParameterBridge.h"
#include "Visuals.h"
#include "WorkspaceData.h"
#include "SurgeSynthProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <algorithm>
#include <cmath>

namespace Surge::GUI::Reference
{
namespace
{
using Rect = juce::Rectangle<float>;
float number(const juce::var &v, const char *key, float fallback = 0)
{
    return v.hasProperty(key) ? static_cast<float>(v[key]) : fallback;
}
juce::String string(const juce::var &v, const char *key, juce::String fallback = {})
{
    return v.hasProperty(key) ? v[key].toString() : fallback;
}
Rect bounds(const juce::var &v)
{
    return {number(v, "x"), number(v, "y"), number(v, "w"), number(v, "h")};
}
Rect arrayRect(const juce::var &v, Rect fallback)
{
    if (auto *a = v.getArray(); a && a->size() == 4)
        return {static_cast<float>((*a)[0]), static_cast<float>((*a)[1]),
                static_cast<float>((*a)[2]), static_cast<float>((*a)[3])};
    return fallback;
}
struct Theme
{
    juce::Colour background{0xff101519}, panel{0xff1a2026}, border{0xff48545d};
    juce::Colour text{0xffe5eaf0}, muted{0xff9ba9b3}, accent{0xffffac4a};
    juce::Colour secondary{0xff72d8ec}, display{0xff090f14}, knob{0xff20282e};
    juce::Colour red{0xffca3e2b}, yellow{0xfff8ce39}, blue{0xff315d7b};
    juce::String style, font;
    float radius{7}, glow{0.15f};
    bool light() const { return background.getPerceivedBrightness() > 0.6f; }
    juce::Colour color(const juce::String &name) const
    {
        if (name == "secondary")
            return secondary;
        if (name == "accent")
            return accent;
        if (name == "red")
            return red;
        if (name == "yellow")
            return yellow;
        if (name == "blue")
            return blue;
        if (name.startsWithChar('#'))
            return juce::Colour::fromString("ff" + name.substring(1));
        return panel;
    }
    void read(const juce::var &v)
    {
        auto p = v["palette"];
        auto readColor = [&](const char *k, juce::Colour &c) {
            auto s = string(p, k);
            if (s.startsWithChar('#'))
                c = juce::Colour::fromString("ff" + s.substring(1));
        };
        readColor("background", background);
        readColor("panel", panel);
        readColor("border", border);
        readColor("text", text);
        readColor("muted", muted);
        readColor("accent", accent);
        readColor("secondary", secondary);
        readColor("display", display);
        readColor("knob", knob);
        readColor("red", red);
        readColor("yellow", yellow);
        readColor("blue", blue);
        style = string(v, "style");
        font = string(v, "font");
        radius = number(v, "radius", 7);
        glow = number(v, "glow", 0.15f);
    }
    juce::Font type(float size, bool bold = false) const
    {
        auto family = font == "serif"  ? juce::Font::getDefaultSerifFontName()
                      : font == "mono" ? juce::Font::getDefaultMonospacedFontName()
                                       : juce::Font::getDefaultSansSerifFontName();
        return juce::Font(
            juce::FontOptions(family, size, bold ? juce::Font::bold : juce::Font::plain));
    }
};
void text(juce::Graphics &g, const Theme &t, const juce::String &s, Rect r, float size,
          juce::Colour color, bool bold = false, int align = juce::Justification::centredLeft)
{
    g.setFont(t.type(size, bold));
    g.setColour(color);
    g.drawFittedText(s, r.toNearestInt(), align, 1, 0.7f);
}
void bezel(juce::Graphics &g, const Theme &t, Rect r, juce::Colour fill, float radius = -1)
{
    if (radius < 0)
        radius = t.radius;
    g.setColour(juce::Colours::black.withAlpha(t.light() ? 0.12f : 0.35f));
    g.fillRoundedRectangle(r.translated(0, 2), radius);
    g.setGradientFill(juce::ColourGradient(fill.brighter(0.07f), r.getTopLeft(), fill.darker(0.12f),
                                           r.getBottomRight(), false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(t.border.withAlpha(0.8f));
    g.drawRoundedRectangle(r.reduced(0.5f), radius, 1);
    g.setColour(juce::Colours::white.withAlpha(t.light() ? 0.55f : 0.09f));
    g.drawLine(r.getX() + radius, r.getY() + 1, r.getRight() - radius, r.getY() + 1, 1);
}
void knob(juce::Graphics &g, const Theme &t, Rect r, float value, juce::Colour accent, bool focus,
          bool enabled)
{
    const auto c = r.getCentre();
    const float rad = std::min(r.getWidth(), r.getHeight()) * 0.43f;
    const float start = -2.35f, end = 2.35f;
    const float a = start + (end - start) * juce::jlimit(0.f, 1.f, value);
    juce::Path arc, track;
    track.addCentredArc(c.x, c.y, rad, rad, 0, start, end, true);
    arc.addCentredArc(c.x, c.y, rad, rad, 0, start, a, true);
    if (!enabled)
        accent = t.muted.withAlpha(0.4f);
    if (t.glow > 0 && enabled)
    {
        g.setColour(accent.withAlpha(std::min(0.18f, t.glow)));
        g.strokePath(arc, juce::PathStrokeType(8));
    }
    g.setColour(t.border.withAlpha(0.65f));
    g.strokePath(track, juce::PathStrokeType(2.6f));
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(t.style == "clear" ? 4.f : 2.6f));
    auto face = Rect(c.x - rad + 4, c.y - rad + 4, (rad - 4) * 2, (rad - 4) * 2);
    g.setColour(juce::Colours::black.withAlpha(0.38f));
    g.fillEllipse(face.translated(1, 2));
    g.setGradientFill(juce::ColourGradient(t.knob.brighter(0.22f), face.getTopLeft(),
                                           t.knob.darker(0.22f), face.getBottomRight(), false));
    g.fillEllipse(face);
    g.setColour(t.light() ? t.border : juce::Colours::white.withAlpha(0.22f));
    g.drawEllipse(face, 1);
    const float inner = rad * 0.43f, outer = rad * 0.76f;
    g.setColour(t.light() ? accent.darker(0.4f) : t.text);
    g.drawLine(c.x + std::sin(a) * inner, c.y - std::cos(a) * inner, c.x + std::sin(a) * outer,
               c.y - std::cos(a) * outer, 1.8f);
    if (focus)
    {
        g.setColour(t.accent);
        g.drawRoundedRectangle(r.reduced(1), 3, 2);
    }
}

class BoundSlider : public juce::Slider
{
  public:
    BoundSlider(ParameterBridge &b, Theme &theme, const juce::var &definition)
        : binding(string(definition, "binding")), spec(definition), bridge(b), theme(theme)
    {
        setComponentID(string(definition, "id"));
        kind = string(spec, "kind", "knob");
        setSliderStyle(kind == "fader"        ? juce::Slider::LinearVertical
                       : kind == "horizontal" ? juce::Slider::LinearHorizontal
                                              : juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRange(0, 1, 0.001);
        setWantsKeyboardFocus(true);
        setMouseDragSensitivity(160);
        onDragStart = [this] {
            bridge.begin(binding);
            dragging = true;
        };
        onValueChange = [this] {
            if (polling)
                return;
            if (!dragging)
                bridge.begin(binding);
            bridge.set(binding, static_cast<float>(getValue()));
            if (!dragging)
                bridge.end(binding);
            repaint();
        };
        onDragEnd = [this] {
            bridge.end(binding);
            dragging = false;
        };
        sync();
    }
    ~BoundSlider() override
    {
        if (dragging)
            bridge.end(binding);
    }
    void sync()
    {
        polling = true;
        setEnabled(bridge.valid(binding) && bridge.enabled(binding));
        if (!dragging)
            setValue(bridge.value(binding), juce::dontSendNotification);
        auto name = string(spec, "label", bridge.label(binding));
        setName(name);
        setTitle(name);
        setDescription(name + ": " + bridge.display(binding));
        setTooltip(bridge.label(binding) + ": " + bridge.display(binding) +
                   "\nDrag to change. Shift for fine adjustment. Double-click to reset.");
        polling = false;
    }
    void mouseDoubleClick(const juce::MouseEvent &) override
    {
        bridge.reset(binding);
        sync();
        repaint();
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat();
        auto accent =
            spec.hasProperty("accent") ? theme.color(string(spec, "accent")) : theme.accent;
        auto label = string(spec, "label", bridge.label(binding));
        auto value = isEnabled() ? bridge.display(binding) : juce::String("—");
        auto ink = spec.hasProperty("ink") ? theme.color(string(spec, "ink")) : theme.text;
        auto valueInk = spec.hasProperty("ink") ? ink.withAlpha(.85f) : theme.muted;
        float fs = number(spec, "fontSize", theme.style == "clear" ? 15 : 11.5f);
        if (kind == "knob")
        {
            const auto labelPosition = string(spec, "labelPosition");
            if (labelPosition == "left" || labelPosition == "right")
            {
                const float side = std::min(r.getHeight(), r.getWidth() * .48f);
                auto face =
                    labelPosition == "left" ? r.removeFromRight(side) : r.removeFromLeft(side);
                knob(g, theme, face, static_cast<float>(getValue()), accent, hasKeyboardFocus(true),
                     isEnabled());
                auto labels = r.reduced(4, std::max(0.f, (r.getHeight() - 32) * .5f));
                const int align = labelPosition == "left" ? juce::Justification::centredRight
                                                          : juce::Justification::centredLeft;
                text(g, theme, label, labels.removeFromTop(16), fs, ink, false, align);
                text(g, theme, value, labels, fs, valueInk, false, align);
                return;
            }
            const auto lower = std::min(32.f, r.getHeight() * 0.42f);
            auto face = r.withTrimmedBottom(lower);
            const float side = std::min(face.getWidth(), face.getHeight());
            face = Rect(face.getCentreX() - side / 2, face.getCentreY() - side / 2, side, side);
            knob(g, theme, face, static_cast<float>(getValue()), accent, hasKeyboardFocus(true),
                 isEnabled());
            text(g, theme, label, r.removeFromBottom(lower).withHeight(lower / 2), fs, ink, false,
                 juce::Justification::centred);
            text(g, theme, value, getLocalBounds().toFloat().removeFromBottom(lower / 2), fs,
                 valueInk, false, juce::Justification::centred);
        }
        else
        {
            text(g, theme, label, r.removeFromTop(17), fs, ink, false,
                 juce::Justification::centred);
            text(g, theme, value, r.removeFromBottom(18), fs, valueInk, false,
                 juce::Justification::centred);
            auto track = r.reduced(kind == "fader" ? r.getWidth() * .4f : 7.f, 5);
            if (kind == "horizontal")
                track = track.withSizeKeepingCentre(track.getWidth(), 5);
            bezel(g, theme, track.expanded(3), theme.display, 3);
            g.setColour(accent);
            auto active = track;
            if (kind == "fader")
                active = track.withTop(track.getBottom() - track.getHeight() * getValue());
            else
                active.setWidth(track.getWidth() * getValue());
            g.fillRoundedRectangle(active, 2);
            Rect handle;
            if (kind == "fader")
                handle = {track.getCentreX() - 12, active.getY() - 4, 24, 8};
            else
                handle = {track.getX() + track.getWidth() * static_cast<float>(getValue()) - 5,
                          track.getCentreY() - 6, 10, 12};
            g.setColour(theme.light() ? theme.knob : theme.text);
            g.fillRoundedRectangle(handle, 2);
            g.setColour(theme.border);
            g.drawRoundedRectangle(handle, 2, 1);
            if (hasKeyboardFocus(true))
            {
                g.setColour(theme.accent);
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 3, 2);
            }
        }
    }
    juce::String getTextFromValue(double) override { return bridge.display(binding); }
    juce::String binding;
    juce::var spec;

  private:
    ParameterBridge &bridge;
    Theme &theme;
    juce::String kind;
    bool dragging{false}, polling{false};
};

class BoundMenu : public juce::ComboBox
{
  public:
    BoundMenu(ParameterBridge &b, Theme &t, const juce::var &d)
        : bridge(b), theme(t), spec(d), binding(string(d, "binding"))
    {
        setComponentID(string(d, "id"));
        setWantsKeyboardFocus(true);
        onChange = [this] {
            if (polling || getSelectedItemIndex() < 0)
                return;
            bridge.begin(binding);
            bridge.set(binding, bridge.choiceValue(binding, getSelectedItemIndex()));
            bridge.end(binding);
        };
        sync();
    }
    void sync()
    {
        polling = true;
        auto c = bridge.choices(binding);
        if (c != items)
        {
            clear(juce::dontSendNotification);
            items = c;
            for (int i = 0; i < c.size(); ++i)
                addItem(c[i], i + 1);
        }
        setSelectedItemIndex(bridge.choiceIndex(binding), juce::dontSendNotification);
        setEnabled(bridge.valid(binding) && !c.isEmpty());
        setName(string(spec, "label", bridge.label(binding)));
        setTitle(getName());
        setTooltip(getName() + ": " + bridge.display(binding));
        setColour(juce::ComboBox::backgroundColourId, theme.display);
        setColour(juce::ComboBox::textColourId, theme.light() ? theme.text : theme.text);
        // Cream themes use dark ink on cream menu surfaces, not on the green plotting well.
        if (theme.light())
            setColour(juce::ComboBox::backgroundColourId, theme.panel.brighter(.08f));
        setColour(juce::ComboBox::outlineColourId, theme.border);
        setColour(juce::ComboBox::arrowColourId, theme.accent);
        polling = false;
    }

  private:
    ParameterBridge &bridge;
    Theme &theme;
    juce::var spec;
    juce::String binding;
    juce::StringArray items;
    bool polling{false};
};

class BoundToggle : public juce::TextButton
{
  public:
    BoundToggle(ParameterBridge &b, Theme &t, const juce::var &d)
        : bridge(b), theme(t), spec(d), binding(string(d, "binding"))
    {
        setComponentID(string(d, "id"));
        setClickingTogglesState(true);
        setWantsKeyboardFocus(true);
        onClick = [this] {
            bridge.begin(binding);
            bridge.set(binding, getToggleState() ? 1.f : 0.f);
            bridge.end(binding);
        };
        sync();
    }
    void sync()
    {
        setButtonText(string(spec, "label", bridge.label(binding)));
        setName(getButtonText());
        setTitle(getName());
        setToggleState(bridge.value(binding) >= .5f, juce::dontSendNotification);
        setEnabled(bridge.valid(binding));
        setColour(juce::TextButton::buttonColourId, theme.panel);
        setColour(juce::TextButton::buttonOnColourId, theme.accent);
        setColour(juce::TextButton::textColourOffId, theme.text);
        setColour(juce::TextButton::textColourOnId, theme.background);
    }

  private:
    ParameterBridge &bridge;
    Theme &theme;
    juce::var spec;
    juce::String binding;
};

class MacroXY : public juce::Component
{
  public:
    MacroXY(ParameterBridge &b, Theme &t) : bridge(b), theme(t)
    {
        setWantsKeyboardFocus(true);
        setTitle("Macro XY: X macro 1, Y macro 2");
        setName(getTitle());
    }
    ~MacroXY() override { finish(); }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat().reduced(3);
        bezel(g, theme, r, theme.display, 18);
        Visuals::grid(g, r.reduced(5), theme.accent.withAlpha(.13f));
        const float x = r.getX() + bridge.value("macro1") * r.getWidth();
        const float y = r.getBottom() - bridge.value("macro2") * r.getHeight();
        g.setGradientFill(juce::ColourGradient(theme.accent.withAlpha(.18f), {x, y},
                                               theme.secondary.withAlpha(.025f), r.getTopLeft(),
                                               true));
        g.fillRoundedRectangle(r, 18);
        g.setColour(theme.accent.withAlpha(.45f));
        g.drawLine(x, r.getY(), x, r.getBottom());
        g.drawLine(r.getX(), y, r.getRight(), y);
        g.setColour(theme.secondary.withAlpha(.15f));
        g.fillEllipse(x - 16, y - 16, 32, 32);
        g.setColour(theme.secondary);
        g.fillEllipse(x - 6, y - 6, 12, 12);
        g.setColour(juce::Colours::white);
        g.drawEllipse(x - 6, y - 6, 12, 12, 2);
        text(g, theme, "X: Macro 1  •  Y: Macro 2", r.removeFromBottom(22), 12, theme.text, false,
             juce::Justification::centred);
        if (hasKeyboardFocus(true))
        {
            g.setColour(theme.accent);
            g.drawRoundedRectangle(r, 18, 2);
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        grabKeyboardFocus();
        bridge.begin("macro1");
        bridge.begin("macro2");
        dragging = true;
        update(e.position);
    }
    void mouseDrag(const juce::MouseEvent &e) override { update(e.position); }
    void mouseUp(const juce::MouseEvent &) override { finish(); }
    bool keyPressed(const juce::KeyPress &k) override
    {
        const auto step = k.getModifiers().isShiftDown() ? .001f : .01f;
        juce::String b;
        float delta = 0;
        if (k == juce::KeyPress::leftKey)
        {
            b = "macro1";
            delta = -step;
        }
        if (k == juce::KeyPress::rightKey)
        {
            b = "macro1";
            delta = step;
        }
        if (k == juce::KeyPress::downKey)
        {
            b = "macro2";
            delta = -step;
        }
        if (k == juce::KeyPress::upKey)
        {
            b = "macro2";
            delta = step;
        }
        if (b.isEmpty())
            return false;
        bridge.begin(b);
        bridge.set(b, bridge.value(b) + delta);
        bridge.end(b);
        repaint();
        return true;
    }

  private:
    void finish()
    {
        if (dragging)
        {
            bridge.end("macro1");
            bridge.end("macro2");
            dragging = false;
        }
    }
    void update(juce::Point<float> p)
    {
        bridge.set("macro1", p.x / std::max(1, getWidth()));
        bridge.set("macro2", 1 - p.y / std::max(1, getHeight()));
        repaint();
    }
    ParameterBridge &bridge;
    Theme &theme;
    bool dragging{false};
};

class PerformanceKeyboard : public juce::MidiKeyboardComponent
{
  public:
    explicit PerformanceKeyboard(juce::MidiKeyboardState &state)
        : juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
    }
    void releaseHeldNotes() { resetAnyKeysInUse(); }
    ~PerformanceKeyboard() override { releaseHeldNotes(); }
};
} // anonymous namespace

struct Workspace::Impl
{
    Workspace &owner;
    SurgeSynthProcessor &processor;
    ParameterBridge parameters;
    juce::var manifest;
    Theme theme;
    VisualData visuals;
    std::vector<RouteRow> routes;
    std::vector<BrowserRow> presetRows;
    juce::Component canvas;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::pair<juce::Component *, Rect>> placements;
    std::vector<std::unique_ptr<juce::TextButton>> actions;
    std::unique_ptr<MacroXY> xy;
    std::unique_ptr<PerformanceKeyboard> keyboard;
    juce::TextButton skinButton{"Skins"}, classicButton{"Full editor"}, sceneA{"A"}, sceneB{"B"};
    juce::TextButton presetButton, previous{"<"}, next{">"};
    int ticks{0};
    float scale{1};
    juce::Point<float> offset;

    Impl(Workspace &o, SurgeSynthProcessor &p) : owner(o), processor(p), parameters(p)
    {
        owner.addAndMakeVisible(canvas);
        canvas.setInterceptsMouseClicks(false, true);
        for (auto *b :
             {&skinButton, &classicButton, &sceneA, &sceneB, &presetButton, &previous, &next})
        {
            canvas.addAndMakeVisible(b);
            b->setWantsKeyboardFocus(true);
            b->setTitle(b->getButtonText());
        }
        skinButton.setComponentID("workspace.skin-selector");
        classicButton.setComponentID("workspace.full-editor");
        sceneA.setComponentID("workspace.scene-a");
        sceneB.setComponentID("workspace.scene-b");
        presetButton.setComponentID("workspace.preset");
        skinButton.onClick = [this] {
            if (owner.onChooseSkin)
                owner.onChooseSkin();
        };
        classicButton.onClick = [this] {
            if (owner.onFullEditor)
                owner.onFullEditor();
        };
        sceneA.onClick = [this] { owner.selectScene(0); };
        sceneB.onClick = [this] { owner.selectScene(1); };
        presetButton.onClick = [this] { action("browser"); };
        previous.onClick = [this] { action("previous-preset"); };
        next.onClick = [this] { action("next-preset"); };
    }
    void action(const juce::String &a)
    {
        if (owner.onAction)
            owner.onAction(a);
    }
    void button(const juce::String &label, const juce::String &actionId, Rect r)
    {
        auto b = std::make_unique<juce::TextButton>(label);
        b->setName(label);
        b->setTitle(label);
        b->setComponentID("workspace.action." + actionId + juce::String(actions.size()));
        b->onClick = [this, actionId] { action(actionId); };
        b->setWantsKeyboardFocus(true);
        canvas.addAndMakeVisible(*b);
        placements.push_back({b.get(), r});
        actions.push_back(std::move(b));
    }
    void styleButton(juce::TextButton &b)
    {
        b.setColour(juce::TextButton::buttonColourId, theme.panel);
        b.setColour(juce::TextButton::buttonOnColourId, theme.accent);
        b.setColour(juce::TextButton::textColourOffId, theme.text);
        b.setColour(juce::TextButton::textColourOnId, theme.background);
    }
    void build()
    {
        controls.clear();
        placements.clear();
        actions.clear();
        xy.reset();
        keyboard.reset();
        if (auto *a = manifest["controls"].getArray())
            for (auto &v : *a)
            {
                std::unique_ptr<juce::Component> c;
                auto kind = string(v, "kind", "knob");
                if (kind == "menu")
                    c = std::make_unique<BoundMenu>(parameters, theme, v);
                else if (kind == "toggle")
                    c = std::make_unique<BoundToggle>(parameters, theme, v);
                else
                    c = std::make_unique<BoundSlider>(parameters, theme, v);
                canvas.addAndMakeVisible(*c);
                placements.push_back({c.get(), bounds(v)});
                controls.push_back(std::move(c));
            }
        if (auto *a = manifest["panels"].getArray())
            for (auto &v : *a)
            {
                const auto kind = string(v, "kind");
                auto r = bounds(v);
                if (kind == "matrix")
                    button("Edit routes  +", "modulation",
                           r.removeFromTop(28).removeFromRight(112).reduced(3));
                if (kind == "browser")
                    button("Browse all presets", "browser", r.removeFromBottom(31).reduced(7, 3));
                if (kind == "effects")
                    button("Full FX editor", "effects",
                           r.removeFromTop(28).removeFromRight(110).reduced(3));
                if (kind == "analyzer")
                    button("Open scope", "analyzer",
                           r.removeFromTop(28).removeFromRight(92).reduced(3));
                if (kind == "xy")
                {
                    xy = std::make_unique<MacroXY>(parameters, theme);
                    xy->setComponentID("workspace.macro-xy");
                    canvas.addAndMakeVisible(*xy);
                    auto pr = arrayRect(v["plot"], r.reduced(12).withTrimmedTop(25));
                    placements.push_back({xy.get(), pr});
                }
                if (kind == "keyboard")
                {
                    keyboard = std::make_unique<PerformanceKeyboard>(processor.midiKeyboardState);
                    keyboard->setComponentID("workspace.performance-keyboard");
                    keyboard->setName("Performance MIDI keyboard");
                    keyboard->setTitle("Performance MIDI keyboard");
                    keyboard->setAvailableRange(24, 108);
                    keyboard->setLowestVisibleKey(36);
                    keyboard->setKeyWidth(17);
                    keyboard->setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
                                        juce::Colour(0xffeeeae2));
                    keyboard->setColour(juce::MidiKeyboardComponent::blackNoteColourId,
                                        theme.display);
                    keyboard->setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                                        theme.accent.withAlpha(.6f));
                    keyboard->setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                                        theme.secondary.withAlpha(.25f));
                    canvas.addAndMakeVisible(*keyboard);
                    placements.push_back(
                        {keyboard.get(), arrayRect(v["plot"], r.reduced(12).withTrimmedTop(50))});
                }
            }
        for (auto *b :
             {&skinButton, &classicButton, &sceneA, &sceneB, &presetButton, &previous, &next})
            styleButton(*b);
        for (auto &b : actions)
            styleButton(*b);
        owner.resized();
    }
    void update()
    {
        for (auto &c : controls)
        {
            if (auto *s = dynamic_cast<BoundSlider *>(c.get()))
                s->sync();
            if (auto *s = dynamic_cast<BoundMenu *>(c.get()))
                s->sync();
            if (auto *s = dynamic_cast<BoundToggle *>(c.get()))
                s->sync();
        }
        sceneA.setToggleState(parameters.getScene() == 0, juce::dontSendNotification);
        sceneB.setToggleState(parameters.getScene() == 1, juce::dontSendNotification);
        auto name = juce::String(processor.surge->storage.getPatch().name);
        presetButton.setButtonText(name.isEmpty() ? "Init" : name);
        presetButton.setTitle("Preset: " + name);
        if ((ticks++ % 8) == 0)
        {
            visuals = Visuals::sample(processor, parameters.getScene());
            routes = modulationRows(processor, parameters.getScene());
            presetRows = browserRows(processor, {}, 16);
        }
    }
    void drawHeader(juce::Graphics &g)
    {
        const float h = number(manifest, "headerHeight", 92);
        bezel(g, theme, {2, 2, 1582, h - 5}, theme.panel, theme.radius);
        juce::Path logo;
        for (int i = 0; i < 51; ++i)
        {
            float x = 22 + i * .9f, y = h * .43f + std::sin(i * .22f) * std::sin(i * .0628f) * 15;
            if (i == 0)
                logo.startNewSubPath(x, y);
            else
                logo.lineTo(x, y);
        }
        g.setColour(theme.accent);
        g.strokePath(logo, juce::PathStrokeType(2.5f));
        text(g, theme, "SURGE XT", {80, 9, 260, h * .52f}, theme.style == "bauhaus" ? 42 : 34,
             theme.text, true);
        text(g, theme, string(manifest, "name").toUpperCase(), {82, h * .62f, 290, 17}, 11,
             theme.accent);
        text(g, theme, "SCENE", {405, 8, 100, 17}, 11, theme.muted, false,
             juce::Justification::centred);
        text(g, theme, "PRESET", {550, 7, 450, 15}, 10, theme.muted, false,
             juce::Justification::centred);
        if (theme.style == "space")
        {
            for (float x : {9.f, 1574.f})
                for (float y : {9.f, h - 12})
                {
                    g.setColour(theme.border);
                    g.fillEllipse(x - 3, y - 3, 6, 6);
                    g.setColour(theme.display);
                    g.drawLine(x - 2, y, x + 2, y);
                }
        }
    }
    void plotBase(juce::Graphics &g, Rect r)
    {
        g.setColour(theme.display);
        g.fillRoundedRectangle(r, 3);
        g.setColour(theme.border.withAlpha(.4f));
        g.drawRoundedRectangle(r, 3, 1);
        Visuals::grid(g, r.reduced(2), theme.secondary.withAlpha(theme.light() ? .16f : .1f));
    }
    void drawEnvelope(juce::Graphics &g, Rect r, int index, juce::Colour color)
    {
        plotBase(g, r);
        r = r.reduced(8);
        const juce::String prefix = "env" + juce::String(std::clamp(index, 1, 2)) + ".";
        auto at = parameters.value(prefix + "attack"), de = parameters.value(prefix + "decay");
        auto su = parameters.value(prefix + "sustain"), re = parameters.value(prefix + "release");
        float ax = r.getX() + r.getWidth() * (.07f + .22f * at),
              dx = ax + r.getWidth() * (.07f + .20f * de);
        float rx = r.getRight() - r.getWidth() * (.08f + .24f * re),
              sy = r.getBottom() - su * r.getHeight();
        juce::Path p;
        p.startNewSubPath(r.getX(), r.getBottom());
        p.lineTo(ax, r.getY());
        p.quadraticTo((ax + dx) * .5f, sy, dx, sy);
        p.lineTo(std::max(dx + 2, rx), sy);
        p.quadraticTo(rx + r.getWidth() * .06f, r.getBottom() - 2, r.getRight(), r.getBottom());
        auto fill = p;
        fill.lineTo(r.getX(), r.getBottom());
        fill.closeSubPath();
        g.setGradientFill(juce::ColourGradient(color.withAlpha(.4f), r.getTopLeft(),
                                               color.withAlpha(.02f), r.getBottomLeft(), false));
        g.fillPath(fill);
        g.setColour(color);
        g.strokePath(p, juce::PathStrokeType(1.7f));
        for (auto q : {juce::Point<float>(r.getX(), r.getBottom()),
                       juce::Point<float>(ax, r.getY()), juce::Point<float>(dx, sy),
                       juce::Point<float>(rx, sy), juce::Point<float>(r.getRight(), r.getBottom())})
            g.fillEllipse(q.x - 2.5f, q.y - 2.5f, 5, 5);
    }
    void drawFilter(juce::Graphics &g, Rect r, int index, juce::Colour color, bool base = true)
    {
        if (base)
            plotBase(g, r);
        r = r.reduced(7, 12);
        float cutoff = parameters.value("filter" + juce::String(index) + ".cutoff");
        float res = parameters.value("filter" + juce::String(index) + ".resonance");
        juce::Path p;
        for (int i = 0; i < 180; ++i)
        {
            const float x = i / 179.f, f = cutoff * .85f + .08f;
            float y = .25f + .7f / (1 + std::exp(-16 * (x - f))) -
                      res * .22f * std::exp(-180 * (x - f) * (x - f));
            y = std::clamp(y, .03f, .96f);
            if (!i)
                p.startNewSubPath(r.getX(), r.getY() + y * r.getHeight());
            else
                p.lineTo(r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight());
        }
        auto fill = p;
        fill.lineTo(r.getBottomRight());
        fill.lineTo(r.getBottomLeft());
        fill.closeSubPath();
        g.setGradientFill(juce::ColourGradient(color.withAlpha(.28f), r.getTopLeft(),
                                               color.withAlpha(.01f), r.getBottomLeft(), false));
        g.fillPath(fill);
        g.setColour(color);
        g.strokePath(p, juce::PathStrokeType(1.7f));
        text(g, theme, "20     100     500     2k     10k",
             r.withY(r.getBottom() - 1).withHeight(11), 9, theme.display.contrasting(.8f), false,
             juce::Justification::centred);
    }
    void drawLfo(juce::Graphics &g, Rect r, int index)
    {
        plotBase(g, r);
        r = r.reduced(6);
        juce::Path p;
        const float phase = parameters.value("lfo" + juce::String(index) + ".phase");
        const float mag = parameters.value("lfo" + juce::String(index) + ".magnitude");
        for (int i = 0; i < 160; ++i)
        {
            float x = i / 159.f,
                  y = .5f -
                      .40f * std::sin((x * 2 + phase) * juce::MathConstants<float>::twoPi) * mag;
            if (!i)
                p.startNewSubPath(r.getX(), r.getY() + y * r.getHeight());
            else
                p.lineTo(r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight());
        }
        g.setColour(theme.secondary);
        g.strokePath(p, juce::PathStrokeType(1.8f));
    }
    void drawMeters(juce::Graphics &g, Rect r)
    {
        const float gap = std::max(3.f, r.getWidth() * .15f);
        float w = (r.getWidth() - gap) / 2;
        for (int ch = 0; ch < 2; ++ch)
        {
            Rect bar(r.getX() + ch * (w + gap), r.getY(), w, r.getHeight() - 16);
            g.setColour(theme.display);
            g.fillRoundedRectangle(bar, 2);
            auto peak = ch ? visuals.right : visuals.left;
            float proportion =
                peak > 0 ? std::clamp((20.f * std::log10(peak) + 60) / 60, 0.f, 1.f) : 0;
            for (int s = 0; s < 24; ++s)
            {
                float y = bar.getBottom() - (s + 1) * bar.getHeight() / 24;
                g.setColour((s > 20 ? theme.secondary : theme.accent)
                                .withAlpha((s / 24.f < proportion) ? 1.f : .1f));
                g.fillRect(Rect(bar.getX() + 2, y + 1, w - 4, bar.getHeight() / 24 - 2));
            }
            text(g, theme, ch ? "R" : "L", {bar.getX(), bar.getBottom() + 2, w, 14}, 10,
                 theme.muted, false, juce::Justification::centred);
        }
    }
    void drawPanel(juce::Graphics &g, const juce::var &v)
    {
        auto r = bounds(v);
        auto kind = string(v, "kind"), variant = string(v, "variant");
        auto fill = v.hasProperty("color") ? theme.color(string(v, "color")) : theme.panel;
        bezel(g, theme, r, fill);
        const int index = std::max(1, static_cast<int>(number(v, "index", 1)));
        auto title = string(v, "title", kind.toUpperCase());
        auto titleColor = theme.text;
        if (v.hasProperty("ink"))
            titleColor = theme.color(string(v, "ink"));
        if (theme.style == "bauhaus" &&
            (string(v, "color") == "red" || string(v, "color") == "blue"))
            titleColor = juce::Colours::white;
        text(g, theme, title, r.reduced(12, 3).withHeight(25), theme.style == "ivory" ? 18 : 15,
             titleColor, true);
        auto content = r.reduced(10).withTrimmedTop(24);
        auto pr =
            arrayRect(v["plot"], content.withHeight(std::max(25.f, content.getHeight() * .56f)));
        if (kind == "oscillator")
        {
            if (!v.hasProperty("plot"))
            {
                if (r.getWidth() > r.getHeight() * 1.7f)
                    pr = {r.getX() + 10, r.getY() + 43, r.getWidth() * .43f, r.getHeight() - 54};
                else
                    pr = {r.getX() + 10, r.getY() + 61, r.getWidth() - 20, r.getHeight() * .38f};
            }
            plotBase(g, pr);
            if (index <= 3)
                Visuals::waveform(g, pr.reduced(5), visuals.oscillator[index - 1], theme.secondary,
                                  theme.accent, variant == "terrain");
        }
        else if (kind == "waveform")
        {
            pr = arrayRect(v["plot"], content.withTrimmedBottom(variant == "terrain" ? 20 : 3));
            plotBase(g, pr);
            Visuals::waveform(g, pr.reduced(8), visuals.oscillator[0], theme.secondary,
                              theme.accent,
                              variant == "terrain" || theme.style == "bauhaus" ||
                                  theme.style == "live" || theme.style == "aurora");
            text(g, theme, "OSCILLATOR PREVIEW", {pr.getX() + 9, pr.getBottom() - 19, 220, 16}, 9,
                 theme.muted);
        }
        else if (kind == "filter" || kind == "inspector")
        {
            drawFilter(g, pr, index, index == 2 ? theme.secondary : theme.accent);
            if (variant == "combined")
                drawFilter(g, pr, 2, theme.secondary, false);
            text(g, theme, "Response preview", {pr.getX() + 4, pr.getY() + 2, 140, 12}, 9,
                 theme.display.contrasting(.8f));
        }
        else if (kind == "envelope" || kind == "amp")
            drawEnvelope(g, pr, index, theme.accent);
        else if (kind == "lfo")
            drawLfo(g, pr, index);
        else if (kind == "analyzer")
        {
            auto meter = content.removeFromRight(std::min(45.f, content.getWidth() * .15f));
            drawMeters(g, meter.reduced(4));
            pr = arrayRect(v["plot"], content.reduced(3));
            plotBase(g, pr);
            if (!visuals.output.empty())
                Visuals::waveform(g, pr.reduced(5), visuals.output, theme.secondary, theme.accent,
                                  false);
            else
            {
                g.setColour(theme.secondary.withAlpha(.5f));
                g.drawLine(pr.getX() + 5, pr.getCentreY(), pr.getRight() - 5, pr.getCentreY());
                text(g, theme, "Live output • use Open scope for spectrum",
                     pr.reduced(9).removeFromBottom(23), 10, theme.muted, false,
                     juce::Justification::centred);
            }
        }
        else if (kind == "output")
            drawMeters(
                g, content.removeFromRight(std::min(58.f, content.getWidth() * .28f)).reduced(4));
        else if (kind == "matrix")
        {
            auto h = std::min(27.f, content.getHeight() * .18f);
            text(g, theme, "SOURCE                         AMOUNT           DESTINATION",
                 content.removeFromTop(22), 10, theme.muted, true);
            for (int i = 0; i < std::min(6, static_cast<int>(content.getHeight() / h)); ++i)
            {
                auto row = content.removeFromTop(h);
                g.setColour(theme.border.withAlpha(.35f));
                g.drawHorizontalLine(static_cast<int>(row.getBottom()), row.getX(), row.getRight());
                text(g, theme, juce::String(i + 1), row.removeFromLeft(20), 10, theme.muted);
                if (i < static_cast<int>(routes.size()))
                {
                    const auto &route = routes[i];
                    const float width = row.getWidth();
                    text(g, theme, route.source, row.removeFromLeft(width * .29f), 11,
                         route.muted ? theme.muted : theme.text);
                    auto amount = row.removeFromLeft(width * .25f);
                    auto bar = amount.reduced(3, 9);
                    g.setColour(theme.border);
                    g.fillRoundedRectangle(bar, 2);
                    bar.setWidth(bar.getWidth() * std::min(1.f, std::abs(route.normalizedDepth)));
                    g.setColour(route.muted ? theme.muted : theme.accent);
                    g.fillRoundedRectangle(bar, 2);
                    text(g, theme, route.amount, amount, 10, theme.text, false,
                         juce::Justification::centred);
                    text(g, theme, route.destination, row.reduced(5, 0), 11, theme.text);
                }
                else
                    text(g, theme, i == 0 ? "No routes in this patch — use Edit routes" : "—", row,
                         11, theme.muted);
            }
        }
        else if (kind == "browser")
        {
            text(g, theme, "PRESETS   /   CATEGORIES", content.removeFromTop(24), 12, theme.accent,
                 true);
            auto patch = processor.surge->storage.getPatch().name;
            text(g, theme, "Current patch", content.removeFromTop(22), 11, theme.muted);
            text(g, theme, juce::String(patch), content.removeFromTop(30), 16, theme.text, true);
            text(g, theme, "Installed preset library", content.removeFromTop(27), 12, theme.text);
            for (auto &entry : presetRows)
            {
                if (content.getHeight() < 61)
                    break;
                auto row = content.removeFromTop(26);
                g.setColour(theme.border);
                g.drawRoundedRectangle(row.withWidth(12).reduced(0, 8), 1, 1);
                text(g, theme, entry.name, row.withTrimmedLeft(22), 12,
                     entry.category ? theme.accent : theme.muted, entry.category);
            }
        }
        else if (kind == "routing")
            drawRouting(g, arrayRect(v["plot"], content), variant, v["flowNodes"]);
        else if (kind == "effects")
        {
            auto slots = content.removeFromTop(variant == "large"
                                                   ? std::min(104.f, content.getHeight() * .46f)
                                                   : std::min(53.f, content.getHeight() * .27f));
            const float sw = slots.getWidth() / 8;
            for (int i = 0; i < (variant == "vertical" ? 0 : 8); ++i)
            {
                Rect s(slots.getX() + i * sw + 2, slots.getY(), sw - 4, slots.getHeight());
                bezel(g, theme, s, theme.display, 4);
                text(g, theme, juce::String(i + 1), s.withHeight(15), 10, theme.muted, false,
                     juce::Justification::centred);
                text(g, theme, parameters.display("fx" + juce::String(i + 1) + ".type"),
                     s.withTrimmedTop(16), 10, theme.text, false, juce::Justification::centred);
            }
            if (content.getHeight() > 36)
                text(g, theme, "16 native FX slots available in the full editor",
                     content.removeFromBottom(18), 10, theme.muted);
        }
        else if (kind == "keyboard")
        {
            text(g, theme, "Playable MIDI keyboard  •  native scene modes available above",
                 content.withHeight(23), 12, theme.muted);
        }
        if (theme.style == "space" || theme.style == "tokyo")
        {
            for (float x : {r.getX() + 5, r.getRight() - 5})
            {
                g.setColour(theme.border);
                g.fillEllipse(x - 1.7f, r.getY() + 5, 3.4f, 3.4f);
            }
        }
    }
    void drawRouting(juce::Graphics &g, Rect r, const juce::String &variant, const juce::var &nodes)
    {
        const auto mode = processor.surge->storage.getPatch()
                              .scene[parameters.getScene()]
                              .filterblock_configuration.val.i;
        const bool parallel = mode >= fc_dual1;
        const juce::StringArray labels =
            parallel ? juce::StringArray{"OSC", "F1 / F2", mode == fc_ring ? "RING" : "MIX", "AMP"}
                     : juce::StringArray{"OSC", "F1", "F2", "AMP"};
        if (variant == "vertical")
        {
            float h = r.getHeight() / 4;
            for (int j = 0; j < 4; ++j)
            {
                Rect n(r.getX() + 2, r.getY() + j * h + 1, r.getWidth() - 4, std::max(12.f, h - 9));
                bezel(g, theme, n, theme.display, 3);
                text(g, theme, labels[j], n, 10, theme.text, false, juce::Justification::centred);
                if (j < 3)
                {
                    g.setColour(theme.accent);
                    g.drawLine(n.getCentreX(), n.getBottom(), n.getCentreX(), n.getBottom() + 8,
                               1.5f);
                }
            }
            return;
        }
        if (variant != "flow")
        {
            const bool compact = r.getHeight() < 60;
            if (!compact)
                text(g, theme, parameters.display("routing"), r.withHeight(21), 12, theme.accent,
                     true);
            auto line = compact ? r : r.withTrimmedTop(27);
            float w = line.getWidth() / 4;
            for (int i = 0; i < 4; ++i)
            {
                Rect n(line.getX() + i * w + 3, line.getY(), w - 12,
                       std::max(20.f, line.getHeight() - 3));
                bezel(g, theme, n, theme.display, 3);
                text(g, theme, labels[i], n, 11, theme.text, false, juce::Justification::centred);
                if (i < 3)
                {
                    g.setColour(theme.accent);
                    g.drawLine(n.getRight(), n.getCentreY(), n.getRight() + 10, n.getCentreY(),
                               1.5f);
                }
            }
            return;
        }
        plotBase(g, r);
        r = r.reduced(16, 26);
        auto node = [&](const char *key, juce::String title, float x, float y, float w, float h) {
            auto n =
                arrayRect(nodes[key], {r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight(),
                                       w * r.getWidth(), h * r.getHeight()});
            bezel(g, theme, n, theme.panel, 7);
            text(g, theme, title, n.reduced(8, 2).withHeight(19), 13, theme.text, true);
            return n;
        };
        auto wire = [&](juce::Point<float> a, juce::Point<float> b) {
            juce::Path p;
            p.startNewSubPath(a);
            p.cubicTo(a.x + (b.x - a.x) * .55f, a.y, b.x - (b.x - a.x) * .55f, b.y, b.x, b.y);
            g.setColour(theme.accent.withAlpha(.15f));
            g.strokePath(p, juce::PathStrokeType(6));
            g.setColour(theme.accent);
            g.strokePath(p, juce::PathStrokeType(2));
        };
        auto mix = node("mixer", "MIXER", .27f, .25f, .13f, .24f);
        for (int i = 0; i < 3; ++i)
        {
            auto key = "osc" + juce::String(i + 1);
            auto n = node(key.toRawUTF8(), "OSC " + juce::String(i + 1), .02f, .08f + i * .23f,
                          .20f, .13f);
            wire({n.getRight(), n.getCentreY()}, {mix.getX(), mix.getCentreY()});
        }
        auto f1 = node("filter1", "FILTER 1", .45f, .15f, .18f, .15f),
             f2 = node("filter2", "FILTER 2", .45f, .43f, .18f, .15f);
        auto amp = node("amp", "AMP", .70f, .29f, .10f, .17f),
             fx = node("fx", "FX CHAIN", .86f, .22f, .11f, .30f);
        wire({mix.getRight(), mix.getCentreY()}, {f1.getX(), f1.getCentreY()});
        if (parallel)
        {
            wire({mix.getRight(), mix.getCentreY()}, {f2.getX(), f2.getCentreY()});
            wire({f1.getRight(), f1.getCentreY()}, {amp.getX(), amp.getCentreY()});
        }
        else
            wire({f1.getRight(), f1.getCentreY()}, {f2.getCentreX(), f2.getY()});
        wire({f2.getRight(), f2.getCentreY()}, {amp.getX(), amp.getCentreY()});
        wire({amp.getRight(), amp.getCentreY()}, {fx.getX(), fx.getCentreY()});
        if (nodes.hasProperty("output"))
        {
            auto out = node("output", "OUT", .97f, .30f, .03f, .13f);
            wire({fx.getRight(), fx.getCentreY()}, {out.getX(), out.getCentreY()});
        }
        node("modsources", "MOD SOURCES", .05f, .80f, .27f, .15f);
        node("modbus", "MOD BUS / ROUTING", .46f, .80f, .30f, .15f);
        text(g, theme, "Routing: " + parameters.display("routing") + "  •  schematic overview",
             {r.getX(), r.getBottom() + 7, r.getWidth(), 17}, 11, theme.muted);
    }
};

Workspace::Workspace(SurgeSynthProcessor &p) : impl(std::make_unique<Impl>(*this, p))
{
    setName("Surge XT reference workspace");
    setTitle(getName());
    setWantsKeyboardFocus(true);
    setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);
    startTimerHz(20);
}
Workspace::~Workspace() { stopTimer(); }
bool Workspace::load(const juce::File &file, juce::String &error)
{
    juce::var candidate;
    if (!loadManifest(file, candidate, error))
        return false;
    if (auto *a = candidate["controls"].getArray())
        for (auto &c : *a)
            if (!impl->parameters.valid(string(c, "binding")))
            {
                error = "Unknown synth binding: " + string(c, "binding");
                return false;
            }
    impl->manifest = candidate;
    impl->theme.read(candidate);
    impl->build();
    impl->ticks = 0;
    refresh();
    return true;
}
void Workspace::refresh()
{
    impl->update();
    repaint();
}
ParameterBridge &Workspace::bridge() { return impl->parameters; }
juce::String Workspace::currentId() const { return string(impl->manifest, "id"); }
void Workspace::selectScene(int scene)
{
    scene = juce::jlimit(0, 1, scene);
    if (impl->parameters.getScene() == scene)
    {
        refresh();
        return;
    }
    impl->parameters.setScene(scene);
    impl->ticks = 0;
    refresh();
    if (onAction)
        onAction(scene == 0 ? "scene-a" : "scene-b");
}
void Workspace::timerCallback()
{
    if (isShowing())
        refresh();
}
void Workspace::visibilityChanged()
{
    if (!isShowing())
    {
        if (impl->keyboard)
            impl->keyboard->releaseHeldNotes();
        impl->parameters.endAll();
    }
}
void Workspace::resized()
{
    impl->scale = std::min(getWidth() / 1586.f, getHeight() / 992.f);
    impl->offset = {(getWidth() - 1586 * impl->scale) * .5f,
                    (getHeight() - 992 * impl->scale) * .5f};
    impl->canvas.setBounds(0, 0, 1586, 992);
    impl->canvas.setTransform(juce::AffineTransform::scale(impl->scale).translated(impl->offset));
    for (auto &p : impl->placements)
        p.first->setBounds(p.second.toNearestInt());
    const float h = number(impl->manifest, "headerHeight", 92);
    const int by = static_cast<int>(std::max(25.f, h * .30f)),
              bh = static_cast<int>(std::max(22.f, std::min(36.f, h * .44f)));
    impl->sceneA.setBounds(410, by, 44, bh);
    impl->sceneB.setBounds(458, by, 44, bh);
    impl->previous.setBounds(540, by, 32, bh);
    impl->presetButton.setBounds(577, by, 460, bh);
    impl->next.setBounds(1042, by, 32, bh);
    impl->skinButton.setBounds(1300, by, 95, bh);
    impl->classicButton.setBounds(1402, by, 155, bh);
}
void Workspace::paint(juce::Graphics &g)
{
    auto &i = *impl;
    g.fillAll(i.theme.background.darker(.25f));
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(i.scale).translated(i.offset));
    g.setColour(i.theme.background);
    g.fillRect(0, 0, 1586, 992);
    if (i.theme.style == "space" || i.theme.style == "ivory")
    {
        g.setColour(i.theme.text.withAlpha(.018f));
        for (int y = 0; y < 992; y += 3)
            g.drawHorizontalLine(y, 0, 1586);
    }
    i.drawHeader(g);
    if (auto *a = i.manifest["panels"].getArray())
        for (auto &v : *a)
            i.drawPanel(g, v);
}
bool Workspace::keyPressed(const juce::KeyPress &key)
{
    if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'k')
    {
        if (onChooseSkin)
            onChooseSkin();
        return true;
    }
    if (key == juce::KeyPress::escapeKey)
    {
        if (onFullEditor)
            onFullEditor();
        return true;
    }
    return false;
}
} // namespace Surge::GUI::Reference
