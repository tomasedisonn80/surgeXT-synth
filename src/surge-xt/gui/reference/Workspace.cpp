/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "Workspace.h"
#include "Manifest.h"
#include "ParameterBridge.h"
#include "PreviewCurves.h"
#include "Visuals.h"
#include "WorkspaceData.h"
#include "ModulationInteraction.h"
#include "SurgeSynthProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
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
juce::String utf8(const char *literal) { return juce::String::fromUTF8(literal); }
juce::String compactPaintValue(juce::String value)
{
    // Abbreviate the painted caption only. Accessible values, tooltips and typed
    // entry keep Surge's complete native display string and parameter parser.
    return value.replace(" semitones", " st")
        .replace(" semitone", " st")
        .replace(" cents", " ct")
        .replace(" milliseconds", " ms")
        .replace(" seconds", " s")
        .replace(" (Center)", " C")
        .replace(" (Left)", " L")
        .replace(" (Right)", " R");
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
    static juce::Colour hexColor(const juce::String &value)
    {
        const auto hex = value.substring(1);
        // Manifests use CSS RRGGBBAA; JUCE expects AARRGGBB.
        return juce::Colour::fromString(hex.length() == 8 ? hex.substring(6) + hex.substring(0, 6)
                                                          : "ff" + hex);
    }
    juce::Colour color(const juce::String &name) const
    {
        if (name == "background")
            return background;
        if (name == "panel")
            return panel;
        if (name == "border")
            return border;
        if (name == "text")
            return text;
        if (name == "muted")
            return muted;
        if (name == "display")
            return display;
        if (name == "knob")
            return knob;
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
            return hexColor(name);
        return panel;
    }
    void read(const juce::var &v)
    {
        auto p = v["palette"];
        auto readColor = [&](const char *k, juce::Colour &c) {
            auto s = string(p, k);
            if (s.startsWithChar('#'))
                c = hexColor(s);
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
#if JUCE_MAJOR_VERSION >= 8
        return juce::Font(
            juce::FontOptions(family, size, bold ? juce::Font::bold : juce::Font::plain));
#else
        return juce::Font(family, size, bold ? juce::Font::bold : juce::Font::plain);
#endif
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
    const bool panel = r.getWidth() > 125 && r.getHeight() > 55;
    const bool metal = t.style == "space";
    const bool ivory = t.style == "ivory";
    const bool flat = t.style == "bauhaus" || t.style == "clear";
    if (panel && !flat)
    {
        g.setColour(juce::Colours::black.withAlpha(t.light() ? .08f : .22f));
        g.fillRoundedRectangle(r.expanded(1).translated(0, 3), radius + 1);
    }
    g.setColour(juce::Colours::black.withAlpha(t.light() ? .14f : .40f));
    g.fillRoundedRectangle(r.translated(0, 1.6f), radius);
    // Interpolate in colour space: Colour::brighter(.07) lifts near-black metal
    // by almost twenty code values, washing out the reference's dark surfaces.
    auto top = fill.interpolatedWith(juce::Colours::white, .018f);
    auto bottom = fill.interpolatedWith(juce::Colours::black, .18f);
    if (t.style == "precision" || t.style == "live")
    {
        top = fill.interpolatedWith(juce::Colours::white, .012f);
        bottom = fill.interpolatedWith(juce::Colours::black, .28f);
    }
    if (ivory && fill != t.display)
    {
        top = fill.interpolatedWith(juce::Colour(0xfffffcf5), .34f);
        bottom = fill.interpolatedWith(juce::Colour(0xffd7bfa8), .13f);
    }
    else if (t.style == "bauhaus")
    {
        top = fill.interpolatedWith(juce::Colours::white, .045f);
        bottom = fill.interpolatedWith(juce::Colours::black, .018f);
    }
    else if (fill == t.panel && t.style == "clear")
    {
        top = juce::Colour(0xfffafafa);
        bottom = juce::Colour(0xfff5f6f7);
    }
    else if (fill == t.panel && t.style == "aurora")
    {
        top = juce::Colour(0xff48325f);
        bottom = juce::Colour(0xff1f1b34);
    }
    else if (metal && fill != t.display)
    {
        top = fill.interpolatedWith(juce::Colour(0xffffedc9), .22f);
        bottom = fill.interpolatedWith(juce::Colour(0xff746346), .16f);
    }
    auto material = juce::ColourGradient(top, r.getTopLeft(), bottom, r.getBottomLeft(), false);
    if (panel && (metal || ivory || t.style == "tokyo"))
    {
        material.addColour(.045, top.interpolatedWith(juce::Colours::white, metal ? .14f : .04f));
        material.addColour(.14, top);
        material.addColour(.86, bottom);
        material.addColour(.98, bottom.interpolatedWith(juce::Colours::black, .06f));
    }
    g.setGradientFill(material);
    g.fillRoundedRectangle(r, radius);
    if (metal && panel)
    {
        juce::Graphics::ScopedSaveState save(g);
        juce::Path clip;
        clip.addRoundedRectangle(r.reduced(2), std::max(0.f, radius - 1));
        g.reduceClipRegion(clip);
        // Deterministic cosmetic brushing. It never represents signal or state.
        for (int y = static_cast<int>(r.getY()) + 2; y < static_cast<int>(r.getBottom()) - 2; ++y)
        {
            const auto grain = static_cast<unsigned>(y * 2654435761u);
            g.setColour((grain & 1 ? juce::Colours::white : juce::Colours::black)
                            .withAlpha(.016f + (grain % 7) * .003f));
            g.drawHorizontalLine(y, r.getX() + 2, r.getRight() - 2);
        }
    }
    g.setColour(t.border.withAlpha(flat ? .45f : .80f));
    g.drawRoundedRectangle(r.reduced(0.5f), radius, 1);
    if (panel && (metal || ivory || t.style == "tokyo"))
    {
        g.setColour(juce::Colours::white.withAlpha(metal ? .62f : ivory ? .72f : .15f));
        g.drawRoundedRectangle(r.reduced(2), std::max(0.f, radius - 1), .8f);
        g.setColour(juce::Colours::black.withAlpha(metal ? .25f : .10f));
        g.drawRoundedRectangle(r.reduced(3.3f), std::max(0.f, radius - 2), .7f);
    }
    g.setColour(juce::Colours::white.withAlpha(t.light() ? .62f : .10f));
    g.drawLine(r.getX() + radius, r.getY() + 1, r.getRight() - radius, r.getY() + 1, 1);
}
void themedButton(juce::Graphics &g, const Theme &theme, juce::TextButton &button, bool highlighted,
                  bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced(.7f);
    const bool selected = button.getToggleState();
    auto fill = selected ? theme.accent : button.findColour(juce::TextButton::buttonColourId);
    if (highlighted)
        fill = fill.brighter(.10f);
    if (down)
        fill = fill.darker(.12f);
    const auto radius = theme.style == "bauhaus" || theme.style == "tokyo" ? 2.f
                        : theme.style == "clear" ? 5.f
                                                 : std::min(5.f, theme.radius);
    bezel(g, theme, r, fill, radius);
    auto ink =
        selected ? fill.contrasting(.95f) : button.findColour(juce::TextButton::textColourOffId);
    if (!button.isEnabled())
        ink = ink.withAlpha(.45f);
    const auto size =
        std::min(theme.style == "clear" ? 14.f : 12.f, std::max(9.f, r.getHeight() * .48f));
    text(g, theme, button.getButtonText(), r.reduced(4, 0), size, ink,
         selected || theme.style == "bauhaus", juce::Justification::centred);
    if (button.hasKeyboardFocus(true))
    {
        g.setColour(theme.accent);
        g.drawRoundedRectangle(r.reduced(1), radius, 2);
    }
}

// Painting is local to each component, so asynchronous native popups never retain
// a pointer to an editor-owned LookAndFeel after its editor has closed.
class ThemeButton : public juce::TextButton
{
  public:
    ThemeButton(Theme &t, const juce::String &caption = {}) : juce::TextButton(caption), theme(t) {}
    void paintButton(juce::Graphics &g, bool over, bool down) override
    {
        themedButton(g, theme, *this, over, down);
    }

  private:
    Theme &theme;
};
/** Header controls retain native button semantics while each skin supplies its
    own measured geometry. The large preset caption is an actual browser button. */
class HeaderButton : public juce::TextButton
{
  public:
    enum Presentation
    {
        chip,
        navigation,
        caption,
        chevron,
        tool
    };
    HeaderButton(Theme &t, const juce::String &label = {}) : juce::TextButton(label), theme(t) {}
    void present(Presentation p, float size = 12.f, bool bold = false,
                 int align = juce::Justification::centred)
    {
        presentation = p;
        fontSize = size;
        heavy = bold;
        justification = align;
    }
    void paintButton(juce::Graphics &g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(.7f);
        auto ink = findColour(juce::TextButton::textColourOffId);
        const bool selected = getToggleState();
        if (presentation == chip || presentation == tool ||
            (presentation == navigation && (selected || over || down)))
        {
            auto fill = selected ? theme.accent : findColour(juce::TextButton::buttonColourId);
            if (theme.style == "precision" && presentation == navigation && selected)
                fill = theme.accent.interpolatedWith(theme.panel, .88f);
            if (over)
                fill = fill.interpolatedWith(juce::Colours::white, .07f);
            if (down)
                fill = fill.interpolatedWith(juce::Colours::black, .13f);
            bezel(g, theme, r, fill,
                  theme.style == "tokyo" || theme.style == "bauhaus" ? 3.f : 5.f);
            if (selected)
            {
                ink = theme.style == "precision" && presentation == navigation
                          ? theme.accent
                          : fill.contrasting(.95f);
                g.setColour(theme.accent.withAlpha(.60f));
                g.drawRoundedRectangle(r.reduced(.4f), 4.f, .8f);
            }
        }
        else if (over || down)
        {
            g.setColour(theme.accent.withAlpha(down ? .17f : .08f));
            g.fillRoundedRectangle(r, 3.f);
        }
        if (presentation == chevron)
        {
            const auto c = r.getCentre();
            const auto direction = getComponentID().contains("previous") ? -1.f : 1.f;
            juce::Path p;
            p.startNewSubPath(c.x - direction * 3.f, c.y - 6.f);
            p.lineTo(c.x + direction * 3.f, c.y);
            p.lineTo(c.x - direction * 3.f, c.y + 6.f);
            g.setColour(ink);
            g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        }
        else
        {
            g.setFont(theme.type(fontSize, heavy || selected));
            g.setColour(ink);
            g.drawFittedText(getButtonText(),
                             r.reduced(presentation == caption ? 2.f : 4.f, 0).toNearestInt(),
                             justification, 1, .75f);
        }
        if (hasKeyboardFocus(true))
        {
            g.setColour(theme.accent);
            g.drawRoundedRectangle(r.reduced(.8f), 3.f, 1.8f);
        }
    }

  private:
    Theme &theme;
    Presentation presentation{chip};
    float fontSize{12.f};
    bool heavy{false};
    int justification{juce::Justification::centred};
};

void knob(juce::Graphics &g, const Theme &t, Rect r, float value, juce::Colour accent, bool focus,
          bool enabled)
{
    if (r.getWidth() <= 0 || r.getHeight() <= 0)
        return;
    const auto c = r.getCentre();
    const float rad = std::min(r.getWidth(), r.getHeight()) * 0.46f;
    const float start = -2.35f, end = 2.35f;
    const float a = start + (end - start) * juce::jlimit(0.f, 1.f, value);
    juce::Path arc, track;
    track.addCentredArc(c.x, c.y, rad, rad, 0, start, end, true);
    arc.addCentredArc(c.x, c.y, rad, rad, 0, start, a, true);
    if (!enabled)
        accent = t.muted.withAlpha(0.4f);
    const bool ivory = t.style == "ivory", space = t.style == "space";
    const bool bauhaus = t.style == "bauhaus", clear = t.style == "clear";
    const bool glass = t.style == "spectral" || t.style == "aurora" || t.style == "live";
    if (t.glow > 0 && enabled && !ivory && !space && !bauhaus)
    {
        g.setColour(accent.withAlpha(std::min(.10f, t.glow * .4f)));
        g.strokePath(arc, juce::PathStrokeType(glass ? 11.f : 7.f));
        g.setColour(accent.withAlpha(std::min(.26f, t.glow)));
        g.strokePath(arc, juce::PathStrokeType(glass ? 6.f : 4.5f));
    }
    if (space)
    {
        for (int tick = 0; tick <= 20; ++tick)
        {
            const auto angle = start + (end - start) * tick / 20.f;
            const auto inner = rad + 1.f, outer = rad + (tick % 5 == 0 ? 4.f : 2.5f);
            g.setColour(t.text.withAlpha(.78f));
            g.drawLine(c.x + std::sin(angle) * inner, c.y - std::cos(angle) * inner,
                       c.x + std::sin(angle) * outer, c.y - std::cos(angle) * outer, .8f);
        }
    }
    else if (!ivory && !bauhaus)
    {
        g.setColour(t.border.withAlpha(clear ? .65f : .32f));
        g.strokePath(track, juce::PathStrokeType(clear ? 3.f : 1.5f));
        g.setColour(accent);
        g.strokePath(arc, juce::PathStrokeType(clear ? 4.2f : 2.2f));
        if (glass)
        {
            g.setColour(accent.interpolatedWith(juce::Colours::white, .6f));
            g.strokePath(arc, juce::PathStrokeType(.8f));
        }
    }
    const auto faceRadius = std::max(1.f, rad - (clear ? 3.4f : glass ? 4.2f : 2.f));
    auto face = Rect(c.x - faceRadius, c.y - faceRadius, faceRadius * 2, faceRadius * 2);
    if (!clear)
    {
        g.setColour(juce::Colours::black.withAlpha(.16f));
        g.fillEllipse(face.expanded(2).translated(0, 3));
        g.setColour(juce::Colours::black.withAlpha(.60f));
        g.fillEllipse(face.expanded(.8f).translated(.5f, 1.5f));
    }
    auto hi = t.knob.interpolatedWith(juce::Colours::white, glass ? .18f : .10f);
    auto lo = t.knob.interpolatedWith(juce::Colours::black, .45f);
    if (ivory || (space && t.knob.getPerceivedBrightness() > .35f))
    {
        hi = ivory ? juce::Colour(0xfffff8e7) : juce::Colour(0xffffe4aa);
        lo = ivory ? juce::Colour(0xffd7c4a4) : juce::Colour(0xff977f51);
    }
    else if (bauhaus || clear)
    {
        hi = bauhaus ? juce::Colour(0xfffff4d9) : juce::Colour(0xfff9fbfc);
        lo = bauhaus ? juce::Colour(0xffebd6ae) : juce::Colour(0xffe7ebed);
    }
    auto faceGradient = juce::ColourGradient(hi, face.getX() + face.getWidth() * .27f,
                                             face.getY() + face.getHeight() * .12f, lo,
                                             face.getRight(), face.getBottom(), false);
    faceGradient.addColour(.48, hi.interpolatedWith(lo, .34f));
    g.setGradientFill(faceGradient);
    g.fillEllipse(face);
    g.setColour(bauhaus          ? juce::Colour(0xff151613)
                : ivory || space ? juce::Colour(0xff958058)
                                 : t.border.withAlpha(clear ? .18f : .48f));
    g.drawEllipse(face, bauhaus ? 3.1f : ivory || space ? 1.8f : .9f);
    if (!clear && !bauhaus)
    {
        juce::Path shine;
        shine.addCentredArc(c.x, c.y, faceRadius - 1.5f, faceRadius - 1.5f, 0, -1.45f, .65f, true);
        g.setColour(juce::Colours::white.withAlpha(ivory || space ? .70f : .35f));
        g.strokePath(shine, juce::PathStrokeType(.8f));
    }
    const float inner = rad * (clear ? .54f : .48f), outer = rad * .84f;
    g.setColour(ivory                ? juce::Colour(0xff264c33)
                : bauhaus || clear   ? juce::Colour(0xff10181c)
                : space              ? juce::Colour(0xff352f20)
                : t.style == "tokyo" ? accent
                                     : t.text);
    g.drawLine(c.x + std::sin(a) * inner, c.y - std::cos(a) * inner, c.x + std::sin(a) * outer,
               c.y - std::cos(a) * outer,
               clear   ? 3.f
               : space ? 1.4f
                       : 1.8f);
    if (focus)
    {
        g.setColour(t.accent);
        g.drawRoundedRectangle(r.reduced(1), 3, 2);
    }
}

/** A non-blocking editor. Values go through Surge's parser, never a guessed unit conversion. */
class ValueEntry : public juce::Component
{
  public:
    ValueEntry(const Theme &t, const juce::String &name, const juce::String &value,
               std::function<bool(const juce::String &, juce::String &)> applyValue)
        : theme(t), applyValue(std::move(applyValue))
    {
        setName("Edit " + name);
        setTitle(getName());
        setFocusContainerType(FocusContainerType::keyboardFocusContainer);
        title.setText(name, juce::dontSendNotification);
        title.setFont(theme.type(15, true));
        title.setColour(juce::Label::textColourId, theme.text);
        addAndMakeVisible(title);
        input.setComponentID("workspace.value-entry.input");
        input.setName(name + " value");
        input.setTitle(input.getName());
        input.setDescription(
            "Enter a value in this parameter's displayed units. Enter applies; Escape cancels.");
        input.setText(value, false);
        input.setColour(juce::TextEditor::backgroundColourId, theme.panel);
        input.setColour(juce::TextEditor::textColourId, theme.text);
        input.setColour(juce::TextEditor::outlineColourId, theme.border);
        input.setColour(juce::TextEditor::focusedOutlineColourId, theme.accent);
        input.onReturnKey = [this] { apply(); };
        input.onEscapeKey = [this] { close(); };
        addAndMakeVisible(input);
        error.setColour(juce::Label::textColourId,
                        theme.light() ? theme.red.darker(.3f) : juce::Colour(0xffffa297));
        error.setFont(theme.type(12));
        error.setJustificationType(juce::Justification::topLeft);
        error.setName("Value validation");
        addAndMakeVisible(error);
        applyButton.setComponentID("workspace.value-entry.apply");
        cancelButton.setComponentID("workspace.value-entry.cancel");
        for (auto *button : {&applyButton, &cancelButton})
        {
            button->setColour(juce::TextButton::buttonColourId, theme.panel);
            button->setColour(juce::TextButton::textColourOffId, theme.text);
            button->setName(button->getButtonText());
            button->setTitle(button->getName());
            addAndMakeVisible(*button);
        }
        applyButton.onClick = [this] { apply(); };
        cancelButton.onClick = [this] { close(); };
        setSize(310, 166);
        juce::Component::SafePointer<ValueEntry> safe(this);
        juce::MessageManager::callAsync([safe] {
            if (safe)
            {
                safe->input.grabKeyboardFocus();
                safe->input.selectAll();
            }
        });
    }
    void paint(juce::Graphics &g) override { g.fillAll(theme.background); }
    void resized() override
    {
        auto r = getLocalBounds().reduced(10);
        title.setBounds(r.removeFromTop(25));
        input.setBounds(r.removeFromTop(29));
        r.removeFromTop(4);
        auto buttons = r.removeFromBottom(28);
        cancelButton.setBounds(buttons.removeFromRight(85));
        buttons.removeFromRight(6);
        applyButton.setBounds(buttons.removeFromRight(85));
        error.setBounds(r);
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key.getKeyCode() != juce::KeyPress::escapeKey)
            return false;
        close();
        return true;
    }

  private:
    void close()
    {
        if (auto *callout = findParentComponentOfClass<juce::CallOutBox>())
            callout->dismiss();
    }
    void apply()
    {
        juce::String message;
        if (applyValue && applyValue(input.getText(), message))
            close();
        else
        {
            if (message.isEmpty())
                message = "This value is not valid for the current parameter.";
            error.setText(message, juce::dontSendNotification);
            input.setDescription(message);
            input.grabKeyboardFocus();
            input.selectAll();
        }
    }
    const Theme theme;
    std::function<bool(const juce::String &, juce::String &)> applyValue;
    juce::Label title, error;
    juce::TextEditor input;
    juce::TextButton applyButton{"Apply"}, cancelButton{"Cancel"};
};

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
        onDragEnd = [this] { cancelGesture(); };
        sync();
    }
    ~BoundSlider() override
    {
        cancelGesture();
        dismissEntry();
    }
    void cancelGesture()
    {
        if (dragging)
        {
            bridge.end(binding);
            dragging = false;
        }
    }
    void dismissEntry()
    {
        if (entryBox)
            entryBox->dismiss();
        entryBox = nullptr;
    }
    void sync()
    {
        polling = true;
        setEnabled(bridge.valid(binding) && bridge.enabled(binding));
        if (entryBox && (!isEnabled() || entryLabel != bridge.label(binding)))
            dismissEntry();
        if (!dragging)
            setValue(bridge.value(binding), juce::dontSendNotification);
        auto name = string(spec, "label", bridge.label(binding));
        setName(name);
        setTitle(name);
        setDescription(name + ": " + bridge.display(binding));
        setTooltip(bridge.label(binding) + ": " + bridge.display(binding) +
                   "\nDrag to change. Shift for fine adjustment. Double-click to reset."
                   "\nReturn, right-click, or Shift+double-click to type a value.");
        setHelpText(getTooltip());
        polling = false;
    }
    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.mods.isPopupMenu())
            showEntry();
        else
            juce::Slider::mouseDown(event);
    }
    void mouseDrag(const juce::MouseEvent &event) override
    {
        if (dragging)
            juce::Slider::mouseDrag(event);
    }
    void mouseDoubleClick(const juce::MouseEvent &event) override
    {
        cancelGesture();
        if (event.mods.isShiftDown() || event.mods.isPopupMenu())
        {
            showEntry();
            return;
        }
        bridge.reset(binding);
        sync();
        repaint();
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key.getKeyCode() == juce::KeyPress::returnKey)
        {
            showEntry();
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::escapeKey && dragging)
        {
            cancelGesture();
            sync();
            repaint();
            return true;
        }
        return juce::Slider::keyPressed(key);
    }
    void focusLost(FocusChangeType cause) override
    {
        cancelGesture();
        juce::Slider::focusLost(cause);
    }
    void visibilityChanged() override
    {
        if (!isShowing())
        {
            cancelGesture();
            dismissEntry();
        }
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat();
        auto accent =
            spec.hasProperty("accent") ? theme.color(string(spec, "accent")) : theme.accent;
        auto label = string(spec, "label", bridge.label(binding));
        auto value = isEnabled() ? compactPaintValue(bridge.display(binding)) : utf8("—");
        auto ink = spec.hasProperty("ink") ? theme.color(string(spec, "ink")) : theme.text;
        auto valueInk = spec.hasProperty("ink") ? ink.withAlpha(.85f) : theme.muted;
        float fs = number(spec, "fontSize", theme.style == "clear" ? 15 : 11.5f);
        if (kind == "value")
        {
            const float labelWidth =
                label.length() <= 3 ? std::min(24.f, r.getWidth() * .25f) : r.getWidth() * .44f;
            text(g, theme, label, r.removeFromLeft(labelWidth), fs, ink);
            auto box = r.reduced(2, 1);
            bezel(g, theme, box, theme.style == "clear" ? theme.panel : theme.display, 3);
            auto fieldInk = spec.hasProperty("ink")  ? ink
                            : theme.style == "clear" ? theme.text
                                                     : theme.display.contrasting(.85f);
            text(g, theme, value, box.reduced(4, 0), fs, fieldInk, theme.style == "clear",
                 juce::Justification::centredRight);
            if (hasKeyboardFocus(true))
            {
                g.setColour(accent);
                g.drawRoundedRectangle(box, 3, 2);
            }
            return;
        }
        if (kind == "knob")
        {
            const auto labelPosition = string(spec, "labelPosition");
            if (labelPosition == "top")
            {
                const float line = std::min(19.f, r.getHeight() * .23f);
                text(g, theme, label, r.removeFromTop(line), fs, ink, false,
                     juce::Justification::centred);
                text(g, theme, value, r.removeFromBottom(line), fs,
                     theme.style == "clear" ? ink : valueInk, theme.style == "clear",
                     juce::Justification::centred);
                auto faceTheme = theme;
                if (theme.style == "space" && binding.startsWith("macro"))
                    faceTheme.knob = theme.display;
                knob(g, faceTheme, r, static_cast<float>(getValue()), accent,
                     hasKeyboardFocus(true), isEnabled());
                return;
            }
            if (labelPosition == "left" || labelPosition == "right")
            {
                const float side = std::min(r.getHeight(), r.getWidth() * .48f);
                auto face =
                    labelPosition == "left" ? r.removeFromRight(side) : r.removeFromLeft(side);
                knob(g, theme, face, static_cast<float>(getValue()), accent, hasKeyboardFocus(true),
                     isEnabled());
                auto labels = r.reduced(4, std::max(0.f, (r.getHeight() - 32) * .5f));
                if (theme.style == "bauhaus" && labelPosition == "left")
                {
                    // Bauhaus scene rows put the label, current value and real
                    // rotary on one baseline rather than stacking two captions.
                    auto nameBounds = labels.removeFromLeft(labels.getWidth() * .53f);
                    text(g, theme, label, nameBounds, fs, ink);
                    text(g, theme, value, labels, fs, valueInk, false,
                         juce::Justification::centredRight);
                    return;
                }
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
            auto faceTheme = theme;
            if (theme.style == "space" && binding.startsWith("macro"))
                faceTheme.knob = theme.display;
            knob(g, faceTheme, face, static_cast<float>(getValue()), accent, hasKeyboardFocus(true),
                 isEnabled());
            text(g, theme, label, r.removeFromBottom(lower).withHeight(lower / 2), fs, ink, false,
                 juce::Justification::centred);
            text(g, theme, value, getLocalBounds().toFloat().removeFromBottom(lower / 2), fs,
                 valueInk, false, juce::Justification::centred);
        }
        else
        {
            const auto labelHeight = std::min(17.f, r.getHeight() * .25f);
            const auto valueHeight = std::min(18.f, r.getHeight() * .25f);
            text(g, theme, label, r.removeFromTop(labelHeight), fs, ink, false,
                 juce::Justification::centred);
            text(g, theme, value, r.removeFromBottom(valueHeight), fs, valueInk, false,
                 juce::Justification::centred);
            auto track =
                r.reduced(kind == "fader" ? r.getWidth() * .4f : std::min(7.f, r.getWidth() * .2f),
                          std::min(5.f, r.getHeight() * .2f));
            if (track.isEmpty())
                return;
            if (kind == "horizontal")
                track =
                    track.withSizeKeepingCentre(track.getWidth(), std::min(5.f, track.getHeight()));
            bezel(g, theme, track.expanded(2), theme.display, 3);
            auto active = track;
            if (kind == "fader")
                active = track.withTop(track.getBottom() - track.getHeight() * getValue());
            else
                active.setWidth(track.getWidth() * getValue());
            const bool hardware = theme.style == "space" || theme.style == "tokyo";
            if (kind == "fader" && hardware)
            {
                for (int tick = 0; tick <= 10; ++tick)
                {
                    const float y = track.getY() + track.getHeight() * tick / 10.f;
                    g.setColour(theme.text.withAlpha(.32f));
                    g.drawLine(track.getX() - 7, y, track.getX() - 3, y, .7f);
                    g.drawLine(track.getRight() + 3, y, track.getRight() + 7, y, .7f);
                }
            }
            if (!hardware || kind == "horizontal")
            {
                g.setGradientFill(juce::ColourGradient(
                    accent.interpolatedWith(juce::Colours::white, .3f), active.getTopLeft(), accent,
                    active.getBottomLeft(), false));
                g.fillRoundedRectangle(active, 2);
            }
            Rect handle;
            if (kind == "fader")
            {
                const auto width = std::min(24.f, static_cast<float>(getWidth()) - 2.f);
                const auto height = std::min(8.f, r.getHeight());
                handle = {track.getCentreX() - width * .5f, active.getY() - height * .5f, width,
                          height};
            }
            else
                handle = {track.getX() + track.getWidth() * static_cast<float>(getValue()) - 5,
                          track.getCentreY() - 6, 10, 12};
            g.setColour(juce::Colours::black.withAlpha(.45f));
            g.fillRoundedRectangle(handle.expanded(.8f).translated(0, 1.5f), 2);
            const auto cap = theme.style == "space"   ? juce::Colour(0xffdbc8a1)
                             : theme.style == "tokyo" ? theme.accent
                             : theme.style == "aurora" || theme.style == "spectral" ? theme.accent
                             : theme.style == "live" ? juce::Colour(0xffd9ba94)
                             : theme.light()         ? theme.knob
                                                     : juce::Colour(0xffc0b4a0);
            auto capGradient = juce::ColourGradient(cap.interpolatedWith(juce::Colours::white, .6f),
                                                    handle.getTopLeft(), cap.darker(.35f),
                                                    handle.getBottomLeft(), false);
            capGradient.addColour(.48, cap);
            capGradient.addColour(.52, cap.darker(.15f));
            g.setGradientFill(capGradient);
            g.fillRoundedRectangle(handle, 2);
            g.setColour(theme.border);
            g.drawRoundedRectangle(handle, 2, 1);
            if (kind == "fader")
            {
                g.setColour(cap.interpolatedWith(juce::Colours::white, .7f));
                g.drawLine(handle.getX() + 2, handle.getCentreY(), handle.getRight() - 2,
                           handle.getCentreY(), 1);
            }
            if (hasKeyboardFocus(true))
            {
                g.setColour(theme.accent);
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 3, 2);
            }
        }
    }
    juce::String getTextFromValue(double) override { return bridge.display(binding); }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        // JUCE's default slider string setter parses displayed Hz/dB values as
        // normalized numbers. Route assistive-technology edits through the same
        // real Surge parser as the typed-value callout instead.
        class ValueInterface final : public juce::AccessibilityValueInterface
        {
          public:
            explicit ValueInterface(BoundSlider &s) : slider(s) {}
            bool isReadOnly() const override { return !slider.isEnabled(); }
            double getCurrentValue() const override { return slider.bridge.value(slider.binding); }
            juce::String getCurrentValueAsString() const override
            {
                return slider.bridge.display(slider.binding);
            }
            void setValue(double value) override
            {
                if (isReadOnly() || !std::isfinite(value))
                    return;
                slider.cancelGesture();
                slider.bridge.set(slider.binding, static_cast<float>(value));
                slider.sync();
                slider.repaint();
            }
            void setValueAsString(const juce::String &value) override
            {
                if (isReadOnly())
                    return;
                slider.cancelGesture();
                juce::String error;
                slider.bridge.setText(slider.binding, value, error);
                slider.sync();
                slider.repaint();
            }
            AccessibleValueRange getRange() const override { return {{0, 1}, .001}; }

          private:
            BoundSlider &slider;
        };
        return std::make_unique<juce::AccessibilityHandler>(
            *this, juce::AccessibilityRole::slider,
            juce::AccessibilityActions{}
                .addAction(juce::AccessibilityActionType::focus, [this] { grabKeyboardFocus(); })
                .addAction(juce::AccessibilityActionType::showMenu, [this] { showEntry(); }),
            juce::AccessibilityHandler::Interfaces{std::make_unique<ValueInterface>(*this)});
    }
    juce::String binding;
    juce::var spec;

  private:
    void showEntry()
    {
        if (!isEnabled())
            return;
        cancelGesture();
        dismissEntry();
        const auto scene = bridge.getScene();
        entryLabel = bridge.label(binding);
        juce::Component::SafePointer<BoundSlider> safe(this);
        auto entry = std::make_unique<ValueEntry>(
            theme, bridge.label(binding), bridge.display(binding),
            [safe, scene, nameAtOpen = entryLabel](const juce::String &value, juce::String &error) {
                if (!safe || safe->bridge.getScene() != scene || !safe->isEnabled() ||
                    safe->bridge.label(safe->binding) != nameAtOpen)
                {
                    error = "The edited parameter is no longer available.";
                    return false;
                }
                const auto applied = safe->bridge.setText(safe->binding, value, error);
                safe->sync();
                safe->repaint();
                return applied;
            });
        auto *parent = findParentComponentOfClass<Workspace>();
        const auto target =
            parent ? parent->getLocalArea(this, getLocalBounds()) : getScreenBounds();
        auto &callout = juce::CallOutBox::launchAsynchronously(std::move(entry), target, parent);
        callout.setDismissalMouseClicksAreAlwaysConsumed(true);
        entryBox = &callout;
    }
    ParameterBridge &bridge;
    Theme &theme;
    juce::String kind;
    bool dragging{false}, polling{false};
    juce::Component::SafePointer<juce::CallOutBox> entryBox;
    juce::String entryLabel;
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
        setEnabled(bridge.valid(binding) && bridge.enabled(binding) && !c.isEmpty());
        setName(string(spec, "label", bridge.label(binding)));
        setTitle(getName());
        setTooltip(getName() + ": " + bridge.display(binding));
        setColour(juce::ComboBox::backgroundColourId, theme.display);
        setColour(juce::ComboBox::textColourId, theme.text);
        // Cream themes use dark ink on cream menu surfaces, not on the green plotting well.
        if (theme.light() && theme.style != "space")
            setColour(juce::ComboBox::backgroundColourId, theme.panel.brighter(.08f));
        if (theme.style == "space")
            setColour(juce::ComboBox::textColourId, theme.display.contrasting(.9f));
        else if (spec.hasProperty("ink"))
            setColour(juce::ComboBox::textColourId, theme.color(string(spec, "ink")));
        setColour(juce::ComboBox::outlineColourId, theme.border);
        setColour(juce::ComboBox::arrowColourId, theme.accent);
        polling = false;
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat().reduced(.5f);
        bezel(g, theme, r, findColour(juce::ComboBox::backgroundColourId),
              theme.style == "bauhaus" || theme.style == "tokyo" ? 2.f : 4.f);
        auto arrow = r.removeFromRight(std::min(20.f, r.getWidth() * .2f));
        const auto c = arrow.getCentre();
        juce::Path p;
        p.startNewSubPath(c.x - 3, c.y - 1.5f);
        p.lineTo(c.x, c.y + 1.5f);
        p.lineTo(c.x + 3, c.y - 1.5f);
        g.setColour(isEnabled() ? findColour(juce::ComboBox::textColourId) : theme.muted);
        g.strokePath(p, juce::PathStrokeType(1.3f));
        if (hasKeyboardFocus(true))
        {
            g.setColour(theme.accent);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 3, 2);
        }
    }
    void resized() override
    {
        juce::ComboBox::resized();
        for (auto *child : getChildren())
            if (auto *label = dynamic_cast<juce::Label *>(child))
            {
                label->setFont(
                    theme.type(number(spec, "fontSize", theme.style == "clear" ? 13.f : 11.f)));
                label->setBounds(5, 0, std::max(1, getWidth() - 25), getHeight());
            }
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
        setEnabled(bridge.valid(binding) && bridge.enabled(binding));
        setColour(juce::TextButton::buttonColourId, theme.panel);
        setColour(juce::TextButton::buttonOnColourId, theme.accent);
        setColour(juce::TextButton::textColourOffId, theme.text);
        setColour(juce::TextButton::textColourOnId, theme.background);
    }
    void paintButton(juce::Graphics &g, bool over, bool down) override
    {
        themedButton(g, theme, *this, over, down);
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
        sync();
    }
    ~MacroXY() override { cancelGesture(); }
    void sync()
    {
        setDescription("X: " + bridge.label("macro1") + ", " + bridge.display("macro1") +
                       ". Y: " + bridge.label("macro2") + ", " + bridge.display("macro2") +
                       ". Arrow keys adjust; Shift makes fine adjustments. "
                       "The Macro 1 and Macro 2 sliders expose each axis separately.");
    }
    void cancelGesture()
    {
        if (dragging)
        {
            bridge.end("macro1");
            bridge.end("macro2");
            dragging = false;
        }
    }
    void visibilityChanged() override
    {
        if (!isShowing())
            cancelGesture();
    }
    void focusLost(FocusChangeType) override { cancelGesture(); }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<juce::AccessibilityHandler>(
            *this, juce::AccessibilityRole::group,
            juce::AccessibilityActions{}.addAction(juce::AccessibilityActionType::focus,
                                                   [this] { grabKeyboardFocus(); }));
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat().reduced(3);
        bezel(g, theme, r, theme.display, 18);
        {
            juce::Graphics::ScopedSaveState save(g);
            juce::Path clip;
            clip.addRoundedRectangle(r.reduced(2), 16);
            g.reduceClipRegion(clip);
            g.setGradientFill(juce::ColourGradient(theme.accent.withAlpha(.26f), r.getTopLeft(),
                                                   theme.secondary.withAlpha(.20f),
                                                   r.getBottomRight(), false));
            g.fillRect(r);
            // The glass is decorative; cursor and crosshairs always use live
            // macro values. No illustrated signal is presented as engine data.
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.10f),
                                                   r.getTopLeft(), juce::Colours::transparentWhite,
                                                   r.getBottomLeft(), false));
            g.fillRect(r.withHeight(r.getHeight() * .48f));
            Visuals::grid(g, r.reduced(5), theme.accent.withAlpha(.28f));
        }
        const float x = r.getX() + bridge.value("macro1") * r.getWidth();
        const float y = r.getBottom() - bridge.value("macro2") * r.getHeight();
        g.setGradientFill(juce::ColourGradient(theme.secondary.withAlpha(.30f), {x, y},
                                               theme.accent.withAlpha(.015f), r.getTopLeft(),
                                               true));
        g.fillRoundedRectangle(r, 18);
        g.setColour(theme.accent.withAlpha(.80f));
        g.drawLine(x, r.getY(), x, r.getBottom());
        g.drawLine(r.getX(), y, r.getRight(), y);
        g.setColour(theme.secondary.withAlpha(.12f));
        g.fillEllipse(x - 23, y - 23, 46, 46);
        g.setColour(theme.secondary.withAlpha(.24f));
        g.fillEllipse(x - 16, y - 16, 32, 32);
        g.setColour(theme.secondary);
        g.fillEllipse(x - 6, y - 6, 12, 12);
        g.setColour(juce::Colours::white);
        g.drawEllipse(x - 6, y - 6, 12, 12, 2);
        text(g, theme, utf8("X: Macro 1  •  Y: Macro 2"), r.removeFromBottom(20), 10, theme.text,
             false, juce::Justification::centred);
        if (hasKeyboardFocus(true))
        {
            g.setColour(theme.accent);
            g.drawRoundedRectangle(r, 18, 2);
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        cancelGesture();
        grabKeyboardFocus();
        bridge.begin("macro1");
        bridge.begin("macro2");
        dragging = true;
        update(e.position);
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (dragging)
            update(e.position);
    }
    void mouseUp(const juce::MouseEvent &) override { cancelGesture(); }
    bool keyPressed(const juce::KeyPress &k) override
    {
        if (k.getKeyCode() == juce::KeyPress::escapeKey && dragging)
        {
            cancelGesture();
            return true;
        }
        const auto step = k.getModifiers().isShiftDown() ? .001f : .01f;
        juce::String b;
        float delta = 0;
        if (k.getKeyCode() == juce::KeyPress::leftKey)
        {
            b = "macro1";
            delta = -step;
        }
        if (k.getKeyCode() == juce::KeyPress::rightKey)
        {
            b = "macro1";
            delta = step;
        }
        if (k.getKeyCode() == juce::KeyPress::downKey)
        {
            b = "macro2";
            delta = -step;
        }
        if (k.getKeyCode() == juce::KeyPress::upKey)
        {
            b = "macro2";
            delta = step;
        }
        if (b.isEmpty())
            return false;
        bridge.begin(b);
        bridge.set(b, bridge.value(b) + delta);
        bridge.end(b);
        sync();
        repaint();
        return true;
    }

  private:
    void update(juce::Point<float> p)
    {
        const auto r = getLocalBounds().toFloat().reduced(3);
        bridge.set("macro1", (p.x - r.getX()) / std::max(1.f, r.getWidth()));
        bridge.set("macro2", (r.getBottom() - p.y) / std::max(1.f, r.getHeight()));
        sync();
        repaint();
    }
    ParameterBridge &bridge;
    Theme &theme;
    bool dragging{false};
};

