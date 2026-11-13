/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#include "Workspace.h"
#include "Manifest.h"
#include "ParameterBridge.h"
#include "PreviewCurves.h"
#include "Visuals.h"
#include "WorkspaceData.h"
#include "WavetableSelection.h"
#include "ModulationInteraction.h"
#include "MatrixLayout.h"
#include "ReferenceOrnaments.h"
#include "SurgeSynthProcessor.h"
#include "RuntimeFont.h"
#include "version.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <charconv>
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
    if (value.endsWith(" Hz"))
    {
        const auto hz = value.dropLastCharacters(3).getDoubleValue();
        if (std::isfinite(hz) && std::abs(hz) >= 1000.0)
            return juce::String(hz / 1000.0, 2) + " kHz";
    }
    return value.replace(".00 dB", ".0 dB")
        .replace(".00 %", " %")
        .replace(" semitones", " st")
        .replace(" semitone", " st")
        .replace(" cents", " ct")
        .replace(" milliseconds", " ms")
        .replace(" seconds", " s")
        .replace(" (Center)", " C")
        .replace(" (Left)", " L")
        .replace(" (Right)", " R");
}
juce::String compactPaintLabel(const juce::String &label)
{
    // Standard face abbreviations preserve the native meaning while avoiding
    // horizontally crushed lettering. Tooltips and accessibility keep the full
    // engine label, and user-authored macro names never pass through this map.
    if (label == "Unison Voices") return "Unison";
    if (label == "Unison Detune") return "Detune";
    if (label == "Skew Vertical") return "V Skew";
    if (label == "Skew Horizontal") return "H Skew";
    if (label == "M1 Amount") return "M1 Amt";
    if (label == "M2 Amount") return "M2 Amt";
    if (label == "M1/2 Offset") return "FM Offset";
    return label;
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
    // These are the same licensed, embedded typefaces used by the full editor.
    // Sharing them avoids loading fonts for every copied knob/entry theme and
    // keeps sans/mono metrics stable when the host's default font changes.
    juce::SharedResourcePointer<Surge::GUI::FontManager> fonts;
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
        // The manifest sizes describe the reference's visible lettering. JUCE's
        // height includes the embedded fonts' ascent/descent, so using the raw
        // number made a 15 px section heading only 9–10 px high on screen.
        // Keep the actual embedded faces, with a measured optical correction;
        // fitted text still respects the component's available rectangle.
        const auto opticalSize = size * (font == "serif" ? 1.05f : font == "mono" ? 1.12f : 1.18f);
        const auto weight = bold ? juce::Font::bold : juce::Font::plain;
        if (font == "mono")
            return fonts->getFiraMonoAtSize(opticalSize, weight).withHeight(opticalSize);
        if (font != "serif")
            return fonts->getLatoAtSize(opticalSize, weight).withHeight(opticalSize);
        const auto family = juce::Font::getDefaultSerifFontName();
#if JUCE_MAJOR_VERSION >= 8
        return juce::Font(juce::FontOptions(family, opticalSize, weight));
#else
        return juce::Font(family, opticalSize, weight);
#endif
    }
};
void text(juce::Graphics &g, const Theme &t, const juce::String &s, Rect r, float size,
          juce::Colour color, bool bold = false, int align = juce::Justification::centredLeft)
{
    auto font = t.type(size, bold);
    if (t.style == "ivory" && bold && size >= 16.f)
        font = t.type(size, true).withExtraKerningFactor(.065f);
    else if (t.style == "space" && bold && size >= 14.f)
    {
        // Dark ink marks the brass section plates; cream ink marks the olive
        // instrument wells. Keep those inner headings upright regardless of
        // their current native label or selected effect name.
        const auto style = color.getPerceivedBrightness() < .4f
                               ? juce::Font::bold | juce::Font::italic
                               : juce::Font::bold;
        font = t.fonts
                   ->getLatoAtSize(size, static_cast<juce::Font::FontStyleFlags>(style))
                   .withHeight(size * 1.18f)
                   .withHorizontalScale(.9f);
    }
    else if (t.style == "tokyo" && bold)
        font = font.withHorizontalScale(.9f);
    g.setFont(font);
    g.setColour(color);
    g.drawFittedText(s, r.toNearestInt(), align, 1, 0.7f);
}

const juce::Image &materialGrain(bool brushed)
{
    // Immutable cosmetic grain, generated once without the synth's RNG. It is
    // not sampled from reference art and contains no signal or parameter state.
    static const auto textures = [] {
        std::array<juce::Image, 2> result;
        for (size_t style = 0; style < result.size(); ++style)
        {
            result[style] = juce::Image(juce::Image::ARGB, 512, 256, true);
            juce::Image::BitmapData pixels(result[style], juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < pixels.height; ++y)
                for (int x = 0; x < pixels.width; ++x)
                {
                    // Avalanche both coordinates; the old linear seed made a
                    // diagonal lattice visible on the otherwise smooth panels.
                    // Longer horizontal fibres are cosmetic brushed metal.
                    auto hash = static_cast<unsigned>((style == 1 ? x / 24 : x) + 0x9e3779b9u);
                    hash ^= static_cast<unsigned>(y + 1) * 0x85ebca6bu;
                    hash ^= hash >> 16;
                    hash *= 0x7feb352du;
                    hash ^= hash >> 15;
                    hash *= 0x846ca68bu;
                    hash ^= hash >> 16;
                    const auto shade = hash & 1u ? juce::Colours::white : juce::Colours::black;
                    const auto alpha = .08f + static_cast<float>((hash >> 1) & 31u) / 160.f;
                    pixels.setPixelColour(x, y, shade.withAlpha(alpha));
                }
        }
        return result;
    }();
    return textures[brushed ? 1 : 0];
}

void bezel(juce::Graphics &g, const Theme &t, Rect r, juce::Colour fill, float radius = -1)
{
    if (radius < 0)
        radius = t.radius;
    const bool panel = r.getWidth() > 125 && r.getHeight() > 55;
    const bool metal = t.style == "space";
    const bool olive = metal && fill == t.panel;
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
        top = juce::Colour(0xff2b1e43);
        bottom = juce::Colour(0xff17142b);
    }
    else if (olive)
    {
        top = fill.interpolatedWith(juce::Colour(0xff253011), .38f);
        bottom = fill.interpolatedWith(juce::Colour(0xff202715), .15f);
    }
    else if (metal && fill != t.display)
    {
        top = fill.interpolatedWith(juce::Colour(0xffedce9b), .11f);
        bottom = fill.interpolatedWith(juce::Colour(0xff5b411f), .32f);
    }
    auto material = juce::ColourGradient(top, r.getTopLeft(), bottom, r.getBottomLeft(), false);
    if (panel && olive)
    {
        // A dark olive title lip opens into the warmer instrument body. Its
        // height follows the physical panel, not the text or processor state.
        const auto lip = std::min(.22f, 27.f / r.getHeight());
        material.addColour(lip, top);
        material.addColour(lip + .025f,
                           fill.interpolatedWith(juce::Colour(0xff87763d), .10f));
        material.addColour(.72, fill);
        material.addColour(.98, bottom);
    }
    else if (panel && (metal || ivory || t.style == "tokyo"))
    {
        material.addColour(.045, top.interpolatedWith(juce::Colours::white, metal ? .09f : .04f));
        material.addColour(.14, top);
        material.addColour(.86, bottom);
        material.addColour(.98, bottom.interpolatedWith(juce::Colours::black, .06f));
    }
    g.setGradientFill(material);
    g.fillRoundedRectangle(r, radius);
    if (panel && fill != t.display &&
        (metal || ivory || t.style == "tokyo" || t.style == "precision"))
    {
        juce::Graphics::ScopedSaveState save(g);
        juce::Path clip;
        clip.addRoundedRectangle(r.reduced(2), std::max(0.f, radius - 1));
        g.reduceClipRegion(clip);
        g.setTiledImageFill(materialGrain(metal), 0, 0,
                            metal ? (olive ? .07f : .20f) : ivory ? .028f : .012f);
        g.fillRect(r);
        if (metal)
        {
            // A broad reflected band breaks the old uniform horizontal stripes.
            // It follows the physical panel, not its contents or the audio clock.
            const auto strength = olive ? .18f : .55f;
            juce::ColourGradient sheen(juce::Colours::white.withAlpha(.12f * strength),
                                       r.getX(), r.getY(),
                                       juce::Colours::white.withAlpha(0.f), r.getRight(),
                                       r.getY() + r.getHeight() * .25f, false);
            sheen.addColour(.30, juce::Colours::white.withAlpha(.035f * strength));
            sheen.addColour(.62, juce::Colours::white.withAlpha(.105f * strength));
            sheen.addColour(.77, juce::Colours::white.withAlpha(.015f * strength));
            g.setGradientFill(sheen);
            g.fillRect(r);
        }
    }
    if (panel && (t.style == "aurora" || t.style == "spectral"))
    {
        juce::Graphics::ScopedSaveState save(g);
        juce::Path clip;
        clip.addRoundedRectangle(r.reduced(1), std::max(0.f, radius - 1));
        g.reduceClipRegion(clip);
        g.setGradientFill(juce::ColourGradient(
            t.accent.withAlpha(t.style == "aurora" ? .04f : .035f),
            r.getX() + r.getWidth() * .14f, r.getY(), t.accent.withAlpha(0.f),
            r.getX() + r.getWidth() * .77f, r.getBottom(), true));
        g.fillRect(r);
        if (t.style == "aurora")
        {
            juce::Path reflection;
            reflection.startNewSubPath(r.getX() + r.getWidth() * .12f, r.getY());
            reflection.lineTo(r.getX() + r.getWidth() * .38f, r.getY());
            reflection.lineTo(r.getX() + r.getWidth() * .14f, r.getBottom());
            reflection.lineTo(r.getX() - r.getWidth() * .12f, r.getBottom());
            reflection.closeSubPath();
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.02f),
                                                   r.getTopLeft(),
                                                   juce::Colours::white.withAlpha(0.f),
                                                   r.getBottomRight(), false));
            g.fillPath(reflection);
        }
    }
    g.setColour(metal ? juce::Colour(0xff3b2c17).withAlpha(.90f)
                      : t.border.withAlpha(flat ? .45f : .80f));
    g.drawRoundedRectangle(r.reduced(0.5f), radius, 1);
    if (panel && (metal || ivory || t.style == "tokyo"))
    {
        g.setColour(metal ? t.border.withAlpha(.88f)
                          : juce::Colours::white.withAlpha(ivory ? .72f : .15f));
        g.drawRoundedRectangle(r.reduced(2), std::max(0.f, radius - 1), .8f);
        g.setColour(juce::Colours::black.withAlpha(metal ? .48f : .10f));
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
    auto ink = selected ? (theme.style == "aurora" || theme.style == "spectral"
                               ? juce::Colours::white
                               : fill.contrasting(.95f))
                        : button.findColour(juce::TextButton::textColourOffId);
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
        tool,
        icon,
        search,
        list
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
        if (theme.style == "bauhaus" &&
            (presentation == caption || presentation == chevron))
            ink = theme.display.contrasting(.9f);
        const bool selected = getToggleState();
        if (presentation == chip || presentation == tool || presentation == search ||
            presentation == list ||
            (presentation == navigation && (selected || over || down)))
        {
            auto fill = selected ? theme.accent
                        : presentation == search || presentation == list
                            ? theme.display
                            : findColour(juce::TextButton::buttonColourId);
            if (theme.style == "precision" && presentation == navigation && selected)
                fill = theme.accent.interpolatedWith(theme.panel, .88f);
            if (over)
                fill = fill.interpolatedWith(juce::Colours::white, .07f);
            if (down)
                fill = fill.interpolatedWith(juce::Colours::black, .13f);
            bezel(g, theme, r, fill,
                  presentation == search || presentation == list ? 7.f
                  : theme.style == "tokyo" || theme.style == "bauhaus" ? 3.f : 5.f);
            if (selected)
            {
                ink = theme.style == "precision" && presentation == navigation
                          ? theme.accent
                      : theme.style == "aurora" || theme.style == "spectral"
                          ? juce::Colours::white
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
        if (presentation == search)
        {
            // This is an accessible button that opens the real native search
            // browser, not an inert text editor painted into the header.
            g.setFont(theme.type(fontSize));
            g.setColour(ink);
            g.drawFittedText(getButtonText(),
                             r.reduced(10.f, 0).withTrimmedRight(26.f).toNearestInt(),
                             juce::Justification::centredLeft, 1, .9f);
            const auto lens = juce::Point<float>(r.getRight() - 17.f, r.getCentreY() - 1.f);
            g.setColour(theme.text);
            g.drawEllipse(lens.x - 4.7f, lens.y - 4.7f, 9.4f, 9.4f, 1.4f);
            g.drawLine(lens.x + 3.5f, lens.y + 3.5f, lens.x + 7.3f, lens.y + 7.3f, 1.4f);
        }
        else if (presentation == list)
        {
            const auto c = r.getCentre();
            g.setColour(ink);
            for (int row = -1; row <= 1; ++row)
            {
                const float y = c.y + row * 5.f;
                g.fillEllipse(c.x - 8.f, y - .9f, 1.8f, 1.8f);
                g.drawLine(c.x - 3.8f, y, c.x + 8.f, y, 1.4f);
            }
        }
        else if (presentation == chevron)
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
        else if (presentation == icon)
        {
            const auto c = r.getCentre();
            const float side = std::min(21.f, std::min(r.getWidth(), r.getHeight()) - 8.f);
            const auto box = Rect(0, 0, side, side).withCentre(c);
            g.setColour(ink);
            juce::Path glyph;
            if (getComponentID().contains("save"))
            {
                glyph.startNewSubPath(box.getX(), box.getY());
                glyph.lineTo(box.getRight() - 4, box.getY());
                glyph.lineTo(box.getRight(), box.getY() + 4);
                glyph.lineTo(box.getRight(), box.getBottom());
                glyph.lineTo(box.getX(), box.getBottom());
                glyph.closeSubPath();
                glyph.addRectangle(box.getX() + 4, box.getY(), side - 9, side * .36f);
                glyph.addRectangle(box.getX() + 4, box.getY() + side * .58f, side - 8,
                                   side * .42f);
            }
            else if (getComponentID().contains("browser"))
            {
                glyph.startNewSubPath(box.getX(), box.getY() + 3);
                glyph.lineTo(box.getX() + 8, box.getY() + 3);
                glyph.lineTo(box.getX() + 11, box.getY() + 6);
                glyph.lineTo(box.getRight(), box.getY() + 6);
                glyph.lineTo(box.getRight(), box.getBottom() - 2);
                glyph.lineTo(box.getX(), box.getBottom() - 2);
                glyph.closeSubPath();
            }
            else
            {
                for (int dot = -1; dot <= 1; ++dot)
                    g.fillEllipse(c.x - 1.7f, c.y + dot * 6.f - 1.7f, 3.4f, 3.4f);
            }
            g.strokePath(glyph, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved,
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
          bool enabled, bool coloredRim = false)
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
    else if ((!ivory && !bauhaus) || ((ivory || bauhaus) && coloredRim))
    {
        g.setColour(bauhaus ? juce::Colour(0xff181b17) : t.border.withAlpha(clear ? .65f : .32f));
        g.strokePath(track, juce::PathStrokeType(bauhaus ? 3.6f : clear ? 3.f : 1.5f));
        g.setColour(ivory ? t.accent : accent);
        g.strokePath(arc, juce::PathStrokeType(bauhaus ? 4.8f : clear ? 4.2f : 2.2f));
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
    auto hi = t.knob.interpolatedWith(juce::Colours::white, glass ? .085f : .075f);
    auto lo = t.knob.interpolatedWith(juce::Colours::black, .60f);
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
    if (space)
    {
        // The hardware dials have a machined annulus and a recessed cap. Both
        // are cosmetic surfaces; the pointer below still uses the native value.
        auto rim = juce::ColourGradient(juce::Colour(0xffffedbf), face.getTopLeft(),
                                         juce::Colour(0xff453b24), face.getBottomRight(), false);
        rim.addColour(.25, juce::Colour(0xffa28a58));
        rim.addColour(.48, juce::Colour(0xffe4cf9a));
        rim.addColour(.72, juce::Colour(0xff695734));
        g.setGradientFill(rim);
        g.drawEllipse(face.reduced(1.2f), std::min(3.1f, faceRadius * .20f));
        const auto cap = face.reduced(std::max(2.5f, faceRadius * .18f));
        // Fine radial cuts are confined to the annulus; they do not replace
        // the dark cap, the graduated scale or the native value pointer.
        for (int cut = 0; cut < 48; ++cut)
        {
            const auto angle = juce::MathConstants<float>::twoPi * cut / 48.f;
            const auto inner = cap.getWidth() * .5f + .3f, outer = faceRadius - .8f;
            g.setColour((cut % 2 == 0 ? juce::Colours::white : juce::Colours::black)
                            .withAlpha(cut % 2 == 0 ? .24f : .20f));
            g.drawLine(c.x + std::sin(angle) * inner, c.y - std::cos(angle) * inner,
                       c.x + std::sin(angle) * outer, c.y - std::cos(angle) * outer, .5f);
        }
        const bool darkCap = t.knob.getPerceivedBrightness() < .35f;
        if (darkCap)
        {
            auto bevel = juce::ColourGradient(juce::Colour(0xff51503b), cap.getTopLeft(),
                                               juce::Colour(0xff080b08), cap.getBottomRight(), false);
            bevel.addColour(.40, juce::Colour(0xff262a20));
            g.setGradientFill(bevel);
            g.fillEllipse(cap);
            const auto inset = cap.reduced(std::max(1.5f, faceRadius * .10f));
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff24281e), inset.getTopLeft(),
                                                   juce::Colour(0xff0b0e0a),
                                                   inset.getBottomRight(), false));
            g.fillEllipse(inset);
            g.setColour(juce::Colour(0xff070a07).withAlpha(.95f));
            g.drawEllipse(inset, 1.f);
        }
        else
        {
            // Opposed brushed-metal reflections are purely the dial material.
            // Their fixed light direction never stands in for a parameter value.
            const auto radius = cap.getWidth() * .5f;
            constexpr int sectors = 48;
            for (int sector = 0; sector < sectors; ++sector)
            {
                const auto angle = juce::MathConstants<float>::twoPi * sector / sectors;
                const auto next = angle + juce::MathConstants<float>::twoPi / sectors + .004f;
                const auto light = std::pow(.5f + .5f * std::cos(2.f * (angle + .65f)), 1.4f);
                g.setColour(juce::Colour(0xff796848).interpolatedWith(
                    juce::Colour(0xffffedbe), light));
                juce::Path reflection;
                reflection.startNewSubPath(c);
                reflection.lineTo(c.x + std::sin(angle) * radius,
                                  c.y - std::cos(angle) * radius);
                reflection.lineTo(c.x + std::sin(next) * radius,
                                  c.y - std::cos(next) * radius);
                reflection.closeSubPath();
                g.fillPath(reflection);
            }
        }
        g.setColour(juce::Colour(0xff282618).withAlpha(.95f));
        g.drawEllipse(cap, 1.2f);
    }
    if (!clear && !bauhaus)
    {
        // Broad upper reflection and a narrow dark lower lip make the smaller
        // dials read as solid caps while leaving the value-dependent arc clear.
        const auto highlight = ivory || space ? .20f : glass ? .14f : .10f;
        g.setGradientFill(juce::ColourGradient(
            juce::Colours::white.withAlpha(highlight), face.getX() + face.getWidth() * .30f,
            face.getY() + face.getHeight() * .18f, juce::Colours::white.withAlpha(0.f),
            face.getRight(), face.getBottom(), true));
        g.fillEllipse(face.reduced(.8f));
        g.setColour(juce::Colours::black.withAlpha(ivory || space ? .15f : .30f));
        g.drawEllipse(face.reduced(1.1f), .8f);
    }
    g.setColour(bauhaus          ? juce::Colour(0xff151613)
                : space          ? juce::Colour(0xff292719)
                : ivory          ? juce::Colour(0xff958058)
                                 : t.border.withAlpha(clear ? .18f : .48f));
    g.drawEllipse(face, bauhaus ? 3.1f : ivory || space ? 1.8f : .9f);
    if (!clear && !bauhaus)
    {
        juce::Path shine;
        shine.addCentredArc(c.x, c.y, faceRadius - 1.5f, faceRadius - 1.5f, 0, -1.45f, .65f, true);
        g.setColour(juce::Colours::white.withAlpha(
            ivory || space ? .70f : t.style == "tokyo" ? .65f : .35f));
        g.strokePath(shine, juce::PathStrokeType(t.style == "tokyo" ? 1.4f : .8f));
    }
    const float inner = rad * (clear ? .54f : .48f), outer = rad * .84f;
    g.setColour(ivory                ? juce::Colour(0xff264c33)
                : bauhaus || clear   ? juce::Colour(0xff10181c)
                : space              ? (t.knob.getPerceivedBrightness() < .35f
                                            ? t.secondary
                                            : juce::Colour(0xff352f20))
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

class CompactSliderLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    juce::Slider::SliderLayout getSliderLayout(juce::Slider &slider) override
    {
        juce::Slider::SliderLayout layout;
        if (static_cast<bool>(slider.getProperties()["reference.performance-fader"]))
        {
            layout.sliderBounds = slider.getLocalBounds().withTrimmedBottom(22);
            return layout;
        }
        const auto labelWidth = static_cast<int>(slider.getProperties()["reference.compact-label-width"]);
        layout.sliderBounds = slider.getLocalBounds().withTrimmedLeft(labelWidth + 3)
                                  .withTrimmedRight(43);
        return layout;
    }
    int getSliderThumbRadius(juce::Slider &) override { return 6; }
};

class BoundSlider : public juce::Slider
{
  public:
    BoundSlider(ParameterBridge &b, Theme &theme, const juce::var &definition)
        : binding(string(definition, "binding")), spec(definition), bridge(b), theme(theme)
    {
        setComponentID(string(definition, "id"));
        kind = string(spec, "kind", "knob");
        if (kind == "horizontal" && static_cast<bool>(spec["compact"]))
        {
            getProperties().set("reference.compact-label-width",
                                number(spec, "labelWidth", 32));
            setLookAndFeel(&compactLookAndFeel);
        }
        if (theme.style == "live" && kind == "fader" &&
            string(spec, "id").startsWith("c.performance-"))
        {
            getProperties().set("reference.performance-fader", true);
            setLookAndFeel(&compactLookAndFeel);
        }
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
        setLookAndFeel(nullptr);
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
        ++entryGeneration;
        if (entryBox)
            entryBox->dismiss();
        entryBox = nullptr;
    }
    bool rebind(const juce::String &nextBinding)
    {
        if (nextBinding.isNotEmpty() && !bridge.valid(nextBinding))
            return false;
        if (binding != nextBinding)
        {
            // Finish the OLD host gesture before replacing its identity. A
            // dismissed callout may still own a queued callback; its generation
            // and exact binding are checked again before accepting typed input.
            cancelGesture();
            dismissEntry();
            bridge.end(binding);
            binding = nextBinding;
        }
        sync();
        repaint();
        return true;
    }
    void sync()
    {
        polling = true;
        setEnabled(bridge.valid(binding) && bridge.enabled(binding));
        getProperties().set("reference.binding", binding);
        if (entryBox && (!isEnabled() || entryLabel != bridge.label(binding)))
            dismissEntry();
        if (!dragging)
            setValue(bridge.value(binding), juce::dontSendNotification);
        auto name = string(spec, "label", bridge.label(binding));
        // An empty visual caption is intentional for compact Live controls,
        // but it must not erase their native screen-reader/keyboard identity.
        // Keep paint()'s caption untouched and follow dynamic algorithm names.
        if (name.trim().isEmpty())
            name = bridge.label(binding);
        const auto sourceCaption = getProperties()["reference.source-caption"].toString();
        if (sourceCaption.isNotEmpty())
            name += " (" + sourceCaption + ")";
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
        if (!spec.hasProperty("label") && !binding.startsWith("macro"))
            label = compactPaintLabel(label);
        // Host parameter names include a redundant M1:/M2: identity prefix.
        // Keep that full name in accessibility/tooltips; the face caption uses
        // the actual assigned macro name and a separate index where designed.
        if (!spec.hasProperty("label") && binding.startsWith("macro"))
        {
            const auto prefix = "M" + binding.substring(5) + ":";
            if (label.startsWithIgnoreCase(prefix))
                label = label.substring(prefix.length()).trimStart();
        }
        auto value = isEnabled() ? compactPaintValue(bridge.display(binding)) : utf8("—");
        if (theme.style == "live" && binding.startsWith("macro"))
        {
            value = bridge.label(binding);
            const auto prefix = "M" + binding.substring(5) + ":";
            if (value.startsWithIgnoreCase(prefix))
                value = value.substring(prefix.length()).trimStart();
        }
        auto ink = spec.hasProperty("ink") ? theme.color(string(spec, "ink")) : theme.text;
        auto valueInk = theme.style == "tokyo" ? theme.text
                            : spec.hasProperty("ink") ? ink.withAlpha(.85f) : theme.muted;
        float fs = number(spec, "fontSize", theme.style == "clear" ? 15 : 11.5f);
        if (kind == "value")
        {
            if (theme.style == "precision" && binding.startsWith("osc"))
            {
                text(g, theme, label, r.withHeight(13), fs - 1.f, theme.muted,
                     false, juce::Justification::centred);
                text(g, theme, value, r.withTrimmedTop(13), fs, ink,
                     false, juce::Justification::centred);
                if (isMouseOverOrDragging() || hasKeyboardFocus(true))
                {
                    g.setColour(accent.withAlpha(hasKeyboardFocus(true) ? 1.f : .35f));
                    g.drawRoundedRectangle(r.reduced(1), 3, 1);
                }
                return;
            }
            const float labelWidth =
                juce::jlimit(0.f, r.getWidth(),
                             number(spec, "labelWidth",
                                    label.length() <= 3 ? std::min(24.f, r.getWidth() * .25f)
                                                        : r.getWidth() * .44f));
            text(g, theme, label, r.removeFromLeft(labelWidth), fs, ink);
            auto box = r.reduced(2, 1);
            const bool paperField = theme.style == "clear" || theme.style == "ivory" ||
                                    theme.style == "bauhaus";
            bezel(g, theme, box, paperField ? theme.panel : theme.display, 3);
            auto fieldInk = paperField ? theme.text
                                      : theme.style == "space" ? theme.accent
                                      : spec.hasProperty("ink") ? ink
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
            const bool emphasizeLabel = (theme.style == "ivory" || theme.style == "space") &&
                                        binding.startsWith("macro");
            if (binding.startsWith("macro") &&
                (theme.style == "precision" || theme.style == "spectral"))
                r.removeFromTop(14.f); // Explicit number band in these manifest bounds.
            const auto labelPosition = string(spec, "labelPosition");
            const auto valuePosition = string(spec, "valuePosition", "bottom");
            const bool hideValue = valuePosition == "hidden";
            const bool valueAtRight = valuePosition == "right";
            const bool valueAtTop = valuePosition == "top";
            const auto lineHeight = std::min(19.f, r.getHeight() * .23f);
            const bool coloredRim =
                (theme.style == "bauhaus" && (binding.startsWith("macro") ||
                                              binding == "master.volume" ||
                                              (binding.startsWith("fx") &&
                                               string(spec, "accent") == "red"))) ||
                (theme.style == "ivory" && (binding.endsWith(".level") ||
                                            binding == "master.volume"));
            auto drawFace = [&](Rect face) {
                const auto requested = number(spec, "faceDiameter", 0.f);
                const auto available = std::min(face.getWidth(), face.getHeight());
                const auto side = requested > 0 ? std::min(requested, available) : available;
                face = face.withSizeKeepingCentre(side, side);
                auto faceTheme = theme;
                if (theme.style == "space" && binding.startsWith("macro"))
                    faceTheme.knob = theme.display;
                knob(g, faceTheme, face, static_cast<float>(getValue()), accent,
                     hasKeyboardFocus(true), isEnabled(), coloredRim);
                if (binding.startsWith("macro") &&
                    (theme.style == "precision" || theme.style == "bauhaus" ||
                     theme.style == "spectral"))
                    text(g, theme, (theme.style == "spectral" ? "M" : "") + binding.substring(5),
                         theme.style == "bauhaus"
                             ? Rect(face.getX() - 8, face.getY() - 1, 14, 14)
                             : Rect(face.getCentreX() - 12, face.getY() - 16, 24, 14),
                         10, ink, false,
                         juce::Justification::centred);
            };
            auto valueOnRight = [&](Rect &face) {
                auto valueBounds = face.removeFromRight(std::min(72.f, face.getWidth() * .40f));
                text(g, theme, value, valueBounds.reduced(2, 0), fs,
                     theme.style == "clear" ? ink : valueInk, theme.style == "clear",
                     juce::Justification::centredLeft);
            };
            if (theme.style == "space" && binding.startsWith("macro"))
            {
                text(g, theme, label, r.withHeight(19), fs, ink, emphasizeLabel,
                     juce::Justification::centred);
                auto faceTheme = theme;
                faceTheme.knob = theme.display;
                faceTheme.text = theme.display; // Dark graduations on the brass macro plate.
                const auto diameter = number(spec, "faceDiameter", 60.f);
                const auto face = Rect(0, 0, diameter, diameter).withCentre(
                    {r.getCentreX(), r.getY() + 62.5f});
                knob(g, faceTheme, face, static_cast<float>(getValue()), accent,
                     hasKeyboardFocus(true), isEnabled());
                auto plate = r.removeFromBottom(24).reduced(1, 0);
                bezel(g, theme, plate, theme.knob.brighter(.18f), 1.5f);
                auto name = bridge.label(binding);
                const auto prefix = "M" + binding.substring(5) + ":";
                if (name.startsWithIgnoreCase(prefix))
                    name = name.substring(prefix.length()).trimStart();
                text(g, theme, name.toUpperCase(), plate.reduced(3, 1), 10.5f,
                     theme.display, true, juce::Justification::centred);
                return;
            }
            if (labelPosition == "top")
            {
                const float line = lineHeight;
                text(g, theme, label, r.removeFromTop(line), fs, ink, emphasizeLabel,
                     juce::Justification::centred);
                if (!hideValue && valueAtTop)
                    text(g, theme, value, r.removeFromTop(line), fs, valueInk,
                         theme.style == "clear", juce::Justification::centred);
                else if (!hideValue && !valueAtRight)
                {
                    auto readout = r.removeFromBottom(line);
                    if (theme.style == "tokyo" && binding.startsWith("osc"))
                    {
                        g.setColour(juce::Colour(0xff121416));
                        g.fillRoundedRectangle(readout.reduced(1.f, .5f), 2.f);
                    }
                    text(g, theme, value, readout, fs,
                         theme.style == "clear" ? ink : valueInk,
                         theme.style == "clear" || theme.style == "tokyo",
                         juce::Justification::centred);
                }
                else if (valueAtRight)
                    valueOnRight(r);
                drawFace(r);
                return;
            }
            if (labelPosition == "left" || labelPosition == "right")
            {
                const float side = std::min(r.getHeight(), r.getWidth() * .48f);
                auto face =
                    labelPosition == "left" ? r.removeFromRight(side) : r.removeFromLeft(side);
                drawFace(face);
                auto labels = r.reduced(4, std::max(0.f, (r.getHeight() - 32) * .5f));
                if (theme.style == "precision" && binding == "master.volume")
                    labels = labels.withTrimmedRight(std::min(29.f, labels.getWidth() * .36f));
                if (theme.style == "bauhaus" && labelPosition == "left")
                {
                    // Bauhaus scene rows put the label, current value and real
                    // rotary on one baseline rather than stacking two captions.
                    auto nameBounds = labels.removeFromLeft(labels.getWidth() * .53f);
                    text(g, theme, label, nameBounds, fs, ink);
                    if (!hideValue)
                        text(g, theme, value, labels, fs, valueInk, false,
                             juce::Justification::centredRight);
                    return;
                }
                const int align = labelPosition == "left" ? juce::Justification::centredRight
                                                          : juce::Justification::centredLeft;
                if (hideValue)
                    text(g, theme, label, labels, fs, ink, emphasizeLabel, align);
                else
                {
                    text(g, theme, label, labels.removeFromTop(16), fs, ink, emphasizeLabel, align);
                    text(g, theme, value, labels, fs, valueInk, false, align);
                }
                return;
            }
            const auto lower = hideValue || valueAtRight ? lineHeight
                                                         : std::min(32.f, r.getHeight() * .42f);
            auto face = r.withTrimmedBottom(lower);
            if (valueAtRight)
                valueOnRight(face);
            drawFace(face);
            text(g, theme, label,
                 r.removeFromBottom(lower).withHeight(hideValue || valueAtRight ? lower : lower / 2),
                 fs, ink, emphasizeLabel,
                 juce::Justification::centred);
            if (!hideValue && !valueAtRight)
                text(g, theme, value, getLocalBounds().toFloat().removeFromBottom(lower / 2), fs,
                     theme.style == "clear" ? ink : valueInk, theme.style == "clear",
                     juce::Justification::centred);
        }
        else if (static_cast<bool>(getProperties()["reference.performance-fader"]))
        {
            text(g, theme, label, r.removeFromBottom(20), 9, ink, false,
                 juce::Justification::centred);
            const auto bottom = getPositionOfValue(0), top = getPositionOfValue(1);
            const auto x = r.getCentreX();
            const auto y = getPositionOfValue(getValue());
            const auto track = Rect(x - 4, top, 8, std::max(1.f, bottom - top));
            bezel(g, theme, track.expanded(4, 0), theme.display, 3);
            g.setColour(accent.withAlpha(.18f));
            g.fillRoundedRectangle(track, 2);
            g.setGradientFill(juce::ColourGradient(accent.withAlpha(.8f), x, y,
                                                  accent.withAlpha(.12f), x, bottom, false));
            g.fillRoundedRectangle(track.withTop(y), 2);
            const auto handle = Rect(x - 10, y - 3.5f, 20, 7);
            g.setGradientFill(juce::ColourGradient(theme.yellow.brighter(.25f), handle.getTopLeft(),
                                                  theme.yellow.darker(.4f), handle.getBottomLeft(), false));
            g.fillRoundedRectangle(handle, 2);
            if (hasKeyboardFocus(true))
            {
                g.setColour(accent);
                g.drawRoundedRectangle(handle.expanded(1), 2, 1.5f);
            }
        }
        else if (kind == "horizontal" && static_cast<bool>(spec["compact"]))
        {
            text(g, theme, label, r.withWidth(number(spec, "labelWidth", 32)), fs, ink);
            text(g, theme, value, r.removeFromRight(40), fs, valueInk, false,
                 juce::Justification::centredRight);
            const auto start = getPositionOfValue(0), end = getPositionOfValue(1);
            const auto y = getHeight() * .5f;
            const auto track = Rect(start, y - 2, std::max(1.f, end - start), 4);
            g.setColour(theme.border);
            g.fillRoundedRectangle(track.expanded(1), 2);
            g.setColour(accent);
            g.fillRoundedRectangle(track.withWidth(track.getWidth() * getValue()), 2);
            const auto x = getPositionOfValue(getValue());
            g.setColour(theme.light() ? juce::Colours::white : theme.text);
            g.fillEllipse(x - 5, y - 5, 10, 10);
            g.setColour(hasKeyboardFocus(true) ? accent : theme.border);
            g.drawEllipse(x - 5, y - 5, 10, 10, hasKeyboardFocus(true) ? 2.f : 1.f);
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
            const bool spectralFader = kind == "fader" && theme.style == "spectral";
            if (kind == "fader" && (theme.style == "precision" || spectralFader))
                track = track.withSizeKeepingCentre(3.f, track.getHeight());
            if (kind == "horizontal")
                track =
                    track.withSizeKeepingCentre(track.getWidth(), std::min(5.f, track.getHeight()));
            bezel(g, theme, track.expanded(2), theme.display, 3);
            if (spectralFader)
            {
                g.setColour(theme.secondary.withAlpha(.20f));
                g.fillRoundedRectangle(track, 1.5f);
            }
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
                const auto travelInk = spectralFader ? theme.secondary : accent;
                g.setGradientFill(juce::ColourGradient(
                    travelInk.interpolatedWith(juce::Colours::white, .3f), active.getTopLeft(),
                    travelInk, active.getBottomLeft(), false));
                g.fillRoundedRectangle(active, 2);
            }
            Rect handle;
            if (kind == "fader")
            {
                const auto width = std::min(24.f, static_cast<float>(getWidth()) - 2.f);
                const auto height = std::min(spectralFader ? 14.f : 8.f, r.getHeight());
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
            if (spectralFader)
            {
                g.setColour(theme.display.withAlpha(.78f));
                g.fillRoundedRectangle(handle.reduced(2.5f, 3.f), 1.f);
                g.setColour(theme.accent.interpolatedWith(juce::Colours::white, .55f));
                g.drawRoundedRectangle(handle.reduced(1.2f), 1.2f, 1.f);
            }
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
    CompactSliderLookAndFeel compactLookAndFeel;
    void showEntry()
    {
        if (!isEnabled())
            return;
        cancelGesture();
        dismissEntry();
        const auto scene = bridge.getScene();
        entryLabel = bridge.label(binding);
        const auto generation = entryGeneration;
        juce::Component::SafePointer<BoundSlider> safe(this);
        auto entry = std::make_unique<ValueEntry>(
            theme, bridge.label(binding), bridge.display(binding),
            [safe, scene, generation, bindingAtOpen = binding,
             nameAtOpen = entryLabel](const juce::String &value, juce::String &error) {
                if (!safe || safe->bridge.getScene() != scene || !safe->isEnabled() ||
                    safe->entryGeneration != generation || safe->binding != bindingAtOpen ||
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
    std::uint64_t entryGeneration{0};
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
            if (polling || !isEnabled() || getSelectedItemIndex() < 0 ||
                getSelectedItemIndex() == bridge.choiceIndex(binding))
                return;
            bridge.begin(binding);
            bridge.set(binding, bridge.choiceValue(binding, getSelectedItemIndex()));
            bridge.end(binding);
        };
        sync();
    }
    ~BoundMenu() override { cancelPopup(); }
    void cancelPopup()
    {
        ++popupGeneration;
        hidePopup();
        if (popupOpen)
            juce::PopupMenu::dismissAllActiveMenus();
        popupOpen = false;
    }
    bool rebind(const juce::String &nextBinding)
    {
        if (nextBinding.isNotEmpty() && !bridge.valid(nextBinding))
            return false;
        if (binding != nextBinding)
        {
            cancelPopup();
            bridge.end(binding);
            binding = nextBinding;
            items.clear();
        }
        sync();
        repaint();
        return true;
    }
    void showPopup() override
    {
        if (!isEnabled())
        {
            hidePopup();
            return;
        }
        if (popupOpen)
            return;
        ++popupGeneration;
        const auto choicesAtOpen = bridge.choices(binding);
        if (choicesAtOpen.isEmpty())
            return;
        const auto generation = popupGeneration;
        const auto scene = bridge.getScene();
        const auto bindingAtOpen = binding;
        juce::PopupMenu menu;
        for (int i = 0; i < choicesAtOpen.size(); ++i)
            menu.addItem(i + 1, choicesAtOpen[i], true, i == bridge.choiceIndex(binding));
        popupOpen = true;
        juce::Component::SafePointer<BoundMenu> safe(this);
        menu.showMenuAsync(
            juce::PopupMenu::Options()
                .withTargetComponent(this)
                .withMinimumWidth(getWidth())
                .withInitiallySelectedItem(getSelectedId()),
            [safe, generation, scene, bindingAtOpen, choicesAtOpen](int result) {
                if (!safe || safe->popupGeneration != generation)
                    return;
                safe->popupOpen = false;
                safe->hidePopup();
                // A popup is asynchronous. A selection/source/scene transition
                // must never reinterpret an old item index in a new parameter.
                if (result <= 0 || result > choicesAtOpen.size() || !safe->isEnabled() ||
                    safe->binding != bindingAtOpen || safe->bridge.getScene() != scene ||
                    safe->bridge.choices(bindingAtOpen) != choicesAtOpen)
                    return;
                safe->bridge.begin(bindingAtOpen);
                safe->bridge.set(bindingAtOpen,
                                 safe->bridge.choiceValue(bindingAtOpen, result - 1));
                safe->bridge.end(bindingAtOpen);
                safe->sync();
                safe->repaint();
            });
    }
    void sync()
    {
        polling = true;
        auto c = bridge.choices(binding);
        if (c != items)
        {
            cancelPopup();
            clear(juce::dontSendNotification);
            items = c;
            for (int i = 0; i < c.size(); ++i)
                addItem(c[i], i + 1);
        }
        setSelectedItemIndex(bridge.choiceIndex(binding), juce::dontSendNotification);
        setEnabled(bridge.valid(binding) && bridge.enabled(binding) && !c.isEmpty());
        if (!isEnabled() && popupOpen)
            cancelPopup();
        getProperties().set("reference.binding", binding);
        auto name = string(spec, "label", bridge.label(binding));
        const auto sourceCaption = getProperties()["reference.source-caption"].toString();
        if (sourceCaption.isNotEmpty())
            name += " (" + sourceCaption + ")";
        setName(name);
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
        if (theme.style == "bauhaus" && binding.startsWith("filter") &&
            binding.endsWith(".type"))
        {
            setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff18364f));
            setColour(juce::ComboBox::textColourId, juce::Colour(0xfff9edd3));
        }
        setColour(juce::ComboBox::outlineColourId, theme.border);
        setColour(juce::ComboBox::arrowColourId, theme.accent);
        updateSegments();
        polling = false;
    }
    void paint(juce::Graphics &g) override
    {
        if (!segments.empty())
        {
            if (hasKeyboardFocus(false))
            {
                g.setColour(theme.accent);
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 3, 1.5f);
            }
            return;
        }
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
                const auto arrowWidth = std::min(20, getWidth() / 5);
                label->setBounds(5, 0, std::max(1, getWidth() - arrowWidth - 5), getHeight());
                label->setVisible(segments.empty());
            }
        if (!segments.empty())
        {
            const auto count = static_cast<int>(segments.size());
            const auto vertical = presentation == segmentedRouting;
            const auto gap = vertical ? 4.f : theme.style == "space" ? 3.f : 0.f;
            const auto extent = static_cast<float>(vertical ? getHeight() : getWidth());
            const auto cell = std::max(1.f, (extent - gap * (count - 1)) / count);
            for (int index = 0; index < count; ++index)
            {
                const auto offset = index * (cell + gap);
                const auto rectangle = vertical ? Rect(0, offset, getWidth(), cell)
                                                : Rect(offset, 0, cell, getHeight());
                segments[index].button->setBounds(rectangle.toNearestInt());
            }
        }
    }
    void mouseDown(const juce::MouseEvent &event) override
    {
        if (!segments.empty())
        {
            grabKeyboardFocus();
            showPopup(); // The gaps still expose every native choice.
            return;
        }
        juce::ComboBox::mouseDown(event);
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (!segments.empty())
        {
            if (key.getKeyCode() == juce::KeyPress::F4Key ||
                (key.getModifiers().isAltDown() && key.getKeyCode() == juce::KeyPress::downKey))
            {
                showPopup();
                return true;
            }
            if (key.getKeyCode() == juce::KeyPress::leftKey ||
                key.getKeyCode() == juce::KeyPress::upKey)
            {
                moveSegment(-1);
                return true;
            }
            if (key.getKeyCode() == juce::KeyPress::rightKey ||
                key.getKeyCode() == juce::KeyPress::downKey)
            {
                moveSegment(1);
                return true;
            }
        }
        return juce::ComboBox::keyPressed(key);
    }

  private:
    enum Presentation { ordinaryMenu, oscillatorFamilies, segmentedRouting, sceneModes, voiceModes };
    class SegmentButton : public juce::TextButton
    {
      public:
        SegmentButton(Theme &theme, bool routing) : theme(theme), routing(routing)
        {
            setWantsKeyboardFocus(true);
            setToggleable(true);
            setClickingTogglesState(false);
        }
        void paintButton(juce::Graphics &g, bool over, bool down) override
        {
            const auto r = getLocalBounds().toFloat().reduced(.6f);
            const bool selected = getToggleState();
            const bool hardware = theme.style == "space";
            auto fill = selected ? (hardware ? juce::Colour(0xffffaa36)
                                             : routing ? theme.red : theme.yellow)
                                 : (hardware ? juce::Colour(0xffd6c09a)
                                             : juce::Colour(0xfffff2d6));
            if (over && isEnabled())
                fill = fill.interpolatedWith(juce::Colours::white, .11f);
            if (down && isEnabled())
                fill = fill.interpolatedWith(juce::Colours::black, .12f);
            bezel(g, theme, r, fill, hardware ? 2.f : routing ? 4.f : 2.f);
            auto ink = selected && routing && !hardware ? juce::Colours::white
                                                        : juce::Colour(0xff171812);
            if (!isEnabled())
                ink = ink.withAlpha(.35f);
            if (hardware && selected)
            {
                g.setColour(juce::Colour(0xffffe6a8).withAlpha(.75f));
                g.drawRoundedRectangle(r.reduced(2), 1.5f, .8f);
            }
            Surge::GUI::Reference::text(g, theme, getButtonText(), r.reduced(2, 0), hardware ? 11.f : 12.f,
                 ink, true, juce::Justification::centred);
            if (hasKeyboardFocus(true))
            {
                g.setColour(hardware ? theme.display : theme.text);
                g.drawRoundedRectangle(r.reduced(2), 2.f, 1.5f);
            }
        }
        void mouseDown(const juce::MouseEvent &event) override
        {
            contextGesture = event.mods.isPopupMenu();
            if (contextGesture)
            {
                if (onContext) onContext();
                return;
            }
            juce::TextButton::mouseDown(event);
        }
        void mouseUp(const juce::MouseEvent &event) override
        {
            if (contextGesture)
            {
                contextGesture = false;
                return;
            }
            juce::TextButton::mouseUp(event);
        }
        bool keyPressed(const juce::KeyPress &key) override
        {
            if (key.getKeyCode() == juce::KeyPress::F4Key ||
                (key.getModifiers().isAltDown() && key.getKeyCode() == juce::KeyPress::downKey))
            {
                if (onContext) onContext();
                return true;
            }
            if (key.getKeyCode() == juce::KeyPress::leftKey || key.getKeyCode() == juce::KeyPress::upKey ||
                key.getKeyCode() == juce::KeyPress::rightKey || key.getKeyCode() == juce::KeyPress::downKey)
            {
                if (onNavigate)
                    onNavigate(key.getKeyCode() == juce::KeyPress::leftKey ||
                               key.getKeyCode() == juce::KeyPress::upKey ? -1 : 1);
                return true;
            }
            return juce::TextButton::keyPressed(key);
        }
        std::function<void()> onContext;
        std::function<void(int)> onNavigate;
      private:
        Theme &theme;
        bool routing{false}, contextGesture{false};
    };
    struct Segment
    {
        juce::String label, description;
        int nativeValue{-1};
        std::vector<int> family;
        std::unique_ptr<SegmentButton> button;
    };
    Presentation requestedPresentation() const
    {
        if (theme.style == "bauhaus")
        {
            if (binding == "routing") return segmentedRouting;
            if (binding.startsWith("osc") && binding.endsWith(".type")) return oscillatorFamilies;
        }
        if (theme.style == "space")
        {
            if (binding == "scene.mode") return sceneModes;
            if (binding == "scene.polymode") return voiceModes;
        }
        return ordinaryMenu;
    }
    int nativeChoice() const
    {
        if (const auto *parameter = bridge.parameter(binding))
            return parameter->valtype == vt_int ? parameter->val.i : -1;
        return -1;
    }
    bool segmentSelected(const Segment &segment) const
    {
        const auto choice = nativeChoice();
        return std::find(segment.family.begin(), segment.family.end(), choice) != segment.family.end();
    }
    void chooseSegment(int index)
    {
        if (index < 0 || index >= static_cast<int>(segments.size()) || !isEnabled()) return;
        const auto &segment = segments[index];
        auto *parameter = bridge.parameter(binding);
        if (!parameter || parameter->valtype != vt_int || segment.nativeValue < parameter->val_min.i ||
            segment.nativeValue > parameter->val_max.i || segmentSelected(segment)) return;
        cancelPopup();
        // Use the same ComboBox callback as every ordinary menu so native undo,
        // host automation and algorithm initialization keep their existing path.
        setSelectedItemIndex(segment.nativeValue - parameter->val_min.i, juce::sendNotificationSync);
        sync();
        repaint();
    }
    void moveSegment(int delta)
    {
        const auto count = static_cast<int>(segments.size());
        if (!count) return;
        int current = -1;
        for (int i = 0; i < count; ++i)
            if (segments[i].button->hasKeyboardFocus(true)) { current = i; break; }
        if (current < 0)
            for (int i = 0; i < count; ++i)
                if (segmentSelected(segments[i])) { current = i; break; }
        if (current < 0 && delta < 0)
            current = 0;
        for (int step = 1; step <= count; ++step)
        {
            const auto candidate = (current + delta * step + count * 2) % count;
            if (segments[candidate].button->isEnabled())
            {
                chooseSegment(candidate);
                segments[candidate].button->grabKeyboardFocus();
                break;
            }
        }
    }
    void updateSegments()
    {
        const auto next = requestedPresentation();
        if (next != presentation)
        {
            segments.clear();
            presentation = next;
            const auto add = [&](juce::String label, int value, std::vector<int> family,
                                 juce::String description) {
                const auto index = static_cast<int>(segments.size());
                auto button = std::make_unique<SegmentButton>(theme, next == segmentedRouting);
                button->setButtonText(label);
                button->setComponentID(getComponentID() + ".segment." + juce::String(index));
                button->onClick = [this, index] { chooseSegment(index); };
                button->onContext = [this] { showPopup(); };
                button->onNavigate = [this](int delta) { moveSegment(delta); };
                addAndMakeVisible(*button);
                segments.push_back({label, description, value, std::move(family), std::move(button)});
            };
            if (next == oscillatorFamilies)
            {
                add("WT", ot_wavetable, {ot_wavetable, ot_window}, "Wavetable or Window oscillator family");
                add("VA", ot_classic, {ot_classic, ot_modern}, "Classic or Modern virtual-analog oscillator family");
                add("FM", ot_FM2, {ot_FM2, ot_FM3}, "FM2 or FM3 oscillator family");
                add("SAMPLE", -1, {}, "Unavailable: Surge has no separate Sample oscillator algorithm. Native wavetable sample files remain available through the full oscillator editor.");
            }
            else if (next == segmentedRouting)
            {
                add(utf8("A → B"), fc_serial1, {fc_serial1}, "Serial 1: Filter 1 then Filter 2");
                add(utf8("B → A"), -1, {}, "Unavailable: this engine has no reversed Filter 2 then Filter 1 routing mode");
                add("Parallel", fc_dual1, {fc_dual1}, "Dual 1: parallel filter paths");
                add("Split", fc_stereo, {fc_stereo}, "Stereo: separate left and right filter paths");
            }
            else if (next == sceneModes)
            {
                add("SINGLE", sm_single, {sm_single}, "Single scene");
                add("SPLIT", sm_split, {sm_split, sm_chsplit}, "Key split; channel split is also available in the full native menu");
                add("STACK", sm_dual, {sm_dual}, "Dual mode: layer both scenes");
            }
            else if (next == voiceModes)
            {
                add("POLY", pm_poly, {pm_poly}, "Polyphonic voice mode");
                add("MONO", pm_mono, {pm_mono, pm_mono_fp}, "Monophonic voice mode");
                add("LEGATO", pm_mono_st, {pm_mono_st, pm_mono_st_fp}, "Mono Single Trigger: legato envelope triggering");
            }
            resized();
        }
        for (auto &segment : segments)
        {
            const auto *parameter = bridge.parameter(binding);
            const bool supported = parameter && parameter->valtype == vt_int &&
                segment.nativeValue >= parameter->val_min.i && segment.nativeValue <= parameter->val_max.i;
            segment.button->setEnabled(isEnabled() && supported);
            segment.button->setToggleState(supported && segmentSelected(segment), juce::dontSendNotification);
            const auto label = getName() + ": " + segment.label;
            segment.button->setName(label);
            segment.button->setTitle(label);
            segment.button->setDescription(segment.description);
            segment.button->setTooltip(segment.description + ". Current: " + bridge.display(binding) +
                                       ". Right-click or Alt+Down for every native choice.");
        }
        if (!segments.empty())
        {
            setDescription("Native segmented choices. Right-click, F4 or Alt+Down opens all native options. "
                           "Arrow keys move between available segments.");
            for (auto *child : getChildren())
                if (auto *label = dynamic_cast<juce::Label *>(child)) label->setVisible(false);
        }
    }
    ParameterBridge &bridge;
    Theme &theme;
    juce::var spec;
    juce::String binding;
    juce::StringArray items;
    bool polling{false};
    bool popupOpen{false};
    std::uint64_t popupGeneration{0};
    Presentation presentation{ordinaryMenu};
    std::vector<Segment> segments;
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
            if (!isEnabled())
                return;
            bridge.begin(binding);
            bridge.set(binding, getToggleState() ? 1.f : 0.f);
            bridge.end(binding);
        };
        sync();
    }
    bool rebind(const juce::String &nextBinding)
    {
        if (nextBinding.isNotEmpty() && !bridge.valid(nextBinding))
            return false;
        if (binding != nextBinding)
        {
            bridge.end(binding);
            binding = nextBinding;
        }
        sync();
        repaint();
        return true;
    }
    void sync()
    {
        setButtonText(string(spec, "label", bridge.label(binding)));
        const auto sourceCaption = getProperties()["reference.source-caption"].toString();
        setName(getButtonText() +
                (sourceCaption.isEmpty() ? juce::String() : " (" + sourceCaption + ")"));
        setTitle(getName());
        setToggleState(bridge.value(binding) >= .5f, juce::dontSendNotification);
        setEnabled(bridge.valid(binding) && bridge.enabled(binding));
        getProperties().set("reference.binding", binding);
        setColour(juce::TextButton::buttonColourId, theme.panel);
        setColour(juce::TextButton::buttonOnColourId, theme.accent);
        setColour(juce::TextButton::textColourOffId, theme.text);
        setColour(juce::TextButton::textColourOnId, theme.background);
    }
    void paintButton(juce::Graphics &g, bool over, bool down) override
    {
        if (string(spec, "glyph") == "power")
        {
            // A mute switch's light means the oscillator is audible. The
            // underlying button state, automation and accessible name stay mute.
            const bool on = binding.endsWith(".mute") ? !getToggleState() : getToggleState();
            auto r = getLocalBounds().toFloat().reduced(1);
            const auto side = std::min(r.getWidth(), r.getHeight());
            r = r.withSizeKeepingCentre(side, side);
            const bool bare = theme.style == "precision" || theme.style == "spectral" ||
                              theme.style == "flow";
            auto active = theme.style == "tokyo" ? theme.red : theme.accent;
            if (!isEnabled())
                active = theme.muted.withAlpha(.45f);
            auto fill = on ? active : theme.panel;
            if (over)
                fill = fill.interpolatedWith(juce::Colours::white, .12f);
            if (down)
                fill = fill.interpolatedWith(juce::Colours::black, .16f);
            if (!bare)
            {
                bezel(g, theme, r, fill, theme.style == "ivory" ? side * .5f : 3.f);
                if (on && theme.style == "space")
                {
                    g.setColour(juce::Colours::white.withAlpha(.25f));
                    g.fillRoundedRectangle(r.reduced(3), 1.5f);
                }
            }
            else if (over || down)
            {
                g.setColour(active.withAlpha(down ? .18f : .08f));
                g.fillRoundedRectangle(r, 3);
            }
            const auto centre = r.getCentre();
            const auto rad = side * .27f;
            auto ink = bare ? (on ? active : theme.muted)
                       : on ? (theme.style == "space" ? theme.display : juce::Colours::white)
                            : theme.muted;
            if (!isEnabled())
                ink = ink.withMultipliedAlpha(.5f);
            g.setColour(ink);
            juce::Path ring;
            ring.addCentredArc(centre.x, centre.y + .5f, rad, rad, 0, .65f,
                               juce::MathConstants<float>::twoPi - .65f, true);
            g.strokePath(ring, juce::PathStrokeType(std::max(1.2f, side * .075f),
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
            g.drawLine(centre.x, centre.y - rad - 1, centre.x, centre.y + rad * .10f,
                       std::max(1.2f, side * .075f));
            if (hasKeyboardFocus(true))
            {
                g.setColour(theme.accent);
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.6f), 3, 1.5f);
            }
            return;
        }
        themedButton(g, theme, *this, over, down);
    }

  private:
    ParameterBridge &bridge;
    Theme &theme;
    juce::var spec;
    juce::String binding;
};

/** A real native FX-slot switch, separate from the view-selection button.
    The stock editor's slot-deactivation bitmask is not a host parameter. */
class EffectPowerButton : public juce::TextButton
{
  public:
    EffectPowerButton(Theme &themeIn, SurgeSynthProcessor &processorIn, int slotIn)
        : theme(themeIn), processor(processorIn), slot(slotIn)
    {
        setWantsKeyboardFocus(true);
        setToggleable(true);
        setName("FX slot " + juce::String(slot + 1) + " power");
        setTitle(getName());
        getProperties().set("reference.fx-slot", slot + 1);
        onClick = [this] {
            if (!processor.surge || slot < 0 || slot >= n_fx_slots)
                return;
            auto &synth = *processor.surge;
            auto &patch = synth.storage.getPatch();
            if (patch.fx[slot].type.val.i == fxt_off)
                return;
            const int previous = patch.fx_disable.val.i;
            const int next = previous ^ (1 << slot);
            // Match the stock EffectChooser's suspend/notification path. This
            // hidden bitmask has no native host automation or undo gesture.
            synth.fx_suspend_bitmask = previous ^ next;
            processor.paramChangeToListeners(nullptr, true, processor.SCT_FX_DEACT,
                                             static_cast<float>(previous),
                                             static_cast<float>(next), 0.f, "");
            patch.fx_disable.val.i = next;
            patch.isDirty = true;
            synth.refresh_editor = true;
            sync();
        };
        sync();
    }
    void sync()
    {
        const bool valid = processor.surge && slot >= 0 && slot < n_fx_slots;
        const auto *patch = valid ? &processor.surge->storage.getPatch() : nullptr;
        const bool assigned = patch && patch->fx[slot].type.val.i != fxt_off;
        const bool disabled = patch && (patch->fx_disable.val.i & (1 << slot)) != 0;
        const bool powered = assigned && !disabled;
        // JUCE ignores setToggleState while disabled. Resolve the checked state
        // before disabling an empty slot, and re-enable a newly assigned slot.
        if (assigned || getToggleState() != powered)
            setEnabled(true);
        setToggleState(powered, juce::dontSendNotification);
        setEnabled(assigned);
        getProperties().set("reference.fx-disabled", disabled);
        const auto algorithm = patch ? juce::String(patch->fx[slot].type.get_display())
                                     : juce::String("Unavailable");
        const auto detail = "FX slot " + juce::String(slot + 1) + ": " + algorithm;
        setDescription(detail + (assigned ? (disabled ? ". Slot disabled." : ". Slot enabled.")
                                           : ". No effect is assigned.") +
                       " Global FX bypass is independent of this switch.");
        setTooltip((assigned ? (disabled ? "Enable " : "Disable ") : "Unavailable: ") +
                   detail + ". Global FX bypass is independent.");
        repaint();
    }
    void paintButton(juce::Graphics &g, bool over, bool down) override
    {
        const auto r = getLocalBounds().toFloat().reduced(1.5f);
        const bool enabled = isEnabled() && getToggleState();
        const auto *tab = dynamic_cast<const juce::Button *>(getParentComponent());
        const bool selected = tab && tab->getToggleState();
        auto ink = theme.text;
        if (theme.style == "live" || theme.style == "aurora")
            ink = theme.accent;
        else if (theme.style == "precision")
            ink = selected ? theme.accent : theme.text;
        else if (theme.style == "ivory")
            ink = selected ? theme.accent.contrasting(.95f) : theme.accent;
        else if (selected)
            ink = (theme.style == "bauhaus" ? theme.yellow : theme.accent).contrasting(.95f);
        if (!enabled)
            ink = ink.withAlpha(isEnabled() ? .45f : .25f);
        if (theme.style == "space")
        {
            const auto lamp = enabled ? theme.accent : theme.panel.brighter(.25f);
            g.setColour(juce::Colours::black.withAlpha(.8f));
            g.fillRoundedRectangle(r, 1.7f);
            g.setColour(theme.border.brighter(.2f));
            g.drawRoundedRectangle(r, 1.7f, .8f);
            if (enabled)
            {
                g.setColour(lamp.withAlpha(.2f));
                g.fillRoundedRectangle(r.expanded(1), 2.5f);
            }
            const auto face = r.reduced(2.4f);
            g.setGradientFill(juce::ColourGradient(lamp.brighter(.38f), face.getCentreX(),
                                                   face.getY(), lamp.darker(.25f),
                                                   face.getCentreX(), face.getBottom(), false));
            g.fillRoundedRectangle(face, .8f);
        }
        else
        {
            const auto centre = r.getCentre();
            const float radius = std::min(r.getWidth(), r.getHeight()) * .38f;
            juce::Path arc;
            arc.addCentredArc(centre.x, centre.y + .5f, radius, radius, 0.f,
                              .65f, juce::MathConstants<float>::twoPi - .65f, true);
            g.setColour(ink);
            g.strokePath(arc, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
            g.drawLine(centre.x, centre.y - radius - 1.f, centre.x,
                       centre.y - .4f, 1.5f);
        }
        if (over || down || hasKeyboardFocus(false))
        {
            g.setColour(theme.accent.withAlpha(down ? .85f : .65f));
            g.drawRoundedRectangle(r, 2.f, hasKeyboardFocus(false) ? 1.5f : .8f);
        }
    }

  private:
    Theme &theme;
    SurgeSynthProcessor &processor;
    int slot;
};

/** Native view tab: choosing a source never writes an audio parameter. */
class ViewTabButton : public juce::TextButton
{
  public:
    ViewTabButton(Theme &theme, float fontSize, bool effect)
        : theme(theme), fontSize(fontSize), effect(effect)
    {
        setWantsKeyboardFocus(true);
        // Expose checked state to assistive technology while keeping selection
        // owned by the view model (clicking the active tab must not turn it off).
        setToggleable(true);
    }
    void addEffectPower(SurgeSynthProcessor &processor, int oneBasedSlot)
    {
        // Spectral's compact tabs deliberately retain the reference's uninterrupted
        // single-line labels. The other FX racks show individual slot switches.
        if (!effect || theme.style == "spectral" || oneBasedSlot < 1 ||
            oneBasedSlot > n_fx_slots)
            return;
        power = std::make_unique<EffectPowerButton>(theme, processor, oneBasedSlot - 1);
        power->setComponentID(getComponentID() + ".power");
        addAndMakeVisible(*power);
        resized();
    }
    void syncEffectPower()
    {
        if (power)
            power->sync();
    }
    void resized() override
    {
        if (!power)
            return;
        const auto r = getLocalBounds();
        const int size = 16;
        int x = 8, y = 4;
        if (theme.style == "space" || theme.style == "aurora")
        {
            x = 26;
            y = (r.getHeight() - size) / 2;
        }
        else if (theme.style == "bauhaus")
        {
            x = r.getWidth() - size - 3;
            y = 3;
        }
        else if (theme.style == "live")
        {
            x = (r.getWidth() - size) / 2;
            y = 4;
        }
        else if (theme.style == "ivory")
            y = 6;
        power->setBounds(x, y, size, size);
    }
    void paintButton(juce::Graphics &g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(.6f);
        const bool selected = getToggleState();
        auto fill = selected ? theme.accent : theme.panel;
        if (effect && selected && theme.style == "bauhaus")
            fill = theme.yellow;
        if (theme.style == "precision" || theme.style == "tokyo" || theme.style == "flow")
            fill = selected ? theme.accent.interpolatedWith(theme.display, .80f) : theme.display;
        if (over)
            fill = fill.interpolatedWith(juce::Colours::white, .07f);
        if (down)
            fill = fill.interpolatedWith(juce::Colours::black, .13f);
        bezel(g, theme, r, fill, theme.style == "bauhaus" || theme.style == "tokyo" ? 2.f : 4.f);
        auto ink = selected ? fill.contrasting(.95f) : theme.text;
        if (selected && (theme.style == "precision" || theme.style == "tokyo" ||
                         theme.style == "flow"))
            ink = theme.accent;
        if (effect && power && (theme.style == "space" || theme.style == "aurora"))
        {
            const auto caption = getButtonText();
            const auto numberCell = r.withWidth(25.f);
            if (theme.style == "space")
            {
                g.setColour(ink.withAlpha(.5f));
                g.drawVerticalLine(static_cast<int>(numberCell.getRight()), r.getY() + 3,
                                   r.getBottom() - 3);
            }
            g.setColour(ink);
            g.setFont(theme.type(theme.style == "space" ? 13.f : fontSize, true));
            g.drawFittedText(caption.upToFirstOccurrenceOf(" ", false, false),
                             numberCell.toNearestInt(), juce::Justification::centred, 1, .7f);
            g.setFont(theme.type(theme.style == "space" ? 12.5f : fontSize, selected));
            const auto label = r.withTrimmedLeft(47.f).withTrimmedRight(4.f);
            g.drawFittedText(caption.fromFirstOccurrenceOf(" ", false, false).trim(),
                             label.toNearestInt(), juce::Justification::centredLeft, 1, .7f);
        }
        else if (effect && r.getHeight() >= 36)
        {
            const auto caption = getButtonText();
            const auto slot = caption.upToFirstOccurrenceOf(" ", false, false);
            const auto algorithm = caption.fromFirstOccurrenceOf(" ", false, false).trim();
            const bool large = r.getHeight() >= 76;
            auto numberCell = r.withHeight(large ? 22.f : 18.f);
            if (power)
            {
                if (theme.style == "live")
                    numberCell = r.withY(23.f).withHeight(15.f);
                else if (theme.style == "bauhaus")
                    numberCell = numberCell.withTrimmedRight(20.f);
                else
                    numberCell = numberCell.withTrimmedLeft(24.f);
            }
            g.setFont(theme.type(std::max(11.f, fontSize), true));
            g.setColour(ink);
            g.drawText(slot, numberCell.toNearestInt(), juce::Justification::centred);
            if (large)
                effectGlyph(g, r.reduced(12, 0).withHeight(36).withCentre(
                                   {r.getCentreX(), r.getCentreY()}), ink, algorithm);
            g.setFont(theme.type(fontSize, theme.style == "bauhaus"));
            g.drawFittedText(algorithm, r.withY(r.getBottom() - (large ? 29.f : 23.f))
                                                   .withHeight(20).reduced(3, 0).toNearestInt(),
                             juce::Justification::centred, 1, .65f);
        }
        else
        {
            g.setFont(theme.type(fontSize, selected || theme.style == "bauhaus"));
            g.setColour(ink);
            g.drawFittedText(getButtonText(), r.reduced(4, 1).toNearestInt(),
                             juce::Justification::centred, 1, .70f);
        }
        if (selected || hasKeyboardFocus(true))
        {
            g.setColour(theme.accent);
            g.drawRoundedRectangle(r.reduced(.4f), 3.f,
                                   hasKeyboardFocus(true) ? 2.f : 1.f);
        }
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (onNavigate && (key.getKeyCode() == juce::KeyPress::leftKey ||
                           key.getKeyCode() == juce::KeyPress::rightKey))
        {
            onNavigate(key.getKeyCode() == juce::KeyPress::leftKey ? -1 : 1);
            return true;
        }
        return juce::TextButton::keyPressed(key);
    }
    std::function<void(int)> onNavigate;