class PresetBrowser : public juce::Component, private juce::ListBoxModel
{
  public:
    PresetBrowser(SurgeSynthProcessor &p, Theme &t, const juce::String &query, int selectedPatch,
                  std::function<void(const juce::String &)> performAction)
        : processor(p), theme(t), performAction(std::move(performAction)),
          list("Installed presets", this)
    {
        setName("Installed preset browser");
        setTitle(getName());
        search.setComponentID("workspace.browser.search");
        search.setName("Search installed presets");
        search.setTitle(search.getName());
        search.setDescription(
            "Search preset names and categories. Return loads the selected preset.");
        search.setTextToShowWhenEmpty("Search presets...", theme.muted);
        search.setColour(juce::TextEditor::backgroundColourId,
                         theme.light() && theme.style != "space" ? theme.panel : theme.display);
        search.setColour(juce::TextEditor::textColourId, theme.text);
        search.setColour(juce::TextEditor::outlineColourId, theme.border);
        search.setColour(juce::TextEditor::focusedOutlineColourId, theme.accent);
        search.setIndents(8, 5);
        search.setFont(theme.type(theme.style == "tokyo" ? 14.f : 13.f));
        search.setText(query, false);
        search.onTextChange = [this] { refreshRows(); };
        search.onReturnKey = [this] {
            if (list.getSelectedRow() < 0 && !rows.empty())
                list.selectRow(0);
            activate(list.getSelectedRow());
        };
        search.onEscapeKey = [this] {
            if (search.getText().isNotEmpty())
                search.clear();
            else if (auto *workspace = findParentComponentOfClass<Workspace>())
                workspace->grabKeyboardFocus();
        };
        addAndMakeVisible(search);
        list.setComponentID("workspace.browser.list");
        list.setTitle("Installed presets");
        list.setDescription("Use arrow keys to select. Return or double-click loads. Category rows "
                            "filter the list.");
        list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setColour(juce::ListBox::outlineColourId, theme.border);
        list.setMultipleSelectionEnabled(false);
        list.setRowHeight(theme.style == "tokyo" ? 27 : 26);
        list.setOutlineThickness(0);
        addAndMakeVisible(list);
        status.setName("Preset search status");
        status.setColour(juce::Label::textColourId, theme.muted);
        status.setFont(theme.type(10));
        addAndMakeVisible(status);
        refreshRows();
        selectPatch(selectedPatch);
    }
    ~PresetBrowser() override { list.setModel(nullptr); }
    juce::String query() const { return search.getText(); }
    int selectedPatchId() const
    {
        const auto index = list.getSelectedRow();
        return validRow(index) ? rows[index].patchId : -1;
    }
    void refreshRows()
    {
        auto updated = browserRows(processor, search.getText(), 100);
        const bool changed = !initialized || updated.size() != rows.size() ||
                             !std::equal(updated.begin(), updated.end(), rows.begin(),
                                         [](const BrowserRow &a, const BrowserRow &b) {
                                             return a.name == b.name && a.patchId == b.patchId &&
                                                    a.category == b.category;
                                         });
        if (!changed)
            return;
        initialized = true;
        const auto selection = selectedPatchId();
        rows = std::move(updated);
        list.deselectAllRows();
        list.updateContent();
        selectPatch(selection);
        const auto count = static_cast<int>(std::count_if(
            rows.begin(), rows.end(), [](const BrowserRow &row) { return !row.category; }));
        status.setText(rows.empty() ? "No matching installed presets"
                       : rows.size() >= 100
                           ? utf8("First 100 rows shown — refine search")
                           : juce::String(count) + utf8(" presets shown • Enter to load"),
                       juce::dontSendNotification);
        list.repaint();
    }
    void resized() override
    {
        auto r = getLocalBounds();
        search.setBounds(r.removeFromTop(std::min(28, r.getHeight())));
        r.removeFromTop(5);
        status.setBounds(r.removeFromBottom(std::min(20, r.getHeight())));
        list.setBounds(r);
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key.getKeyCode() != juce::KeyPress::escapeKey)
            return false;
        search.grabKeyboardFocus();
        search.selectAll();
        return true;
    }

  private:
    bool validRow(int row) const { return row >= 0 && static_cast<size_t>(row) < rows.size(); }
    void selectPatch(int id)
    {
        if (id < 0)
            return;
        for (size_t i = 0; i < rows.size(); ++i)
            if (!rows[i].category && rows[i].patchId == id)
            {
                list.selectRow(static_cast<int>(i));
                break;
            }
    }
    void activate(int row)
    {
        if (!validRow(row))
            return;
        const auto entry = rows[row];
        if (entry.category)
        {
            search.setText(entry.name, true);
            search.grabKeyboardFocus();
            search.selectAll();
        }
        else if (entry.patchId >= 0 && performAction)
            performAction("load-preset:" + juce::String(entry.patchId));
    }
    int getNumRows() override { return static_cast<int>(rows.size()); }
    juce::String getNameForRow(int row) override
    {
        return validRow(row) ? (rows[row].category ? "Category: " : "Preset: ") + rows[row].name
                             : juce::String();
    }
    juce::String getTooltipForRow(int row) override
    {
        return validRow(row) ? rows[row].name + utf8(rows[row].category ? " — Enter to filter"
                                                                        : " — Enter to load")
                             : juce::String();
    }
    void paintListBoxItem(int row, juce::Graphics &g, int width, int height, bool selected) override
    {
        if (!validRow(row))
            return;
        auto r =
            juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
        if (selected)
        {
            g.setColour(theme.accent.withAlpha(theme.light() ? .24f : .18f));
            g.fillRoundedRectangle(r.reduced(1), 3);
            g.setColour(theme.accent);
            g.drawRoundedRectangle(r.reduced(1), 3, 1);
        }
        const auto &entry = rows[row];
        auto icon = r.removeFromLeft(entry.category ? 33.f : 25.f);
        if (entry.category)
        {
            juce::Path chevron;
            chevron.addTriangle(3, height * .5f - 3, 3, height * .5f + 3, 7, height * .5f);
            g.setColour(theme.text.withAlpha(.7f));
            g.fillPath(chevron);
            const float y = height * .5f - 5;
            juce::Path folder;
            folder.startNewSubPath(12, y + 1);
            folder.lineTo(12, y - 2);
            folder.lineTo(18, y - 2);
            folder.lineTo(20, y + 1);
            folder.lineTo(27, y + 1);
            folder.lineTo(27, y + 10);
            folder.lineTo(12, y + 10);
            folder.closeSubPath();
            g.setColour(theme.style == "bauhaus" ? theme.text : theme.text.withAlpha(.76f));
            if (theme.light() && theme.style != "space")
                g.strokePath(folder, juce::PathStrokeType(1.2f));
            else
                g.fillPath(folder);
        }
        else
        {
            g.setColour(theme.accent.withAlpha(selected ? 1.f : .35f));
            g.fillRoundedRectangle(icon.getRight() - 8, height * .5f - 2, 3, 4, 1);
        }
        text(g, theme, entry.name, r.reduced(2, 0), theme.style == "tokyo" ? 14.f : 13.f,
             theme.text, entry.category);
    }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &) override { activate(row); }
    void returnKeyPressed(int row) override { activate(row); }

    SurgeSynthProcessor &processor;
    Theme &theme;
    std::function<void(const juce::String &)> performAction;
    std::vector<BrowserRow> rows;
    juce::TextEditor search;
    juce::ListBox list;
    juce::Label status;
    bool initialized{false};
};