  private:
    Theme &theme;
    float fontSize;
    bool effect;
    std::unique_ptr<EffectPowerButton> power;
};

/** Three real native buttons: previous table, categorized catalog and next table.
    Paint consumes a cached name only; no catalog/file work or locks occur there. */
class WavetableSelector : public juce::Component
{
    class SelectorButton : public juce::TextButton
    {
      public:
        SelectorButton(Theme &theme, int direction) : theme(theme), direction(direction)
        {
            setWantsKeyboardFocus(true);
        }
        void paintButton(juce::Graphics &g, bool over, bool down) override
        {
            const auto r = getLocalBounds().toFloat().reduced(1);
            if (isEnabled() && (over || down))
            {
                g.setColour(theme.accent.withAlpha(down ? .17f : .09f));
                g.fillRoundedRectangle(r, 2);
            }
            const auto ink = theme.display.getPerceivedBrightness() < .45f &&
                                     theme.text.getPerceivedBrightness() < .45f
                                 ? theme.display.contrasting(.9f)
                                 : theme.text;
            g.setColour(isEnabled() ? ink : ink.withAlpha(.45f));
            if (direction != 0)
            {
                const auto centre = r.getCentre();
                juce::Path arrow;
                arrow.startNewSubPath(centre.x + direction * 3, centre.y);
                arrow.lineTo(centre.x - direction * 2, centre.y - 3.5f);
                arrow.lineTo(centre.x - direction * 2, centre.y + 3.5f);
                arrow.closeSubPath();
                g.fillPath(arrow);
            }
            else
            {
                const auto size = theme.style == "clear" ? 13.f : 11.f;
                g.setFont(theme.type(std::min(size, r.getHeight() * .65f)));
                g.drawFittedText(getButtonText(), r.reduced(4, 0).toNearestInt(),
                                 juce::Justification::centred, 1, .70f);
            }
            if (hasKeyboardFocus(false))
            {
                g.setColour(theme.accent);
                g.drawRoundedRectangle(r.reduced(.5f), 2, 1.5f);
            }
        }
        bool keyPressed(const juce::KeyPress &key) override
        {
            if (isEnabled() && onNavigate &&
                (key.getKeyCode() == juce::KeyPress::leftKey ||
                 key.getKeyCode() == juce::KeyPress::rightKey))
            {
                onNavigate(key.getKeyCode() == juce::KeyPress::leftKey ? -1 : 1);
                return true;
            }
            return juce::TextButton::keyPressed(key);
        }
        std::function<void(int)> onNavigate;

      private:
        Theme &theme;
        int direction;
    };

  public:
    WavetableSelector(SurgeSynthProcessor &processor, Theme &theme, const juce::String &panelId,
                       std::function<bool(const WavetableSelectionRequest &)> select,
                       std::function<bool()> scriptsAvailable)
        : processor(processor), theme(theme), select(std::move(select)),
          scriptsAvailable(std::move(scriptsAvailable)), previous(theme, -1),
          browse(theme, 0), next(theme, 1)
    {
        setInterceptsMouseClicks(false, true);
        setComponentID("workspace.wavetable." + panelId);
        for (auto *button : {&previous, &browse, &next})
        {
            addAndMakeVisible(*button);
            button->onNavigate = [this](int direction) { jog(direction); };
        }
        previous.setComponentID(getComponentID() + ".previous");
        browse.setComponentID(getComponentID() + ".browse");
        next.setComponentID(getComponentID() + ".next");
        previous.setName("Previous wavetable");
        browse.setName("Choose wavetable");
        next.setName("Next wavetable");
        previous.onClick = [this] { jog(-1); };
        browse.onClick = [this] { showCatalog(); };
        next.onClick = [this] { jog(1); };
    }
    ~WavetableSelector() override { cancelPopup(); }

    void sync(int nextScene, int nextOscillator)
    {
        if (scene != nextScene || oscillator != nextOscillator)
        {
            cancelPopup();
            scene = nextScene;
            oscillator = nextOscillator;
            state.reset();
        }
        auto fresh = snapshotWavetableSelection(processor, scene, oscillator);
        if (!fresh)
        {
            if (!state)
            {
                for (auto *button : {&previous, &browse, &next})
                    button->setEnabled(false);
                browse.setButtonText(oscillator < 0 ? "Unavailable" : "Updating");
            }
            // A busy loader keeps the last complete snapshot and keyboard
            // focus. Any attempted action independently revalidates its target.
            return;
        }
        if (state && state->target != fresh->target)
            cancelPopup();
        state = std::move(fresh);
        const auto source = "Scene " + juce::String(scene == 0 ? "A" : "B") + " oscillator " +
                            juce::String(oscillator + 1);
        const auto name = juce::String(state->target.tableName);
        setName("Wavetable for " + source);
        setTitle(getName());
        getProperties().set("reference.wavetable-name", name);
        getProperties().set("reference.wavetable-scene", scene);
        getProperties().set("reference.wavetable-oscillator", oscillator);
        getProperties().set("reference.wavetable-busy", state->loadQueued);
        const auto algorithm = juce::String(processor.surge->storage.getPatch()
                                                .scene[scene].osc[oscillator].type.get_display());
        browse.setButtonText(state->applicable ? name : algorithm);
        previous.setName("Previous wavetable for " + source);
        next.setName("Next wavetable for " + source);
        browse.setName("Choose wavetable for " + source + ": " + name);
        // Preserve focus while an accepted request is briefly queued. Further
        // actions wait for the real load, rather than stacking duplicate undo
        // entries or making a focused button disappear during every selection.
        const bool available = state->applicable && !processor.surge->storage.wt_list.empty();
        browse.setEnabled(available);
        previous.setEnabled(available && adjacentAvailable(false));
        next.setEnabled(available && adjacentAvailable(true));
        juce::String status;
        if (!state->applicable)
            status = "The current oscillator algorithm does not use a wavetable.";
        else if (state->loadQueued)
            status = "Loading wavetable. The displayed name is the current table.";
        else if (processor.surge->storage.wt_list.empty())
            status = "No installed wavetables are available.";
        else
            status = "Choose an installed wavetable; Left/Right chooses the previous/next table.";
        for (auto *button : {&previous, &browse, &next})
        {
            button->setTitle(button->getName());
            button->setTooltip(button->getName() + "\n" + status);
        }
        repaint();
    }
    void cancelPopup()
    {
        ++generation;
        if (popupOpen)
            juce::PopupMenu::dismissAllActiveMenus();
        popupOpen = false;
    }
    void visibilityChanged() override
    {
        if (!isShowing())
            cancelPopup();
    }
    void resized() override
    {
        auto r = getLocalBounds();
        const int arrowWidth = std::min(r.getWidth() / 3, std::min(28, r.getHeight()));
        previous.setBounds(r.removeFromLeft(arrowWidth));
        next.setBounds(r.removeFromRight(arrowWidth));
        browse.setBounds(r);
    }
    void paint(juce::Graphics &g) override
    {
        bezel(g, theme, getLocalBounds().toFloat().reduced(.6f), theme.display,
              theme.style == "bauhaus" || theme.style == "tokyo" ? 2.f : 3.f);
        g.setColour(theme.border.withAlpha(.5f));
        g.drawVerticalLine(previous.getRight(), 4.f, static_cast<float>(getHeight() - 4));
        g.drawVerticalLine(next.getX(), 4.f, static_cast<float>(getHeight() - 4));
    }

  private:
    bool canChoose(const Patch &entry) const
    {
        return !entry.path.empty() &&
               (entry.path.extension() != ".wtscript" || scriptsAvailable());
    }
    bool adjacentAvailable(bool forward) const
    {
        const auto &storage = processor.surge->storage;
        if (!state || storage.wtOrdering.size() != storage.wt_list.size())
            return false;
        const int id = storage.getAdjacentWaveTable(state->target.tableId, forward);
        return id >= 0 && id < static_cast<int>(storage.wt_list.size()) &&
               canChoose(storage.wt_list[id]);
    }
    void submit(const WavetableSelectionRequest &request)
    {
        if (request.target.scene != scene || request.target.oscillator != oscillator ||
            resolveWavetableSelection(processor, request) < 0)
            return;
        if (select(request))
        {
            cancelPopup();
            sync(scene, oscillator);
            if (browse.isShowing() && browse.isEnabled())
                browse.grabKeyboardFocus();
        }
    }
    void jog(int direction)
    {
        sync(scene, oscillator);
        if (!state || !state->applicable || state->loadQueued ||
            !adjacentAvailable(direction > 0))
            return;
        const auto &storage = processor.surge->storage;
        const int id = storage.getAdjacentWaveTable(state->target.tableId, direction > 0);
        submit({state->target, storage.wt_list[id].path});
    }
    void showCatalog()
    {
        sync(scene, oscillator);
        if (popupOpen || !browse.isEnabled() || !state || state->loadQueued)
            return;
        const auto &storage = processor.surge->storage;
        juce::PopupMenu menu;
        std::vector<fs::path> paths;
        std::vector<bool> added(storage.wt_list.size(), false);
        const auto append = [&](juce::PopupMenu &to, int id) {
            if (id < 0 || id >= static_cast<int>(storage.wt_list.size()) || added[id])
                return;
            const auto &entry = storage.wt_list[id];
            added[id] = true;
            paths.push_back(entry.path);
            to.addItem(static_cast<int>(paths.size()), juce::String(entry.name), canChoose(entry),
                       id == state->target.tableId);
        };
        const auto appendCategory = [&](int category) {
            if (category < 0 || category >= static_cast<int>(storage.wt_category.size()))
                return;
            juce::PopupMenu items;
            for (const auto id : storage.wtOrdering)
                if (id >= 0 && id < static_cast<int>(storage.wt_list.size()) &&
                    storage.wt_list[id].category == category)
                    append(items, id);
            // Preserve valid entries even while an external catalog provider
            // has no explicit ordering yet; never synthesize file names.
            for (int id = 0; id < static_cast<int>(storage.wt_list.size()); ++id)
                if (storage.wt_list[id].category == category)
                    append(items, id);
            if (items.getNumItems() > 0)
                menu.addSubMenu(juce::String(storage.wt_category[category].name), items);
        };
        for (const auto category : storage.wtCategoryOrdering)
            appendCategory(category);
        for (int category = 0; category < static_cast<int>(storage.wt_category.size()); ++category)
            appendCategory(category);
        for (int id = 0; id < static_cast<int>(storage.wt_list.size()); ++id)
            append(menu, id);
        if (paths.empty())
            return;

        popupOpen = true;
        const auto openedGeneration = ++generation;
        const auto target = state->target;
        juce::Component::SafePointer<WavetableSelector> safe(this);
        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(&browse).withMinimumWidth(getWidth()),
            [safe, openedGeneration, target, paths = std::move(paths)](int result) {
                if (!safe || safe->generation != openedGeneration)
                    return;
                safe->popupOpen = false;
                if (result > 0 && result <= static_cast<int>(paths.size()))
                    safe->submit({target, paths[static_cast<size_t>(result - 1)]});
            });
    }

    SurgeSynthProcessor &processor;
    Theme &theme;
    std::function<bool(const WavetableSelectionRequest &)> select;
    std::function<bool()> scriptsAvailable;
    SelectorButton previous, browse, next;
    int scene{-1}, oscillator{-1};
    std::optional<WavetableSelectionSnapshot> state;
    std::uint64_t generation{0};
    bool popupOpen{false};
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
        if (theme.style == "live")
        {
            g.setColour(juce::Colour(0xff030b09));
            g.drawRoundedRectangle(r.expanded(1), 19, 3);
            auto brass = juce::ColourGradient(juce::Colour(0xffffe0ac), r.getTopLeft(),
                                               juce::Colour(0xff8a5a2c), r.getBottomRight(), false);
            brass.addColour(.32, juce::Colour(0xff6c725b));
            brass.addColour(.63, juce::Colour(0xffdab884));
            g.setGradientFill(brass);
            g.drawRoundedRectangle(r, 18, 2);
            g.setColour(juce::Colour(0xffc9e9da).withAlpha(.6f));
            g.drawRoundedRectangle(r.reduced(2), 16, .7f);
        }
        {
            juce::Graphics::ScopedSaveState save(g);
            juce::Path clip;
            clip.addRoundedRectangle(r.reduced(2), 16);
            g.reduceClipRegion(clip);
            g.setGradientFill(juce::ColourGradient(theme.accent.withAlpha(.16f), r.getTopLeft(),
                                                   theme.secondary.withAlpha(.055f),
                                                   r.getBottomRight(), false));
            g.fillRect(r);
            // The glass is decorative; cursor and crosshairs always use live
            // macro values. No illustrated signal is presented as engine data.
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.045f),
                                                   r.getTopLeft(), juce::Colours::transparentWhite,
                                                   r.getBottomLeft(), false));
            g.fillRect(r.withHeight(r.getHeight() * .48f));
            if (theme.style == "live")
            {
                // Layered glass reflections, not waveform data. Their broad
                // translucent surfaces leave the actual XY cursor and axes clear.
                for (int layer = 0; layer < 5; ++layer)
                {
                    const float offset = layer * r.getHeight() * .035f;
                    juce::Path reflection;
                    reflection.startNewSubPath(r.getX(), r.getY() + r.getHeight() * .13f + offset);
                    reflection.cubicTo(r.getX() + r.getWidth() * .58f,
                                       r.getY() + r.getHeight() * .12f + offset,
                                       r.getX() + r.getWidth() * .24f,
                                       r.getY() + r.getHeight() * .63f - offset,
                                       r.getRight(), r.getY() + r.getHeight() * .42f + offset);
                    reflection.lineTo(r.getRight(), r.getY() + r.getHeight() * .66f + offset);
                    reflection.cubicTo(r.getX() + r.getWidth() * .63f,
                                       r.getY() + r.getHeight() * .87f - offset,
                                       r.getX() + r.getWidth() * .31f,
                                       r.getY() + r.getHeight() * .60f + offset,
                                       r.getX(), r.getY() + r.getHeight() * .90f - offset);
                    reflection.closeSubPath();
                    g.setGradientFill(juce::ColourGradient(theme.accent.withAlpha(.025f),
                                                           r.getTopLeft(),
                                                           theme.secondary.withAlpha(.035f),
                                                           r.getBottomRight(), false));
                    g.fillPath(reflection);
                }
            }
            Visuals::grid(g, r.reduced(5), theme.accent.withAlpha(.28f));
        }
        const float x = r.getX() + bridge.value("macro1") * r.getWidth();
        const float y = r.getBottom() - bridge.value("macro2") * r.getHeight();
        g.setGradientFill(juce::ColourGradient(theme.secondary.withAlpha(.22f), {x, y},
                                               theme.secondary.withAlpha(0.f),
                                               {x + r.getWidth() * .38f, y},
                                               true));
        g.fillRoundedRectangle(r, 18);
        g.setColour(theme.accent.withAlpha(.80f));
        g.drawLine(x, r.getY(), x, r.getBottom());
        g.drawLine(r.getX(), y, r.getRight(), y);
        g.setColour(theme.secondary.withAlpha(theme.style == "live" ? .07f : .12f));
        g.fillEllipse(x - 23, y - 23, 46, 46);
        g.setColour(theme.secondary.withAlpha(.24f));
        g.fillEllipse(x - 16, y - 16, 32, 32);
        g.setColour(theme.secondary);
        g.fillEllipse(x - 6, y - 6, 12, 12);
        g.setColour(juce::Colours::white);
        g.drawEllipse(x - 6, y - 6, 12, 12, 2);
        if (theme.style != "live")
            text(g, theme, "X: " + bridge.label("macro1") + "  /  Y: " + bridge.label("macro2"),
                 r.withTrimmedTop(r.getHeight() - 20), 10, theme.text, false,
                 juce::Justification::centred);
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
          list("Installed presets", this), back(t, "<"), fullBrowser(t, "...")
    {
        setName("Installed preset browser");
        setTitle(getName());
        search.setComponentID("workspace.browser.search");
        search.setName("Search installed presets");
        search.setTitle(search.getName());
        search.setDescription("Search real preset names and categories in the current catalog scope. Return loads the selected preset.");
        search.setTextToShowWhenEmpty("Search presets...", theme.muted);
        search.setColour(juce::TextEditor::backgroundColourId, browserFill());
        search.setColour(juce::TextEditor::textColourId, theme.text);
        search.setColour(juce::TextEditor::outlineColourId, theme.border.withAlpha(.6f));
        search.setColour(juce::TextEditor::focusedOutlineColourId, theme.accent);
        search.setIndents(25, 5);
        search.setFont(theme.type(theme.style == "tokyo" ? 13.f : 12.f));
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
            else if (categoryId >= 0)
                clearCategory();
            else if (auto *workspace = findParentComponentOfClass<Workspace>())
                workspace->grabKeyboardFocus();
        };
        addAndMakeVisible(search);
        list.setComponentID("workspace.browser.list");
        list.setTitle("Installed presets");
        list.setDescription("Arrow keys select; Return or double-click loads a preset or opens a category.");
        list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setColour(juce::ListBox::outlineColourId, theme.border);
        list.setMultipleSelectionEnabled(false);
        list.setRowHeight(theme.style == "tokyo" ? 26 : theme.style == "flow" ? 20 :
                          theme.style == "bauhaus" ? 21 : 24);
        list.setOutlineThickness(0);
        addAndMakeVisible(list);
        status.setName("Preset search status");
        status.setColour(juce::Label::textColourId, theme.muted);
        status.setFont(theme.type(9.5f));
        status.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(status);
        styleButton(back);
        styleButton(fullBrowser);
        back.setComponentID("workspace.browser.back");
        back.setTitle("Return to all preset categories");
        back.setTooltip("Return to all preset categories");
        back.onClick = [this] { clearCategory(); };
        addAndMakeVisible(back);
        fullBrowser.setComponentID("workspace.browser.full");
        fullBrowser.setTitle("Open the complete native preset browser");
        fullBrowser.setTooltip(fullBrowser.getTitle());
        fullBrowser.onClick = [this] { if (this->performAction) this->performAction("browser"); };
        addAndMakeVisible(fullBrowser);

        juce::StringArray captions;
        if (theme.style == "bauhaus")
            captions = {"BROWSER", "PATCH", "HISTORY"};
        else if (theme.style == "tokyo")
            captions = {"PRESETS", "FAVORITES", "HISTORY"};
        else if (theme.style == "flow")
            captions = {"PRESETS", "CATEGORIES", "FAVORITES"};
        else if (theme.style == "space")
            captions = {"ALL", "FACTORY", "USER"};
        else
            captions = {"All", "Factory", "User", "Fav"};
        for (int i = 0; i < captions.size(); ++i)
        {
            auto button = std::make_unique<ThemeButton>(theme, captions[i]);
            styleButton(*button);
            button->setComponentID("workspace.browser.tab." + juce::String(i));
            button->setTitle(captions[i] == "HISTORY" ? "Presets loaded during this browser session"
                              : captions[i] == "PATCH" ? "Inspect the current patch metadata"
                                                        : "Show " + captions[i].toLowerCase());
            button->setTooltip(button->getTitle());
            button->setToggleable(true);
            button->onClick = [this, i] { setPage(i); };
            button->setToggleState(i == 0, juce::dontSendNotification);
            addAndMakeVisible(*button);
            tabs.push_back(std::move(button));
        }
        for (auto *label : {&detailTitle, &detailCategory, &detailComment, &detailTags})
        {
            label->setColour(juce::Label::textColourId, theme.text);
            label->setJustificationType(juce::Justification::topLeft);
            label->setBorderSize(juce::BorderSize<int>(0));
            label->setMinimumHorizontalScale(1.f);
            addAndMakeVisible(*label);
        }
        detailTitle.setName("Selected or current preset name");
        detailCategory.setName("Actual preset category and author");
        detailComment.setName("Actual preset description");
        detailTags.setName("Actual preset tags");
        detailTitle.setFont(theme.type(theme.style == "flow" ? 13.f : 14.f, true));
        detailCategory.setFont(theme.type(10.5f));
        detailCategory.setColour(juce::Label::textColourId, theme.muted);
        detailComment.setFont(theme.type(11.5f));
        detailTags.setFont(theme.type(10.f));
        refreshRows();
        selectPatch(selectedPatch >= 0 ? selectedPatch : processor.surge->patchid);
        refreshDetails();
    }
    ~PresetBrowser() override { list.setModel(nullptr); }
    void paint(juce::Graphics &g) override
    {
        auto area = getLocalBounds().toFloat();
        if (theme.style == "space")
        {
            auto insetTheme = theme;
            insetTheme.style = "precision";
            bezel(g, insetTheme, area, juce::Colour(0xff251a0e), 5);
        }
        else
        {
            // This component owns its tab/search/card hierarchy. Cover the old
            // generic panel caption beneath the native top navigation tabs.
            g.setColour(browserFill());
            g.fillRoundedRectangle(area, theme.style == "aurora" ? 8.f : 2.f);
        }
        if (!detailBounds.isEmpty())
        {
            auto card = detailBounds.toFloat();
            g.setColour(theme.border.withAlpha(theme.style == "bauhaus" ? .5f : .55f));
            g.drawLine(card.getX(), card.getY(), card.getRight(), card.getY(), 1);
            if (theme.style == "aurora" || theme.style == "flow")
                bezel(g, theme, card.reduced(0, 3), theme.panel, 7);
            if (!thumbnail.isEmpty())
            {
                auto thumb = thumbnail.toFloat();
                if (theme.style == "bauhaus")
                {
                    // Original geometric patch-card ornament, containing no
                    // waveform, preset metadata or parameter-state pixels.
                    g.setColour(theme.red);
                    juce::Path arc;
                    arc.addPieSegment(thumb, -juce::MathConstants<float>::halfPi,
                                      0.f, 0.f);
                    g.fillPath(arc);
                    g.setColour(theme.text);
                    g.fillRect(thumb.withTrimmedTop(thumb.getHeight() * .45f));
                    juce::Path triangle;
                    triangle.addTriangle(thumb.getX(), thumb.getBottom(), thumb.getX(),
                                         thumb.getCentreY(), thumb.getRight(), thumb.getCentreY());
                    g.setColour(theme.secondary);
                    g.fillPath(triangle);
                }
                else
                {
                    g.setGradientFill(juce::ColourGradient(theme.accent.withAlpha(.45f),
                                                           thumb.getCentre(),
                                                           theme.display, thumb.getBottomRight(), true));
                    g.fillRoundedRectangle(thumb, 4);
                    // A folder engraving distinguishes this decorative mark
                    // from an analyzer or a frozen sound preview.
                    juce::Path folder;
                    folder.startNewSubPath(thumb.getX() + 8, thumb.getY() + 14);
                    folder.lineTo(thumb.getX() + 8, thumb.getY() + 9);
                    folder.lineTo(thumb.getX() + 20, thumb.getY() + 9);
                    folder.lineTo(thumb.getX() + 25, thumb.getY() + 14);
                    folder.lineTo(thumb.getRight() - 7, thumb.getY() + 14);
                    folder.lineTo(thumb.getRight() - 7, thumb.getBottom() - 9);
                    folder.lineTo(thumb.getX() + 8, thumb.getBottom() - 9);
                    folder.closeSubPath();
                    g.setColour(theme.text.withAlpha(.75f));
                    g.strokePath(folder, juce::PathStrokeType(1.2f));
                }
            }
        }
    }
    void paintOverChildren(juce::Graphics &g) override
    {
        if (search.isVisible())
        {
            auto icon = search.getBounds().toFloat().withWidth(25).reduced(7, 7);
            g.setColour(theme.muted);
            g.drawEllipse(icon.withSizeKeepingCentre(9, 9), 1.4f);
            const auto centre = icon.getCentre();
            g.drawLine(centre.x + 3, centre.y + 3, centre.x + 7, centre.y + 7, 1.4f);
        }
    }
    juce::String query() const { return search.getText(); }
    int selectedPatchId() const
    {
        const auto index = list.getSelectedRow();
        return validRow(index) ? rows[index].patchId : -1;
    }
    void refreshRows()
    {
        rememberCurrentPatch();
        auto updated = browserRows(processor, search.getText(), 100, filter, categoryId);
        if (page == Page::categories)
        {
            updated = browserRows(processor, {}, 100, filter);
            updated.erase(std::remove_if(updated.begin(), updated.end(),
                                         [this](const auto &row) {
                                             return !row.category ||
                                                 (search.getText().isNotEmpty() &&
                                                  !row.name.containsIgnoreCase(search.getText()));
                                         }), updated.end());
        }
        else if (page == Page::history)
        {
            updated.clear();
            const auto &catalog = processor.surge->storage.patch_list;
            for (const auto &path : history)
                for (size_t id = 0; id < catalog.size(); ++id)
                    if (path_to_string(catalog[id].path) == path &&
                        (search.getText().isEmpty() ||
                         juce::String(catalog[id].name).containsIgnoreCase(search.getText())))
                    {
                        updated.push_back({juce::String(catalog[id].name), static_cast<int>(id),
                                           false, catalog[id].category, 0, catalog[id].isFavorite});
                        break;
                    }
        }
        const bool changed = !initialized || updated.size() != rows.size() ||
                             !std::equal(updated.begin(), updated.end(), rows.begin(),
                                         [](const BrowserRow &a, const BrowserRow &b) {
                                             return a.name == b.name && a.patchId == b.patchId &&
                                                    a.category == b.category &&
                                                    a.categoryId == b.categoryId &&
                                                    a.patchCount == b.patchCount && a.favorite == b.favorite;
                                         });
        if (changed)
        {
            initialized = true;
            const auto selection = selectedPatchId();
            rows = std::move(updated);
            list.deselectAllRows();
            list.updateContent();
            selectPatch(selection >= 0 ? selection : processor.surge->patchid);
            list.repaint();
        }
        const auto count = static_cast<int>(std::count_if(rows.begin(), rows.end(),
                                                [](const auto &row) { return !row.category; }));
        status.setText(rows.empty() ? "No matching installed presets"
                       : rows.size() >= 100 ? "First 100 rows; refine search"
                       : page == Page::history ? juce::String(count) + " loaded in this browser session"
                       : categoryId >= 0 ? juce::String(count) + " presets in selected folder"
                       : juce::String(count) + " presets / " +
                             juce::String(static_cast<int>(rows.size()) - count) + " categories",
                       juce::dontSendNotification);
        refreshDetails();
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced(theme.style == "space" ? 4 : 0);
        const bool topTabs = theme.style == "bauhaus" || theme.style == "tokyo" || theme.style == "flow";
        const int tabsHeight = theme.style == "tokyo" ? 30 : 27;
        const auto placeTabs = [&] {
            auto row = r.removeFromTop(std::min(tabsHeight, r.getHeight()));
            const int gap = theme.style == "bauhaus" || theme.style == "tokyo" ? 1 : 4;
            const int width = (row.getWidth() - gap * (static_cast<int>(tabs.size()) - 1)) /
                              static_cast<int>(tabs.size());
            for (size_t i = 0; i < tabs.size(); ++i)
                tabs[i]->setBounds(row.getX() + static_cast<int>(i) * (width + gap), row.getY(),
                                   width, row.getHeight());
            r.removeFromTop(topTabs ? 5 : theme.style == "aurora" ? 12 : 8);
        };
        if (topTabs)
            placeTabs();
        auto searchRow = r.removeFromTop(std::min(29, r.getHeight()));
        fullBrowser.setBounds(searchRow.removeFromRight(26));
        searchRow.removeFromRight(4);
        back.setVisible(categoryId >= 0);
        if (back.isVisible())
        {
            back.setBounds(searchRow.removeFromLeft(23));
            searchRow.removeFromLeft(3);
        }
        search.setBounds(searchRow);
        r.removeFromTop(theme.style == "space" ? 7 : theme.style == "aurora" ? 14 :
                        theme.style == "bauhaus" ? 15 : 8);
        if (!topTabs)
            placeTabs();
        const bool showStatus = theme.style == "tokyo";
        status.setVisible(showStatus);
        if (showStatus)
            status.setBounds(r.removeFromBottom(std::min(18, r.getHeight())));
        detailBounds = {};
        thumbnail = {};
        const bool hasCard = theme.style == "bauhaus" || theme.style == "aurora" || theme.style == "flow";
        if (hasCard)
        {
            const int height = page == Page::patch ? r.getHeight()
                               : theme.style == "flow" ? 202 : theme.style == "bauhaus" ? 98 : 109;
            detailBounds = r.removeFromBottom(std::min(height, r.getHeight()));
            if (!r.isEmpty())
                r.removeFromBottom(8);
        }
        list.setBounds(r);
        list.setVisible(page != Page::patch);
        auto details = detailBounds.reduced(8, 10);
        const bool hasThumb = hasCard && theme.style != "flow";
        if (hasThumb)
        {
            thumbnail = details.removeFromLeft(theme.style == "bauhaus" ? 48 : 52);
            thumbnail.setHeight(std::min(71, thumbnail.getHeight()));
            details.removeFromLeft(9);
        }
        detailTitle.setBounds(details.removeFromTop(22));
        detailCategory.setBounds(details.removeFromTop(theme.style == "flow" ? 31 : 17));
        detailTags.setBounds(details.removeFromBottom(std::min(theme.style == "flow" ? 40 : 14,
                                                              details.getHeight())));
        detailComment.setBounds(details);
        for (auto *label : {&detailTitle, &detailCategory, &detailComment, &detailTags})
            label->setVisible(hasCard);
        repaint();
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key.getKeyCode() != juce::KeyPress::escapeKey)
            return false;
        if (categoryId >= 0)
            clearCategory();
        else
        {
            search.grabKeyboardFocus();
            search.selectAll();
        }
        return true;
    }

  private:
    enum class Page { presets, patch, history, categories };
    juce::Colour browserFill() const
    {
        return theme.style == "space" ? juce::Colour(0xff251a0e)
               : theme.style == "tokyo" ? juce::Colour(0xff080b0d)
               : theme.style == "bauhaus" ? theme.panel
               : theme.style == "aurora" ? juce::Colour(0xff21182f) : theme.display;
    }
    void styleButton(juce::TextButton &button)
    {
        button.setColour(juce::TextButton::buttonColourId,
                         theme.style == "space" ? theme.panel : browserFill());
        button.setColour(juce::TextButton::textColourOffId, theme.text);
        button.setWantsKeyboardFocus(true);
    }
    void setPage(int index)
    {
        page = Page::presets;
        filter = BrowserFilter::all;
        categoryId = -1;
        if (theme.style == "bauhaus")
            page = index == 1 ? Page::patch : index == 2 ? Page::history : Page::presets;
        else if (theme.style == "tokyo")
        {
            page = index == 2 ? Page::history : Page::presets;
            filter = index == 1 ? BrowserFilter::favorites : BrowserFilter::all;
        }
        else if (theme.style == "flow")
        {
            page = index == 1 ? Page::categories : Page::presets;
            filter = index == 2 ? BrowserFilter::favorites : BrowserFilter::all;
        }
        else
            filter = index == 1 ? BrowserFilter::factory : index == 2 ? BrowserFilter::user
                                      : index == 3 ? BrowserFilter::favorites : BrowserFilter::all;
        for (size_t i = 0; i < tabs.size(); ++i)
            tabs[i]->setToggleState(static_cast<int>(i) == index, juce::dontSendNotification);
        initialized = false;
        refreshRows();
        resized();
    }
    void clearCategory()
    {
        categoryId = -1;
        initialized = false;
        refreshRows();
        resized();
    }
    bool validRow(int row) const { return row >= 0 && static_cast<size_t>(row) < rows.size(); }
    void rememberCurrentPatch()
    {
        const auto &catalog = processor.surge->storage.patch_list;
        const int id = processor.surge->patchid;
        if (id < 0 || static_cast<size_t>(id) >= catalog.size())
            return;
        const auto path = path_to_string(catalog[id].path);
        if (path == lastLoadedPath)
            return;
        lastLoadedPath = path;
        history.erase(std::remove(history.begin(), history.end(), path), history.end());
        history.insert(history.begin(), path);
        if (history.size() > 32)
            history.resize(32);
    }
    void refreshDetails()
    {
        auto &storage = processor.surge->storage;
        const auto selected = selectedPatchId();
        const bool current = page == Page::patch || selected < 0 || selected == processor.surge->patchid;
        juce::String name, category, comment, tags;
        if (current)
        {
            const auto &patch = storage.getPatch();
            name = juce::String(patch.name).substring(0, 160);
            category = juce::String(patch.category).substring(0, 160);
            if (!patch.author.empty())
                category += (category.isEmpty() ? "" : " / ") + juce::String(patch.author).substring(0, 100);
            comment = juce::String(patch.comment).substring(0, 320);
            for (const auto &tag : patch.tags)
            {
                if (tags.length() > 100)
                    break;
                tags += (tags.isEmpty() ? "" : "  /  ") + juce::String(tag.tag).substring(0, 40);
            }
        }
        else if (selected >= 0 && static_cast<size_t>(selected) < storage.patch_list.size())
        {
            const auto &patch = storage.patch_list[selected];
            name = juce::String(patch.name);
            if (patch.category >= 0 && static_cast<size_t>(patch.category) < storage.patch_category.size())
                category = juce::String(storage.patch_category[patch.category].name);
            comment = "Enter or double-click to load this installed preset.";
            if (patch.isFavorite)
                tags = "Favorite";
        }
        if (name.isEmpty())
            name = "Current patch";
        detailTitle.setText(name, juce::dontSendNotification);
        detailCategory.setText(category, juce::dontSendNotification);
        detailComment.setText(comment, juce::dontSendNotification);
        detailTags.setText(tags, juce::dontSendNotification);
    }
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
            categoryId = entry.categoryId;
            if (page == Page::categories)
            {
                page = Page::presets;
                for (size_t i = 0; i < tabs.size(); ++i)
                    tabs[i]->setToggleState(i == 0, juce::dontSendNotification);
            }
            initialized = false;
            refreshRows();
            resized();
            list.grabKeyboardFocus();
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
        return validRow(row) ? rows[row].name + utf8(rows[row].category ? " — Enter to open folder"
                                                                        : " — Enter to load")
                             : juce::String();
    }
    void paintListBoxItem(int row, juce::Graphics &g, int width, int height, bool selected) override
    {
        if (!validRow(row))
            return;
        auto r = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
        const auto &entry = rows[row];
        if (selected)
        {
            auto fill = theme.accent;
            if (theme.style == "space")
                fill = juce::Colour(0xff9a4e10);
            g.setGradientFill(juce::ColourGradient(fill, r.getTopLeft(), fill.darker(.2f),
                                                  r.getBottomLeft(), false));
            g.fillRoundedRectangle(r.reduced(1, .5f), 3);
        }
        else if (theme.style == "tokyo" && !entry.category)
        {
            g.setColour(theme.border.withAlpha(.16f));
            g.drawHorizontalLine(height - 1, 2.f, width - 2.f);
        }
        auto ink = selected && theme.style == "flow" ? theme.display
                   : selected && theme.style == "bauhaus" ? juce::Colours::white : theme.text;
        auto icon = r.removeFromLeft(entry.category ? 30.f : theme.style == "flow" ? 38.f : 33.f);
        if (entry.category)
        {
            juce::Path chevron;
            chevron.addTriangle(3, height * .5f - 3, 3, height * .5f + 3, 7, height * .5f);
            g.setColour(ink.withAlpha(.75f));
            g.fillPath(chevron);
            const float y = height * .5f - 4;
            juce::Path folder;
            folder.startNewSubPath(12, y + 1);
            folder.lineTo(12, y - 2);
            folder.lineTo(18, y - 2);
            folder.lineTo(20, y + 1);
            folder.lineTo(26, y + 1);
            folder.lineTo(26, y + 8);
            folder.lineTo(12, y + 8);
            folder.closeSubPath();
            if (theme.style == "bauhaus")
                g.strokePath(folder, juce::PathStrokeType(1.2f));
            else
                g.fillPath(folder);
            auto count = r.removeFromRight(theme.style == "bauhaus" ? 28.f : 39.f);
            if (theme.style == "bauhaus")
            {
                g.setColour(theme.border.withAlpha(.35f));
                g.fillRoundedRectangle(count.reduced(1, 2), 3);
            }
            text(g, theme, juce::String(entry.patchCount), count, 10.5f, ink.withAlpha(.8f),
                 false, juce::Justification::centredRight);
        }
        else if (entry.favorite)
        {
            auto star = juce::Path();
            star.addStar({width - 11.f, height * .5f}, 5, 3.f, 6.f);
            g.setColour(ink);
            g.strokePath(star, juce::PathStrokeType(1));
            r.removeFromRight(20);
        }
        text(g, theme, entry.name, r.reduced(2, 0), theme.style == "tokyo" ? 13.f : 12.f,
             ink, entry.category && theme.style != "space");
    }
    void selectedRowsChanged(int) override { refreshDetails(); }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &) override { activate(row); }
    void returnKeyPressed(int row) override { activate(row); }

    SurgeSynthProcessor &processor;
    Theme &theme;
    std::function<void(const juce::String &)> performAction;
    std::vector<BrowserRow> rows;
    juce::TextEditor search;
    juce::ListBox list;
    juce::Label status, detailTitle, detailCategory, detailComment, detailTags;
    ThemeButton back, fullBrowser;
    std::vector<std::unique_ptr<ThemeButton>> tabs;
    juce::Rectangle<int> detailBounds, thumbnail;
    std::vector<std::string> history;
    std::string lastLoadedPath;
    BrowserFilter filter{BrowserFilter::all};
    Page page{Page::presets};
    int categoryId{-1};
    bool initialized{false};
};

/** Native MIDI performance input. These lanes are controllers, not patch parameters. */
class MidiController : public juce::Slider
{
    class LaneLookAndFeel : public juce::LookAndFeel_V4
    {
      public:
        juce::Slider::SliderLayout getSliderLayout(juce::Slider &slider) override
        {
            juce::Slider::SliderLayout layout;
            layout.sliderBounds = slider.getLocalBounds().withTrimmedTop(2).withTrimmedBottom(22);
            return layout;
        }
        int getSliderThumbRadius(juce::Slider &) override { return 0; }
    };

  public:
    MidiController(SurgeSynthProcessor &processor, Theme &theme, const juce::String &controller)
        : processor(processor), theme(theme)
    {
        using M = SurgeSynthProcessor::midiR;
        type = controller == "pitch" ? M::PITCHWHEEL
               : controller == "mod" ? M::MODWHEEL
               : controller == "expression" ? M::EXPRESSION : M::AFTERTOUCH;
        spring = type == M::PITCHWHEEL || type == M::AFTERTOUCH;
        const auto name = type == M::PITCHWHEEL ? "Pitch bend"
                          : type == M::MODWHEEL ? "Modulation wheel (CC 1)"
                          : type == M::EXPRESSION ? "Expression (CC 11)" : "Channel aftertouch";
        caption = type == M::PITCHWHEEL ? "Pitch" : type == M::MODWHEEL ? "Mod"
                  : type == M::EXPRESSION ? "Exp" : "Touch";
        setComponentID("workspace.midi." + controller);
        setName(name);
        setTitle(name);
        getProperties().set("reference.midi-controller", controller);
        getProperties().set("reference.midi-channel", 1);
        getProperties().set("reference.midi-spring-return", spring);
        setLookAndFeel(&laneLookAndFeel);
        setSliderStyle(juce::Slider::LinearVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRange(type == M::PITCHWHEEL ? -8192 : 0, type == M::PITCHWHEEL ? 8191 : 127, 1);
        setWantsKeyboardFocus(true);
        setScrollWheelEnabled(!spring);
        setMouseDragSensitivity(128);
        setTooltip(juce::String(name) + "; MIDI channel 1. " +
                   (spring ? "Release the pointer or arrow key to return to zero. "
                           : "The last applied controller value is retained. ") +
                   "Arrow keys adjust; Shift is fine. Return or right-click enters a MIDI value. "
                   "Double-click resets to zero. Values follow the engine's applied input.");
        setHelpText(getTooltip());
        onDragStart = [this] { dragging = true; };
        onValueChange = [this] {
            if (!polling)
                requestValue(juce::roundToInt(getValue()));
        };
        onDragEnd = [this] { cancelGesture(); };
        sync();
    }
    ~MidiController() override
    {
        cancelGesture();
        dismissEntry();
        setLookAndFeel(nullptr);
    }
    void sync()
    {
        setEnabled(processor.surge != nullptr);
        const int applied = appliedValue();
        getProperties().set("reference.midi-applied", applied);
        if (!dragging && !keyboardHeld)
        {
            const juce::ScopedValueSetter<bool> guard(polling, true);
            setValue(applied, juce::dontSendNotification);
        }
        setDescription(getName() + ": " + juce::String(applied) + "; MIDI channel 1. " +
                       (spring ? "Returns to zero when this control's gesture ends."
                               : "Latched MIDI controller; changing skins does not reset it."));
    }
    void cancelGesture()
    {
        dragging = false;
        keyboardHeld = false;
        // A view change may end our gesture, but must not cancel a controller
        // held by an external keyboard or reset latched Mod/Expression input.
        if (ownsSpring)
        {
            ownsSpring = false;
            processor.queueMidiController(type, 0);
        }
        sync();
    }
    void dismissEntry()
    {
        ++entryGeneration;
        if (entryBox)
            entryBox->dismiss();
        entryBox = nullptr;
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
    void mouseDoubleClick(const juce::MouseEvent &) override
    {
        cancelGesture();
        requestValue(0);
    }
    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key.getKeyCode() == juce::KeyPress::returnKey)
        {
            showEntry();
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::escapeKey)
        {
            cancelGesture();
            return true;
        }
        const int direction = key.getKeyCode() == juce::KeyPress::upKey ||
                                      key.getKeyCode() == juce::KeyPress::rightKey ? 1
                              : key.getKeyCode() == juce::KeyPress::downKey ||
                                      key.getKeyCode() == juce::KeyPress::leftKey ? -1 : 0;
        if (direction == 0 || key.getModifiers().isCtrlDown() ||
            key.getModifiers().isAltDown() || key.getModifiers().isCommandDown())
            return juce::Slider::keyPressed(key);
        // A completed spring release may already have reached the processor
        // before the next UI timer refresh. Start each new key gesture from
        // the applied input; repeats retain their queued target until consumed.
        const int base = keyboardHeld ? juce::roundToInt(getValue()) : appliedValue();
        keyboardHeld = true;
        const int step = type == SurgeSynthProcessor::midiR::PITCHWHEEL
                             ? (key.getModifiers().isShiftDown() ? 16 : 256) : 1;
        const int next = juce::jlimit(static_cast<int>(getMinimum()),
                                     static_cast<int>(getMaximum()),
                                     base + direction * step);
        setValue(next, juce::dontSendNotification);
        requestValue(next);
        return true;
    }
    bool keyStateChanged(bool down) override
    {
        if (!down && keyboardHeld &&
            !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::upKey) &&
            !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::downKey) &&
            !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::leftKey) &&
            !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::rightKey))
        {
            cancelGesture();
            return true;
        }
        return juce::Slider::keyStateChanged(down);
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
    juce::String getTextFromValue(double value) override
    {
        return juce::String(juce::roundToInt(value));
    }
    double getValueFromText(const juce::String &value) override
    {
        int parsed{};
        return parseValue(value, parsed) ? parsed : appliedValue();
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class ValueInterface final : public juce::AccessibilityValueInterface
        {
          public:
            explicit ValueInterface(MidiController &control) : control(control) {}
            bool isReadOnly() const override { return !control.isEnabled(); }
            double getCurrentValue() const override { return control.appliedValue(); }
            juce::String getCurrentValueAsString() const override
            {
                return juce::String(control.appliedValue());
            }
            void setValue(double value) override
            {
                if (control.isEnabled() && std::isfinite(value))
                    control.requestValue(juce::roundToInt(juce::jlimit(
                        control.getMinimum(), control.getMaximum(), value)));
            }
            void setValueAsString(const juce::String &value) override
            {
                int parsed{};
                if (control.isEnabled() && control.parseValue(value, parsed))
                    control.requestValue(parsed);
            }
            AccessibleValueRange getRange() const override
            {
                return {{control.getMinimum(), control.getMaximum()}, 1};
            }
          private:
            MidiController &control;
        };
        return std::make_unique<juce::AccessibilityHandler>(
            *this, juce::AccessibilityRole::slider, juce::AccessibilityActions{},
            juce::AccessibilityHandler::Interfaces{std::make_unique<ValueInterface>(*this)});
    }
    void paint(juce::Graphics &g) override
    {
        const auto r = getLocalBounds().toFloat();
        const auto track = Rect(r.getCentreX() - 10.f, 2.f, 20.f, r.getHeight() - 24.f);
        const auto amber = juce::Colour(0xffddb083);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff101512), track.getTopLeft(),
                                               juce::Colour(0xff281e17), track.getBottomRight(), false));
        g.fillRoundedRectangle(track, 3.f);
        g.setColour(amber.withAlpha(.34f));
        g.drawRoundedRectangle(track.reduced(.5f), 3.f, .8f);
        const auto y = juce::jlimit(track.getY(), track.getBottom(), getPositionOfValue(getValue()));
        const auto zero = type == SurgeSynthProcessor::midiR::PITCHWHEEL
                              ? getPositionOfValue(0) : track.getBottom();
        auto active = track.reduced(1.3f, 0).withTop(std::min(y, zero))
                          .withBottom(std::max(y, zero));
        if (active.getHeight() > 0)
        {
            g.setGradientFill(juce::ColourGradient(amber.withAlpha(.75f), active.getTopLeft(),
                                                   amber.darker(.65f).withAlpha(.35f),
                                                   active.getBottomLeft(), false));
            g.fillRoundedRectangle(active, 2.f);
        }
        g.setColour(juce::Colours::white.withAlpha(.06f));
        g.drawVerticalLine(juce::roundToInt(track.getX() + 3), track.getY() + 3,
                           track.getBottom() - 3);
        g.setColour(amber.withAlpha(.9f));
        g.drawLine(track.getX() + 2, y, track.getRight() - 2, y, 1.2f);
        text(g, theme, caption, r.withTop(track.getBottom() + 5), 10.5f, theme.text,
             false, juce::Justification::centred);
        if (hasKeyboardFocus(true) || isMouseOverOrDragging())
        {
            g.setColour(theme.accent.withAlpha(hasKeyboardFocus(true) ? .95f : .45f));
            g.drawRoundedRectangle(track.expanded(2, 1), 4.f, 1.f);
        }
    }

  private:
    int appliedValue() const
    {
        if (!processor.surge)
            return 0;
        using M = SurgeSynthProcessor::midiR;
        const auto &synth = *processor.surge;
        return type == M::PITCHWHEEL ? synth.pitchbendMIDIVal.load(std::memory_order_relaxed)
               : type == M::MODWHEEL ? synth.modwheelCC.load(std::memory_order_relaxed)
               : type == M::EXPRESSION ? synth.expressionCC.load(std::memory_order_relaxed)
                                      : synth.channelAftertouchMIDIVal.load(std::memory_order_relaxed);
    }
    bool requestValue(int value)
    {
        value = juce::jlimit(static_cast<int>(getMinimum()), static_cast<int>(getMaximum()), value);
        const bool accepted = processor.queueMidiController(type, value);
        if (accepted && spring)
            ownsSpring = value != 0;
        getProperties().set("reference.midi-last-input-accepted", accepted);
        if (accepted)
            getProperties().set("reference.midi-last-request", value);
        sync();
        repaint();
        return accepted;
    }
    bool parseValue(const juce::String &value, int &parsed) const
    {
        auto token = value.trim().toStdString();
        if (!token.empty() && token.front() == '+')
        {
            token.erase(token.begin());
            if (token.empty() || token.front() == '-' || token.front() == '+')
                return false;
        }
        const auto result = std::from_chars(token.data(), token.data() + token.size(), parsed);
        return !token.empty() && result.ec == std::errc{} &&
               result.ptr == token.data() + token.size() &&
               parsed >= getMinimum() && parsed <= getMaximum();
    }
    void showEntry()
    {
        if (!isEnabled())
            return;
        cancelGesture();
        dismissEntry();
        const auto generation = entryGeneration;
        juce::Component::SafePointer<MidiController> safe(this);
        auto entry = std::make_unique<ValueEntry>(
            theme, getName(), juce::String(appliedValue()),
            [safe, generation](const juce::String &value, juce::String &error) {
                int parsed{};
                if (!safe || !safe->isEnabled() || safe->entryGeneration != generation)
                {
                    error = "This MIDI controller is no longer available.";
                    return false;
                }
                if (!safe->parseValue(value, parsed))
                {
                    error = "Enter a whole MIDI value from " +
                            juce::String(static_cast<int>(safe->getMinimum())) + " to " +
                            juce::String(static_cast<int>(safe->getMaximum())) + ".";
                    return false;
                }
                if (!safe->requestValue(parsed))
                {
                    error = "The MIDI input queue is full. Try again after audio resumes.";
                    return false;
                }
                juce::MessageManager::callAsync([safe] {
                    if (safe && safe->isShowing())
                        safe->grabKeyboardFocus();
                });
                return true;
            });
        auto *parent = findParentComponentOfClass<Workspace>();
        const auto target = parent ? parent->getLocalArea(this, getLocalBounds()) : getScreenBounds();
        auto &callout = juce::CallOutBox::launchAsynchronously(std::move(entry), target, parent);
        callout.setDismissalMouseClicksAreAlwaysConsumed(true);
        entryBox = &callout;
    }
    SurgeSynthProcessor &processor;
    Theme &theme;
    juce::String caption;
    SurgeSynthProcessor::midiR::Type type{SurgeSynthProcessor::midiR::MODWHEEL};
    bool spring{false}, dragging{false}, keyboardHeld{false}, ownsSpring{false}, polling{false};
    LaneLookAndFeel laneLookAndFeel;
    juce::Component::SafePointer<juce::CallOutBox> entryBox;
    std::uint64_t entryGeneration{0};
};