class PerformanceKeyboard : public juce::MidiKeyboardComponent
{
  public:
    explicit PerformanceKeyboard(juce::MidiKeyboardState &state)
        : juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
    }
    void releaseHeldNotes()
    {
        // Public in JUCE 7 and 8; releases only keys owned by this component.
        juce::MidiKeyboardComponent::focusLost(juce::Component::focusChangedDirectly);
    }
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
    juce::dsp::FFT outputFFT{11};
    std::array<float, 2048> outputHistory{};
    std::array<float, 4096> fftData{};
    size_t outputWrite{0}, outputCount{0};
    std::uint32_t lastDroppedFrames{0};
    std::vector<float> spectrumDb;
    std::vector<std::vector<float>> spectrumHistory;
    std::array<PreviewCurve, 2> filterCurves;
    std::array<PreviewCurve, 12> lfoCurves;
    bool audioSubscribed{false};
    std::vector<RouteRow> routes;
    ModulationEditorModel routeEditor;
    std::vector<std::unique_ptr<ModulationRowInteraction>> routeControls;
    RouteKey inspectedRoute;
    juce::Component canvas;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::pair<juce::Component *, Rect>> placements;
    std::vector<std::unique_ptr<juce::TextButton>> actions;
    std::unique_ptr<MacroXY> xy;
    std::unique_ptr<PresetBrowser> browser;
    std::unique_ptr<PerformanceKeyboard> keyboard;
    juce::String browserQuery;
    int browserSelectedPatch{-1};
    HeaderButton skinButton{theme, "Skins"}, classicButton{theme, "Full editor"},
        sceneA{theme, "A"}, sceneB{theme, "B"};
    HeaderButton presetButton{theme}, previous{theme, "<"}, next{theme, ">"};
    HeaderButton browserButton{theme, "Browser"}, saveButton{theme, "Save"},
        modButton{theme, "Mod"}, fxButton{theme, "FX"}, synthButton{theme, "Synth"};
    Rect headerPresetBounds, headerCategoryBounds;
    int ticks{0};
    float scale{1};
    juce::Point<float> offset;

    Impl(Workspace &o, SurgeSynthProcessor &p)
        : owner(o), processor(p), parameters(p), routeEditor(p)
    {
        owner.addAndMakeVisible(canvas);
        canvas.setInterceptsMouseClicks(false, true);
        for (auto *b : {&skinButton, &classicButton, &sceneA, &sceneB, &presetButton, &previous,
                        &next, &browserButton, &saveButton, &modButton, &fxButton, &synthButton})
        {
            canvas.addAndMakeVisible(b);
            b->setWantsKeyboardFocus(true);
            b->setName(b->getButtonText());
            b->setTitle(b->getButtonText());
        }
        skinButton.setComponentID("workspace.skin-selector");
        classicButton.setComponentID("workspace.full-editor");
        sceneA.setComponentID("workspace.scene-a");
        sceneB.setComponentID("workspace.scene-b");
        presetButton.setComponentID("workspace.preset");
        previous.setComponentID("workspace.previous-preset");
        next.setComponentID("workspace.next-preset");
        browserButton.setComponentID("workspace.navigation.browser");
        saveButton.setComponentID("workspace.navigation.save");
        modButton.setComponentID("workspace.navigation.modulation");
        fxButton.setComponentID("workspace.navigation.effects");
        synthButton.setComponentID("workspace.navigation.synth");
        synthButton.setName("Current synthesizer workspace");
        synthButton.setTitle(synthButton.getName());
        synthButton.setTooltip("Focus the current synthesizer workspace");
        synthButton.setToggleState(true, juce::dontSendNotification);
        synthButton.onClick = [this] { owner.grabKeyboardFocus(); };
        skinButton.setTooltip("Choose a native workspace or skin (Ctrl/Command+K)");
        classicButton.setTooltip("Open the full Surge XT editor; Escape also opens it");
        browserButton.setTooltip("Open the native preset browser");
        saveButton.setTooltip("Save the current patch with the native editor");
        modButton.setTooltip("Open the full modulation editor");
        fxButton.setTooltip("Open the full effects editor");
        previous.setName("Previous preset");
        previous.setTitle(previous.getName());
        next.setName("Next preset");
        next.setTitle(next.getName());
        sceneA.setName("Select Scene A");
        sceneA.setTitle(sceneA.getName());
        sceneB.setName("Select Scene B");
        sceneB.setTitle(sceneB.getName());
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
        browserButton.onClick = [this] { action("browser"); };
        saveButton.onClick = [this] { action("save"); };
        modButton.onClick = [this] { action("modulation"); };
        fxButton.onClick = [this] { action("effects"); };
    }
    void action(const juce::String &a)
    {
        if (owner.onAction)
            owner.onAction(a);
    }
    void button(const juce::String &label, const juce::String &actionId, Rect r)
    {
        auto b = std::make_unique<ThemeButton>(theme, label);
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
    void cancelInteractions()
    {
        for (auto &row : routeControls)
            row->cancelGesture();
        routeEditor.end();
        for (auto &control : controls)
            if (auto *slider = dynamic_cast<BoundSlider *>(control.get()))
            {
                slider->cancelGesture();
                slider->dismissEntry();
            }
        if (xy)
            xy->cancelGesture();
        if (keyboard)
            keyboard->releaseHeldNotes();
        parameters.endAll();
    }
    void build()
    {
        cancelInteractions();
        routeControls.clear();
        routes.clear();
        inspectedRoute = {};
        if (browser)
        {
            browserQuery = browser->query();
            browserSelectedPatch = browser->selectedPatchId();
        }
        controls.clear();
        placements.clear();
        actions.clear();
        xy.reset();
        browser.reset();
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
                {
                    button("Browse all presets", "browser", r.removeFromBottom(31).reduced(7, 3));
                    browser = std::make_unique<PresetBrowser>(
                        processor, theme, browserQuery, browserSelectedPatch,
                        [this](const juce::String &a) { action(a); });
                    browser->setComponentID("workspace.browser");
                    canvas.addAndMakeVisible(*browser);
                    placements.push_back({browser.get(), r.reduced(10).withTrimmedTop(24)});
                }
                if (kind == "effects")
                    button("Full FX editor", "effects",
                           r.removeFromTop(28).removeFromRight(110).reduced(3));
                if (kind == "analyzer")
                    button(r.getWidth() < 320 ? "Scope" : "Open scope", "analyzer",
                           r.removeFromTop(28)
                               .removeFromRight(r.getWidth() < 320 ? 54.f : 92.f)
                               .reduced(3));
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
        for (auto *b : {&skinButton, &classicButton, &sceneA, &sceneB, &presetButton, &previous,
                        &next, &browserButton, &saveButton, &modButton, &fxButton, &synthButton})
            styleButton(*b);
        for (auto &b : actions)
            styleButton(*b);
        presetButton.setColour(juce::TextButton::buttonColourId,
                               theme.style == "clear" ? theme.panel : theme.display);
        presetButton.setColour(juce::TextButton::textColourOffId,
                               theme.style == "clear" ? theme.text
                                                      : theme.display.contrasting(.95f));
        owner.resized();
    }
    void setAudioSubscription(bool active)
    {
        if (active == audioSubscribed)
            return;
        if (active)
        {
            // Only the consumer index is touched; never reset a running producer.
            processor.referenceAudio.popall();
            processor.referenceAudio.subscribe();
            lastDroppedFrames = processor.referenceAudio.droppedFrames();
        }
        else
        {
            processor.referenceAudio.unsubscribe();
            visuals.output.clear();
            visuals.left = visuals.right = 0;
            spectrumDb.clear();
            spectrumHistory.clear();
            outputWrite = outputCount = 0;
        }
        audioSubscribed = active;
    }
    struct RoutePlacement
    {
        Rect bounds, amount;
        size_t index;
    };
    std::vector<RoutePlacement> routePlacements() const
    {
        std::vector<RoutePlacement> result;
        if (auto *panels = manifest["panels"].getArray())
            for (const auto &panel : *panels)
            {
                if (string(panel, "kind") != "matrix")
                    continue;
                auto content = bounds(panel).reduced(10).withTrimmedTop(24);
                const bool accessible = theme.style == "clear";
                const bool twoColumns =
                    theme.style == "tokyo" && string(panel, "variant") == "horizontal";
                const int columns = twoColumns ? 2 : 1;
                const int maximumRows = accessible || theme.style == "space" || twoColumns ? 8 : 6;
                const float gap = twoColumns ? 14.f : 0.f;
                const float width = (content.getWidth() - gap) / columns;
                const float height = juce::jlimit(
                    accessible ? 24.f : 16.f, accessible ? 29.f : 30.f,
                    (content.getHeight() - 22.f) / static_cast<float>(maximumRows / columns));
                const int rowsPerColumn =
                    std::min(maximumRows / columns,
                             static_cast<int>(std::max(0.f, content.getHeight() - 22.f) / height));
                for (int column = 0; column < columns; ++column)
                    for (int row = 0; row < rowsPerColumn; ++row)
                    {
                        const auto index = static_cast<size_t>(column * rowsPerColumn + row);
                        if (index >= routes.size())
                            break;
                        Rect rectangle(content.getX() + column * (width + gap),
                                       content.getY() + 22 + row * height, width, height);
                        auto amount = rectangle.withTrimmedLeft(20);
                        const auto fieldWidth = amount.getWidth();
                        amount.removeFromLeft(fieldWidth * (accessible ? .70f : .29f));
                        if (!accessible)
                            amount.setWidth(fieldWidth * .25f);
                        result.push_back({rectangle, amount, index});
                    }
            }
        return result;
    }
    void syncRouteControls()
    {
        if (routeEditor.isEditing())
            return; // A dragged route must survive the engine's deletion at zero depth.
        const auto geometry = routePlacements();
        bool rebuild = geometry.size() != routeControls.size();
        if (!rebuild)
            for (size_t i = 0; i < geometry.size(); ++i)
                if (routeControls[i]->route().key != routes[geometry[i].index].key)
                    rebuild = true;
        if (rebuild)
        {
            routeControls.clear();
            for (size_t i = 0; i < geometry.size(); ++i)
            {
                auto row = std::make_unique<ModulationRowInteraction>(
                    routeEditor, routes[geometry[i].index], theme.accent);
                row->setComponentID("workspace.modulation.row." +
                                    juce::String(static_cast<int>(i)));
                row->depthSlider().setComponentID("workspace.modulation.depth." +
                                                  juce::String(static_cast<int>(i)));
                row->onSelected = [this](const RouteRow &selected) {
                    inspectedRoute = selected.key;
                    for (auto &control : routeControls)
                        control->setSelected(control->route().key == inspectedRoute);
                    owner.repaint();
                };
                row->onChanged = [this] {
                    // Delete/mute may remove this row. Defer rebuilding until its
                    // native callback and mouse event have unwound.
                    juce::Component::SafePointer<Workspace> safe(&owner);
                    juce::MessageManager::callAsync([safe] {
                        if (safe)
                        {
                            safe->impl->ticks = 0;
                            safe->refresh();
                        }
                    });
                };
                canvas.addAndMakeVisible(*row);
                routeControls.push_back(std::move(row));
            }
        }
        for (size_t i = 0; i < geometry.size(); ++i)
        {
            auto &control = *routeControls[i];
            const auto rectangle = geometry[i].bounds.toNearestInt();
            control.setBounds(rectangle);
            control.setAmountBounds(
                geometry[i].amount.toNearestInt().translated(-rectangle.getX(), -rectangle.getY()));
            control.setRow(routes[geometry[i].index]);
            control.setSelected(control.route().key == inspectedRoute);
        }
    }
    void update()
    {
        setAudioSubscription(owner.isShowing());
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
        presetButton.setName("Open preset browser: " + name);
        presetButton.setTitle("Preset: " + name);
        if (xy)
            xy->sync();
        if ((ticks++ % 8) == 0)
        {
            auto output = std::move(visuals.output);
            const auto leftPeak = visuals.left, rightPeak = visuals.right;
            visuals = Visuals::sample(processor, parameters.getScene());
            visuals.output = std::move(output);
            visuals.left = leftPeak;
            visuals.right = rightPeak;
            std::array<bool, 2> filtersNeeded{};
            std::array<bool, 12> lfosNeeded{};
            if (auto *panels = manifest["panels"].getArray())
                for (const auto &panel : *panels)
                {
                    const auto kind = string(panel, "kind");
                    const int index = static_cast<int>(number(panel, "index", 1)) - 1;
                    if ((kind == "filter" || kind == "inspector") && index >= 0 && index < 2)
                        filtersNeeded[index] = true;
                    if (kind == "filter" && string(panel, "variant") == "combined")
                        filtersNeeded[1] = true;
                    if (kind == "routing" && string(panel, "variant") == "flow")
                        filtersNeeded = {true, true};
                    if (kind == "lfo" && index >= 0 && index < 12)
                        lfosNeeded[index] = true;
                }
            for (int i = 0; i < 2; ++i)
                if (filtersNeeded[i])
                    filterCurves[i] = filterResponse(processor, parameters.getScene(), i);
            for (int i = 0; i < 12; ++i)
                if (lfosNeeded[i])
                    lfoCurves[i] = lfoCurve(processor, parameters.getScene(), i);
            auto updatedRoutes = modulationRows(processor, parameters.getScene());
            if (!routeEditor.isEditing())
                routes = std::move(updatedRoutes);
            else
                for (auto &row : routes)
                    if (routeEditor.isEditing(row.key))
                    {
                        row.normalizedDepth = routeEditor.currentDepth();
                        const auto match = std::find_if(
                            updatedRoutes.begin(), updatedRoutes.end(),
                            [&row](const auto &updated) { return updated.key == row.key; });
                        row.amount = match == updatedRoutes.end() ? "0" : match->amount;
                    }
            syncRouteControls();
            if (browser)
                browser->refreshRows();
        }
        if (audioSubscribed)
        {
            auto [left, right] = processor.referenceAudio.popall();
            const auto dropped = processor.referenceAudio.droppedFrames();
            if (dropped != lastDroppedFrames)
            {
                // Never splice an audio history across lost display frames.
                lastDroppedFrames = dropped;
                outputWrite = outputCount = 0;
                spectrumDb.clear();
                spectrumHistory.clear();
                visuals.output.clear();
                visuals.left = visuals.right = 0;
                return;
            }
            visuals.left *= .8f;
            visuals.right *= .8f;
            const auto frames = std::min(left.size(), right.size());
            if (frames > 0)
            {
                for (size_t i = 0; i < frames; ++i)
                {
                    if (std::isfinite(left[i]))
                        visuals.left = std::max(visuals.left, std::abs(left[i]));
                    if (std::isfinite(right[i]))
                        visuals.right = std::max(visuals.right, std::abs(right[i]));
                    const float sample = (left[i] + right[i]) * .5f;
                    outputHistory[outputWrite] = std::isfinite(sample) ? sample : 0.f;
                    outputWrite = (outputWrite + 1) % outputHistory.size();
                    outputCount = std::min(outputCount + 1, outputHistory.size());
                }
                const auto count = std::min<size_t>(512, frames);
                visuals.output.resize(count);
                for (size_t i = 0; i < count; ++i)
                {
                    const auto at = frames - count + i;
                    const float value = (left[at] + right[at]) * .5f;
                    visuals.output[i] = std::isfinite(value) ? value : 0.f;
                }
                if (outputCount == outputHistory.size())
                {
                    fftData.fill(0.f);
                    for (size_t i = 0; i < outputHistory.size(); ++i)
                    {
                        const float window =
                            .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i /
                                                 (outputHistory.size() - 1));
                        fftData[i] =
                            outputHistory[(outputWrite + i) % outputHistory.size()] * window;
                    }
                    outputFFT.performFrequencyOnlyForwardTransform(fftData.data());
                    spectrumDb.resize(160);
                    const auto rate = std::max(1.0, processor.getSampleRate());
                    const auto top = std::min(20000.0, rate * .5);
                    for (size_t i = 0; i < spectrumDb.size(); ++i)
                    {
                        const auto frequency =
                            20.0 *
                            std::pow(top / 20.0, static_cast<double>(i) / (spectrumDb.size() - 1));
                        const auto bin =
                            std::clamp(frequency * outputHistory.size() / rate, 1.0, 1023.0);
                        const auto low = static_cast<size_t>(bin);
                        const auto fraction = static_cast<float>(bin - low);
                        const auto magnitude =
                            (fftData[low] + fraction * (fftData[low + 1] - fftData[low])) *
                            (4.f / static_cast<float>(outputHistory.size()));
                        spectrumDb[i] = juce::jlimit(
                            -96.f, 12.f, 20.f * std::log10(std::max(1.e-8f, magnitude)));
                    }
                    spectrumHistory.push_back(spectrumDb);
                    if (spectrumHistory.size() > 64)
                        spectrumHistory.erase(spectrumHistory.begin());
                }
            }
        }
    }
    void layoutHeader()
    {
        using H = HeaderButton;
        const auto &style = theme.style;
        auto put = [](H &b, int x, int y, int w, int h, H::Presentation mode = H::chip,
                      float font = 12.f, bool bold = false) {
            b.setBounds(x, y, w, h);
            b.present(mode, font, bold);
            b.setVisible(true);
        };
        for (auto *b : {&skinButton, &classicButton, &sceneA, &sceneB, &presetButton, &previous,
                        &next, &browserButton, &saveButton, &modButton, &fxButton, &synthButton})
        {
            b->setVisible(true);
            b->present(H::chip);
        }
        synthButton.setVisible(false);
        sceneA.setButtonText("A");
        sceneB.setButtonText("B");
        browserButton.setButtonText("Browser");
        saveButton.setButtonText("Save");
        modButton.setButtonText("Mod");
        fxButton.setButtonText("FX");
        skinButton.setButtonText("Skins");
        classicButton.setButtonText("Editor");
        synthButton.setButtonText("Synth");
        headerCategoryBounds = {};
        if (style == "precision")
        {
            headerPresetBounds = {645, 48, 530, 45};
            headerCategoryBounds = {680, 77, 305, 15};
            put(synthButton, 292, 58, 61, 26, H::navigation, 12, true);
            put(browserButton, 365, 56, 72, 30, H::navigation);
            put(modButton, 449, 56, 38, 30, H::navigation);
            put(fxButton, 501, 56, 32, 30, H::navigation);
            put(skinButton, 544, 56, 70, 30, H::navigation);
            put(previous, 648, 52, 30, 36, H::chevron);
            put(presetButton, 682, 49, 304, 29, H::caption, 21, true);
            put(next, 991, 52, 31, 36, H::chevron);
            put(classicButton, 1034, 59, 65, 25, H::navigation, 11);
            put(saveButton, 1116, 59, 48, 25, H::chip, 11);
            put(sceneA, 1211, 50, 49, 28, H::chip, 19, true);
            put(sceneB, 1261, 50, 49, 28, H::chip, 17);
        }
        else if (style == "ivory")
        {
            headerPresetBounds = {548, 14, 434, 46};
            headerCategoryBounds = {568, 64, 393, 16};
            put(sceneA, 409, 25, 50, 35, H::chip, 15, true);
            put(sceneB, 461, 25, 50, 35, H::chip, 15);
            put(previous, 554, 19, 38, 36, H::chevron);
            put(presetButton, 596, 15, 341, 44, H::caption, 23);
            put(next, 940, 19, 36, 36, H::chevron);
            put(browserButton, 1011, 25, 110, 35, H::chip, 13, true);
            put(saveButton, 1135, 25, 89, 35, H::chip, 13, true);
            put(skinButton, 1233, 25, 39, 35, H::chip, 10);
            put(classicButton, 1279, 25, 31, 35, H::navigation, 9);
            put(modButton, 1017, 65, 49, 18, H::navigation, 10);
            put(fxButton, 1075, 65, 37, 18, H::navigation, 10);
        }
        else if (style == "spectral")
        {
            headerPresetBounds = {585, 44, 288, 34};
            put(previous, 587, 46, 31, 30, H::chevron);
            put(presetButton, 620, 45, 220, 32, H::caption, 19, true);
            put(next, 841, 46, 30, 30, H::chevron);
            put(browserButton, 891, 45, 96, 33, H::chip, 13);
            put(saveButton, 994, 45, 61, 33, H::chip, 12);
            put(classicButton, 1060, 45, 38, 33, H::chip, 10);
            put(modButton, 1116, 46, 32, 32, H::navigation, 10);
            put(fxButton, 1153, 46, 31, 32, H::navigation, 10);
            put(sceneA, 1204, 49, 36, 26, H::chip, 13, true);
            put(sceneB, 1242, 49, 37, 26, H::chip, 13);
            put(skinButton, 1297, 46, 45, 32, H::chip, 10);
        }
        else if (style == "bauhaus")
        {
            headerPresetBounds = {632, 27, 355, 47};
            headerCategoryBounds = {685, 57, 265, 15};
            put(sceneA, 502, 31, 37, 32, H::chip, 18, true);
            put(sceneB, 542, 31, 37, 32, H::chip, 18);
            put(previous, 636, 33, 37, 35, H::chevron);
            put(presetButton, 681, 28, 272, 30, H::caption, 20, true);
            put(next, 955, 33, 30, 35, H::chevron);
            put(browserButton, 996, 29, 39, 35, H::navigation, 9);
            put(saveButton, 1041, 29, 39, 35, H::navigation, 10);
            put(skinButton, 1084, 29, 28, 35, H::navigation, 8.5f);
            put(modButton, 1319, 6, 50, 23, H::navigation, 10);
            put(fxButton, 1373, 6, 37, 23, H::navigation, 10);
            put(classicButton, 1414, 6, 82, 23, H::navigation, 11);
        }
        else if (style == "space")
        {
            headerPresetBounds = {682, 25, 335, 40};
            put(sceneA, 309, 99, 42, 44, H::chip, 23, true);
            put(sceneB, 357, 99, 42, 44, H::chip, 23, true);
            put(previous, 649, 28, 30, 34, H::chevron);
            put(presetButton, 689, 27, 321, 35, H::caption, 20, true);
            put(next, 1019, 28, 29, 34, H::chevron);
            put(saveButton, 1054, 27, 56, 31, H::chip, 13, true);
            put(classicButton, 1116, 27, 63, 31, H::chip, 12, true);
            put(skinButton, 1185, 27, 56, 31, H::chip, 12, true);
            put(browserButton, 1279, 19, 99, 29, H::navigation, 12, true);
            put(modButton, 1384, 19, 43, 29, H::navigation, 11);
            put(fxButton, 1433, 19, 36, 29, H::navigation, 11);
        }
        else if (style == "tokyo")
        {
            headerPresetBounds = {691, 21, 365, 43};
            headerCategoryBounds = {727, 45, 290, 16};
            put(sceneA, 501, 26, 55, 32, H::chip, 22, true);
            put(sceneB, 604, 26, 55, 32, H::chip, 22, true);
            put(previous, 692, 23, 30, 36, H::chevron);
            put(presetButton, 725, 22, 289, 25, H::caption, 19, true);
            put(next, 1019, 24, 32, 34, H::chevron);
            put(saveButton, 1061, 25, 33, 31, H::chip, 10);
            put(browserButton, 16, 84, 83, 31, H::chip, 14, true);
            put(synthButton, 290, 84, 107, 31, H::chip, 17, true);
            put(modButton, 404, 84, 144, 31, H::navigation, 14);
            modButton.setButtonText("MOD MATRIX");
            put(fxButton, 555, 84, 77, 31, H::navigation, 14);
            put(classicButton, 640, 84, 114, 31, H::navigation, 13);
            put(skinButton, 762, 84, 109, 31, H::navigation, 13);
        }
        else if (style == "aurora")
        {
            headerPresetBounds = {722, 3, 292, 57};
            headerCategoryBounds = {750, 7, 238, 16};
            sceneA.setButtonText("A  Scene A");
            sceneB.setButtonText("B  Scene B");
            put(sceneA, 418, 16, 96, 35, H::chip, 15, true);
            put(sceneB, 521, 16, 96, 35, H::chip, 15, true);
            put(previous, 682, 17, 32, 32, H::chevron);
            put(presetButton, 727, 25, 253, 31, H::caption, 24, true);
            put(next, 983, 17, 32, 32, H::chevron);
            put(modButton, 333, 18, 32, 31, H::chip, 10);
            put(fxButton, 370, 18, 32, 31, H::chip, 10);
            put(saveButton, 631, 17, 34, 33, H::chip, 10);
            put(browserButton, 1126, 16, 71, 35, H::chip, 13);
            put(skinButton, 1203, 16, 43, 35, H::chip, 11);
            put(classicButton, 1260, 16, 36, 35, H::chip, 10);
        }
        else if (style == "clear")
        {
            headerPresetBounds = {771, 3, 294, 56};
            headerCategoryBounds = {786, 43, 239, 15};
            put(browserButton, 315, 12, 98, 39, H::chip, 16, true);
            put(synthButton, 416, 12, 72, 39, H::navigation, 16, true);
            synthButton.setButtonText("Patch");
            put(classicButton, 491, 12, 59, 39, H::navigation, 16, true);
            classicButton.setButtonText("Edit");
            put(fxButton, 554, 12, 75, 39, H::navigation, 15, true);
            fxButton.setButtonText("Effects");
            put(previous, 734, 12, 36, 39, H::chevron);
            put(presetButton, 785, 17, 236, 28, H::caption, 23, true);
            put(next, 1025, 12, 37, 39, H::chevron);
            put(saveButton, 1078, 12, 43, 39, H::navigation, 12);
            put(modButton, 1128, 12, 58, 39, H::navigation, 12);
            put(skinButton, 1477, 12, 50, 39, H::navigation, 12);
            put(sceneA, 428, 78, 217, 38, H::chip, 22, true);
            put(sceneB, 655, 78, 265, 38, H::chip, 22, true);
            sceneA.setButtonText("A  Scene A");
            sceneB.setButtonText("B  Scene B");
        }
        else if (style == "flow")
        {
            headerPresetBounds = {605, 4, 508, 53};
            headerCategoryBounds = {651, 39, 380, 15};
            put(sceneA, 355, 4, 109, 25, H::chip, 15, true);
            put(sceneB, 470, 4, 108, 25, H::chip, 15, true);
            sceneA.setButtonText("SCENE A");
            sceneB.setButtonText("SCENE B");
            put(previous, 607, 10, 32, 40, H::chevron);
            put(presetButton, 642, 18, 391, 24, H::caption, 18, true);
            put(next, 1036, 10, 34, 40, H::chevron);
            put(browserButton, 1169, 12, 47, 37, H::chip, 10);
            put(saveButton, 1223, 12, 47, 37, H::chip, 11);
            put(modButton, 1290, 12, 44, 37, H::chip, 11);
            put(fxButton, 1339, 12, 44, 37, H::chip, 11);
            put(classicButton, 1403, 12, 59, 37, H::chip, 11);
            put(skinButton, 1529, 12, 46, 37, H::chip, 11);
        }
        else // live performance
        {
            headerPresetBounds = {479, 59, 449, 49};
            headerCategoryBounds = {592, 87, 278, 15};
            put(browserButton, 522, 10, 94, 28, H::chip, 13);
            put(synthButton, 617, 10, 92, 28, H::chip, 13, true);
            synthButton.setButtonText("Synthesis");
            put(modButton, 711, 10, 94, 28, H::chip, 13);
            modButton.setButtonText("Modulation");
            put(fxButton, 807, 10, 92, 28, H::chip, 13);
            fxButton.setButtonText("Effects");
            put(classicButton, 902, 10, 102, 28, H::chip, 13);
            classicButton.setButtonText("Full editor");
            put(skinButton, 1026, 10, 34, 28, H::chip, 9);
            put(previous, 481, 63, 38, 39, H::chevron);
            put(presetButton, 589, 62, 282, 27, H::caption, 20, true);
            put(next, 885, 63, 38, 39, H::chevron);
            put(saveButton, 942, 70, 67, 28, H::chip, 12);
            put(sceneA, 1047, 70, 31, 28, H::chip, 17, true);
            put(sceneB, 1082, 70, 30, 28, H::chip, 16);
        }
        previous.present(H::chevron);
        next.present(H::chevron);
        // Preset background is painted once around the arrows/caption. Give its
        // actual child buttons the appropriate ink without changing the patch.
        const auto presetInk = style == "ivory"   ? juce::Colour(0xfffff9e8)
                               : style == "space" ? juce::Colour(0xffffb53b)
                               : style == "clear" ? theme.text
                               : style == "tokyo" ? theme.secondary
                                                  : theme.text;
        for (auto *b : {&presetButton, &previous, &next})
            b->setColour(juce::TextButton::textColourOffId, presetInk);
    }
    void headerMeters(juce::Graphics &g, Rect r, bool horizontal = false)
    {
        const auto low = theme.style == "ivory" ? juce::Colour(0xff659e36)
                         : theme.style == "precision" || theme.style == "tokyo" ? theme.secondary
                                                                                : theme.accent;
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto peak = channel ? visuals.right : visuals.left;
            const float amount =
                peak > 0 ? juce::jlimit(0.f, 1.f, (20.f * std::log10(peak) + 60.f) / 60.f) : 0.f;
            auto bar = horizontal ? Rect(r.getX(), r.getY() + channel * r.getHeight() * .55f,
                                         r.getWidth(), r.getHeight() * .37f)
                                  : Rect(r.getX() + channel * r.getWidth() * .56f, r.getY(),
                                         r.getWidth() * .35f, r.getHeight());
            g.setColour(theme.display);
            g.fillRoundedRectangle(bar, 1.f);
            for (int segment = 0; segment < 24; ++segment)
            {
                auto ink =
                    segment > 20 ? theme.secondary.interpolatedWith(theme.accent, .75f) : low;
                g.setColour(ink.withAlpha(segment / 24.f < amount ? 1.f : .09f));
                if (horizontal)
                    g.fillRect(Rect(bar.getX() + segment * bar.getWidth() / 24.f, bar.getY(),
                                    std::max(.5f, bar.getWidth() / 24.f - 1.f), bar.getHeight()));
                else
                    g.fillRect(Rect(bar.getX(),
                                    bar.getBottom() - (segment + 1) * bar.getHeight() / 24.f,
                                    bar.getWidth(), std::max(.5f, bar.getHeight() / 24.f - .7f)));
            }
        }
    }
    void drawHeader(juce::Graphics &g)
    {
        const auto &style = theme.style;
        const float h = number(manifest, "headerHeight", 92);
        const auto ink = style == "space" ? juce::Colour(0xff17150e) : theme.text;
        if (style == "precision")
        {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343b42), 0, 0,
                                                   juce::Colour(0xff252c32), 1586, 31, false));
            g.fillRoundedRectangle({1, 1, 1584, 35}, 10);
            text(g, theme, "Surge XT", {650, 5, 286, 23}, 15, theme.text, false,
                 juce::Justification::centred);
            text(g, theme, "HYBRID SYNTHESIZER", {18, 8, 230, 17}, 9, theme.muted);
            bezel(g, theme, {1, 32, 1584, 78}, theme.background, 0);
        }
        else if (style == "space")
        {
            for (auto r : {Rect(5, 3, 464, 76), Rect(474, 3, 164, 76), Rect(643, 3, 618, 76),
                           Rect(1266, 3, 314, 76)})
                bezel(g, theme, r, juce::Colour(0xffbda685), 7);
        }
        else if (style == "tokyo")
        {
            for (auto r : {Rect(2, 1, 482, 68), Rect(489, 1, 183, 68), Rect(678, 1, 426, 68),
                           Rect(1110, 1, 473, 68)})
                bezel(g, theme, r, theme.panel, 5);
        }
        else if (style == "flow")
        {
            for (auto r : {Rect(2, 1, 332, 60), Rect(341, 1, 250, 60), Rect(598, 1, 521, 60),
                           Rect(1126, 1, 458, 60)})
                bezel(g, theme, r, theme.panel, 3);
        }
        else if (style == "live")
        {
            bezel(g, theme, {1, 0, 1584, 47}, theme.panel, 11);
            bezel(g, theme, {3, 57, 458, 52}, theme.panel, 16);
            bezel(g, theme, {1032, 57, 548, 52}, theme.panel, 14);
            text(g, theme, "Preset Browser", {27, 66, 166, 31}, 15, ink, true);
        }
        else if (style == "spectral")
            bezel(g, theme, {19, 18, 1548, 77}, theme.display, 5);
        else if (style == "aurora")
        {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff302045), 0, 0,
                                                   juce::Colour(0xff0e0a19), 0, h, false));
            g.fillRect(0.f, 0.f, 1586.f, h);
        }
        else
            bezel(g, theme, {2, 2, 1582, h - 4}, theme.panel, style == "ivory" ? 12.f : 2.f);

        if (!headerPresetBounds.isEmpty() && style != "aurora" && style != "clear")
            bezel(g, theme, headerPresetBounds,
                  style == "ivory" ? juce::Colour(0xff153721) : theme.display,
                  style == "live"    ? 15.f
                  : style == "ivory" ? 9.f
                                     : 4.f);

        auto line = [&](float x, float y, float height) {
            g.setColour(ink.withAlpha(style == "bauhaus" ? .60f : .25f));
            g.drawLine(x, y, x, y + height, 1.f);
        };
        auto logo = [&](float x, float y, float w, float height) {
            if (style == "bauhaus")
            {
                g.setColour(theme.red);
                juce::Path disk;
                disk.addPieSegment(x, y, w, height, juce::MathConstants<float>::pi,
                                   juce::MathConstants<float>::twoPi, 0.f);
                g.fillPath(disk);
                g.setColour(ink);
                g.fillRect(Rect(x + w * .52f, y + height * .38f, w * .36f, height * .62f));
                juce::Path triangle;
                triangle.addTriangle(x + w * .52f, y + height * .45f, x + w, y + height * .45f,
                                     x + w * .52f, y + height * .86f);
                g.setColour(theme.yellow);
                g.fillPath(triangle);
            }
            else if (style == "space" || style == "tokyo")
            {
                for (int i = 0; i < 3; ++i)
                {
                    juce::Path slash;
                    slash.startNewSubPath(x + i * w * .23f, y + height);
                    slash.lineTo(x + w * .43f + i * w * .23f, y);
                    g.setColour(style == "space" ? juce::Colour(0xffb96322) : theme.red);
                    g.strokePath(slash, juce::PathStrokeType(style == "space" ? 10.f : 3.2f));
                }
            }
            else if (style == "flow")
            {
                for (int i = 0; i < 9; ++i)
                {
                    const float px = x + (i % 3) * w * .34f, py = y + (i / 3) * height * .34f;
                    g.setColour(theme.accent.withAlpha(.65f + (i % 2) * .35f));
                    g.drawEllipse(px, py, 4, 4, 1.4f);
                }
            }
            else
            {
                juce::Path wave;
                for (int point = 0; point <= 80; ++point)
                {
                    const float t = point / 80.f;
                    const float px = x + t * w;
                    const float py = y + height * .5f +
                                     std::sin(t * 12.2f) *
                                         std::sin(t * juce::MathConstants<float>::pi) * height *
                                         .45f;
                    if (point == 0)
                        wave.startNewSubPath(px, py);
                    else
                        wave.lineTo(px, py);
                }
                if (style == "spectral" || style == "aurora")
                {
                    for (int layer = -3; layer <= 3; ++layer)
                    {
                        g.setColour(theme.accent.withAlpha(.12f));
                        g.strokePath(wave, juce::PathStrokeType(4.f),
                                     juce::AffineTransform::translation(0, layer * 1.6f));
                    }
                }
                if (style == "ivory")
                {
                    g.setColour(juce::Colours::black.withAlpha(.3f));
                    g.strokePath(wave, juce::PathStrokeType(3.5f),
                                 juce::AffineTransform::translation(0, 1.5f));
                }
                g.setColour(style == "ivory" ? juce::Colour(0xffa0803c) : theme.accent);
                g.strokePath(wave, juce::PathStrokeType(style == "clear" ? 3.8f : 2.2f));
            }
        };
        auto brand = [&](Rect r, float size, bool upper = true) {
            auto font = theme.type(size, style != "ivory");
            if (style == "space")
                font.setStyleFlags(juce::Font::bold | juce::Font::italic);
            g.setFont(font);
            g.setColour(ink);
            const auto prefix = upper ? juce::String("SURGE ") : juce::String("Surge ");
            juce::GlyphArrangement prefixGlyphs;
            prefixGlyphs.addLineOfText(font, prefix, 0.f, 0.f);
            const float prefixWidth =
                std::min(r.getWidth() * .79f,
                         prefixGlyphs.getBoundingBox(0, prefixGlyphs.getNumGlyphs(), true).getWidth());
            g.drawFittedText(prefix, r.withWidth(prefixWidth).toNearestInt(),
                             juce::Justification::centredLeft, 1, .9f);
            g.setColour(style == "precision" || style == "tokyo" || style == "flow" ? theme.accent
                                                                                    : ink);
            g.drawFittedText("XT", r.withTrimmedLeft(prefixWidth).toNearestInt(),
                             juce::Justification::centredLeft, 1, .9f);
        };
        if (style == "precision")
        {
            logo(25, 49, 44, 38);
            brand({80, 43, 161, 36}, 32);
            text(g, theme, "H Y B R I D  S Y N T H E S I Z E R", {82, 79, 180, 14}, 10, ink);
            line(269, 49, 42);
            line(1346, 49, 42);
            text(g, theme, "SCENE", {1211, 80, 100, 17}, 11, theme.muted, false,
                 juce::Justification::centred);
            headerMeters(g, {1533, 47, 23, 47});
        }
        else if (style == "ivory")
        {
            logo(30, 23, 54, 34);
            brand({101, 13, 200, 39}, 36);
            text(g, theme, "HYBRID SYNTHESIZER", {102, 52, 202, 18}, 13, ink);
            line(322, 25, 39);
            line(1298, 25, 38);
            text(g, theme, "SCENE", {350, 31, 55, 22}, 14, ink);
            headerMeters(g, {1536, 17, 21, 54});
        }
        else if (style == "spectral")
        {
            logo(37, 29, 75, 45);
            brand({123, 29, 172, 36}, 30);
            text(g, theme, "H Y B R I D  S Y N T H E S I Z E R", {125, 65, 188, 12}, 8,
                 theme.muted);
            line(305, 37, 36);
            text(g, theme, "S P E C T R A L", {319, 36, 106, 14}, 10, theme.accent, true);
            text(g, theme, "O B S E R V A T O R Y", {319, 52, 113, 14}, 9, theme.accent, true);
            // The observatory badge is cosmetic vector artwork, not an audio display.
            g.setColour(theme.accent.withAlpha(.12f));
            g.fillEllipse(425, 20, 103, 60);
            g.setColour(theme.accent.withAlpha(.5f));
            g.drawEllipse(440, 28, 62, 49, 1);
            g.drawLine(471, 28, 471, 78, 1);
            g.drawLine(440, 60, 502, 60, 1);
            text(g, theme, "PRESET", {662, 28, 128, 13}, 9, theme.muted, false,
                 juce::Justification::centred);
            text(g, theme, "Scene", {1204, 32, 72, 14}, 11, theme.muted);
            text(g, theme, parameters.display("master.volume"), {1484, 49, 67, 24}, 11, theme.text,
                 false, juce::Justification::centred);
        }
        else if (style == "bauhaus")
        {
            logo(20, 14, 49, 55);
            brand({82, 7, 241, 58}, 53);
            text(g, theme, "HYBRID", {339, 13, 126, 18}, 15, ink, true);
            text(g, theme, "SYNTHESIZER", {339, 29, 134, 18}, 15, ink, true);
            text(g, theme, "OPEN SOURCE", {339, 45, 134, 18}, 15, ink, true);
            line(478, 8, 69);
            line(614, 8, 69);
            line(1120, 8, 69);
            line(1305, 8, 69);
            text(g, theme, "SCENE", {502, 8, 87, 18}, 13, ink, true);
            text(g, theme, "PRESET", {635, 7, 135, 18}, 13, ink, true);
            text(g, theme, "VOICES", {1141, 8, 108, 18}, 12, ink, true);
            headerMeters(g, {1327, 40, 155, 27}, true);
        }
        else if (style == "space")
        {
            logo(34, 18, 60, 43);
            brand({108, 7, 245, 58}, 48);
            text(g, theme, "HYBRID", {367, 15, 93, 16}, 13, ink, true);
            text(g, theme, "SYNTHESIZER", {367, 30, 93, 16}, 13, ink, true);
            text(g, theme, "OPEN SOURCE", {367, 45, 93, 16}, 13, ink, true);
            text(g, theme, "PATCH MEMORY", {488, 7, 135, 18}, 12, ink, true,
                 juce::Justification::centred);
            bezel(g, theme, {508, 28, 89, 34}, theme.display, 4);
            text(g, theme, parameters.getScene() == 0 ? "SCENE A" : "SCENE B", {511, 30, 83, 28},
                 16, theme.secondary, true, juce::Justification::centred);
            text(g, theme, "CURRENT PATCH", {695, 7, 213, 17}, 12, ink, true);
            text(g, theme, "SURGE XT  /  HYBRID ENGINE", {1280, 52, 192, 16}, 11, ink);
            for (int i = 0; i < 5; ++i)
            {
                g.setColour(theme.display);
                g.fillRoundedRectangle({1482, 14 + i * 10.f, 70, 5}, 2.5f);
            }
        }
        else if (style == "tokyo")
        {
            logo(21, 17, 29, 36);
            brand({66, 7, 215, 49}, 43);
            text(g, theme, "HYBRID SYNTHESIZER", {305, 10, 164, 22}, 14, ink);
            text(g, theme, "SURGE XT", {305, 36, 163, 20}, 12, theme.muted);
            text(g, theme, "SCENE", {500, 3, 107, 18}, 15, ink);
            text(g, theme, "PRESET", {692, 3, 126, 18}, 15, ink);
            text(g, theme, "VOICES  " + parameters.display("scene.polylimit"), {1133, 11, 114, 21},
                 12, ink);
            text(g, theme, "SCENE  " + juce::String(parameters.getScene() ? "B" : "A"),
                 {1133, 36, 114, 20}, 12, ink);
            headerMeters(g, {1312, 15, 174, 33}, true);
        }
        else if (style == "aurora")
        {
            logo(19, 13, 61, 34);
            brand({94, 1, 191, 38}, 34);
            text(g, theme, "Hybrid Synthesizer - Liquid Aurora", {95, 38, 226, 18}, 13,
                 theme.muted);
            text(g, theme, "Voices  " + parameters.display("scene.polylimit"), {1321, 10, 109, 18},
                 12, theme.muted);
            text(g, theme, "Output", {1321, 34, 76, 17}, 12, theme.muted);
            headerMeters(g, {1442, 16, 121, 31}, true);
        }
        else if (style == "clear")
        {
            logo(17, 14, 50, 34);
            brand({79, 5, 139, 34}, 32, false);
            text(g, theme, "Hybrid Synthesizer", {80, 38, 183, 19}, 14, ink);
            text(g, theme, "SURGE XT", {229, 9, 80, 18}, 11, theme.muted, true);
            text(g, theme, "PRESET", {786, 1, 120, 17}, 12, ink, true);
            text(g, theme, "Voices  " + parameters.display("scene.polylimit"), {1241, 10, 207, 20},
                 13, ink, true);
            headerMeters(g, {1241, 36, 212, 13}, true);
        }
        else if (style == "flow")
        {
            logo(24, 17, 24, 26);
            brand({70, 3, 169, 35}, 31);
            text(g, theme, "H Y B R I D  S Y N T H E S I Z E R", {70, 37, 222, 16}, 10,
                 theme.accent);
            text(g, theme, "PRESET", {772, 6, 172, 13}, 10, theme.muted, false,
                 juce::Justification::centred);
            text(g, theme, parameters.display("scene.mode"), {355, 33, 223, 23}, 13, theme.muted,
                 false, juce::Justification::centred);
        }
        else
        {
            logo(25, 8, 31, 28);
            brand({65, 3, 146, 30}, 27);
            text(g, theme, "H Y B R I D  S Y N T H E S I Z E R", {66, 31, 174, 12}, 8, ink);
            text(g, theme, "Live Performance Command Deck", {266, 13, 237, 22}, 13, theme.secondary,
                 true);
            text(g, theme, "OUTPUT", {1168, 12, 59, 20}, 10, theme.muted);
            headerMeters(g, {1227, 17, 62, 11}, true);
            text(g, theme, "Scene mode", {1156, 62, 202, 17}, 11, theme.muted, false,
                 juce::Justification::centred);
            text(g, theme, parameters.display("scene.mode"), {1156, 81, 202, 20}, 15, theme.accent,
                 true, juce::Justification::centred);
        }
        if (!headerPresetBounds.isEmpty())
        {
            if (!headerCategoryBounds.isEmpty())
            {
                const auto category = juce::String(processor.surge->storage.getPatch().category);
                text(g, theme, category.isEmpty() ? "Current patch" : category,
                     headerCategoryBounds, style == "ivory" ? 13.f : 11.f,
                     style == "ivory"     ? ink
                     : style == "bauhaus" ? juce::Colour(0xfff6e5c3)
                                          : theme.muted,
                     false, juce::Justification::centred);
            }
        }
    }
    void plotBase(juce::Graphics &g, Rect r, bool pale = false)
    {
        g.setColour(pale ? theme.panel.brighter(.03f) : theme.display);
        g.fillRoundedRectangle(r, 3);
        g.setColour(theme.border.withAlpha(.4f));
        g.drawRoundedRectangle(r, 3, 1);
        Visuals::grid(g, r.reduced(2), theme.secondary.withAlpha(theme.light() ? .16f : .1f));
    }
    void unavailableOscillator(juce::Graphics &g, Rect r, int index)
    {
        auto caption = visuals.oscillatorStatus[juce::jlimit(0, 2, index)];
        if (caption.isEmpty())
            caption = "Oscillator preview unavailable";
        g.setColour(theme.display.contrasting(.8f));
        g.setFont(theme.type(r.getWidth() < 120 ? 8.f : 10.f));
        g.drawFittedText(caption, r.reduced(3).toNearestInt(), juce::Justification::centred,
                         r.getHeight() >= 35 ? 3 : 1, .8f);
    }
    void drawEnvelope(juce::Graphics &g, Rect r, int index, juce::Colour color,
                      bool lfoEnvelope = false)
    {
        plotBase(g, r, theme.style == "clear");
        r = r.reduced(8);
        const juce::String prefix = lfoEnvelope
                                        ? "lfo" + juce::String(std::clamp(index, 1, 12)) + "."
                                        : "env" + juce::String(std::clamp(index, 1, 2)) + ".";
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
        const bool miniature = r.getHeight() < 65;
        const auto frame = r;
        r = miniature ? r.reduced(4, 3) : r.reduced(7, 15);
        const auto &curve = filterCurves[juce::jlimit(0, 1, index - 1)];
        if (!curve.available || curve.values.size() < 2)
        {
            text(g, theme, curve.caption, r, 10, theme.display.contrasting(.8f), false,
                 juce::Justification::centred);
            return;
        }
        if (base && !miniature)
        {
            const float frequencies[]{20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
            for (float frequency : frequencies)
            {
                const auto x =
                    r.getX() + std::log(frequency / filterPreviewMinimumHz) /
                                   std::log(filterPreviewMaximumHz / filterPreviewMinimumHz) *
                                   r.getWidth();
                g.setColour(theme.secondary.withAlpha(.10f));
                g.drawLine(x, r.getY(), x, r.getBottom(), .7f);
                const auto label = frequency >= 1000 ? juce::String(frequency / 1000.f, 0) + "k"
                                                     : juce::String(static_cast<int>(frequency));
                text(g, theme, label, {x - 13, frame.getBottom() - 13, 26, 12},
                     r.getWidth() > 400 ? 11.f : 9.f, theme.display.contrasting(.80f), false,
                     juce::Justification::centred);
            }
            for (float db = filterPreviewMinimumDb; db <= filterPreviewMaximumDb; db += 12)
            {
                const auto y =
                    r.getBottom() - (db - filterPreviewMinimumDb) /
                                        (filterPreviewMaximumDb - filterPreviewMinimumDb) *
                                        r.getHeight();
                g.setColour(theme.secondary.withAlpha(db == 0 ? .18f : .09f));
                g.drawLine(r.getX(), y, r.getRight(), y, db == 0 ? 1.f : .7f);
            }
        }
        juce::Path p;
        for (size_t i = 0; i < curve.values.size(); ++i)
        {
            const float x = static_cast<float>(i) / static_cast<float>(curve.values.size() - 1);
            const float y = 1.f - std::clamp((curve.values[i] - filterPreviewMinimumDb) /
                                                 (filterPreviewMaximumDb - filterPreviewMinimumDb),
                                             0.f, 1.f);
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
        if (!miniature)
        {
            const auto binding = "filter" + juce::String(index) + ".cutoff";
            if (const auto *parameter = parameters.parameter(binding))
            {
                auto cutoff = parameter->val.f;
                const auto &scene =
                    processor.surge->storage.getPatch().scene[parameters.getScene()];
                if (index == 2 && scene.f2_cutoff_is_offset.val.b)
                    cutoff += scene.filterunit[0].cutoff.val.f;
                const auto hz = 440.f * std::pow(2.f, cutoff / 12.f);
                const auto position =
                    juce::jlimit(0.f, 1.f,
                                 std::log(hz / filterPreviewMinimumHz) /
                                     std::log(filterPreviewMaximumHz / filterPreviewMinimumHz));
                const auto at = static_cast<size_t>(position * (curve.values.size() - 1));
                const auto y = r.getBottom() -
                               r.getHeight() *
                                   std::clamp((curve.values[at] - filterPreviewMinimumDb) /
                                                  (filterPreviewMaximumDb - filterPreviewMinimumDb),
                                              0.f, 1.f);
                const auto x = r.getX() + position * r.getWidth();
                g.setColour(theme.display);
                g.fillEllipse(x - 3.7f, y - 3.7f, 7.4f, 7.4f);
                g.setColour(color);
                g.drawEllipse(x - 3.7f, y - 3.7f, 7.4f, 7.4f, 1.5f);
            }
        }
    }
    void drawLfo(juce::Graphics &g, Rect r, int index)
    {
        plotBase(g, r, theme.style == "clear");
        r = r.reduced(6);
        const auto &curve = lfoCurves[juce::jlimit(0, 11, index - 1)];
        if (!curve.available || curve.values.size() < 2)
        {
            text(g, theme, curve.caption, r, 10,
                 theme.style == "clear" ? theme.text : theme.display.contrasting(.8f), false,
                 juce::Justification::centred);
            return;
        }
        juce::Path p;
        for (size_t i = 0; i < curve.values.size(); ++i)
        {
            const float x = static_cast<float>(i) / static_cast<float>(curve.values.size() - 1),
                        y = .5f - .40f * curve.values[i];
            if (!i)
                p.startNewSubPath(r.getX(), r.getY() + y * r.getHeight());
            else
                p.lineTo(r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight());
        }
        g.setColour(theme.style == "clear" ? theme.accent : theme.secondary);
        g.strokePath(p, juce::PathStrokeType(1.8f));
    }
    void drawSpectrum(juce::Graphics &g, Rect r, bool history)
    {
        r = r.reduced(6);
        const auto captionInk = theme.display.contrasting(.8f);
        if (spectrumDb.empty())
        {
            text(g, theme, "Awaiting live audio", r, 11, captionInk, false,
                 juce::Justification::centred);
            return;
        }
        auto graph = r.withTrimmedBottom(15);
        if (history)
        {
            // Columns are actual captured FFT frames, oldest left. Empty time stays blank.
            const auto columnWidth = graph.getWidth() / 64.f;
            for (size_t frame = 0; frame < spectrumHistory.size(); ++frame)
            {
                const auto &bins = spectrumHistory[frame];
                for (size_t bin = 0; bin < bins.size(); ++bin)
                {
                    const float strength = std::clamp((bins[bin] + 90.f) / 78.f, 0.f, 1.f);
                    auto color = theme.display.interpolatedWith(theme.secondary,
                                                                std::min(1.f, strength * 1.6f));
                    if (strength > .6f)
                        color = color.interpolatedWith(theme.accent, (strength - .6f) * 2.5f);
                    g.setColour(color);
                    g.fillRect(Rect(graph.getX() + frame * columnWidth,
                                    graph.getBottom() - (bin + 1) * graph.getHeight() / bins.size(),
                                    columnWidth + .5f, graph.getHeight() / bins.size() + .5f));
                }
            }
        }
        else
        {
            juce::Path path;
            for (size_t i = 0; i < spectrumDb.size(); ++i)
            {
                const auto x = graph.getX() + graph.getWidth() * i / (spectrumDb.size() - 1);
                const auto y =
                    graph.getBottom() -
                    graph.getHeight() * std::clamp((spectrumDb[i] + 96.f) / 108.f, 0.f, 1.f);
                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }
            auto fill = path;
            fill.lineTo(graph.getBottomRight());
            fill.lineTo(graph.getBottomLeft());
            fill.closeSubPath();
            const auto ink = theme.style == "clear" ? theme.accent : theme.secondary;
            g.setGradientFill(juce::ColourGradient(ink.withAlpha(.45f), graph.getTopLeft(),
                                                   ink.withAlpha(.025f), graph.getBottomLeft(),
                                                   false));
            g.fillPath(fill);
            g.setColour(ink);
            g.strokePath(path, juce::PathStrokeType(1.4f));
        }
        text(g, theme,
             utf8(history ? "Live STFT • time → / log frequency ↑"
                          : "20 Hz     100     1k     10k     • dBFS"),
             r.removeFromBottom(13), 9, captionInk, false, juce::Justification::centred);
    }
    void drawMeters(juce::Graphics &g, Rect r, bool horizontal = false)
    {
        if (horizontal)
        {
            bezel(g, theme, r, theme.display, 3);
            auto channels = r.reduced(5);
            const float rowHeight = channels.getHeight() * .5f;
            for (int ch = 0; ch < 2; ++ch)
            {
                auto row = channels.removeFromTop(rowHeight);
                const float peak = ch ? visuals.right : visuals.left;
                const float db = peak > 0 ? 20.f * std::log10(peak) : -100.f;
                const float proportion = std::clamp((db + 60.f) / 60.f, 0.f, 1.f);
                text(g, theme, ch ? "R" : "L", row.removeFromLeft(14), 11, theme.text, true);
                if (row.getWidth() > 160)
                    text(g, theme, peak > 0 ? juce::String(db, 1) + " dB" : utf8("−∞ dB"),
                         row.removeFromRight(48), 10, theme.muted, false,
                         juce::Justification::centredRight);
                auto bar = row.withSizeKeepingCentre(row.getWidth(),
                                                     std::min(13.f, row.getHeight() - 4.f));
                for (int segment = 0; segment < 24; ++segment)
                {
                    auto colour = segment > 21   ? juce::Colour(0xffff6831)
                                  : segment > 16 ? theme.accent
                                                 : juce::Colour(0xffa6d93d);
                    g.setColour(colour.withAlpha(segment / 24.f < proportion ? 1.f : .13f));
                    g.fillRect(Rect(bar.getX() + segment * bar.getWidth() / 24.f, bar.getY(),
                                    std::max(.5f, bar.getWidth() / 24.f - 1.f), bar.getHeight()));
                }
            }
            return;
        }
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
        auto titleBounds = r.reduced(12, 3).withHeight(25);
        if (kind == "matrix")
            titleBounds.setWidth(std::max(25.f, titleBounds.getWidth() - 112.f));
        else if (kind == "effects")
            titleBounds.setWidth(std::max(25.f, titleBounds.getWidth() - 110.f));
        else if (kind == "analyzer")
            titleBounds.setWidth(
                std::max(25.f, titleBounds.getWidth() - (r.getWidth() < 320 ? 54.f : 92.f)));
        text(g, theme, title, titleBounds, number(v, "fontSize", theme.style == "ivory" ? 18 : 15),
             titleColor, true);
        auto content = r.reduced(10).withTrimmedTop(24);
        auto pr =
            arrayRect(v["plot"], content.withHeight(std::max(25.f, content.getHeight() * .56f)));
        const auto curveInk =
            theme.style == "ivory" || theme.style == "space" || theme.style == "tokyo"
                ? theme.secondary
                : theme.accent;
        const auto waveformInk = theme.style == "clear" ? theme.accent : theme.secondary;
        if (kind == "oscillator")
        {
            if (!v.hasProperty("plot"))
            {
                if (r.getWidth() > r.getHeight() * 1.7f)
                    pr = {r.getX() + 10, r.getY() + 43, r.getWidth() * .43f, r.getHeight() - 54};
                else
                    pr = {r.getX() + 10, r.getY() + 61, r.getWidth() - 20, r.getHeight() * .38f};
            }
            if (theme.style != "aurora")
                plotBase(g, pr);
            if (index <= 3)
            {
                const bool layers =
                    variant == "terrain" || theme.style == "aurora" || theme.style == "live";
                if (layers && visuals.wavetable[index - 1].available())
                    Visuals::wavetable(g, pr.reduced(5), visuals.wavetable[index - 1], waveformInk,
                                       theme.accent,
                                       theme.style == "aurora" || theme.style == "live"
                                           ? Visuals::WavetableView::ribbons
                                           : Visuals::WavetableView::perspective);
                else
                    Visuals::waveform(g, pr.reduced(5), visuals.oscillator[index - 1], waveformInk,
                                      theme.style == "tokyo" ? waveformInk : theme.accent,
                                      variant == "terrain");
                if (visuals.oscillator[index - 1].empty())
                    unavailableOscillator(g, pr, index - 1);
            }
        }
        else if (kind == "waveform")
        {
            pr = arrayRect(v["plot"], content.withTrimmedBottom(variant == "terrain" ? 20 : 3));
            if (theme.style != "aurora" && theme.style != "live")
                plotBase(g, pr);
            if (visuals.wavetable[0].available())
                Visuals::wavetable(
                    g, pr.reduced(8), visuals.wavetable[0], theme.secondary, theme.accent,
                    theme.style == "aurora" || theme.style == "live" || theme.style == "bauhaus"
                        ? Visuals::WavetableView::ribbons
                        : Visuals::WavetableView::perspective);
            else
                Visuals::waveform(g, pr.reduced(8), visuals.oscillator[0], theme.secondary,
                                  theme.accent,
                                  variant == "terrain" || theme.style == "bauhaus" ||
                                      theme.style == "live" || theme.style == "aurora");
            if (visuals.oscillator[0].empty())
                unavailableOscillator(g, pr.withTrimmedBottom(22), 0);
        }
        else if (kind == "inspector" && theme.style == "clear")
        {
            if (routes.empty())
                text(g, theme, "Choose a modulation route to inspect", content, 14, theme.muted,
                     false, juce::Justification::centred);
            else
            {
                const auto found =
                    std::find_if(routes.begin(), routes.end(), [this](const RouteRow &candidate) {
                        return candidate.key == inspectedRoute;
                    });
                const auto &route = found == routes.end() ? routes.front() : *found;
                auto heading = Rect(r.getX() + 14, r.getY() + 39, r.getWidth() - 28, 32);
                g.setColour(theme.border.withAlpha(.12f));
                g.fillRoundedRectangle(heading, 5);
                text(g, theme, route.source + utf8("   →   ") + route.destination,
                     heading.reduced(9, 0), 15, theme.text, true);
                auto tile = pr;
                bezel(g, theme, tile, theme.accent, 5);
                const auto trace = tile.reduced(10, 15);
                g.setColour(juce::Colours::white.withAlpha(.85f));
                const auto depth = std::clamp(route.normalizedDepth, -1.f, 1.f);
                g.drawLine(trace.getX(), trace.getCentreY() + depth * trace.getHeight() * .5f,
                           trace.getRight(), trace.getCentreY() - depth * trace.getHeight() * .5f,
                           2);
                text(g, theme, "Amount", {tile.getRight() + 16, tile.getY() - 1, 110, 20}, 14,
                     theme.text, true);
                text(g, theme, route.amount, {tile.getRight() + 16, tile.getY() + 20, 260, 32}, 22,
                     theme.text, true);
                text(g, theme, route.muted ? "Muted route" : "Scaled to destination range",
                     {r.getX() + 14, r.getBottom() - 41, r.getWidth() - 28, 25}, 13, theme.text);
            }
        }
        else if (kind == "filter" || kind == "inspector")
        {
            drawFilter(g, pr, index,
                       theme.style == "bauhaus" ? theme.secondary
                       : index == 2             ? theme.secondary
                                                : curveInk);
            if (variant == "combined")
                drawFilter(g, pr, 2, theme.secondary, false);
        }
        else if (kind == "envelope" || kind == "amp")
            drawEnvelope(g, pr, index, curveInk);
        else if (kind == "lfo")
        {
            if (static_cast<bool>(v["envelope"]))
                drawEnvelope(g, pr, index, curveInk, true);
            else
                drawLfo(g, pr, index);
        }
        else if (kind == "analyzer")
        {
            auto meter = content.removeFromRight(std::min(45.f, content.getWidth() * .15f));
            drawMeters(g, meter.reduced(4));
            pr = arrayRect(v["plot"], content.reduced(3));
            plotBase(g, pr);
            if (variant != "scope")
                drawSpectrum(g, pr, variant == "spectrogram");
            else if (!visuals.output.empty())
                Visuals::waveform(g, pr.reduced(5), visuals.output, theme.secondary, theme.accent,
                                  false);
            else
            {
                g.setColour(theme.secondary.withAlpha(.5f));
                g.drawLine(pr.getX() + 5, pr.getCentreY(), pr.getRight() - 5, pr.getCentreY());
                text(g, theme, "Awaiting live output", pr.reduced(9).removeFromBottom(23), 10,
                     theme.muted, false, juce::Justification::centred);
            }
        }
        else if (kind == "output")
        {
            if (theme.style == "space")
                drawMeters(g,
                           content.removeFromRight(std::min(108.f, content.getWidth() * .46f))
                               .reduced(2, 13),
                           true);
            else
                drawMeters(
                    g,
                    content.removeFromRight(std::min(58.f, content.getWidth() * .28f)).reduced(4));
        }
        else if (kind == "matrix")
        {
            if (theme.style == "space")
            {
                g.setColour(theme.display);
                g.fillRoundedRectangle(content.expanded(3), 3);
            }
            const bool accessible = theme.style == "clear";
            const bool twoColumns = theme.style == "tokyo" && variant == "horizontal";
            const int columns = twoColumns ? 2 : 1;
            const int maximumRows = accessible || theme.style == "space" || twoColumns ? 8 : 6;
            const float rowText = accessible ? 13.f : 11.f;
            const float gap = twoColumns ? 14.f : 0.f;
            const float width = (content.getWidth() - gap) / columns;
            const float h = juce::jlimit(accessible ? 24.f : 16.f, accessible ? 29.f : 30.f,
                                         (content.getHeight() - 22.f) /
                                             static_cast<float>(maximumRows / columns));
            const auto rowsPerColumn =
                std::min(maximumRows / columns,
                         static_cast<int>(std::max(0.f, content.getHeight() - 22.f) / h));
            auto fields = [&](Rect row) {
                std::array<Rect, 4> result;
                result[0] = row.removeFromLeft(20);
                const auto fieldWidth = row.getWidth();
                result[1] = row.removeFromLeft(fieldWidth * (accessible ? .26f : .29f));
                if (accessible)
                {
                    result[3] = row.removeFromLeft(fieldWidth * .44f);
                    result[2] = row;
                }
                else
                {
                    result[2] = row.removeFromLeft(fieldWidth * .25f);
                    result[3] = row;
                }
                return result;
            };
            for (int column = 0; column < columns; ++column)
            {
                auto table = Rect(content.getX() + column * (width + gap), content.getY(), width,
                                  content.getHeight());
                const auto heading = fields(table.removeFromTop(22));
                text(g, theme, "#", heading[0], 10, theme.muted, true);
                text(g, theme, "SOURCE", heading[1], accessible ? 11.f : 10.f, theme.muted, true);
                text(g, theme, "AMOUNT", heading[2], accessible ? 11.f : 10.f, theme.muted, true);
                text(g, theme, "DESTINATION", heading[3].reduced(5, 0), accessible ? 11.f : 10.f,
                     theme.muted, true);
                for (int rowIndex = 0; rowIndex < rowsPerColumn; ++rowIndex)
                {
                    const int index = column * rowsPerColumn + rowIndex;
                    auto row = table.removeFromTop(h);
                    const auto cell = fields(row);
                    g.setColour(theme.border.withAlpha(.35f));
                    g.drawHorizontalLine(static_cast<int>(row.getBottom()), row.getX(),
                                         row.getRight());
                    text(g, theme, juce::String(index + 1), cell[0], accessible ? 12.f : 10.f,
                         theme.muted);
                    if (index < static_cast<int>(routes.size()))
                    {
                        const auto &route = routes[index];
                        text(g, theme, route.source, cell[1], rowText,
                             route.muted ? theme.muted : theme.text);
                        auto amount = cell[2];
                        auto bar =
                            accessible ? amount.withTrimmedLeft(amount.getWidth() * .48f) : amount;
                        bar = bar.reduced(3, std::max(3.f, (bar.getHeight() - 7.f) * .5f));
                        g.setColour(theme.border);
                        g.fillRoundedRectangle(bar, 2);
                        bar.setWidth(bar.getWidth() *
                                     std::min(1.f, std::abs(route.normalizedDepth)));
                        g.setColour(route.muted ? theme.muted : theme.accent);
                        g.fillRoundedRectangle(bar, 2);
                        if (accessible)
                            amount.setWidth(amount.getWidth() * .48f);
                        text(g, theme, route.amount, amount, accessible ? 12.f : 10.f, theme.text,
                             false, juce::Justification::centred);
                        text(g, theme, route.destination, cell[3].reduced(5, 0), rowText,
                             theme.text);
                    }
                    else
                        text(g, theme,
                             utf8(index == 0 ? "No routes in this patch — use Edit routes" : "—"),
                             row.withTrimmedLeft(20), rowText, theme.muted);
                }
            }
        }
        else if (kind == "browser")
        {
            // Search and rows are native, accessible child controls populated from
            // the actual installed library. Never paint an inert mock preset list.
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
                text(g, theme, juce::String(i + 1), s.withHeight(15), 10,
                     theme.display.contrasting(.65f), false, juce::Justification::centred);
                text(g, theme, parameters.display("fx" + juce::String(i + 1) + ".type"),
                     s.withTrimmedTop(16), 10, theme.display.contrasting(.9f), false,
                     juce::Justification::centred);
            }
            if (content.getHeight() > 36)
                text(g, theme, "16 native FX slots available in the full editor",
                     content.removeFromBottom(18), 10, theme.muted);
        }
        else if (kind == "keyboard")
        {
            text(g, theme, utf8("Playable MIDI keyboard  •  native scene modes available above"),
                 content.withHeight(23), 12, theme.muted);
        }
        if (theme.style == "space" || theme.style == "tokyo")
        {
            for (float x : {r.getX() + 5, r.getRight() - 5})
            {
                for (float y : {r.getY() + 6, r.getBottom() - 6})
                {
                    const float size = theme.style == "space" ? 7.f : 5.f;
                    Rect screw(x - size * .5f, y - size * .5f, size, size);
                    g.setColour(juce::Colours::black.withAlpha(.6f));
                    g.fillEllipse(screw.expanded(.6f).translated(0, .6f));
                    g.setGradientFill(juce::ColourGradient(
                        theme.border.brighter(.25f), screw.getTopLeft(), theme.border.darker(.4f),
                        screw.getBottomRight(), false));
                    g.fillEllipse(screw);
                    g.setColour(theme.display);
                    g.drawLine(x - size * .27f, y + .7f, x + size * .27f, y - .7f, 1.2f);
                }
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
                text(g, theme, labels[j], n, 10, theme.display.contrasting(.9f), false,
                     juce::Justification::centred);
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
                text(g, theme, labels[i], n, 11, theme.display.contrasting(.9f), false,
                     juce::Justification::centred);
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
            if (title == "MOD SOURCES" && n.getWidth() < 110)
            {
                g.setColour(theme.text);
                g.setFont(theme.type(13, true));
                g.drawFittedText("MOD\nSOURCES", n.reduced(6).toNearestInt(),
                                 juce::Justification::centred, 2, .85f);
            }
            else
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
            auto preview = n.withTrimmedLeft(n.getWidth() * .52f).withHeight(22).reduced(4, 2);
            if (!visuals.oscillator[i].empty())
                Visuals::waveform(g, preview, visuals.oscillator[i], theme.accent, theme.secondary,
                                  false);
            else
                unavailableOscillator(g, preview, i);
            wire({n.getRight(), n.getCentreY()}, {mix.getX(), mix.getCentreY()});
        }
        auto f1 = node("filter1", "FILTER 1", .45f, .15f, .18f, .15f),
             f2 = node("filter2", "FILTER 2", .45f, .43f, .18f, .15f);
        auto amp = node("amp", "AMP", .70f, .29f, .10f, .17f),
             fx = node("fx", "FX CHAIN", .86f, .22f, .11f, .30f);
        for (int index = 0; index < 2; ++index)
        {
            const auto &curve = filterCurves[index];
            auto plot = (index == 0 ? f1 : f2).withTrimmedTop(58).reduced(8, 2);
            if (plot.getHeight() > 4 && curve.available && curve.values.size() > 1)
            {
                juce::Path trace;
                for (size_t i = 0; i < curve.values.size(); ++i)
                {
                    const float x = plot.getX() + plot.getWidth() * i / (curve.values.size() - 1);
                    const float y =
                        plot.getBottom() -
                        plot.getHeight() *
                            std::clamp((curve.values[i] - filterPreviewMinimumDb) /
                                           (filterPreviewMaximumDb - filterPreviewMinimumDb),
                                       0.f, 1.f);
                    if (i == 0)
                        trace.startNewSubPath(x, y);
                    else
                        trace.lineTo(x, y);
                }
                g.setColour(theme.accent);
                g.strokePath(trace, juce::PathStrokeType(1.2f));
            }
        }
        drawEnvelope(g, amp.withTrimmedTop(22).reduced(4), 1, theme.accent);
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
        auto modbus = node("modbus", "MOD BUS / ROUTING", .46f, .80f, .30f, .15f);
        text(g, theme, juce::String(static_cast<int>(routes.size())) + " route entries shown",
             modbus.reduced(8).withTrimmedTop(20), 11, theme.muted);
        text(g, theme,
             "Routing: " + parameters.display("routing") + utf8("  •  schematic overview"),
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
Workspace::~Workspace()
{
    stopTimer();
    impl->setAudioSubscription(false);
    impl->cancelInteractions();
}
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
void Workspace::selectScene(int scene, bool notify)
{
    scene = juce::jlimit(0, 1, scene);
    if (impl->parameters.getScene() == scene)
    {
        refresh();
        return;
    }
    impl->cancelInteractions();
    impl->parameters.setScene(scene);
    impl->ticks = 0;
    refresh();
    if (notify && onAction)
        onAction(scene == 0 ? "scene-a" : "scene-b");
}
void Workspace::timerCallback()
{
    if (isShowing())
        refresh();
}
void Workspace::visibilityChanged()
{
    impl->setAudioSubscription(isShowing());
    if (!isShowing())
        impl->cancelInteractions();
}
void Workspace::resized()
{
    impl->cancelInteractions();
    impl->scale = std::min(getWidth() / 1586.f, getHeight() / 992.f);
    impl->offset = {(getWidth() - 1586 * impl->scale) * .5f,
                    (getHeight() - 992 * impl->scale) * .5f};
    impl->canvas.setBounds(0, 0, 1586, 992);
    impl->canvas.setTransform(juce::AffineTransform::scale(impl->scale).translated(impl->offset));
    for (auto &p : impl->placements)
        p.first->setBounds(p.second.toNearestInt());
    impl->layoutHeader();
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
        impl->cancelInteractions();
        if (onFullEditor)
            onFullEditor();
        return true;
    }
    return false;
}
} // namespace Surge::GUI::Reference