class PerformanceKeyboard : public juce::MidiKeyboardComponent
{
  public:
    explicit PerformanceKeyboard(juce::MidiKeyboardState &state, bool useFooterLabels = false)
        : juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard),
          footerLabels(useFooterLabels)
    {
        getProperties().set("reference.keyboard-footer-labels", footerLabels);
    }
    juce::String getWhiteNoteText(int midiNoteNumber) override
    {
        return footerLabels ? juce::String{}
                            : juce::MidiKeyboardComponent::getWhiteNoteText(midiNoteNumber);
    }
    juce::String noteName(int midiNoteNumber) const
    {
        // Preserve JUCE's existing octave convention; these names describe the
        // same MIDI identities as the playable keys, including the terminal C.
        return juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, true,
                                                 getOctaveForMiddleC());
    }
    Rect keyBoundsOnCanvas(int midiNoteNumber) const
    {
        return getRectangleForKey(midiNoteNumber).translated(static_cast<float>(getX()),
                                                             static_cast<float>(getY()));
    }
    void drawFooter(juce::Graphics &g, const Theme &theme)
    {
        juce::StringArray labels;
        for (int note = getRangeStart(); note <= getRangeEnd(); ++note)
            if (note % 12 == 0)
            {
                const auto key = keyBoundsOnCanvas(note);
                const auto name = noteName(note);
                text(g, theme, name, {key.getCentreX() - 14, key.getBottom() + 3, 28, 15},
                     10, theme.text.withAlpha(.85f), false, juce::Justification::centred);
                labels.add(name);
            }
        getProperties().set("reference.keyboard-octave-labels", labels.joinIntoString(" "));
        getProperties().set("reference.keyboard-octave-label-count", labels.size());
    }
    void releaseHeldNotes()
    {
        // Public in JUCE 7 and 8; releases only keys owned by this component.
        juce::MidiKeyboardComponent::focusLost(juce::Component::focusChangedDirectly);
    }
    ~PerformanceKeyboard() override { releaseHeldNotes(); }

  private:
    const bool footerLabels;
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
    std::array<std::array<float, 2048>, 2> stereoHistory{};
    std::array<std::vector<float>, 2> stereoOutput, stereoSpectrumDb;
    std::array<float, 4096> fftData{};
    size_t outputWrite{0}, outputCount{0};
    size_t framesSinceSpectrum{0};
    std::uint64_t capturedSamples{0};
    double capturedSampleRate{0};
    std::uint32_t lastDroppedFrames{0};
    std::vector<float> spectrumDb;
    std::vector<std::vector<float>> spectrumHistory;
    std::vector<double> spectrumHistoryTimes;
    std::array<PreviewCurve, 2> filterCurves;
    std::array<PreviewCurve, 12> lfoCurves;
    std::array<PreviewCurve, n_fx_slots> effectCurves;
    bool audioSubscribed{false};
    std::vector<RouteRow> routes;
    ModulationEditorModel routeEditor;
    std::vector<std::unique_ptr<ModulationRowInteraction>> routeControls;
    RouteKey inspectedRoute;
    juce::Component canvas;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::pair<juce::Component *, Rect>> placements;
    std::vector<std::unique_ptr<juce::TextButton>> actions;
    struct SelectionState
    {
        juce::String id, selected;
        juce::var definition;
        juce::var choice() const
        {
            if (auto *choices = definition["choices"].getArray())
                for (const auto &candidate : *choices)
                    if (string(candidate, "id") == selected)
                        return candidate;
            return {};
        }
    };
    struct SelectionBinding
    {
        juce::Component *control{};
        juce::String group;
        juce::var bindings;
    };
    struct SelectionTab
    {
        juce::String group, choice;
        std::unique_ptr<ViewTabButton> button;
    };
    std::vector<SelectionState> selections;
    std::vector<SelectionBinding> selectionBindings;
    std::vector<SelectionTab> selectionTabs;
    struct AnalyzerState
    {
        juce::String id, selected;
        juce::StringArray modes;
    };
    struct AnalyzerTab
    {
        juce::String panel, mode;
        std::unique_ptr<ViewTabButton> button;
    };
    std::vector<AnalyzerState> analyzers;
    std::vector<AnalyzerTab> analyzerTabs;
    struct WavetableControl
    {
        juce::var panel;
        std::unique_ptr<WavetableSelector> selector;
    };
    std::vector<WavetableControl> wavetableControls;
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
    HeaderButton presetSearchButton{theme, "Search presets..."},
        presetListButton{theme, "Browse presets"};
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
                        &next, &browserButton, &saveButton, &modButton, &fxButton, &synthButton,
                        &presetSearchButton, &presetListButton})
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
        presetSearchButton.setComponentID("workspace.header.preset-search");
        presetListButton.setComponentID("workspace.header.preset-list");
        presetSearchButton.setName("Open preset browser to search presets");
        presetListButton.setName("Open the native preset list");
        for (auto *b : {&presetSearchButton, &presetListButton})
        {
            b->setTitle(b->getName());
            b->setDescription("Opens the native preset browser to search and load installed presets");
            b->setTooltip("Open the native preset browser to search and load installed presets");
            b->onClick = [this] { action("browser"); };
        }
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
    SelectionState *selection(const juce::String &id)
    {
        for (auto &candidate : selections)
            if (candidate.id == id)
                return &candidate;
        return nullptr;
    }
    const SelectionState *selection(const juce::String &id) const
    {
        for (const auto &candidate : selections)
            if (candidate.id == id)
                return &candidate;
        return nullptr;
    }
    juce::var selectedPanelChoice(const juce::var &panel) const
    {
        if (auto *selected = selection(string(panel["selection"], "id")))
            return selected->choice();
        return {};
    }
    juce::String selectionCaption(const SelectionState &state) const
    {
        const auto selected = state.choice();
        if (string(selected, "source") == "fx")
        {
            const auto index = static_cast<int>(number(selected, "index", 1));
            return "FX " + juce::String(index) + ": " +
                   parameters.display("fx" + juce::String(index) + ".type");
        }
        return string(selected, "label");
    }
    void applySelectionBindings(SelectionState &state)
    {
        const auto caption = selectionCaption(state);
        for (auto &mapping : selectionBindings)
            if (mapping.group == state.id)
            {
                const auto value = mapping.bindings[juce::Identifier(state.selected)];
                const auto binding = value.isString() ? value.toString() : juce::String();
                mapping.control->getProperties().set("reference.source-caption", caption);
                if (auto *slider = dynamic_cast<BoundSlider *>(mapping.control))
                    slider->rebind(binding);
                else if (auto *menu = dynamic_cast<BoundMenu *>(mapping.control))
                    menu->rebind(binding);
                else if (auto *toggle = dynamic_cast<BoundToggle *>(mapping.control))
                    toggle->rebind(binding);
            }
    }
    bool selectView(const juce::String &id, const juce::String &choice)
    {
        auto *state = selection(id);
        if (state == nullptr)
            return false;
        bool present = false;
        if (auto *choices = state->definition["choices"].getArray())
            for (const auto &candidate : *choices)
                present = present || string(candidate, "id") == choice;
        if (!present)
            return false;
        const auto focusVisibleSource = [&] {
            // A tabs-only container is navigation among simultaneously visible
            // modules (for example Ivory's three oscillator cards). Give it a
            // real keyboard-focus action without painting another hidden plot
            // or writing an oscillator algorithm parameter.
            bool navigationOnly = false;
            if (auto *panels = manifest["panels"].getArray())
                for (const auto &panel : *panels)
                    navigationOnly = navigationOnly ||
                                     (string(panel, "kind") == "tabs" &&
                                      string(panel["selection"], "id") == id);
            const auto selected = state->choice();
            if (!navigationOnly)
                return;
            const auto source = string(selected, "source");
            const auto index = static_cast<int>(number(selected, "index", 1));
            juce::String destinationGroup;
            if (source != "oscillator" && source != "waveform")
                if (auto *panels = manifest["panels"].getArray())
                    for (const auto &panel : *panels)
                    {
                        if (string(panel, "kind") == "tabs" || destinationGroup.isNotEmpty())
                            continue;
                        const auto group = string(panel["selection"], "id");
                        if (group.isEmpty() || group == id)
                            continue;
                        if (auto *choices = panel["selection"]["choices"].getArray())
                            for (const auto &candidate : *choices)
                                if (string(candidate, "source") == source &&
                                    static_cast<int>(number(candidate, "index", 1)) == index)
                                {
                                    destinationGroup = group;
                                    selectView(group, string(candidate, "id"));
                                    break;
                                }
                    }
            const auto family = source == "oscillator" || source == "waveform" ? "osc"
                                : source == "envelope" ? "env" : "lfo";
            const auto prefix = juce::String(family) + juce::String(index) + ".";
            for (auto &control : controls)
                if (dynamic_cast<BoundSlider *>(control.get()) != nullptr &&
                    control->getProperties()["reference.binding"].toString().startsWith(prefix) &&
                    (destinationGroup.isEmpty() ||
                     std::any_of(selectionBindings.begin(), selectionBindings.end(),
                                 [&](const auto &mapping) {
                                     return mapping.control == control.get() &&
                                            mapping.group == destinationGroup;
                                 })) &&
                    control->isEnabled() && control->isShowing())
                {
                    control->grabKeyboardFocus();
                    break;
                }
        };
        if (state->selected == choice)
        {
            focusVisibleSource();
            return true;
        }
        cancelInteractions();
        state->selected = choice;
        applySelectionBindings(*state);
        ticks = 0;
        // Rebind stable components. In particular, never clear the button which
        // is currently executing this callback by calling build() here.
        owner.refresh();
        focusVisibleSource();
        return true;
    }
    void moveView(const juce::String &id, const juce::String &from, int delta)
    {
        auto *state = selection(id);
        if (state == nullptr)
            return;
        if (auto *choices = state->definition["choices"].getArray())
            for (int i = 0; i < choices->size(); ++i)
                if (string(choices->getReference(i), "id") == from)
                {
                    const int next = (i + delta + choices->size()) % choices->size();
                    const auto target = string(choices->getReference(next), "id");
                    selectView(id, target);
                    for (auto &tab : selectionTabs)
                        if (tab.group == id && tab.choice == target && tab.button->isShowing())
                            tab.button->grabKeyboardFocus();
                    return;
                }
    }
    void syncViewTabs()
    {
        for (auto &tab : selectionTabs)
            if (auto *state = selection(tab.group))
            {
                if (auto *choices = state->definition["choices"].getArray())
                    for (const auto &choice : *choices)
                        if (string(choice, "id") == tab.choice)
                        {
                            auto caption = string(choice, "label");
                            if (string(choice, "source") == "fx")
                            {
                                const auto slot = static_cast<int>(number(choice, "index", 1));
                                caption = juce::String(slot) + "  " +
                                          parameters.display("fx" + juce::String(slot) + ".type");
                            }
                            tab.button->setButtonText(caption);
                            tab.button->syncEffectPower();
                            tab.button->setName("Select " + caption);
                            tab.button->setTitle(tab.button->getName());
                            tab.button->setTooltip("Select " + caption +
                                                   ". Left and right arrows select adjacent views.");
                            tab.button->setToggleState(state->selected == tab.choice,
                                                       juce::dontSendNotification);
                            break;
                        }
            }
        // Algorithm changes can change the accessible FX source caption while
        // preserving the same stable control and selected slot.
        for (auto &mapping : selectionBindings)
            if (auto *state = selection(mapping.group))
                mapping.control->getProperties().set("reference.source-caption",
                                                     selectionCaption(*state));
    }
    void buildViewTabs()
    {
        for (const auto &state : selections)
        {
            const auto *choices = state.definition["choices"].getArray();
            if (choices == nullptr || choices->isEmpty())
                continue;
            const auto rectangle = arrayRect(state.definition["tabsRect"], {});
            const int columns = juce::jlimit(
                1, choices->size(),
                static_cast<int>(number(state.definition, "columns", choices->size())));
            const int rows = (choices->size() + columns - 1) / columns;
            const float gap = 4.f;
            const float width = (rectangle.getWidth() - gap * (columns - 1)) / columns;
            const float height = (rectangle.getHeight() - gap * (rows - 1)) / rows;
            for (int i = 0; i < choices->size(); ++i)
            {
                const auto &choice = choices->getReference(i);
                auto button = std::make_unique<ViewTabButton>(
                    theme, number(state.definition, "fontSize", theme.style == "clear" ? 13.f : 11.f),
                    string(state.definition, "kind") == "fx");
                const auto choiceId = string(choice, "id");
                const auto group = state.id;
                button->setComponentID("workspace.selection." + group + "." + choiceId);
                if (string(choice, "source") == "fx")
                    button->addEffectPower(processor, static_cast<int>(number(choice, "index", 1)));
                button->onClick = [this, group, choiceId] { selectView(group, choiceId); };
                button->onNavigate = [this, group, choiceId](int delta) {
                    moveView(group, choiceId, delta);
                };
                canvas.addAndMakeVisible(*button);
                placements.push_back(
                    {button.get(),
                     {rectangle.getX() + (i % columns) * (width + gap),
                      rectangle.getY() + (i / columns) * (height + gap), width, height}});
                selectionTabs.push_back({group, choiceId, std::move(button)});
            }
        }
        syncViewTabs();
    }
    void syncWavetableControls()
    {
        for (auto &control : wavetableControls)
        {
            auto oscillator = static_cast<int>(number(control.panel, "index", 1)) - 1;
            const auto selected = selectedPanelChoice(control.panel);
            if (selected.isObject())
            {
                const auto source = string(selected, "source");
                oscillator = source == "oscillator" || source == "waveform"
                                 ? static_cast<int>(number(selected, "index", 1)) - 1
                                 : -1;
            }
            control.selector->sync(parameters.getScene(), oscillator);
        }
    }
    juce::String analyzerCaption(const juce::String &mode) const
    {
        if (mode == "waterfall")
            return theme.style == "clear" || theme.style == "live" ? "Spectrogram" : "Waterfall";
        if (mode == "scope")
            return theme.style == "live" ? "Scope" : "Oscilloscope";
        if (mode == "vectorscope")
            return "Vectorscope";
        return mode == "meter" ? "Meter" : "Spectrum";
    }
    juce::String analyzerMode(const juce::var &panel) const
    {
        for (const auto &state : analyzers)
            if (state.id == string(panel, "id"))
                return state.selected;
        const auto variant = string(panel, "variant");
        return variant == "spectrogram" ? "waterfall" : variant == "scope" ? "scope" : "spectrum";
    }
    void selectAnalyzer(const juce::String &panel, const juce::String &mode, bool focus = false)
    {
        for (auto &state : analyzers)
            if (state.id == panel && state.modes.contains(mode))
            {
                state.selected = mode;
                for (auto &tab : analyzerTabs)
                    if (tab.panel == panel)
                    {
                        tab.button->setToggleState(tab.mode == mode, juce::dontSendNotification);
                        if (focus && tab.mode == mode)
                            tab.button->grabKeyboardFocus();
                    }
                // This is a display selection only: never edit parameters, undo,
                // MIDI state, or the audio consumer's capture history.
                owner.repaint();
                return;
            }
    }
    void buildAnalyzerTabs(const juce::var &panel)
    {
        const auto *modes = panel["analyzerModes"].getArray();
        const auto rectangle = arrayRect(panel["analyzerModesRect"], {});
        if (modes == nullptr || modes->size() < 2 || rectangle.isEmpty())
            return;
        AnalyzerState state;
        state.id = string(panel, "id");
        for (const auto &mode : *modes)
            state.modes.add(mode.toString());
        state.selected = string(panel, "analyzerDefault", analyzerMode(panel));
        if (!state.modes.contains(state.selected))
            state.selected = state.modes[0];
        const float width = (rectangle.getWidth() - 4.f * (modes->size() - 1)) / modes->size();
        for (int i = 0; i < modes->size(); ++i)
        {
            const auto panelId = state.id, mode = state.modes[i];
            auto tab = std::make_unique<ViewTabButton>(theme, theme.style == "clear" ? 12.f : 11.f,
                                                      false);
            const auto caption = analyzerCaption(mode);
            tab->setButtonText(caption);
            tab->setComponentID("workspace.analyzer." + panelId + "." + mode);
            tab->setName("Analyzer " + caption);
            tab->setTitle(tab->getName());
            tab->setDescription("Display the captured stereo output as " + caption +
                                ". Left and right arrows select adjacent analyzer views.");
            tab->setTooltip(tab->getDescription());
            tab->setToggleState(mode == state.selected, juce::dontSendNotification);
            tab->onClick = [this, panelId, mode] { selectAnalyzer(panelId, mode); };
            tab->onNavigate = [this, panelId, mode](int delta) {
                for (const auto &current : analyzers)
                    if (current.id == panelId)
                    {
                        const int count = current.modes.size();
                        const auto nextMode = current.modes[(current.modes.indexOf(mode) + delta +
                                                            count) % count];
                        selectAnalyzer(panelId, nextMode, true);
                        return;
                    }
            };
            canvas.addAndMakeVisible(*tab);
            placements.push_back({tab.get(), {rectangle.getX() + i * (width + 4.f),
                                             rectangle.getY(), width, rectangle.getHeight()}});
            analyzerTabs.push_back({panelId, mode, std::move(tab)});
        }
        analyzers.push_back(std::move(state));
    }
    void cancelInteractions()
    {
        for (auto &control : wavetableControls)
            control.selector->cancelPopup();
        for (auto &row : routeControls)
            row->cancelGesture();
        routeEditor.end();
        for (auto &control : controls)
            if (auto *slider = dynamic_cast<BoundSlider *>(control.get()))
            {
                slider->cancelGesture();
                slider->dismissEntry();
            }
            else if (auto *menu = dynamic_cast<BoundMenu *>(control.get()))
                menu->cancelPopup();
            else if (auto *midi = dynamic_cast<MidiController *>(control.get()))
            {
                midi->cancelGesture();
                midi->dismissEntry();
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
        selectionBindings.clear();
        selectionTabs.clear();
        selections.clear();
        analyzerTabs.clear();
        analyzers.clear();
        wavetableControls.clear();
        xy.reset();
        browser.reset();
        keyboard.reset();
        if (auto *panels = manifest["panels"].getArray())
            for (const auto &panel : *panels)
                if (panel["selection"].isObject())
                {
                    const auto definition = panel["selection"];
                    selections.push_back(
                        {string(definition, "id"), string(definition, "default"), definition});
                }
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
                if (v.hasProperty("selectionGroup"))
                    selectionBindings.push_back(
                        {c.get(), string(v, "selectionGroup"), v["bindings"]});
                controls.push_back(std::move(c));
            }
        for (auto &state : selections)
            applySelectionBindings(state);
        if (auto *a = manifest["panels"].getArray())
            for (auto &v : *a)
            {
                const auto kind = string(v, "kind");
                auto r = bounds(v);
                const auto tabs = arrayRect(v["selection"]["tabsRect"], {});
                const auto actionFits = [&](Rect actionBounds) {
                    return number(v, "titleHeight", 28) >= 24 &&
                           (tabs.isEmpty() || !tabs.intersects(actionBounds));
                };
                if (theme.style == "live" && string(v, "id") == "p.scene")
                    for (int scene = 0; scene < 2; ++scene)
                    {
                        auto button = std::make_unique<ThemeButton>(theme,
                            scene == 0 ? "SCENE A" : "SCENE B");
                        button->setComponentID(scene == 0 ? "workspace.scene-card.a"
                                                         : "workspace.scene-card.b");
                        button->setName(scene == 0 ? "Select Scene A" : "Select Scene B");
                        button->setTitle(button->getName());
                        button->setWantsKeyboardFocus(true);
                        button->setToggleable(true);
                        button->getProperties().set("reference.scene-target", scene);
                        button->onClick = [this, scene] { owner.selectScene(scene); };
                        canvas.addAndMakeVisible(*button);
                        placements.push_back({button.get(), scene == 0 ? Rect(14, 127, 209, 31)
                                                                      : Rect(238, 127, 203, 31)});
                        actions.push_back(std::move(button));
                    }
                if (kind == "oscillator" && v.hasProperty("wavetableRect"))
                {
                    auto selector = std::make_unique<WavetableSelector>(
                        processor, theme, string(v, "id"),
                        [this](const WavetableSelectionRequest &request) {
                            return owner.onSelectWavetable
                                       ? owner.onSelectWavetable(request)
                                       : applyWavetableSelection(processor, request);
                        },
                        [this] { return static_cast<bool>(owner.onSelectWavetable); });
                    canvas.addAndMakeVisible(*selector);
                    placements.push_back({selector.get(), arrayRect(v["wavetableRect"], {})});
                    wavetableControls.push_back({v, std::move(selector)});
                }
                if (kind == "matrix" && actionFits(r.withHeight(28).removeFromRight(112)))
                    button("Edit routes  +", "modulation",
                           r.removeFromTop(28).removeFromRight(112).reduced(3));
                if (kind == "browser")
                {
                    browser = std::make_unique<PresetBrowser>(
                        processor, theme, browserQuery, browserSelectedPatch,
                        [this](const juce::String &a) { action(a); });
                    browser->setComponentID("workspace.browser");
                    canvas.addAndMakeVisible(*browser);
                    // Native browser chrome follows each concept's measured
                    // sidebar. The complete preset browser remains available
                    // through the component's accessible ellipsis action.
                    const auto browserBounds = theme.style == "space"
                        ? r.reduced(12, 0).withTop(r.getY() + 44).withBottom(r.getBottom() - 18)
                        : theme.style == "aurora"
                            ? r.reduced(9, 0).withTop(r.getY() + 41).withBottom(r.getBottom() - 9)
                        : theme.style == "tokyo"
                            ? r.reduced(9, 0).withTop(r.getY() + 9).withBottom(r.getBottom() - 14)
                        : theme.style == "bauhaus"
                            ? r.reduced(8, 0).withTop(r.getY() + 2).withBottom(r.getBottom() - 7)
                            : r.reduced(8, 0).withTop(r.getY() + 2).withBottom(r.getBottom() - 8);
                    placements.push_back({browser.get(), browserBounds});
                }
                if (kind == "effects" && actionFits(r.withHeight(28).removeFromRight(110)))
                    button("Full FX editor", "effects",
                           r.removeFromTop(28).removeFromRight(110).reduced(3));
                if (kind == "analyzer" && v.hasProperty("analyzerModesRect"))
                    buildAnalyzerTabs(v);
                else if (kind == "analyzer" && theme.style != "spectral" &&
                    theme.style != "bauhaus" && theme.style != "flow" &&
                    actionFits(r.withHeight(28).removeFromRight(r.getWidth() < 320 ? 54.f : 92.f)))
                    button(r.getWidth() < 320 ? "Scope" : "Open scope", "analyzer",
                           r.removeFromTop(28)
                               .removeFromRight(r.getWidth() < 320 ? 54.f : 92.f)
                               .reduced(3));
                if (kind == "midi")
                    if (const auto *names = v["controllers"].getArray(); names && !names->isEmpty())
                        for (int index = 0; index < names->size(); ++index)
                        {
                            auto midi = std::make_unique<MidiController>(
                                processor, theme, names->getReference(index).toString());
                            const int left = juce::roundToInt(r.getX() +
                                r.getWidth() * index / names->size());
                            const int right = juce::roundToInt(r.getX() +
                                r.getWidth() * (index + 1) / names->size());
                            canvas.addAndMakeVisible(*midi);
                            placements.push_back({midi.get(),
                                Rect(static_cast<float>(left), r.getY(),
                                     static_cast<float>(right - left), r.getHeight())});
                            controls.push_back(std::move(midi));
                        }
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
                    keyboard = std::make_unique<PerformanceKeyboard>(processor.midiKeyboardState,
                                                                      theme.style == "live");
                    keyboard->setComponentID("workspace.performance-keyboard");
                    keyboard->setName("Performance MIDI keyboard");
                    keyboard->setTitle("Performance MIDI keyboard");
                    keyboard->setAvailableRange(theme.style == "live" ? 0 : 24,
                                                theme.style == "live" ? 120 : 108);
                    keyboard->setLowestVisibleKey(theme.style == "live" ? 0 : 36);
                    keyboard->setKeyWidth(theme.style == "live" ? 8.f : 17.f);
                    if (theme.style == "live")
                        keyboard->setScrollButtonsVisible(false);
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
        buildViewTabs();
        syncWavetableControls();
        for (auto *b : {&skinButton, &classicButton, &sceneA, &sceneB, &presetButton, &previous,
                        &next, &browserButton, &saveButton, &modButton, &fxButton, &synthButton,
                        &presetSearchButton, &presetListButton})
            styleButton(*b);
        for (auto &b : actions)
            styleButton(*b);
        presetButton.setColour(juce::TextButton::buttonColourId,
                               theme.style == "clear" ? theme.panel : theme.display);
        presetButton.setColour(juce::TextButton::textColourOffId,
                               theme.style == "clear" ? theme.text
                                                      : theme.display.contrasting(.95f));
        presetSearchButton.setColour(juce::TextButton::textColourOffId, theme.muted);
        owner.resized();
    }
    void clearAnalyzerCapture()
    {
        visuals.output.clear();
        visuals.left = visuals.right = 0;
        for (auto &channel : stereoOutput)
            channel.clear();
        for (auto &channel : stereoSpectrumDb)
            channel.clear();
        spectrumDb.clear();
        spectrumHistory.clear();
        spectrumHistoryTimes.clear();
        outputWrite = outputCount = framesSinceSpectrum = 0;
        capturedSamples = 0;
        capturedSampleRate = 0;
    }
    void analyzeOutputWindow(double rate)
    {
        const auto transform = [&](const auto &samples, auto &bins) {
            fftData.fill(0.f);
            for (size_t i = 0; i < outputHistory.size(); ++i)
            {
                const float window =
                    .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i /
                                         (outputHistory.size() - 1));
                fftData[i] = samples[(outputWrite + i) % samples.size()] * window;
            }
            outputFFT.performFrequencyOnlyForwardTransform(fftData.data());
            bins.resize(160);
            const auto top = std::clamp(rate * .5, 20.0, 20000.0);
            for (size_t i = 0; i < bins.size(); ++i)
            {
                const auto frequency =
                    20.0 * std::pow(top / 20.0, static_cast<double>(i) / (bins.size() - 1));
                const auto bin =
                    std::clamp(frequency * outputHistory.size() / rate, 1.0, 1023.0);
                const auto low = static_cast<size_t>(bin);
                const auto fraction = static_cast<float>(bin - low);
                const auto magnitude =
                    (fftData[low] + fraction * (fftData[low + 1] - fftData[low])) *
                    (4.f / static_cast<float>(outputHistory.size()));
                bins[i] = juce::jlimit(
                    -96.f, 12.f, 20.f * std::log10(std::max(1.e-8f, magnitude)));
            }
        };
        transform(outputHistory, spectrumDb);
        for (int channel = 0; channel < 2; ++channel)
            transform(stereoHistory[channel], stereoSpectrumDb[channel]);
        spectrumHistory.push_back(spectrumDb);
        spectrumHistoryTimes.push_back(static_cast<double>(capturedSamples) / rate);
        // A fixed audio hop and sample-clock timestamps make the time
        // axis independent of UI cadence. Keep at most eight seconds
        // and 256 analysis frames, including after a sample-rate change.
        const auto newest = spectrumHistoryTimes.back();
        while (spectrumHistory.size() > 256 ||
               (!spectrumHistoryTimes.empty() &&
                spectrumHistoryTimes.front() < newest - 8.0))
        {
            spectrumHistory.erase(spectrumHistory.begin());
            spectrumHistoryTimes.erase(spectrumHistoryTimes.begin());
        }
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
            clearAnalyzerCapture();
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
                const auto layout = MatrixLayout::make(bounds(panel), theme.style,
                                                       string(panel, "variant"),
                                                       number(panel, "titleHeight", 24));
                const auto count = std::min(static_cast<int>(routes.size()),
                                            layout.columns * layout.rowsPerColumn);
                for (int index = 0; index < count; ++index)
                {
                    const auto cells = layout.row(index);
                    result.push_back({cells.row, cells.slider, static_cast<size_t>(index)});
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
        syncViewTabs();
        syncWavetableControls();
        for (auto &c : controls)
        {
            if (auto *s = dynamic_cast<BoundSlider *>(c.get()))
                s->sync();
            if (auto *s = dynamic_cast<BoundMenu *>(c.get()))
                s->sync();
            if (auto *s = dynamic_cast<BoundToggle *>(c.get()))
                s->sync();
            if (auto *midi = dynamic_cast<MidiController *>(c.get()))
                midi->sync();
        }
        sceneA.setToggleState(parameters.getScene() == 0, juce::dontSendNotification);
        sceneB.setToggleState(parameters.getScene() == 1, juce::dontSendNotification);
        for (auto &button : actions)
            if (button->getProperties().contains("reference.scene-target"))
                button->setToggleState(static_cast<int>(button->getProperties()["reference.scene-target"]) ==
                                           parameters.getScene(), juce::dontSendNotification);
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
            std::array<bool, n_fx_slots> effectsNeeded{};
            if (auto *panels = manifest["panels"].getArray())
                for (const auto &panel : *panels)
                {
                    auto kind = string(panel, "kind");
                    int index = static_cast<int>(number(panel, "index", 1)) - 1;
                    const auto selected = selectedPanelChoice(panel);
                    if (selected.isObject())
                    {
                        kind = string(selected, "source");
                        index = static_cast<int>(number(selected, "index", 1)) - 1;
                    }
                    if ((kind == "filter" || kind == "inspector") && index >= 0 && index < 2)
                        filtersNeeded[index] = true;
                    if (kind == "filter" && string(panel, "variant") == "combined")
                        filtersNeeded[1] = true;
                    if (kind == "routing" && string(panel, "variant") == "flow")
                        filtersNeeded = {true, true};
                    if ((kind == "lfo" || kind == "lfo-envelope") && index >= 0 && index < 12)
                        lfosNeeded[index] = true;
                    if ((kind == "effects" || kind == "fx") && index >= 0 && index < n_fx_slots)
                        effectsNeeded[index] = true;
                }
            for (int i = 0; i < 2; ++i)
                if (filtersNeeded[i])
                    filterCurves[i] = filterResponse(processor, parameters.getScene(), i);
            for (int i = 0; i < 12; ++i)
                if (lfosNeeded[i])
                    lfoCurves[i] = lfoCurve(processor, parameters.getScene(), i);
            for (int i = 0; i < n_fx_slots; ++i)
                if (effectsNeeded[i])
                    effectCurves[i] = distortionTransfer(processor, i);
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
                clearAnalyzerCapture();
                return;
            }
            const auto rate = std::max(1.0, processor.getSampleRate());
            if (capturedSampleRate != rate)
            {
                clearAnalyzerCapture();
                capturedSampleRate = rate;
            }
            visuals.left *= .8f;
            visuals.right *= .8f;
            const auto frames = std::min(left.size(), right.size());
            if (frames > 0)
            {
                for (size_t i = 0; i < frames; ++i)
                {
                    const float l = std::isfinite(left[i]) ? left[i] : 0.f;
                    const float r = std::isfinite(right[i]) ? right[i] : 0.f;
                    visuals.left = std::max(visuals.left, std::abs(l));
                    visuals.right = std::max(visuals.right, std::abs(r));
                    stereoHistory[0][outputWrite] = l;
                    stereoHistory[1][outputWrite] = r;
                    outputHistory[outputWrite] = l * .5f + r * .5f;
                    outputWrite = (outputWrite + 1) % outputHistory.size();
                    outputCount = std::min(outputCount + 1, outputHistory.size());
                    ++capturedSamples;
                    if (++framesSinceSpectrum == outputHistory.size())
                    {
                        // Analyze every complete audio hop, including multiple
                        // windows delivered by one refresh. The tap bounds each
                        // update to four FFT windows, all on the UI thread.
                        framesSinceSpectrum = 0;
                        analyzeOutputWindow(rate);
                    }
                }
                const auto count = std::min<size_t>(512, outputCount);
                visuals.output.resize(count);
                stereoOutput[0].resize(count);
                stereoOutput[1].resize(count);
                for (size_t i = 0; i < count; ++i)
                {
                    const auto at = (outputWrite + outputHistory.size() - count + i) %
                                    outputHistory.size();
                    const auto l = stereoHistory[0][at];
                    const auto r = stereoHistory[1][at];
                    stereoOutput[0][i] = l;
                    stereoOutput[1][i] = r;
                    visuals.output[i] = l * .5f + r * .5f;
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
                        &next, &browserButton, &saveButton, &modButton, &fxButton, &synthButton,
                        &presetSearchButton, &presetListButton})
        {
            b->setVisible(true);
            b->present(H::chip);
        }
        synthButton.setVisible(false);
        presetSearchButton.setVisible(false);
        presetListButton.setVisible(false);
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
            put(browserButton, 996, 29, 39, 35, H::icon);
            put(saveButton, 1041, 29, 39, 35, H::icon);
            put(skinButton, 1084, 29, 28, 35, H::icon);
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
            // The native browser's PRESETS tab occupies this title row. Keep
            // the covered header action out of both the paint and focus trees.
            browserButton.setVisible(false);
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
            put(presetSearchButton, 196, 69, 202, 30, H::search, 12);
            put(presetListButton, 411, 69, 36, 30, H::list);
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
        const auto presetInk = style == "ivory" || style == "bauhaus"
                                                    ? juce::Colour(0xfffff9e8)
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
    void headerCpu(juce::Graphics &g, Rect label, float fontSize, Rect bar = {})
    {
        // cpu_level is the engine's measured process-duration / block-duration
        // ratio. Read only; painting never changes its smoothing or audio state.
        const float load = processor.surge->cpu_level.load(std::memory_order_relaxed);
        const bool available = std::isfinite(load) && load >= 0.f;
        const auto value = available ? juce::String(static_cast<double>(load) * 100.0, 1) + "%"
                                     : juce::String("--");
        text(g, theme, "CPU " + value, label, fontSize, theme.muted);
        if (!bar.isEmpty())
        {
            bezel(g, theme, bar, theme.display, bar.getHeight() * .5f);
            if (available && load > 0.f)
            {
                auto fill = bar.reduced(2.f);
                fill.setWidth(fill.getWidth() * std::clamp(load, 0.f, 1.f));
                g.setColour(load >= 1.f ? theme.red : theme.accent);
                g.fillRoundedRectangle(fill, fill.getHeight() * .5f);
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
            juce::Path folder;
            folder.startNewSubPath(28, 79);
            folder.lineTo(33, 79);
            folder.lineTo(35, 81);
            folder.lineTo(41, 81);
            folder.lineTo(41, 89);
            folder.lineTo(28, 89);
            folder.closeSubPath();
            g.setColour(ink);
            g.strokePath(folder, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            text(g, theme, "Preset Browser", {51, 66, 141, 31}, 15, ink, true);
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
            // Static engraved emblem; the waveform below remains live engine data.
            observatoryEmblem(g, {417, 20, 162, 70}, theme.accent, theme.secondary);
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
            headerCpu(g, {1133, 36, 114, 20}, 12);
            text(g, theme, "OUT L", {1267, 13, 39, 15}, 10, theme.muted, false,
                 juce::Justification::centredRight);
            text(g, theme, "R", {1267, 33, 39, 15}, 10, theme.muted, false,
                 juce::Justification::centredRight);
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
            headerCpu(g, {1321, 34, 109, 17}, 12);
            text(g, theme, "L", {1427, 15, 12, 14}, 10, theme.muted);
            text(g, theme, "R", {1427, 33, 12, 14}, 10, theme.muted);
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
            headerCpu(g, {1168, 12, 59, 20}, 10, {1225, 20, 62, 8});
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
    void plotBase(juce::Graphics &g, Rect r, bool pale = false, bool grid = true)
    {
        g.setColour(pale ? theme.panel.brighter(.03f) : theme.display);
        g.fillRoundedRectangle(r, 3);
        g.setColour(theme.border.withAlpha(.4f));
        g.drawRoundedRectangle(r, 3, 1);
        if (grid)
            Visuals::grid(g, r.reduced(2), theme.secondary.withAlpha(theme.light() ? .12f : .065f));
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
        if (theme.style != "aurora")
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
        const juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(r.toNearestInt());
        if (base && theme.style != "aurora")
            plotBase(g, r, false, false); // The filter draws its own logarithmic axes.
        const bool miniature = r.getHeight() < 65;
        const bool ivoryReadouts = theme.style == "ivory" && !miniature;
        const bool spectral = theme.style == "spectral", tokyo = theme.style == "tokyo";
        const bool clear = theme.style == "clear", live = theme.style == "live";
        const bool dbReadouts = !miniature && (spectral || tokyo || clear);
        const auto frame = r;
        r = miniature ? r.reduced(4, 3) : r.reduced(7, 15);
        if (ivoryReadouts)
            r = frame.reduced(7, 9).withTrimmedBottom(20);
        if (dbReadouts)
            r = r.withTrimmedLeft(clear ? 30.f : 21.f);
        if (live)
            color = theme.accent;
        else if (clear)
            color = color.brighter(.22f);
        const auto axisInk = theme.display.contrasting(dbReadouts ? .90f : .80f);
        const auto gridInk = dbReadouts ? axisInk : theme.secondary;
        const auto axisSize = clear      ? 12.f
                              : tokyo    ? 11.f
                              : spectral ? 10.f
                                         : r.getWidth() > 400 ? 11.f : 9.f;
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
                g.setColour(gridInk.withAlpha(clear ? .14f : .10f));
                g.drawLine(x, r.getY(), x, r.getBottom(), .7f);
                const auto label = frequency >= 1000 ? juce::String(frequency / 1000.f, 0) + "k"
                                                     : juce::String(static_cast<int>(frequency));
                // Compact cards retain the full logarithmic grid, with enough
                // separation for the actual frequency captions to remain readable.
                const bool caption = !dbReadouts || r.getWidth() >= 260 || frequency == 20 ||
                                     frequency == 100 || frequency == 1000 || frequency == 20000;
                if (!ivoryReadouts && caption)
                    text(g, theme, label, dbReadouts
                             ? Rect(std::clamp(x - 14, r.getX() - 4, frame.getRight() - 29),
                                    frame.getBottom() - 14, 28, 13)
                             : Rect(std::clamp(x - 13, frame.getX(), frame.getRight() - 26),
                                    frame.getBottom() - 13, 26, 12),
                         axisSize, axisInk, false,
                         juce::Justification::centred);
            }
            // Multiples of 12 dB include the real unity-gain line. Starting at
            // the -42 dB display limit would miss 0 dB entirely.
            for (float db = std::ceil(filterPreviewMinimumDb / 12.f) * 12.f;
                 db <= filterPreviewMaximumDb; db += 12)
            {
                const auto y =
                    r.getBottom() - (db - filterPreviewMinimumDb) /
                                        (filterPreviewMaximumDb - filterPreviewMinimumDb) *
                                        r.getHeight();
                g.setColour(gridInk.withAlpha(db == 0 ? .25f : clear ? .14f : .09f));
                g.drawLine(r.getX(), y, r.getRight(), y, db == 0 ? 1.f : .7f);
                if (dbReadouts)
                    text(g, theme, (db > 0 ? "+" : "") + juce::String(static_cast<int>(db)),
                         {frame.getX() + 2, y - 6.5f, r.getX() - frame.getX() - 7, 13},
                         axisSize, axisInk, false, juce::Justification::centredRight);
            }
            if (dbReadouts)
                text(g, theme, "dB", {frame.getX() + 2, frame.getBottom() - 14,
                                      r.getX() - frame.getX() - 7, 13},
                     axisSize, axisInk, false, juce::Justification::centredRight);
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
        const auto fillAlpha = spectral ? .46f : clear ? .44f : tokyo ? .38f : live ? .34f : .28f;
        g.setGradientFill(juce::ColourGradient(color.withAlpha(fillAlpha), r.getTopLeft(),
                                               color.withAlpha(.01f), r.getBottomLeft(), false));
        g.fillPath(fill);
        if (spectral || tokyo)
        {
            g.setColour(color.withAlpha(spectral ? .12f : .08f));
            g.strokePath(p, juce::PathStrokeType(spectral ? 5.f : 4.f));
        }
        g.setColour(color);
        g.strokePath(p, juce::PathStrokeType(clear ? 2.7f : spectral || tokyo ? 2.f : 1.7f));
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
                const auto sample = position * (curve.values.size() - 1);
                const auto at = static_cast<size_t>(sample);
                const auto next = std::min(at + 1, curve.values.size() - 1);
                const auto magnitude = [&](size_t i) {
                    return std::clamp((curve.values[i] - filterPreviewMinimumDb) /
                                          (filterPreviewMaximumDb - filterPreviewMinimumDb),
                                      0.f, 1.f);
                };
                const auto y = r.getBottom() -
                               r.getHeight() * (magnitude(at) + (sample - at) *
                                                                    (magnitude(next) - magnitude(at)));
                const auto x = r.getX() + position * r.getWidth();
                const auto radius = clear ? 5.8f : 3.7f;
                const Rect marker(x - radius, y - radius, radius * 2, radius * 2);
                g.setColour(clear ? color : theme.display);
                g.fillEllipse(marker);
                g.setColour(clear ? juce::Colours::white : color.brighter(.12f));
                g.drawEllipse(marker, clear ? 2.f : 1.6f);
            }
        }
        if (base && ivoryReadouts)
        {
            const auto prefix = "filter" + juce::String(index) + ".";
            const std::array<juce::String, 3> captions{
                "FREQ  " + compactPaintValue(parameters.display(prefix + "cutoff")),
                "RES  " + parameters.display(prefix + "resonance").replace(".00 ", " "),
                (index == 1 ? "SHAPER  " : "ENV  ") +
                    parameters.display(index == 1 ? "filter.drive" : prefix + "envmod")
                        .replace(".00 ", " ")};
            auto footer = frame.reduced(7, 0).removeFromBottom(24).withHeight(17);
            const auto width = footer.getWidth() / 3.f;
            for (const auto &caption : captions)
                text(g, theme, caption, footer.removeFromLeft(width), 10,
                     theme.display.contrasting(.95f), false, juce::Justification::centred);
        }
    }
    void drawLfo(juce::Graphics &g, Rect r, int index)
    {
        const juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(r.toNearestInt());
        if (theme.style != "aurora")
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
        const bool spectral = theme.style == "spectral", tokyo = theme.style == "tokyo";
        const bool clear = theme.style == "clear", live = theme.style == "live";
        const auto color = clear || live ? theme.accent : theme.secondary;
        if (spectral || tokyo || clear || live)
        {
            auto fill = p;
            const auto floor = spectral ? r.getCentreY() : r.getBottom();
            fill.lineTo(r.getRight(), floor);
            fill.lineTo(r.getX(), floor);
            fill.closeSubPath();
            if (spectral)
            {
                juce::ColourGradient gradient(theme.accent.withAlpha(.38f), r.getTopLeft(),
                                               theme.accent.withAlpha(.28f), r.getBottomLeft(), false);
                gradient.addColour(.5, theme.secondary.withAlpha(.12f));
                g.setGradientFill(gradient);
            }
            else
                g.setGradientFill(juce::ColourGradient(
                    color.withAlpha(clear ? .23f : tokyo ? .20f : .13f), r.getTopLeft(),
                    color.withAlpha(.01f), r.getBottomLeft(), false));
            g.fillPath(fill);
        }
        if (spectral || tokyo)
        {
            g.setColour((spectral ? theme.accent : color).withAlpha(spectral ? .16f : .10f));
            g.strokePath(p, juce::PathStrokeType(spectral ? 4.8f : 4.f));
        }
        if (spectral)
        {
            juce::ColourGradient gradient(theme.accent.brighter(.12f), r.getTopLeft(),
                                           theme.accent, r.getBottomLeft(), false);
            gradient.addColour(.5, theme.secondary);
            g.setGradientFill(gradient);
        }
        else
            g.setColour(color);
        g.strokePath(p, juce::PathStrokeType(clear ? 2.1f : tokyo ? 2.f : 1.8f));
    }
    void drawSpectrum(juce::Graphics &g, Rect r, bool history)
    {
        const bool externalAxes = theme.style == "ivory" || theme.style == "clear" ||
                                  theme.style == "live";
        const auto axisInk = externalAxes ? theme.text : theme.display.contrasting(.8f);
        const auto leftInset = theme.style == "spectral" ? 32.f : 28.f;
        auto graph = externalAxes ? r.reduced(1)
                     : Rect(r.getX() + leftInset, r.getY() + 5,
                            std::max(1.f, r.getWidth() - leftInset -
                                              (theme.style == "precision" ? 9.f : 1.f)),
                            std::max(1.f, r.getHeight() - 15));
        const auto top = std::clamp(processor.getSampleRate() * .5, 20.001, 20000.0);
        const auto frequencyPosition = [top](float hz) {
            return static_cast<float>(std::log(std::clamp<double>(hz, 20, top) / 20.0) /
                                      std::log(top / 20.0));
        };
        const auto frequencyCaption = [](float hz) {
            return hz >= 1000 ? juce::String(static_cast<int>(hz / 1000)) + "k"
                              : juce::String(static_cast<int>(hz));
        };
        g.setColour(theme.display);
        g.fillRect(graph);
        const std::array<float, 10> frequencies{20, 50, 100, 200, 500, 1000, 2000, 5000,
                                                10000, 20000};
        if (history)
        {
            if (!spectrumHistory.empty())
            {
                const double latest = spectrumHistoryTimes.back();
                for (size_t frame = 0; frame < spectrumHistory.size(); ++frame)
                {
                    const auto &bins = spectrumHistory[frame];
                    const double end = spectrumHistoryTimes[frame];
                    const double start = frame ? spectrumHistoryTimes[frame - 1]
                                               : end - outputHistory.size() /
                                                           std::max(1.0, capturedSampleRate);
                    const auto x1 = graph.getX() + graph.getWidth() *
                        static_cast<float>(std::clamp((start - latest + 8.0) / 8.0, 0.0, 1.0));
                    const auto x2 = graph.getX() + graph.getWidth() *
                        static_cast<float>(std::clamp((end - latest + 8.0) / 8.0, 0.0, 1.0));
                    for (size_t bin = 0; bin < bins.size(); ++bin)
                    {
                        const float strength = std::clamp((bins[bin] + 96.f) / 96.f, 0.f, 1.f);
                        auto colour = theme.display;
                        if (theme.style == "clear" || theme.style == "flow")
                        {
                            const std::array<juce::Colour, 6> heat{
                                juce::Colour(0xff11122e), juce::Colour(0xff163d92),
                                juce::Colour(0xff02adbd), juce::Colour(0xff48d868),
                                juce::Colour(0xffffe542), juce::Colour(0xffff5734)};
                            const auto at = std::min(4, static_cast<int>(strength * 5));
                            colour = heat[at].interpolatedWith(heat[at + 1], strength * 5 - at);
                        }
                        else if (theme.style == "live")
                        {
                            // The performance display uses a mint-to-amber heat
                            // scale. Colour follows measured dB strength; it is
                            // independent of frequency and of the capture clock.
                            const std::array<juce::Colour, 6> heat{
                                theme.display, juce::Colour(0xff123c2c),
                                juce::Colour(0xff50bb86), juce::Colour(0xffd38c42),
                                juce::Colour(0xffffae58), juce::Colour(0xffffe8ae)};
                            const auto at = std::min(4, static_cast<int>(strength * 5));
                            colour = heat[at].interpolatedWith(heat[at + 1], strength * 5 - at);
                        }
                        else
                        {
                            colour = theme.display.interpolatedWith(theme.accent,
                                                                    std::min(1.f, strength * 1.7f));
                            if (strength > .48f)
                                colour = colour.interpolatedWith(theme.secondary,
                                                                 (strength - .48f) / .52f);
                        }
                        g.setColour(colour);
                        g.fillRect(Rect(x1, graph.getBottom() - (bin + 1) * graph.getHeight() /
                                                                       bins.size(),
                                        std::max(.25f, x2 - x1),
                                        graph.getHeight() / bins.size() + .15f));
                    }
                }
            }
            for (const auto hz : frequencies)
            {
                if (hz > top || (theme.style == "live" && hz != 20 && hz != 50 && hz != 200 &&
                                 hz != 1000 && hz != 5000 && hz != 20000))
                    continue;
                const auto y = graph.getBottom() - graph.getHeight() * frequencyPosition(hz);
                g.setColour(axisInk.withAlpha(.12f));
                g.drawLine(graph.getX(), y, graph.getRight(), y, .5f);
                const bool right = theme.style == "live";
                text(g, theme, frequencyCaption(hz),
                     {right ? graph.getRight() + 7 : graph.getX() - 34, y - 6,
                      right ? 38.f : 29.f, 12}, theme.style == "clear" ? 10.f : 9.f,
                     axisInk, theme.style == "clear",
                     right ? juce::Justification::centredRight : juce::Justification::centredRight);
            }
            if (theme.style != "live")
                for (int second = -8; second <= 0; ++second)
                {
                    const auto x = graph.getX() + graph.getWidth() * (second + 8) / 8.f;
                    text(g, theme, second ? juce::String(second) + " s" : "0",
                         {x - 18, graph.getBottom() + 3, 36, 12},
                         theme.style == "clear" ? 10.f : 9.f, axisInk,
                         theme.style == "clear", juce::Justification::centred);
                }
        }
        else
        {
            for (int db = 0; db >= -96; db -= 12)
            {
                const auto y = graph.getY() - db * graph.getHeight() / 96.f;
                g.setColour(axisInk.withAlpha(.16f));
                g.drawLine(graph.getX(), y, graph.getRight(), y, .5f);
                text(g, theme, juce::String(db), {graph.getX() - 31, y - 6, 25, 12},
                     9.5f, axisInk, false, juce::Justification::centredRight);
            }
            for (const auto hz : frequencies)
            {
                if (hz > top)
                    continue;
                const auto x = graph.getX() + graph.getWidth() * frequencyPosition(hz);
                g.setColour(axisInk.withAlpha(.12f));
                g.drawLine(x, graph.getY(), x, graph.getBottom(), .5f);
                text(g, theme, frequencyCaption(hz), {x - 15, graph.getBottom() + 3, 30, 12},
                     theme.style == "ivory" ? 10.f : 9.f, axisInk, false,
                     juce::Justification::centred);
            }
            if (!spectrumDb.empty())
            {
                juce::Path path;
                for (size_t i = 0; i < spectrumDb.size(); ++i)
                {
                    const auto x = graph.getX() + graph.getWidth() * i / (spectrumDb.size() - 1);
                    const auto y = graph.getBottom() - graph.getHeight() *
                        std::clamp((spectrumDb[i] + 96.f) / 96.f, 0.f, 1.f);
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
                if (theme.style == "spectral")
                    g.setGradientFill(juce::ColourGradient(theme.secondary.withAlpha(.65f),
                        graph.getTopLeft(), theme.accent.withAlpha(.5f), graph.getTopRight(), false));
                else
                    g.setGradientFill(juce::ColourGradient(ink.withAlpha(.55f), graph.getTopLeft(),
                        ink.withAlpha(.08f), graph.getBottomLeft(), false));
                g.fillPath(fill);
                if (theme.style == "spectral")
                    g.setGradientFill(juce::ColourGradient(theme.secondary, graph.getTopLeft(),
                        theme.accent, graph.getTopRight(), false));
                else
                    g.setColour(ink);
                g.strokePath(path, juce::PathStrokeType(1.4f));
            }
        }
        if (spectrumDb.empty())
            text(g, theme, "Awaiting live audio", graph.reduced(5), 11,
                 theme.display.contrasting(.8f), false, juce::Justification::centred);
    }
    void drawScope(juce::Graphics &g, Rect r, bool vectorscope)
    {
        r = r.reduced(8);
        if (vectorscope)
        {
            const auto side = std::min(r.getWidth(), r.getHeight() - 16);
            const auto graph = r.withSizeKeepingCentre(side, side).withY(r.getY());
            const auto centre = graph.getCentre();
            const auto radius = side * .46f;
            g.setColour(theme.secondary.withAlpha(.18f));
            g.drawEllipse(graph.reduced(side * .04f), 1);
            g.drawLine(centre.x - radius, centre.y, centre.x + radius, centre.y);
            g.drawLine(centre.x, centre.y - radius, centre.x, centre.y + radius);
            const auto count = std::min(stereoOutput[0].size(), stereoOutput[1].size());
            if (count > 1)
            {
                juce::Path path;
                for (size_t i = 0; i < count; ++i)
                {
                    // Mid/side projection: mono rises vertically, opposite
                    // polarity runs horizontally, with no invented rotation.
                    const auto left = stereoOutput[0][i], right = stereoOutput[1][i];
                    const auto x = centre.x + radius * std::clamp((right - left) * .5f, -1.f, 1.f);
                    const auto y = centre.y - radius * std::clamp((left + right) * .5f, -1.f, 1.f);
                    if (i == 0)
                        path.startNewSubPath(x, y);
                    else
                        path.lineTo(x, y);
                }
                g.setColour(theme.secondary.withAlpha(.85f));
                g.strokePath(path, juce::PathStrokeType(1.1f));
            }
            text(g, theme, count ? utf8("L / R  ·  mid / side") : "Awaiting live audio",
                 r.removeFromBottom(15), 10,
                 theme.display.contrasting(.8f), false, juce::Justification::centred);
            return;
        }
        auto graph = r.withTrimmedBottom(15);
        g.setColour(theme.secondary.withAlpha(.22f));
        g.drawLine(graph.getX(), graph.getCentreY(), graph.getRight(), graph.getCentreY());
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto &samples = stereoOutput[channel];
            if (samples.size() < 2)
                continue;
            juce::Path path;
            for (size_t i = 0; i < samples.size(); ++i)
            {
                const auto x = graph.getX() + graph.getWidth() * i / (samples.size() - 1);
                const auto y = graph.getCentreY() - graph.getHeight() * .47f *
                    std::clamp(samples[i], -1.f, 1.f);
                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }
            g.setColour((channel ? theme.accent : theme.secondary).withAlpha(.85f));
            g.strokePath(path, juce::PathStrokeType(1.3f));
        }
        const auto count = std::min(stereoOutput[0].size(), stereoOutput[1].size());
        const auto duration = count ? (count - 1) * 1000.0 / std::max(1.0, capturedSampleRate) : 0.0;
        text(g, theme, count ? utf8("L / R  ·  ") + juce::String(duration, 1) + " ms" : "Awaiting live audio",
             r.removeFromBottom(15), 10, theme.display.contrasting(.8f), false,
             juce::Justification::centred);
    }
    void drawStereoBands(juce::Graphics &g, Rect r)
    {
        g.setColour(juce::Colour(0xff20201a));
        g.fillRoundedRectangle(r, 5);
        const auto graph = r.reduced(12, 18).withTrimmedRight(24);
        const auto width = (graph.getWidth() - 16.f) * .5f;
        const auto ivory = juce::Colour(0xfffff5d2), yellow = juce::Colour(0xffffd91c);
        for (int db : {0, -6, -12, -18, -24, -36, -48})
        {
            const auto y = graph.getY() - db * graph.getHeight() / 48.f;
            g.setColour(ivory.withAlpha(.28f));
            const float dash[]{6.f, 3.f};
            g.drawDashedLine(juce::Line<float>(graph.getX(), y, graph.getRight(), y), dash, 2, .7f);
            text(g, theme, juce::String(db), {graph.getRight() + 6, y - 6, 24, 12}, 9,
                 ivory, false, juce::Justification::centredRight);
        }
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto &bins = stereoSpectrumDb[channel];
            const auto x = graph.getX() + channel * (width + 16.f);
            for (size_t band = 0; band < 8; ++band)
            {
                float db = -96;
                for (size_t i = band * bins.size() / 8; i < (band + 1) * bins.size() / 8; ++i)
                    db = std::max(db, bins[i]);
                const auto height = graph.getHeight() * std::clamp((db + 48.f) / 48.f, 0.f, 1.f);
                const Rect bar(x + band * width / 8.f, graph.getBottom() - height,
                               std::max(1.f, width / 8.f - 3.f), height);
                g.setGradientFill(juce::ColourGradient(ivory, graph.getTopLeft(), yellow,
                                                       graph.getBottomLeft(), false));
                g.fillRect(bar);
            }
            text(g, theme, channel ? "R" : "L", {x, graph.getBottom() + 3, width, 14}, 11,
                 ivory, true, juce::Justification::centred);
        }
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
        if (r.getWidth() < 6 || r.getHeight() < 20)
            return;
        const bool continuous = theme.style == "ivory" || theme.style == "spectral" ||
                                theme.style == "clear" || theme.style == "live";
        const bool labelsAbove = theme.style == "clear" || theme.style == "spectral";
        const float gap = theme.style == "ivory" ? std::min(14.f, r.getWidth() * .4f)
                                                 : std::max(3.f, r.getWidth() * .15f);
        float w = (r.getWidth() - gap) / 2;
        for (int ch = 0; ch < 2; ++ch)
        {
            Rect bar(r.getX() + ch * (w + gap), r.getY(), w, r.getHeight() - 16);
            g.setColour(theme.display);
            g.fillRoundedRectangle(bar, 2);
            auto peak = ch ? visuals.right : visuals.left;
            float proportion =
                peak > 0 ? std::clamp((20.f * std::log10(peak) + 60) / 60, 0.f, 1.f) : 0;
            if (continuous)
            {
                auto active = bar.reduced(theme.style == "clear" ? 2.f : 1.f);
                active = active.removeFromBottom(active.getHeight() * proportion);
                auto bottom = theme.style == "ivory" ? juce::Colour(0xff58c546)
                              : theme.style == "spectral" ? theme.secondary : theme.accent;
                auto top = theme.style == "ivory" ? juce::Colour(0xffe5e66a)
                           : theme.style == "spectral" ? theme.accent
                           : theme.accent.interpolatedWith(juce::Colours::white, .65f);
                g.setGradientFill(juce::ColourGradient(top, bar.getTopLeft(), bottom,
                                                       bar.getBottomLeft(), false));
                g.fillRect(active);
                g.setColour((theme.style == "clear" ? theme.accent : theme.border).withAlpha(.85f));
                g.drawRoundedRectangle(bar, 1, theme.style == "clear" ? 2.f : .7f);
            }
            else for (int s = 0; s < 24; ++s)
            {
                float y = bar.getBottom() - (s + 1) * bar.getHeight() / 24;
                const auto colour = theme.style == "precision"
                                        ? (s > 16 ? theme.accent : theme.secondary)
                                        : (s > 20 ? theme.secondary : theme.accent);
                g.setColour(colour
                                .withAlpha((s / 24.f < proportion) ? 1.f : .1f));
                g.fillRect(Rect(bar.getX() + 2, y + 1, w - 4, bar.getHeight() / 24 - 2));
            }
            if (peak >= 1.f)
            {
                g.setColour(theme.red);
                g.fillRect(bar.withHeight(2));
            }
            const auto caption = theme.style == "ivory"
                ? (peak > 0 ? juce::String(20.f * std::log10(peak), 1) : utf8("−∞"))
                : juce::String(ch ? "R" : "L");
            text(g, theme, caption,
                 {bar.getCentreX() - std::max(w, 27.f) * .5f,
                  labelsAbove ? bar.getY() - 16 : bar.getBottom() + 2,
                  std::max(w, 27.f), 14}, theme.style == "ivory" ? 9.5f : 10.f,
                 theme.text, false, juce::Justification::centred);
        }
    }
    void drawPanel(juce::Graphics &g, const juce::var &v)
    {
        auto r = bounds(v);
        auto kind = string(v, "kind"), variant = string(v, "variant");
        if (kind == "midi")
            return; // Each genuine MIDI lane paints its own glass and label.
        auto fill = v.hasProperty("color") ? theme.color(string(v, "color")) : theme.panel;
        const auto panelId = string(v, "id");
        const bool connectedLivePanel = theme.style == "live" &&
            (panelId == "p.wave" || panelId == "p.xy" || panelId == "p.scene-level" ||
             panelId == "p.filter-chain");
        if (!connectedLivePanel)
            bezel(g, theme, r, fill);
        if (theme.style == "aurora" && kind == "macros")
        {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1d1a35), r.getTopLeft(),
                                                   juce::Colour(0xff19172e),
                                                   r.getBottomLeft(), false));
            g.fillRoundedRectangle(r.reduced(1), std::max(0.f, theme.radius - 1.f));
        }
        if (theme.style == "bauhaus" && panelId == "output-panel")
        {
            // The output plate has its own warm vertical coating. Keep the
            // bezel and all live analyzer/slider content above this material.
            auto coating = juce::ColourGradient(juce::Colour(0xfff7c124), r.getTopLeft(),
                                                  juce::Colour(0xfffddb5c),
                                                  r.getBottomLeft(), false);
            coating.addColour(.45, juce::Colour(0xfffcd55a));
            coating.addColour(.80, juce::Colour(0xfffede65));
            g.setGradientFill(coating);
            g.fillRoundedRectangle(r.reduced(1), std::max(0.f, theme.radius - 1.f));
        }
        int index = std::max(1, static_cast<int>(number(v, "index", 1)));
        bool lfoEnvelope = static_cast<bool>(v["envelope"]);
        const auto selected = selectedPanelChoice(v);
        if (selected.isObject() && kind != "tabs")
        {
            auto source = string(selected, "source");
            index = std::max(1, static_cast<int>(number(selected, "index", 1)));
            lfoEnvelope = source == "lfo-envelope";
            if (source == "fx")
                kind = "effects";
            else if (source == "lfo-envelope")
                kind = "lfo";
            else if (source == "oscillator" && kind == "waveform")
                kind = "waveform";
            else
                kind = source;
        }
        auto title = string(v, "title", kind.toUpperCase());
        juce::var inheritedEffect;
        if (kind == "tabs")
            if (auto *panels = manifest["panels"].getArray())
                for (const auto &parent : *panels)
                    if (string(parent, "kind") == "effects" &&
                        r.getY() >= bounds(parent).getY() +
                                        std::max(1.f, number(parent, "titleHeight", 24)) &&
                        bounds(parent).contains(r))
                    {
                        const auto effect = selectedPanelChoice(parent);
                        if (string(effect, "source") == "fx")
                        {
                            inheritedEffect = effect;
                            title = parameters.display("fx" +
                                juce::String(static_cast<int>(number(effect, "index", 1))) +
                                ".type").toUpperCase();
                        }
                    }
        auto titleColor = theme.text;
        if (v.hasProperty("ink"))
            titleColor = theme.color(string(v, "ink"));
        if (theme.style == "bauhaus" &&
            (string(v, "color") == "red" || string(v, "color") == "blue"))
            titleColor = juce::Colours::white;
        const auto titleInset = number(v, "titleInset", 12);
        auto titleY = theme.style == "ivory" && (kind == "oscillator" || kind == "filter")
                          ? 11.f : theme.style == "precision" ? 6.f
                          : theme.style == "spectral" ? 5.f : 3.f;
        if (theme.style == "clear")
        {
            const auto id = string(v, "id");
            titleY = kind == "matrix" ? 3.f : kind == "oscillator" ? 9.f
                         : id == "panel.osc-title" || id == "panel.macros-main" ? 13.f : 8.f;
        }
        auto titleBounds = Rect(r.getX() + titleInset, r.getY() + titleY,
                                std::max(0.f, r.getWidth() - titleInset - 12.f), 25);
        if (kind == "matrix")
            titleBounds.setWidth(std::max(25.f, titleBounds.getWidth() - 112.f));
        else if (kind == "effects")
            titleBounds.setWidth(std::max(25.f, titleBounds.getWidth() - 110.f));
        else if (kind == "analyzer")
        {
            const auto modeTabs = arrayRect(v["analyzerModesRect"], {});
            if (!modeTabs.isEmpty() && modeTabs.intersects(titleBounds))
                titleBounds.setRight(std::max(titleBounds.getX(), modeTabs.getX() - 8));
            else if (modeTabs.isEmpty() && theme.style != "spectral" &&
                     theme.style != "bauhaus" && theme.style != "flow")
                titleBounds.setWidth(
                    std::max(25.f, titleBounds.getWidth() - (r.getWidth() < 320 ? 54.f : 92.f)));
        }
        const auto tabs = arrayRect(v["selection"]["tabsRect"], {});
        const auto titleHeight = number(v, "titleHeight", 24);
        if (!tabs.isEmpty() && tabs.intersects(titleBounds) &&
            tabs.getX() > titleBounds.getX() + 80)
            titleBounds.setRight(tabs.getX() - 8);
        if (titleHeight > 0 && (tabs.isEmpty() || !tabs.intersects(titleBounds)))
            text(g, theme, title, titleBounds.withHeight(std::min(25.f, titleHeight)),
                 number(v, "fontSize", theme.style == "ivory" ? 18 : 15), titleColor, true);
        auto content = r.reduced(10).withTrimmedTop(titleHeight);
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
                plotBase(g, pr, false, theme.style == "tokyo" || theme.style == "flow");
            if (index <= 3)
            {
                const bool layers =
                    variant == "terrain" || theme.style == "aurora" || theme.style == "live";
                const bool paperTerrain = theme.style == "bauhaus" && variant == "terrain";
                if (layers && visuals.wavetable[index - 1].available())
                    Visuals::wavetable(g, pr.reduced(5), visuals.wavetable[index - 1],
                                       paperTerrain ? juce::Colour(0xffe4dac4) : waveformInk,
                                       paperTerrain ? juce::Colour(0xffc7bea9)
                                           : theme.style == "tokyo" ? waveformInk : theme.accent,
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
            const auto oscillator = juce::jlimit(0, 2, index - 1);
            const auto primary = theme.style == "aurora" ? juce::Colour(0xff4bd1ff)
                                                        : theme.secondary;
            const auto secondary = theme.style == "aurora" ? juce::Colour(0xffef4dff)
                                                          : theme.accent;
            pr = arrayRect(v["plot"], content.withTrimmedBottom(variant == "terrain" ? 20 : 3));
            if (theme.style != "aurora" && theme.style != "live")
                plotBase(g, pr, false, theme.style != "bauhaus");
            if (visuals.wavetable[oscillator].available())
                Visuals::wavetable(
                    g, pr.reduced(8), visuals.wavetable[oscillator], primary, secondary,
                    theme.style == "bauhaus" ? Visuals::WavetableView::harmonicRibbons
                    : theme.style == "aurora" || theme.style == "live"
                        ? Visuals::WavetableView::ribbons
                        : Visuals::WavetableView::perspective);
            else
                Visuals::waveform(g, pr.reduced(8), visuals.oscillator[oscillator], primary,
                                  secondary,
                                  variant == "terrain" || theme.style == "bauhaus" ||
                                      theme.style == "live" || theme.style == "aurora");
            if (visuals.oscillator[oscillator].empty())
                unavailableOscillator(g, pr.withTrimmedBottom(22), oscillator);
            if (theme.style == "spectral" && visuals.wavetable[oscillator].available())
            {
                const auto &table = visuals.wavetable[oscillator];
                const auto name = table.name.isNotEmpty() ? table.name : "Current wavetable";
                text(g, theme, "WT: " + name, {pr.getX() + 12, pr.getY() + 7, 340, 18},
                     13, theme.text);
                text(g, theme, juce::String(table.sourceFrames) + " frames / " +
                                   juce::String(table.sourceFrameSize) + " samples per frame",
                     {pr.getX() + 12, pr.getY() + 27, 340, 17}, 11, theme.muted);
                text(g, theme, "WT POS " + parameters.display("osc" +
                                     juce::String(oscillator + 1) + ".param0"),
                     {pr.getRight() - 160, pr.getY() + 7, 148, 18}, 11, theme.secondary,
                     false, juce::Justification::centredRight);
            }
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
            if (lfoEnvelope)
                drawEnvelope(g, pr, index, curveInk, true);
            else
                drawLfo(g, pr, index);
        }
        else if (kind == "analyzer")
        {
            const auto mode = analyzerMode(v);
            pr = arrayRect(v["plot"], content.withTrimmedRight(45).reduced(3));
            if (theme.style == "bauhaus")
                drawStereoBands(g, pr);
            else
            {
                plotBase(g, pr);
                if (mode == "spectrum" || mode == "waterfall")
                    drawSpectrum(g, pr, mode == "waterfall");
                else if (mode == "scope" || mode == "vectorscope")
                    drawScope(g, pr, mode == "vectorscope");
                else if (mode == "meter")
                    drawMeters(g, pr.reduced(std::max(20.f, pr.getWidth() * .34f), 10));
                if (theme.style == "ivory")
                {
                    const Rect meters(pr.getRight() + 28, pr.getY() - 3, 34,
                                      pr.getHeight() + 16);
                    text(g, theme, "OUT", meters.withY(meters.getY() - 30).withHeight(20),
                         12, theme.text, false, juce::Justification::centred);
                    drawMeters(g, meters);
                }
                else if (theme.style == "precision")
                    drawMeters(g, {pr.getRight() + 9, pr.getY() - 34,
                                   std::max(10.f, r.getRight() - pr.getRight() - 21),
                                   pr.getHeight() + 42});
                else if (theme.style == "spectral")
                    drawMeters(g, {pr.getRight() + 17, pr.getY(), 30, pr.getHeight() + 9});
                else if (theme.style != "clear" && theme.style != "live")
                    drawMeters(g, content.removeFromRight(std::min(45.f, content.getWidth() * .15f))
                                      .reduced(4));
            }
        }
        else if (kind == "xy" && theme.style == "live")
        {
            // The pad's mouse hit area is unchanged. These labels sit beneath
            // its declared plot and always describe the actual macro axes.
            auto footer = Rect(pr.getX() - 9, pr.getBottom() + 10, pr.getWidth() + 18, 18);
            const auto width = footer.getWidth() * .5f;
            for (int axis = 0; axis < 2; ++axis)
            {
                const auto binding = axis ? "macro2" : "macro1";
                auto cell = footer.removeFromLeft(width).reduced(3, 0);
                auto value = cell.removeFromRight(35);
                text(g, theme, juce::String(axis ? "Y: " : "X: ") + parameters.label(binding),
                     cell, 9.5f, theme.text, false, juce::Justification::centredRight);
                bezel(g, theme, value.reduced(2, 1), theme.display, 4);
                text(g, theme, juce::String(parameters.value(binding), 2), value, 10,
                     theme.accent.interpolatedWith(juce::Colours::white, .55f), false,
                     juce::Justification::centred);
            }
        }
        else if (kind == "keyboard" && theme.style == "live" && keyboard)
        {
            // All geometry follows the placed, playable JUCE keyboard. The
            // strips project real scene ranges onto its visible MIDI notes.
            const auto keys = keyboard->getBounds().toFloat();
            const auto well = keys.expanded(10, 0).withTop(keys.getY() - 76)
                                  .withBottom(keys.getBottom() + 23);
            bezel(g, theme, well, theme.display, 6);
            const auto *modeParameter = parameters.parameter("scene.mode");
            const auto *splitParameter = parameters.parameter("scene.splitpoint");
            const auto *activeParameter = parameters.parameter("scene.active");
            const bool available = modeParameter && splitParameter && activeParameter;
            const int mode = available ? modeParameter->val.i : -1;
            const int split = available ? juce::jlimit(0, 127, splitParameter->val.i) : -1;
            const int active = available ? juce::jlimit(0, 1, activeParameter->val.i) : -1;
            std::array<juce::Range<int>, 2> noteRanges{}, channelRanges{};
            for (auto &channels : channelRanges)
                channels = {keyboard->getMidiChannel(), keyboard->getMidiChannel() + 1};
            switch (mode)
            {
            case sm_single:
                noteRanges[active] = {0, 128};
                break;
            case sm_split:
                // calculateChannelMask uses key < split for A; the split key is B.
                noteRanges[0] = {0, split};
                noteRanges[1] = {split, 128};
                break;
            case sm_dual:
                noteRanges[0] = noteRanges[1] = {0, 128};
                break;
            case sm_chsplit:
            {
                // The engine's zero-based boundary is splitpoint / 8 + 1.
                const int lastChannelA = split / 8 + 1;
                channelRanges[0] = {1, lastChannelA + 1};
                channelRanges[1] = {lastChannelA + 1, 17};
                for (int scene = 0; scene < 2; ++scene)
                    if (!channelRanges[scene].isEmpty())
                        noteRanges[scene] = {0, 128};
                break;
            }
            default:
                break;
            }

            auto modeText = available ? parameters.display("scene.mode") : "Unavailable";
            if (mode == sm_split)
                modeText += utf8("  ·  split MIDI ") + juce::String(split);
            modeText += utf8("  ·  MIDI ch ") + juce::String(keyboard->getMidiChannel());
            text(g, theme, "Key ranges", {keys.getX(), well.getY() + 7, 100, 14},
                 11.5f, theme.text, true);
            text(g, theme, modeText,
                 {keys.getX() + 104, well.getY() + 7, keys.getWidth() - 104, 14},
                 10.5f, theme.muted, false, juce::Justification::centredRight);

            const juce::Range<int> shown(keyboard->getRangeStart(), keyboard->getRangeEnd() + 1);
            const int rangeCount = static_cast<int>(!noteRanges[0].isEmpty()) +
                                   static_cast<int>(!noteRanges[1].isEmpty());
            juce::Array<juce::var> rangeProperties;
            juce::StringArray descriptions;
            int row = 0;
            for (int scene = 0; scene < 2; ++scene)
            {
                const auto notes = noteRanges[scene];
                if (notes.isEmpty())
                    continue;
                const auto channels = channelRanges[scene];
                juce::Array<juce::var> state;
                for (const int value : {scene, notes.getStart(), notes.getEnd() - 1,
                                       channels.getStart(), channels.getEnd() - 1})
                    state.add(value);
                rangeProperties.add(juce::var(state));
                const auto visible = notes.getIntersectionWith(shown);
                auto caption = juce::String(scene == 0 ? "Scene A" : "Scene B");
                if (mode == sm_chsplit)
                    caption += utf8("  ·  Ch ") + juce::String(channels.getStart()) +
                               utf8("–") + juce::String(channels.getEnd() - 1);
                caption += visible.isEmpty()
                               ? utf8("  ·  outside shown keys")
                               : utf8("  ·  ") + keyboard->noteName(visible.getStart()) +
                                     utf8(" – ") + keyboard->noteName(visible.getEnd() - 1);
                const auto ink = scene == 0 ? theme.accent : theme.secondary;
                const float captionY = keys.getY() - (rangeCount == 1 ? 48.f : 54.f) + row * 27.f;
                const float barY = keys.getY() - (rangeCount == 1 ? 24.f : 38.f) + row * 27.f;
                text(g, theme, caption, {keys.getX(), captionY, keys.getWidth(), 12}, 10.5f,
                     ink.interpolatedWith(theme.text, .35f));
                descriptions.add(caption);
                ++row;
                if (visible.isEmpty())
                    continue;
                const auto first = keyboard->keyBoundsOnCanvas(visible.getStart());
                const auto last = keyboard->keyBoundsOnCanvas(visible.getEnd() - 1);
                const Rect bar(first.getX(), barY, last.getRight() - first.getX(), 8);
                g.setColour(ink.withAlpha(.09f));
                g.fillRoundedRectangle(bar.expanded(3, 3), 6);
                g.setColour(ink.withAlpha(.16f));
                g.fillRoundedRectangle(bar.expanded(1.5f, 1.5f), 5);
                g.setGradientFill(juce::ColourGradient(ink.brighter(.3f), bar.getTopLeft(),
                                                      ink.darker(.5f), bar.getBottomLeft(), false));
                g.fillRoundedRectangle(bar, 4);
                g.setColour(ink.interpolatedWith(juce::Colours::white, .55f));
                g.drawRoundedRectangle(bar.reduced(.5f), 3.5f, .8f);
                const float radius = std::min(2.5f, bar.getWidth() * .25f);
                g.fillEllipse(bar.getRight() - radius * 2 - 1, bar.getCentreY() - radius,
                              radius * 2, radius * 2);
            }
            auto &properties = keyboard->getProperties();
            properties.set("reference.keyboard-mode", mode);
            properties.set("reference.keyboard-active-scene", active);
            properties.set("reference.keyboard-split-point", split);
            properties.set("reference.keyboard-channel", keyboard->getMidiChannel());
            // Rows are [scene, first MIDI note, last MIDI note, first channel,
            // last channel], inclusive. Paint clips the full 0..127 note ranges;
            // channels describe this keyboard unless Channel Split supplies groups.
            properties.set("reference.keyboard-ranges", juce::var(rangeProperties));
            keyboard->setDescription("Playable MIDI keyboard. " + modeText +
                                     ". Visible keys: " + descriptions.joinIntoString("; "));
            keyboard->drawFooter(g, theme);
        }
        else if (kind == "output")
        {
            if (theme.style == "space")
                drawMeters(g, arrayRect(v["plot"],
                           content.removeFromRight(std::min(108.f, content.getWidth() * .46f))
                               .reduced(2, 13)),
                           true);
            else
                drawMeters(g, arrayRect(v["plot"],
                    content.removeFromRight(std::min(58.f, content.getWidth() * .28f)).reduced(4)));
        }
        else if (kind == "matrix")
        {
            const auto layout = MatrixLayout::make(r, theme.style, variant, titleHeight);
            const bool clear = theme.style == "clear", ivory = theme.style == "ivory";
            const bool space = theme.style == "space", tokyo = theme.style == "tokyo";
            const bool spectral = theme.style == "spectral", aurora = theme.style == "aurora";
            const bool bauhaus = theme.style == "bauhaus", flow = theme.style == "flow";
            const bool live = theme.style == "live";
            const auto ink = space ? juce::Colour(0xffeee0b5) : theme.text;
            const auto quiet = space ? juce::Colour(0xffcbb883) : theme.muted;
            const auto active = space ? juce::Colour(0xffff9e24) : theme.accent;
            juce::Graphics::ScopedSaveState matrixClip(g);
            g.reduceClipRegion(r.toNearestInt());

            if (space)
            {
                bezel(g, theme, layout.body().expanded(3, 5), juce::Colour(0xff19180b), 4);
                g.setColour(juce::Colour(0xffa59360).withAlpha(.55f));
                g.drawRoundedRectangle(layout.body().expanded(1, 3), 3, .8f);
            }
            else if (ivory)
            {
                auto frame = r.reduced(7, 0).withTrimmedTop(30).withTrimmedBottom(8);
                g.setColour(juce::Colour(0xfffff8eb).withAlpha(.44f));
                g.fillRoundedRectangle(frame, 14);
                g.setColour(juce::Colour(0xffbca98e).withAlpha(.7f));
                g.drawRoundedRectangle(frame, 14, 1);
                g.setColour(juce::Colours::white.withAlpha(.65f));
                g.drawRoundedRectangle(frame.reduced(1.5f), 12.5f, .7f);
            }

            const auto titleCase = clear || spectral || aurora || live;
            auto headingText = [&](const char *label, Rect field, bool centred = false) {
                if (field.isEmpty())
                    return;
                const auto caption = juce::String(label);
                text(g, theme, titleCase ? caption : caption.toUpperCase(), field,
                     layout.headerFont, clear || bauhaus ? ink : quiet, clear || bauhaus,
                     centred ? juce::Justification::centred : juce::Justification::centredLeft);
            };
            auto readOnlyCell = [&](Rect field, const juce::String &label, juce::Colour color,
                                    float fontSize) {
                if (field.isEmpty())
                    return;
                const auto height = std::min(field.getHeight() - 2.f,
                                             ivory ? 25.f : aurora ? 19.f : tokyo ? 22.f : 24.f);
                auto face = field.withSizeKeepingCentre(field.getWidth(), height);
                if (!clear && !live)
                {
                    auto fill = theme.display;
                    auto edge = theme.border.withAlpha(.8f);
                    if (ivory)
                    {
                        fill = juce::Colour(0xffefe1cd);
                        edge = juce::Colour(0xffc0ad91);
                    }
                    else if (bauhaus)
                    {
                        fill = juce::Colour(0xfffff6df);
                        edge = juce::Colour(0xff9e967a);
                    }
                    else if (space)
                    {
                        fill = juce::Colour(0xff1e1b0d);
                        edge = juce::Colour(0xff76663c);
                    }
                    else if (aurora)
                    {
                        fill = juce::Colour(0xff28223e);
                        edge = juce::Colour(0xff5b4d77).withAlpha(.7f);
                    }
                    else if (spectral)
                        edge = juce::Colour(0xff414d70).withAlpha(.85f);
                    else if (flow)
                        edge = juce::Colour(0xff294b61).withAlpha(.85f);
                    const float radius = tokyo || space ? 2.f : 4.f;
                    g.setGradientFill(juce::ColourGradient(fill.brighter(.06f), face.getTopLeft(),
                                                          fill.darker(.06f),
                                                          face.getBottomLeft(), false));
                    g.fillRoundedRectangle(face, radius);
                    g.setColour(edge);
                    g.drawRoundedRectangle(face.reduced(.4f), radius, .8f);
                    g.setColour((ivory || bauhaus ? juce::Colours::white : edge.brighter(.3f))
                                    .withAlpha(.3f));
                    g.drawHorizontalLine(static_cast<int>(face.getY() + 1), face.getX() + 4,
                                         face.getRight() - 4);
                }
                // No chevron: these are real route labels, not inactive fake selectors.
                text(g, theme, label, face.reduced(clear || live ? 0.f : 7.f, 0), fontSize,
                     color, clear || bauhaus);
            };
            for (int column = 0; column < layout.columns; ++column)
            {
                const auto heading = layout.header(column);
                if (spectral || flow || live)
                {
                    g.setGradientFill(juce::ColourGradient(theme.panel.brighter(.12f),
                                                          heading.row.getTopLeft(), theme.display,
                                                          heading.row.getBottomLeft(), false));
                    g.fillRoundedRectangle(heading.row, spectral ? 5.f : 2.f);
                }
                g.setColour((space ? juce::Colour(0xff76663c) : theme.border).withAlpha(.45f));
                g.drawHorizontalLine(static_cast<int>(heading.row.getBottom()),
                                     heading.row.getX(), heading.row.getRight());
                headingText("#", heading.number);
                headingText("Source", heading.source.withTrimmedLeft(clear || live ? 0.f : 7.f));
                headingText("Destination",
                            heading.destination.withTrimmedLeft(clear || live ? 0.f : 7.f));
                headingText(spectral ? "Depth" : "Amount", heading.amount, true);
                headingText("Curve", heading.curve, true);
                const auto rangeHeading =
                    aurora ? heading.polarity.withSizeKeepingCentre(48, heading.row.getHeight())
                           : heading.polarity;
                headingText(space ? "Bip" : aurora ? "Bipolar" : "Range", rangeHeading, true);
                if (!heading.arrow.isEmpty())
                    text(g, theme, utf8("→"), heading.arrow, layout.headerFont + 2, quiet, false,
                         juce::Justification::centred);

                for (int rowIndex = 0; rowIndex < layout.rowsPerColumn; ++rowIndex)
                {
                    const int index = column * layout.rowsPerColumn + rowIndex;
                    const auto cell = layout.row(index);
                    if (clear && rowIndex % 2 == 0)
                    {
                        g.setColour(theme.border.withAlpha(.075f));
                        g.fillRect(cell.row);
                    }
                    if (!ivory && !aurora && !bauhaus)
                    {
                        g.setColour((space ? juce::Colour(0xff76663c) : theme.border)
                                        .withAlpha(flow ? .55f : .3f));
                        g.drawHorizontalLine(static_cast<int>(cell.row.getBottom()),
                                             cell.row.getX(), cell.row.getRight());
                    }
                    text(g, theme, juce::String(index + 1), cell.number, layout.rowFont - .5f,
                         quiet, clear || bauhaus);
                    if (index >= static_cast<int>(routes.size()))
                    {
                        text(g, theme,
                             utf8(index == 0 ? "No routes in this patch — use Edit routes" : "—"),
                             cell.row.withTrimmedLeft(cell.source.getX() - cell.row.getX()),
                             layout.rowFont, quiet);
                        continue;
                    }
                    const auto &route = routes[index];
                    const auto color = route.muted ? quiet : ink;
                    const auto accent = route.muted ? quiet : active;
                    if (clear && route.key == inspectedRoute)
                    {
                        g.setColour(theme.accent.withAlpha(.15f));
                        g.fillRoundedRectangle(cell.row, 2);
                    }
                    readOnlyCell(cell.source, route.source, color, layout.rowFont);
                    auto destination = route.destination;
                    const juce::String scenePrefix = parameters.getScene() == 0 ? "A " : "B ";
                    if (destination.startsWith(scenePrefix))
                        destination = destination.substring(scenePrefix.length());
                    readOnlyCell(cell.destination, destination, color, layout.rowFont);
                    if (!cell.arrow.isEmpty())
                        text(g, theme, utf8("→"), cell.arrow, layout.rowFont + 1, color, false,
                             juce::Justification::centred);

                    const auto depth = juce::jlimit(-1.f, 1.f, route.normalizedDepth);
                    const auto decimals = std::abs(depth) > 0.f && std::abs(depth) < .01f ? 3 : 2;
                    const auto depthLabel = (depth >= 0 ? "+" : "") + juce::String(depth, decimals);
                    text(g, theme, depthLabel, cell.value.reduced(1, 0),
                         clear ? 13.f : tokyo ? 12.f : layout.rowFont - .2f, color,
                         clear || bauhaus || ivory,
                         layout.valuePosition == MatrixLayout::ValuePosition::above && ivory
                             ? juce::Justification::centredRight
                             : juce::Justification::centred);

                    // The transparent native slider has its own look-and-feel thumb inset.
                    // Paint its exact endpoint positions, not a second approximation that
                    // would make clicks and a visible handle disagree. The same layout
                    // above supplied both row bounds and the slider's separate hit region.
                    auto track = cell.slider.reduced(7, 0).withSizeKeepingCentre(
                        std::max(1.f, cell.slider.getWidth() - 14), layout.trackHeight);
                    if (index < static_cast<int>(routeControls.size()) &&
                        routeControls[index]->route().key == route.key)
                    {
                        auto &control = *routeControls[index];
                        auto &slider = control.depthSlider();
                        const auto origin = control.getX() + slider.getX();
                        const auto left = slider.getPositionOfValue(-1);
                        const auto right = slider.getPositionOfValue(1);
                        track = Rect(origin + left,
                                     control.getY() + slider.getY() + slider.getHeight() * .5f -
                                         layout.trackHeight * .5f,
                                     std::max(1.f, right - left), layout.trackHeight);
                    }
                    const auto midpoint = track.getCentreX();
                    const auto handleX = track.getX() + (depth + 1.f) * .5f * track.getWidth();
                    const auto radius = std::min(3.f, track.getHeight() * .5f);
                    auto trackColor = theme.border.withAlpha(.65f);
                    if (ivory || bauhaus)
                        trackColor = juce::Colour(0xffe4d7bb);
                    else if (space)
                        trackColor = juce::Colour(0xff3b3018);
                    else if (aurora)
                        trackColor = juce::Colour(0xff51416d);
                    else if (clear)
                        trackColor = juce::Colour(0xffdce2e5);
                    g.setColour(trackColor);
                    g.fillRoundedRectangle(track, radius);
                    if (ivory || bauhaus || spectral || space)
                    {
                        g.setColour((ivory || bauhaus ? juce::Colour(0xff8c846c) : quiet)
                                        .withAlpha(.55f));
                        g.drawRoundedRectangle(track, radius, .7f);
                    }
                    // Native normalized depth is signed. The centre tick is zero and
                    // coloured travel extends from zero to its actual slider position.
                    auto travel = track.withX(std::min(midpoint, handleX))
                                      .withWidth(std::max(.8f, std::abs(handleX - midpoint)));
                    auto fill = clear && depth < 0 ? theme.red : accent;
                    if (bauhaus)
                        fill = juce::Colour(0xffffcf21);
                    if (spectral || aurora || flow)
                    {
                        g.setColour(fill.withAlpha(.12f));
                        g.fillRoundedRectangle(travel.expanded(2), radius + 1);
                    }
                    g.setGradientFill(juce::ColourGradient(fill.darker(.18f), travel.getTopLeft(),
                                                          fill.brighter(.15f),
                                                          travel.getBottomLeft(), false));
                    g.fillRoundedRectangle(travel, radius);
                    g.setColour(quiet.withAlpha(.6f));
                    g.drawLine(midpoint, track.getY() - 1, midpoint, track.getBottom() + 1, .7f);
                    if (bauhaus || aurora || flow)
                    {
                        const auto diameter = bauhaus ? 13.f : flow ? 10.f : 8.f;
                        Rect handle(handleX - diameter * .5f,
                                    track.getCentreY() - diameter * .5f, diameter, diameter);
                        g.setColour(juce::Colours::black.withAlpha(.28f));
                        g.fillEllipse(handle.expanded(1).translated(0, 1));
                        g.setColour(bauhaus ? juce::Colour(0xfffffdf4)
                                           : aurora ? juce::Colour(0xffead8ff) : accent);
                        g.fillEllipse(handle);
                        g.setColour(bauhaus ? juce::Colour(0xff3d443d) : accent.brighter(.3f));
                        g.drawEllipse(handle, bauhaus ? 1.2f : .8f);
                    }
                    else
                    {
                        g.setColour(fill.brighter(.3f));
                        g.fillRoundedRectangle(handleX - 1.2f, track.getY() - .5f, 2.4f,
                                               track.getHeight() + 1.f, 1);
                    }

                    if (!cell.curve.isEmpty())
                    {
                        readOnlyCell(cell.curve, {}, color, layout.rowFont);
                        auto curve = cell.curve.reduced(tokyo ? 3.f : 5.f, 5.f);
                        const auto y = curve.getCentreY();
                        // Surge applies this route's linear signed depth. Do not display
                        // invented S-curves or non-functional curve-selection controls.
                        g.setColour((flow || spectral || aurora ? accent : color)
                                        .withAlpha(route.muted ? .5f : .9f));
                        g.drawLine(curve.getX(), y + depth * curve.getHeight() * .4f,
                                   curve.getRight(), y - depth * curve.getHeight() * .4f,
                                   bauhaus ? 1.8f : 1.3f);
                    }
                    if (!cell.polarity.isEmpty())
                    {
                        const auto range = space || aurora
                                               ? utf8(route.bipolar ? "±" : "+")
                                               : juce::String(route.bipolar ? "Bipolar" : "Unipolar");
                        // This is native source metadata, not a painted checkbox or toggle.
                        text(g, theme, range, cell.polarity, space || aurora ? 16.f : 11.5f,
                             route.bipolar ? accent : quiet, false,
                             juce::Justification::centred);
                    }
                }
            }
            if (layout.columns == 2)
            {
                const auto x = r.getCentreX();
                g.setColour(theme.border.withAlpha(.8f));
                g.drawLine(x, r.getY() + layout.tableTop, x, layout.body().getBottom() + 5, 1);
            }
        }
        else if (kind == "browser")
        {
            // Search and rows are native, accessible child controls populated from
            // the actual installed library. Never paint an inert mock preset list.
        }
        else if (kind == "routing")
            drawRouting(g, arrayRect(v["plot"], content), variant, v["flowNodes"]);
        else if ((kind == "effects" &&
                  (variant == "compact" || (selected.isObject() && v["plot"].isArray()))) ||
                 (kind == "tabs" && inheritedEffect.isObject() && v["plot"].isArray()))
        {
            const auto slot = inheritedEffect.isObject()
                                  ? static_cast<int>(number(inheritedEffect, "index", 1))
                                  : index;
            const auto algorithm = parameters.display("fx" + juce::String(slot) + ".type");
            const auto &curve = effectCurves[juce::jlimit(0, n_fx_slots - 1, slot - 1)];
            const auto ink = theme.style == "clear" ? (slot % 2 ? theme.accent : theme.red)
                            : variant == "compact" || theme.style == "precision"
                                  ? theme.accent : theme.secondary;
            if (theme.style == "precision" && variant != "compact")
                text(g, theme, algorithm.toUpperCase(),
                     {r.getX() + 47, r.getY() + 103, r.getWidth() - 61, 24},
                     15.5f, theme.text, true);
            if (variant != "compact")
                bezel(g, theme, pr, theme.display, 4);
            juce::Graphics::ScopedSaveState saved(g);
            g.reduceClipRegion(pr.toNearestInt());
            if (curve.available && curve.values.size() > 1 && pr.getHeight() >= 44)
            {
                const auto plot = pr.reduced(7, 5).withTrimmedTop(12).withTrimmedBottom(14);
                const auto labelInk = theme.display.contrasting(.75f);
                const auto axisInk = theme.display.contrasting(.32f);
                g.setColour(axisInk.withAlpha(.55f));
                g.drawLine(plot.getX(), plot.getCentreY(), plot.getRight(), plot.getCentreY(), .7f);
                g.drawLine(plot.getCentreX(), plot.getY(), plot.getCentreX(), plot.getBottom(), .7f);
                g.setColour(axisInk.withAlpha(.18f));
                g.drawLine(plot.getX(), plot.getBottom(), plot.getRight(), plot.getY(), .7f);

                juce::Path transfer;
                bool clipped = false;
                for (std::size_t i = 0; i < curve.values.size(); ++i)
                {
                    const auto value = curve.values[i];
                    clipped = clipped || std::abs(value) > 1.f;
                    const auto x = plot.getX() + plot.getWidth() * i / (curve.values.size() - 1);
                    const auto y = plot.getCentreY() -
                                   std::clamp(value, -1.f, 1.f) * plot.getHeight() * .5f;
                    if (i == 0)
                        transfer.startNewSubPath(x, y);
                    else
                        transfer.lineTo(x, y);
                }
                if (!theme.light())
                {
                    g.setColour(ink.withAlpha(.12f));
                    g.strokePath(transfer, juce::PathStrokeType(5));
                }
                g.setColour(ink);
                g.strokePath(transfer, juce::PathStrokeType(1.8f,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                const auto model = curve.caption.upToFirstOccurrenceOf(" transfer", false, false);
                text(g, theme, pr.getWidth() >= 130 ? model + " transfer" : "Transfer",
                     pr.reduced(4, 0).withHeight(16), 9, labelInk, false,
                     juce::Justification::centred);
                text(g, theme, clipped ? "Shaper only / clipped"
                                      : pr.getWidth() >= 195 ? "EQ, feedback, output excluded"
                                                            : "Shaper only",
                     pr.reduced(4, 0).withTop(pr.getBottom() - 17), 8.5f, labelInk,
                     false, juce::Justification::centred);
            }
            else
            {
                effectGlyph(g, pr.getHeight() < 44 ? pr.reduced(4)
                                                  : pr.reduced(9, 10).withTrimmedBottom(10),
                            ink, algorithm);
                if (pr.getHeight() >= 38)
                    text(g, theme, algorithm,
                         pr.reduced(3, 0).withTop(pr.getBottom() - 17), 8.5f,
                         theme.display.contrasting(.7f), false, juce::Justification::centred);
            }
        }
        else if (kind == "effects" && !selected.isObject())
        {
            // Selected FX panels use native slot tabs and rebound detail
            // controls. Only legacy manifests need this painted slot strip.
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
            if (variant == "vertical" && theme.style == "tokyo")
                for (int slot = 1; slot <= 8; ++slot)
                {
                    const float centreY = r.getY() + 40.5f + (slot - 1) * 21.f;
                    text(g, theme, juce::String(slot),
                         {r.getX() + 12, centreY - 9, 25, 18}, 13, theme.text, true,
                         juce::Justification::centred);
                    const bool active = parameters.choiceIndex("fx" + juce::String(slot) + ".type") > 0;
                    const auto lamp = Rect(r.getX() + 48, centreY - 3.5f, 7, 7);
                    g.setColour(active ? theme.accent : theme.border.withAlpha(.5f));
                    if (active)
                        g.fillEllipse(lamp);
                    else
                        g.drawEllipse(lamp, 1);
                }
            if (variant != "vertical" && content.getHeight() > 36)
                text(g, theme, "16 native FX slots available in the full editor",
                     content.removeFromBottom(18), 10, theme.muted);
        }
        else if (kind == "scene" && theme.style == "space" && string(v, "id") == "space-info")
        {
            auto status = arrayRect(v["plot"], content);
            bezel(g, theme, status, theme.display, 2);
            status = status.reduced(8, 3);
            const auto voiceCount = processor.surge->storage.activeVoiceCount.load(std::memory_order_relaxed);
            const std::array<juce::String, 4> lines{
                "SCENE " + juce::String(parameters.getScene() == 0 ? "A" : "B") +
                    "     VOICES " + juce::String(voiceCount),
                "POLYPHONY " + parameters.display("scene.polylimit"),
                "SAMPLE RATE " + juce::String(processor.getSampleRate() / 1000., 1) + " kHz",
                "SURGE XT " + juce::String(Surge::Build::MajorVersionStr) + "." +
                    juce::String(Surge::Build::SubVersionStr)};
            const auto row = status.getHeight() / lines.size();
            for (const auto &line : lines)
                text(g, theme, line, status.removeFromTop(row), 11.5f, theme.secondary);
        }
        if (theme.style == "ivory")
        {
            const auto panelId = string(v, "id");
            if (panelId == "ivory.macros")
                botanicalEngraving(g, {1472, 96, 108, 39}, theme.border.darker(.3f));
            else if (panelId == "ivory.envelopes")
                botanicalEngraving(g, {1465, 599, 115, 60}, theme.border.darker(.3f));
            else if (panelId == "ivory.fx1")
                botanicalEngraving(g, {1036, 829, 91, 108}, theme.border.darker(.3f));
        }
        if (theme.style == "space" && string(v, "id") == "space-mixer")
            text(g, theme, "RING", {449, 447, 56, 18}, 13, theme.display, true,
                 juce::Justification::centred);
        if (theme.style == "tokyo" && string(v, "id") == "tokyo-filter-2")
            text(g, theme, "MODE", {1526, 329, 46, 17}, 10, theme.text, false,
                 juce::Justification::centred);
        if (theme.style == "tokyo" && string(v, "id") == "tokyo-globals")
        {
            text(g, theme, parameters.label("osc1.param6").toUpperCase(),
                 {1166, 881, 77, 18}, 10, theme.text, false, juce::Justification::centred);
            text(g, theme, "VOICES", {1478, 881, 68, 18}, 10, theme.text, false,
                 juce::Justification::centred);
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
        auto &patch = processor.surge->storage.getPatch();
        const auto &scene = patch.scene[parameters.getScene()];
        const auto mode = scene.filterblock_configuration.val.i;
        // Wide has two stereo copies of F1 -> F2. Serial 3 puts F2 in the
        // feedback branch, not between F1 and the output (QuadFilterChain.cpp).
        const bool parallel = mode == fc_dual1 || mode == fc_dual2 ||
                              mode == fc_stereo || mode == fc_ring;
        const bool feedbackFilter = mode == fc_serial3;
        const bool feedbackActive = mode != fc_serial1 && std::abs(scene.feedback.val.f) > .0001f;
        const auto audioInk = theme.accent;
        const auto controlInk = theme.secondary.interpolatedWith(theme.accent, .55f);
        auto stroke = [&](const juce::Path &path, juce::Colour ink, bool control,
                          bool active = true, bool arrow = true) {
            if (path.isEmpty())
                return;
            const auto width = control ? 1.15f : variant == "flow" ? 2.2f : 1.8f;
            const auto opacity = active ? 1.f : .24f;
            if (!control && active)
            {
                g.setColour(ink.withAlpha(variant == "flow" ? .16f : .12f));
                g.strokePath(path, juce::PathStrokeType(width + 4.f,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour(ink.withAlpha(opacity));
            if (control)
            {
                const float pattern[]{4.f, 3.f};
                juce::Path dashes;
                juce::PathStrokeType(width).createDashedStroke(dashes, path, pattern, 2);
                g.fillPath(dashes);
            }
            else
                g.strokePath(path, juce::PathStrokeType(width,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const auto length = path.getLength();
            if (arrow && length > 8.f)
            {
                const auto end = path.getPointAlongPath(length);
                const auto before = path.getPointAlongPath(length - 4.f);
                const auto delta = end - before;
                const auto magnitude = std::max(.001f, delta.getDistanceFromOrigin());
                const auto direction = delta / magnitude;
                const auto perpendicular = juce::Point<float>(-direction.y, direction.x);
                const auto base = end - direction * (control ? 5.f : 6.f);
                juce::Path head;
                head.addTriangle(end, base + perpendicular * 2.8f,
                                 base - perpendicular * 2.8f);
                g.fillPath(head);
            }
        };
        auto wire = [&](juce::Point<float> a, juce::Point<float> b, bool active = true) {
            juce::Path path;
            path.startNewSubPath(a);
            const auto middle = (a.x + b.x) * .5f;
            if (variant == "flow" && std::abs(b.y - a.y) > 6.f && std::abs(b.x - a.x) > 20.f)
            {
                const auto corner = std::min({12.f, std::abs(b.x - a.x) * .25f,
                                              std::abs(b.y - a.y) * .5f});
                const auto horizontal = b.x > a.x ? 1.f : -1.f;
                const auto vertical = b.y > a.y ? 1.f : -1.f;
                path.lineTo(middle - corner * horizontal, a.y);
                path.quadraticTo(middle, a.y, middle, a.y + corner * vertical);
                path.lineTo(middle, b.y - corner * vertical);
                path.quadraticTo(middle, b.y, middle + corner * horizontal, b.y);
                path.lineTo(b);
            }
            else
                path.cubicTo(middle, a.y, middle, b.y, b.x, b.y);
            stroke(path, audioInk, false, active);
        };
        auto pin = [&](juce::Point<float> point, juce::Colour ink, bool active = true) {
            g.setColour(theme.display);
            g.fillEllipse(point.x - 3.6f, point.y - 3.6f, 7.2f, 7.2f);
            g.setColour(ink.withAlpha(active ? 1.f : .3f));
            if (active)
                g.fillEllipse(point.x - 2.7f, point.y - 2.7f, 5.4f, 5.4f);
            else
                g.drawEllipse(point.x - 2.7f, point.y - 2.7f, 5.4f, 5.4f, 1.f);
        };
        auto filterOn = [&](int index) { return scene.filterunit[index].type.val.i != 0; };
        const juce::StringArray labels =
            feedbackFilter ? juce::StringArray{"OSC", "F1", "AMP", "F2 FB"}
            : parallel ? juce::StringArray{"OSC", "F1 / F2", mode == fc_ring ? "RING" :
                                            mode == fc_stereo ? "L / R" : "MIX", "AMP"}
                       : juce::StringArray{"OSC", "F1", "F2", "AMP"};
        if (variant != "flow")
        {
            juce::Graphics::ScopedSaveState saved(g);
            g.reduceClipRegion(r.expanded(1.f).toNearestInt());
            const bool vertical = variant == "vertical";
            const bool compact = r.getHeight() < 60;
            if (!vertical && !compact)
                text(g, theme, parameters.display("routing"), r.withHeight(21), 12,
                     theme.accent, true);
            auto line = vertical || compact ? r : r.withTrimmedTop(27);
            auto stageLabels = labels;
            // The industrial strip includes the real mixer/FX groups. The
            // parallel filter pair remains one node to avoid implying serial DSP.
            if (theme.style == "tokyo" && !vertical && !feedbackFilter)
                stageLabels = parallel ? juce::StringArray{"OSC", "MIX", "F1/F2",
                                   mode == fc_ring ? "RING" : mode == fc_stereo ? "L/R" : "SUM", "AMP", "FX"}
                               : juce::StringArray{"OSC", "MIX", "F1", "F2", "AMP", "FX"};
            const auto count = stageLabels.size();
            const auto step = (vertical ? line.getHeight() : line.getWidth()) / count;
            std::array<Rect, 6> boxes{};
            for (int i = 0; i < count; ++i)
            {
                auto n = vertical
                    ? Rect(line.getX() + 2, line.getY() + i * step + 1,
                           line.getWidth() - 4, std::max(12.f, step - 9))
                    : Rect(line.getX() + i * step + 3, line.getY(), step - 12,
                           std::max(20.f, line.getHeight() - 3));
                boxes[i] = n;
                const auto label = stageLabels[i];
                const bool enabled = label == "F1" ? filterOn(0) : label == "F2" || label == "F2 FB"
                    ? filterOn(1) : label.contains("F1") ? filterOn(0) || filterOn(1) : false;
                bezel(g, theme, n, enabled ? theme.display.interpolatedWith(audioInk, .10f)
                                          : theme.display, 3);
                g.setColour(audioInk.withAlpha(enabled ? .85f : .3f));
                g.drawRoundedRectangle(n.reduced(.6f), 3, .9f);
                text(g, theme, label, n.reduced(2, 0), vertical ? 10.f : 10.5f,
                     enabled ? audioInk : theme.display.contrasting(.9f), enabled,
                     juce::Justification::centred);
            }
            for (int i = 0; i + 1 < count; ++i)
            {
                // F2 FB is a feedback stage. Do not connect it as a main-output
                // filter merely to fill the available number of boxes.
                if (feedbackFilter && (stageLabels[i] == "F2 FB" || stageLabels[i + 1] == "F2 FB"))
                    continue;
                const auto &a = boxes[i];
                const auto &b = boxes[i + 1];
                juce::Path link;
                if (vertical)
                {
                    link.startNewSubPath(a.getCentreX(), a.getBottom());
                    link.lineTo(b.getCentreX(), b.getY());
                }
                else
                {
                    link.startNewSubPath(a.getRight(), a.getCentreY());
                    link.lineTo(b.getX(), b.getCentreY());
                }
                stroke(link, audioInk, false);
            }
            if (feedbackFilter)
            {
                const auto index = stageLabels.indexOf("F2 FB");
                const auto filter = stageLabels.indexOf("F1");
                const auto amplifier = stageLabels.indexOf("AMP");
                if (index >= 0 && filter >= 0 && amplifier >= 0)
                {
                    const auto &from = boxes[amplifier];
                    const auto &via = boxes[index];
                    const auto &to = boxes[filter];
                    juce::Path send, back;
                    if (vertical)
                    {
                        send.startNewSubPath(from.getCentreX(), from.getBottom());
                        send.lineTo(via.getCentreX(), via.getY());
                        back.startNewSubPath(via.getX(), via.getCentreY());
                        back.lineTo(line.getX(), via.getCentreY());
                        back.lineTo(line.getX(), to.getCentreY());
                        back.lineTo(to.getX(), to.getCentreY());
                    }
                    else
                    {
                        send.startNewSubPath(from.getRight(), from.getCentreY());
                        send.lineTo(via.getX(), via.getCentreY());
                        back.startNewSubPath(via.getCentreX(), via.getBottom());
                        back.lineTo(via.getCentreX(), line.getBottom());
                        back.lineTo(to.getCentreX(), line.getBottom());
                        back.lineTo(to.getCentreX(), to.getBottom());
                    }
                    stroke(send, audioInk, false, feedbackActive);
                    stroke(back, audioInk, false, feedbackActive);
                }
            }
            return;
        }

        juce::Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(r.toNearestInt());
        const auto plot = r;
        g.setColour(theme.display);
        g.fillRoundedRectangle(plot, 3);
        // The cartographer has a fine drafting grid, unlike the signal plots.
        for (int x = 0; x <= static_cast<int>(plot.getWidth()); x += 10)
        {
            g.setColour(theme.secondary.withAlpha(x % 50 == 0 ? .075f : .027f));
            g.drawVerticalLine(static_cast<int>(plot.getX()) + x, plot.getY(), plot.getBottom());
        }
        for (int y = 0; y <= static_cast<int>(plot.getHeight()); y += 10)
        {
            g.setColour(theme.secondary.withAlpha(y % 50 == 0 ? .075f : .027f));
            g.drawHorizontalLine(static_cast<int>(plot.getY()) + y, plot.getX(), plot.getRight());
        }
        r = r.reduced(16, 26);
        auto rectangle = [&](const char *key, float x, float y, float w, float h) {
            return arrayRect(nodes[key], {r.getX() + x * r.getWidth(),
                r.getY() + y * r.getHeight(), w * r.getWidth(), h * r.getHeight()});
        };
        std::array<Rect, 3> oscillators{};
        for (int i = 0; i < 3; ++i)
            oscillators[i] = rectangle(("osc" + juce::String(i + 1)).toRawUTF8(),
                                       .02f, .08f + i * .23f, .20f, .13f);
        const auto mix = rectangle("mixer", .27f, .25f, .13f, .24f);
        const auto f1 = rectangle("filter1", .45f, .15f, .18f, .15f);
        const auto f2 = rectangle("filter2", .45f, .43f, .18f, .15f);
        const auto amp = rectangle("amp", .70f, .29f, .10f, .17f);
        const auto fx = rectangle("fx", .86f, .22f, .11f, .30f);
        const auto out = rectangle("output", .97f, .30f, .03f, .13f);
        const auto sources = rectangle("modsources", .05f, .80f, .27f, .15f);
        const auto modbus = rectangle("modbus", .46f, .80f, .30f, .15f);
        auto node = [&](Rect n, const juce::String &title, bool active = true,
                        float titleInset = 10.f) {
            bezel(g, theme, n, theme.panel, 7);
            g.setColour(theme.secondary.withAlpha(active ? .80f : .28f));
            g.drawRoundedRectangle(n.reduced(.5f), 7, .9f);
            if (title.isNotEmpty())
                text(g, theme, title, n.reduced(titleInset, 4).withHeight(18), 12.5f,
                     theme.text.withAlpha(active ? 1.f : .48f), true);
        };
        // Draw audio paths behind the node surfaces. Parallel paths have an
        // explicit summing/ring junction, while Wide remains a serial pair.
        for (int i = 0; i < 3; ++i)
            wire({oscillators[i].getRight(), oscillators[i].getCentreY()},
                 {mix.getX(), mix.getY() + 30.f + i * 22.f},
                 parameters.value("osc" + juce::String(i + 1) + ".mute") < .5f);
        wire({mix.getRight(), mix.getCentreY()}, {f1.getX(), f1.getCentreY()});
        const juce::Point<float> junction((f1.getRight() + amp.getX()) * .5f, amp.getCentreY());
        if (parallel)
        {
            wire({mix.getRight(), mix.getCentreY()}, {f2.getX(), f2.getCentreY()});
            wire({f1.getRight(), f1.getCentreY()}, {junction.x - 10, junction.y});
            wire({f2.getRight(), f2.getCentreY()}, {junction.x - 10, junction.y});
            wire({junction.x + 10, junction.y}, {amp.getX(), amp.getCentreY()});
        }
        else if (feedbackFilter)
        {
            wire({f1.getRight(), f1.getCentreY()}, {amp.getX(), amp.getCentreY()});
            juce::Path feedback;
            feedback.startNewSubPath(amp.getCentreX(), amp.getBottom());
            feedback.cubicTo(amp.getCentreX(), f2.getCentreY(), f2.getRight() + 30,
                             f2.getCentreY(), f2.getRight(), f2.getCentreY());
            stroke(feedback, audioInk, false, feedbackActive);
            juce::Path back;
            back.startNewSubPath(f2.getX(), f2.getCentreY());
            back.cubicTo(f1.getX() - 25, f2.getCentreY(), f1.getX() - 25,
                         f1.getCentreY(), f1.getX(), f1.getCentreY());
            stroke(back, audioInk, false, feedbackActive);
        }
        else
        {
            juce::Path serial;
            serial.startNewSubPath(f1.getRight(), f1.getCentreY());
            serial.cubicTo(f1.getRight() + 24, f1.getCentreY(), f1.getRight() + 24,
                           f2.getY() - 12, f2.getCentreX(), f2.getY() - 12);
            serial.lineTo(f2.getCentreX(), f2.getY());
            stroke(serial, audioInk, false);
            wire({f2.getRight(), f2.getCentreY()}, {amp.getX(), amp.getCentreY()});
        }
        wire({amp.getRight(), amp.getCentreY()}, {fx.getX(), fx.getCentreY()});
        if (nodes.hasProperty("output"))
            wire({fx.getRight(), fx.getCentreY()}, {out.getX(), out.getCentreY()});

        // These are read-only grouped connections from the last bounded native
        // route snapshot. An unused chip has a hollow port and no invented wire.
        std::array<int, 7> sourceCounts{}, sourceActive{};
        std::array<int, 16> targetCounts{}, targetActive{};
        std::array<Rect, 6> chips{};
        for (int i = 0; i < 6; ++i)
            chips[i] = {427, 454.f + i * (123.f + 4.f) / 6.f, 66, (123.f - 20.f) / 6.f};
        // Respect the same manifest-defined positions as the actual view tabs.
        for (const auto &selectionState : selections)
            if (selectionState.id == "flow.mod-sources")
            {
                const auto area = arrayRect(selectionState.definition["tabsRect"], {});
                for (int i = 0; i < 6 && !area.isEmpty(); ++i)
                    chips[i] = {area.getX(), area.getY() + i * (area.getHeight() + 4.f) / 6.f,
                                area.getWidth(), (area.getHeight() - 20.f) / 6.f};
            }
        const std::array<Rect, 16> targets{oscillators[0], oscillators[1], oscillators[2],
            mix, f1, f2, amp, fx, out, chips[0], chips[1], chips[2], chips[3], chips[4], chips[5], modbus};
        const auto sourceSlot = [&](const RouteRow &route) {
            const auto id = route.key.sourceId;
            if (id == ms_ampeg) return 0;
            if (id == ms_filtereg) return 1;
            if (id == ms_lfo3 && route.key.sourceIndex == 2 && scene.lfo[2].shape.val.i != lt_formula)
                return 2;
            if (id >= ms_lfo1 && id <= ms_lfo3) return 3 + id - ms_lfo1;
            return 6; // Macros, MIDI, scene LFOs and other real sources.
        };
        const auto destinationSlot = [&](const RouteRow &route) {
            if (route.key.parameterId < 0 ||
                static_cast<size_t>(route.key.parameterId) >= patch.param_ptr.size()) return -1;
            const auto *parameter = patch.param_ptr[route.key.parameterId];
            if (!parameter || static_cast<int>(parameter->ctrltype) != route.key.targetType)
                return -1;
            const auto entry = parameter->ctrlgroup_entry;
            switch (parameter->ctrlgroup)
            {
            case cg_OSC: return entry >= 0 && entry < 3 ? entry : -1;
            case cg_MIX: return 3;
            case cg_FILTER: return entry >= 0 && entry < 2 ? 4 + entry : -1;
            case cg_ENV: return entry >= 0 && entry < 2 ? 9 + entry : -1;
            case cg_LFO: return entry >= ms_lfo1 && entry <= ms_lfo3 ? 12 + entry - ms_lfo1 : 15;
            case cg_FX: return 7;
            default:
                if (parameter == &scene.vca_level || parameter == &scene.vca_velsense ||
                    parameter == &scene.volume || parameter == &scene.pan || parameter == &scene.width)
                    return 6;
                if (parameter == &patch.volume) return 8;
                return 15; // A global/unpictured module stays identified at the bus.
            }
        };
        for (const auto &route : routes)
        {
            const auto target = destinationSlot(route);
            if (target < 0) continue;
            const auto source = sourceSlot(route);
            ++sourceCounts[source];
            ++targetCounts[target];
            if (!route.muted && std::abs(route.normalizedDepth) > .000001f)
            {
                ++sourceActive[source];
                ++targetActive[target];
            }
        }
        for (int i = 0; i < 6; ++i)
        {
            const auto colour = i < 3 ? juce::Colour(0xffbca4ef) : theme.secondary;
            if (sourceCounts[i] > 0)
            {
                const auto chip = chips[i];
                juce::Path ingress, egress;
                const auto sourceY = sources.getY() + 15 + i * (sources.getHeight() - 30) / 5.f;
                ingress.startNewSubPath(sources.getRight(), sourceY);
                ingress.cubicTo(sources.getRight() + 13, chip.getCentreY(), chip.getX() - 12,
                                chip.getCentreY(), chip.getX(), chip.getCentreY());
                egress.startNewSubPath(chip.getRight() + 5, chip.getCentreY());
                const auto y = modbus.getCentreY() - 9 + i * 3.f;
                const auto lane = chip.getRight() + 27 + i * 1.5f;
                const auto corner = std::min(10.f, std::abs(y - chip.getCentreY()) * .5f);
                const auto direction = y > chip.getCentreY() ? 1.f : -1.f;
                egress.lineTo(lane - corner, chip.getCentreY());
                egress.quadraticTo(lane, chip.getCentreY(), lane,
                                   chip.getCentreY() + corner * direction);
                egress.lineTo(lane, y - corner * direction);
                egress.quadraticTo(lane, y, lane + corner, y);
                egress.lineTo(modbus.getX(), y);
                stroke(ingress, colour, true, sourceActive[i] > 0);
                stroke(egress, colour, true, sourceActive[i] > 0);
            }
            pin({chips[i].getRight() + 5, chips[i].getCentreY()}, colour, sourceActive[i] > 0);
        }
        if (sourceCounts[6] > 0)
        {
            // The unpictured native sources use a lane below every source chip.
            // Passing this cable through the chip column implied an LFO link.
            juce::Path other;
            const auto start = juce::Point<float>(sources.getCentreX(), sources.getBottom());
            const auto end = juce::Point<float>(modbus.getCentreX(), modbus.getBottom());
            const auto y = std::min(plot.getBottom() - 6, chips.back().getBottom() + 6);
            other.startNewSubPath(start);
            other.lineTo(start.x, y - 10);
            other.quadraticTo(start.x, y, start.x + 10, y);
            other.lineTo(end.x - 10, y);
            other.quadraticTo(end.x, y, end.x, y - 10);
            other.lineTo(end);
            stroke(other, controlInk, true, sourceActive[6] > 0);
            text(g, theme, "Other sources" + utf8(" • ") + juce::String(sourceCounts[6]) +
                              (sourceCounts[6] == 1 ? " route" : " routes"),
                 {end.x + 14, y - 22, std::min(198.f, plot.getRight() - end.x - 24), 14},
                 9.5f, controlInk);
        }
        for (int i = 0; i < 15; ++i)
        {
            if (targetCounts[i] == 0) continue;
            const auto target = targets[i];
            const auto ink = i >= 9 ? juce::Colour(0xffbca4ef) : controlInk;
            juce::Path route;
            if (i >= 9)
            {
                const auto start = juce::Point<float>(modbus.getX(), modbus.getBottom() - 4);
                const auto end = juce::Point<float>(target.getRight() + 9, target.getCentreY());
                route.startNewSubPath(start);
                route.cubicTo(start.x - 55, start.y + 17, end.x + 45, end.y,
                              end.x, end.y);
            }
            else
            {
                const auto x = modbus.getX() + 15 + i * (modbus.getWidth() - 30) / 8.f;
                const auto end = juce::Point<float>(target.getCentreX(), target.getBottom() + 3);
                const auto bendY = std::min(modbus.getY() - 18 - (i % 3) * 14.f,
                                            std::max(end.y + 22, 405.f + (i % 3) * 12));
                route.startNewSubPath(x, modbus.getY());
                route.lineTo(x, bendY + 12);
                route.quadraticTo(x, bendY, x + (end.x > x ? 12.f : -12.f), bendY);
                if (i < 3 || i == 4)
                {
                    // Reach upper stacked modules around their right edge;
                    // a vertical line through lower modules would suggest a
                    // connection to those unrelated processors.
                    const auto lane = target.getRight() + 14.f + (i % 3) * 7.f;
                    route.lineTo(lane + 12.f, bendY);
                    route.quadraticTo(lane, bendY, lane, bendY - 12);
                    route.lineTo(lane, end.y + 24);
                    route.quadraticTo(lane, end.y + 12, lane - 12, end.y + 12);
                    route.lineTo(end.x + 12, end.y + 12);
                    route.quadraticTo(end.x, end.y + 12, end.x, end.y);
                }
                else
                {
                    route.lineTo(end.x + (end.x > x ? -12.f : 12.f), bendY);
                    route.quadraticTo(end.x, bendY, end.x, bendY - 12);
                    route.lineTo(end);
                }
            }
            stroke(route, ink, true, targetActive[i] > 0);
        }

        node(mix, "MIXER");
        for (int i = 0; i < 3; ++i)
        {
            const auto n = oscillators[i];
            const bool active = parameters.value("osc" + juce::String(i + 1) + ".mute") < .5f;
            node(n, {}, active);
            text(g, theme, "OSC " + juce::String(i + 1),
                 {n.getX() + 37, n.getY() + 4, 68, 19}, 12.5f, theme.text, true);
            const auto preview = Rect(n.getX() + 102, n.getY() + 10,
                                      std::max(15.f, n.getWidth() - 112), 30);
            if (!visuals.oscillator[i].empty())
                Visuals::waveform(g, preview, visuals.oscillator[i], theme.accent,
                                  theme.secondary, false);
            else
                unavailableOscillator(g, preview, i);
            pin({n.getRight(), n.getCentreY()}, audioInk, active);
        }
        for (int i = 0; i < 2; ++i)
        {
            const auto n = i == 0 ? f1 : f2;
            node(n, "FILTER " + juce::String(i + 1), filterOn(i), 37.f);
            g.setColour(audioInk.withAlpha(filterOn(i) ? .8f : .2f));
            g.fillEllipse(n.getX() + 16.f, n.getY() + 11.f, 5.f, 5.f);
            const auto &curve = filterCurves[i];
            const auto area = n.withTrimmedTop(40).reduced(10, 4);
            if (area.getHeight() > 4 && curve.available && curve.values.size() > 1)
            {
                juce::Path trace;
                for (size_t j = 0; j < curve.values.size(); ++j)
                {
                    const auto x = area.getX() + area.getWidth() * j / (curve.values.size() - 1);
                    const auto y = area.getBottom() - area.getHeight() * std::clamp(
                        (curve.values[j] - filterPreviewMinimumDb) /
                        (filterPreviewMaximumDb - filterPreviewMinimumDb), 0.f, 1.f);
                    if (j == 0) trace.startNewSubPath(x, y); else trace.lineTo(x, y);
                }
                g.setColour(audioInk.withAlpha(.15f));
                g.strokePath(trace, juce::PathStrokeType(4.f));
                g.setColour(audioInk);
                g.strokePath(trace, juce::PathStrokeType(1.3f));
            }
            pin({n.getRight(), n.getCentreY()}, audioInk);
        }
        node(amp, "AMP");
        drawEnvelope(g, amp.withTrimmedTop(22).reduced(4), 1, theme.accent);
        node(fx, "FX SLOTS (1-8)");
        for (int i = 0; i < 8; ++i)
        {
            const float y = fx.getY() + 40 + i * 24;
            const bool groupEnabled = i < 4 ? patch.fx_bypass.val.i != fxb_no_fx
                : i < 6 ? patch.fx_bypass.val.i == fxb_all_fx
                        : patch.fx_bypass.val.i == fxb_all_fx || patch.fx_bypass.val.i == fxb_no_sends;
            const bool enabled = groupEnabled && patch.fx[i].type.val.i != fxt_off &&
                                 (patch.fx_disable.val.i & (1 << i)) == 0;
            text(g, theme, juce::String(i + 1), {fx.getX() + 8, y - 9, 17, 18}, 10,
                 theme.text, false, juce::Justification::centred);
            g.setColour(audioInk.withAlpha(enabled ? .95f : .18f));
            g.fillEllipse(fx.getX() + 36, y - 2.6f, 5.2f, 5.2f);
        }
        if (nodes.hasProperty("output"))
        {
            node(out, "OUT");
            const auto c = out.getCentre().translated(0, 10);
            juce::Path speaker;
            speaker.startNewSubPath(c.x - 10, c.y - 4);
            speaker.lineTo(c.x - 5, c.y - 4);
            speaker.lineTo(c.x + 1, c.y - 10);
            speaker.lineTo(c.x + 1, c.y + 10);
            speaker.lineTo(c.x - 5, c.y + 4);
            speaker.lineTo(c.x - 10, c.y + 4);
            speaker.closeSubPath();
            speaker.addCentredArc(c.x, c.y, 8, 10, 0, .6f, 2.55f, true);
            g.setColour(theme.text);
            g.strokePath(speaker, juce::PathStrokeType(1.25f));
        }
        for (int i = 0; i < 3; ++i)
            pin({mix.getX(), mix.getY() + 30.f + i * 22.f}, audioInk,
                parameters.value("osc" + juce::String(i + 1) + ".mute") < .5f);
        for (const auto n : {mix, amp, fx})
            pin({n.getRight(), n.getCentreY()}, audioInk);
        pin({f1.getX(), f1.getCentreY()}, audioInk);
        pin({amp.getX(), amp.getCentreY()}, audioInk);
        pin({fx.getX(), fx.getCentreY()}, audioInk);
        if (nodes.hasProperty("output")) pin({out.getX(), out.getCentreY()}, audioInk);
        if (parallel)
        {
            pin({f2.getX(), f2.getCentreY()}, audioInk);
            g.setColour(theme.panel);
            g.fillEllipse(junction.x - 11, junction.y - 11, 22, 22);
            g.setColour(audioInk);
            g.drawEllipse(junction.x - 11, junction.y - 11, 22, 22, 1.2f);
            text(g, theme, mode == fc_ring ? utf8("×") : mode == fc_stereo ? "LR" : "+",
                 {junction.x - 9, junction.y - 9, 18, 18}, 16, theme.text, true,
                 juce::Justification::centred);
        }
        node(sources, {});
        text(g, theme, "MOD", sources.withTrimmedTop(26).withHeight(18), 13, theme.text, true,
             juce::Justification::centred);
        text(g, theme, "SOURCES", sources.withTrimmedTop(44).withHeight(18), 12, theme.text, true,
             juce::Justification::centred);
        juce::Path sourceIcon;
        sourceIcon.startNewSubPath(sources.getCentreX() - 10, sources.getY() + 20);
        sourceIcon.cubicTo(sources.getCentreX() - 3, sources.getY() + 6,
                           sources.getCentreX() + 3, sources.getY() + 30,
                           sources.getCentreX() + 10, sources.getY() + 17);
        stroke(sourceIcon, controlInk, false, true, false);
        node(modbus, {});
        const auto iconX = modbus.getX() + 28, iconY = modbus.getCentreY();
        g.setColour(controlInk);
        g.drawRect(iconX, iconY - 10, 15.f, 17.f, 1.f);
        for (const auto p : {juce::Point<float>(iconX, iconY - 10),
                            juce::Point<float>(iconX + 15, iconY - 10),
                            juce::Point<float>(iconX, iconY + 7),
                            juce::Point<float>(iconX + 15, iconY + 7)})
            g.fillRoundedRectangle(p.x - 2.5f, p.y - 2.5f, 5, 5, 1);
        text(g, theme, "MOD BUS & ROUTING",
             {modbus.getX() + 58, modbus.getY() + 12, modbus.getWidth() - 64, 20},
             12.5f, theme.text, true);
        int count = 0;
        for (const auto active : targetActive) count += active;
        text(g, theme, juce::String(count) + " active / " + juce::String(static_cast<int>(routes.size())) + " routes",
             {modbus.getX() + 58, modbus.getY() + 35, modbus.getWidth() - 64, 18},
             10.5f, theme.muted);
        const auto legend = Rect(out.getRight() - 148, modbus.getCentreY() - 6, 148, 67);
        node(legend, {});
        for (int i = 0; i < 2; ++i)
        {
            const auto y = legend.getY() + 14 + i * 19;
            juce::Path line;
            line.startNewSubPath(legend.getX() + 12, y);
            line.lineTo(legend.getX() + 41, y);
            stroke(line, i == 0 ? audioInk : controlInk, i == 1, true, false);
            text(g, theme, i == 0 ? "Audio (signal)" : "Modulation", {legend.getX() + 49, y - 8, 93, 16},
                 9.5f, theme.text);
        }
        pin({legend.getX() + 26, legend.getY() + 53}, controlInk, false);
        text(g, theme, "Module port", {legend.getX() + 49, legend.getY() + 45, 93, 16}, 9.5f,
             theme.text);
        text(g, theme, parameters.display("routing") + utf8(" • topology overview"),
             {plot.getX() + 8, plot.getY() + 8, 190, 14}, 9.5f, theme.muted);
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
    std::vector<juce::var> selectionDefinitions;
    juce::StringArray groupIds;
    if (auto *panels = candidate["panels"].getArray())
        for (const auto &panel : *panels)
            if (panel.hasProperty("selection"))
            {
                const auto definition = panel["selection"];
                const auto id = string(definition, "id");
                const auto kind = string(definition, "kind");
                const auto *choices = definition["choices"].getArray();
                const auto tabs = arrayRect(definition["tabsRect"], {});
                if (!definition.isObject() || id.isEmpty() || groupIds.contains(id) ||
                    (kind != "source" && kind != "fx") || choices == nullptr ||
                    choices->isEmpty() || choices->size() > 16 || tabs.isEmpty() ||
                    !bounds(panel).contains(tabs))
                {
                    error = "Invalid or duplicate panel selection: " + id;
                    return false;
                }
                juce::StringArray choiceIds;
                for (const auto &choice : *choices)
                {
                    const auto choiceId = string(choice, "id");
                    const auto source = string(choice, "source");
                    const auto index = number(choice, "index");
                    const int maximum = source == "fx"          ? 16
                                        : source == "envelope"   ? 2
                                        : source == "lfo" || source == "lfo-envelope" ? 12
                                        : source == "oscillator" || source == "waveform" ? 3
                                                                                           : 0;
                    if (!choice.isObject() || choiceId.isEmpty() || choiceIds.contains(choiceId) ||
                        string(choice, "label").isEmpty() || index < 1 || index > maximum ||
                        std::floor(index) != index || (kind == "fx" && source != "fx") ||
                        (kind == "source" && source == "fx"))
                    {
                        error = "Invalid source choice in panel selection: " + id;
                        return false;
                    }
                    choiceIds.add(choiceId);
                }
                if (!choiceIds.contains(string(definition, "default")))
                {
                    error = "Panel selection default does not name a choice: " + id;
                    return false;
                }
                groupIds.add(id);
                selectionDefinitions.push_back(definition);
            }
    if (auto *a = candidate["controls"].getArray())
        for (auto &c : *a)
        {
            if (!impl->parameters.valid(string(c, "binding")))
            {
                error = "Unknown synth binding: " + string(c, "binding");
                return false;
            }
            if (c.hasProperty("selectionGroup") || c.hasProperty("bindings"))
            {
                const auto group = string(c, "selectionGroup");
                const auto groupIndex = groupIds.indexOf(group);
                auto *mapping = c["bindings"].getDynamicObject();
                if (groupIndex < 0 || mapping == nullptr)
                {
                    error = "Control references an unknown panel selection: " + group;
                    return false;
                }
                const auto *choices = selectionDefinitions[groupIndex]["choices"].getArray();
                for (const auto &choice : *choices)
                {
                    const auto id = juce::Identifier(string(choice, "id"));
                    const auto binding = mapping->getProperty(id);
                    // A JSON null disables a genuinely unavailable operation;
                    // every other choice must resolve before any UI is exposed.
                    if (!mapping->hasProperty(id) ||
                        (!binding.isVoid() &&
                         (!binding.isString() || binding.toString().isEmpty() ||
                          !impl->parameters.valid(binding.toString()))))
                    {
                        error = "Unknown or missing selection binding for " + string(c, "id") +
                                ": " + id.toString();
                        return false;
                    }
                }
            }
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
bool Workspace::selectView(const juce::String &groupId, const juce::String &choiceId)
{
    return impl->selectView(groupId, choiceId);
}
juce::String Workspace::selectedView(const juce::String &groupId) const
{
    if (const auto *selected = impl->selection(groupId))
        return selected->selected;
    return {};
}
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
    if (i.theme.style == "precision")
    {
        // The exposed lower lip is part of the graphite enclosure, independent
        // of parameter and signal state. Keep the functional panels above it.
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff11171b), 793, 938,
                                               juce::Colour(0xff363b41), 793, 992, false));
        g.fillRect(0.f, 936.f, 1586.f, 56.f);
        g.setColour(juce::Colour(0xff080d10));
        g.fillRoundedRectangle({.5f, 31.f, 1585.f, 921.f}, 15.f);
        g.setColour(juce::Colour(0xff5f686e).withAlpha(.55f));
        g.drawRoundedRectangle({.5f, 31.f, 1585.f, 921.f}, 15.f, .8f);
    }
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
