/* Native decorative engraving for reference skins. GPL-3.0-or-later.
   These bounded vector ornaments represent no parameter, meter or signal. */
#ifndef SURGE_REFERENCE_ORNAMENTS_H
#define SURGE_REFERENCE_ORNAMENTS_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace Surge::GUI::Reference
{
/** Algorithm-family pictogram. This is a labelled category icon, not a signal plot. */
inline void effectGlyph(juce::Graphics &g, juce::Rectangle<float> area,
                        juce::Colour ink, const juce::String &algorithm)
{
    if (area.isEmpty())
        return;
    juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(area.toNearestInt());
    const auto size = std::min(area.getWidth(), area.getHeight());
    const auto square = juce::Rectangle<float>(size, size).withCentre(area.getCentre());
    g.addTransform(juce::AffineTransform::scale(size / 40.f).translated(square.getX(),
                                                                 square.getY()));
    g.setColour(ink);
    const auto name = algorithm.toLowerCase();
    juce::Path path;
    if (name.contains("delay") || name.contains("echo"))
    {
        for (int i = 0; i < 3; ++i)
            path.addCentredArc(9, 20, 8.f + i * 8.f, 7.f + i * 6.f, 0,
                               .45f, 2.7f, true);
        g.fillEllipse(6, 18, 3, 3);
    }
    else if (name.contains("reverb"))
    {
        for (int i = 0; i < 19; ++i)
        {
            const float a = i * 2.399963f;
            const float radius = 3.f + std::sqrt(static_cast<float>(i)) * 3.1f;
            g.fillEllipse(19 + std::cos(a) * radius, 19 + std::sin(a) * radius, 2, 2);
        }
    }
    else if (name.contains("phaser"))
    {
        for (int i = 0; i < 3; ++i)
            path.addEllipse(5.f + i * 9.f, 9, 12, 22);
    }
    else if (name.contains("distort") || name.contains("waveshap") || name.contains("saturat"))
    {
        // A categorical saturation mark only. Live Distortion detail panes use
        // the separately evaluated native transfer, never this decorative icon.
        path.startNewSubPath(3, 31);
        path.cubicTo(13, 31, 14, 30, 20, 20);
        path.cubicTo(26, 10, 27, 9, 37, 9);
    }
    else if (name.contains("eq") || name.contains("compress"))
    {
        const bool compression = name.contains("compress");
        const juce::Point<float> points[] = {{4, 31}, {15, compression ? 21.f : 14.f},
                                             {25, compression ? 14.f : 21.f}, {36, 8}};
        path.startNewSubPath(points[0]);
        for (int i = 1; i < 4; ++i)
            path.lineTo(points[i]);
        for (auto point : points)
        {
            g.drawEllipse(point.x - 1.5f, point.y - 1.5f, 3, 3, 1);
        }
    }
    else if (name.contains("limit") || name.contains("vocod"))
    {
        for (int i = 0; i < 7; ++i)
            g.fillRect(5.f + i * 4.5f, 29.f - i * 3.f, 2.f, 4.f + i * 3.f);
    }
    else
    {
        const int lines = name.contains("chorus") || name.contains("ensemble") ? 3 : 1;
        for (int line = 0; line < lines; ++line)
        {
            const float centre = 20.f + (line - (lines - 1) * .5f) * 9.f;
            path.startNewSubPath(2, centre);
            for (int x = 1; x <= 36; ++x)
                path.lineTo(x + 2.f, centre - std::sin(x * .31f) * (lines == 1 ? 8.f : 3.f));
        }
    }
    g.strokePath(path, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));
}

inline void botanicalEngraving(juce::Graphics &g, juce::Rectangle<float> area,
                              juce::Colour ink, bool mirror = false)
{
    if (area.isEmpty())
        return;
    juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(area.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(area.getWidth() / 120.f,
                                               area.getHeight() / 100.f)
                       .translated(area.getX(), area.getY()));
    if (mirror)
        g.addTransform(juce::AffineTransform(-1, 0, 120, 0, 1, 0));
    auto stroke = [&](juce::Path &path, float width, float alpha) {
        g.setColour(ink.withMultipliedAlpha(alpha));
        g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    };
    // A hand-built fan of tapering stems and paired engraved leaves. There is
    // no random or clock input, so idle repainting cannot alter the artwork.
    for (int branch = 0; branch < 6; ++branch)
    {
        const float angle = -2.95f + branch * .285f;
        const float length = 58.f + (branch % 3) * 18.f;
        const juce::Point<float> origin(117, 98);
        const juce::Point<float> end(origin.x + std::cos(angle) * length,
                                     origin.y + std::sin(angle) * length);
        const auto delta = end - origin;
        juce::Path stem;
        stem.startNewSubPath(origin);
        stem.cubicTo(origin.x + delta.x * .25f, origin.y + delta.y * .13f,
                     origin.x + delta.x * .70f, origin.y + delta.y * .78f, end.x, end.y);
        stroke(stem, .8f, .72f);
        for (int leaf = 1; leaf <= 7; ++leaf)
        {
            const float t = leaf / 8.f;
            const auto centre = origin + delta * t;
            for (int side : {-1, 1})
            {
                const float leafAngle = angle + side * (.55f + .10f * (leaf % 2));
                const float size = 8.f + (1.f - t) * 5.f;
                const juce::Point<float> tip(centre.x + std::cos(leafAngle) * size,
                                             centre.y + std::sin(leafAngle) * size);
                const auto normal = juce::Point<float>(-(tip.y - centre.y), tip.x - centre.x) * .17f;
                const auto mid = (centre + tip) * .5f;
                juce::Path shape;
                shape.startNewSubPath(centre);
                shape.quadraticTo(mid + normal, tip);
                shape.quadraticTo(mid - normal, centre);
                stroke(shape, .65f, .62f);
                g.setColour(ink.withMultipliedAlpha(.4f));
                g.drawLine({centre, tip}, .45f);
            }
        }
    }
}

inline void observatoryEmblem(juce::Graphics &g, juce::Rectangle<float> area,
                              juce::Colour violet, juce::Colour cyan)
{
    if (area.isEmpty())
        return;
    juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(area.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(area.getWidth() / 170.f,
                                               area.getHeight() / 72.f)
                       .translated(area.getX(), area.getY()));
    g.setGradientFill(juce::ColourGradient(violet.withAlpha(.45f), 82, 52,
                                          violet.withAlpha(0.f), 82, -5, true));
    g.fillRect(0.f, 0.f, 170.f, 72.f);
    for (int star = 0; star < 38; ++star)
    {
        const auto n = static_cast<unsigned>(star * 73471 + 9137);
        const float x = 2.f + n % 164, y = 3.f + (n / 43) % 46;
        g.setColour(cyan.interpolatedWith(juce::Colours::white, .7f)
                        .withAlpha(.12f + (star % 5) * .07f));
        g.fillEllipse(x, y, star % 9 == 0 ? 1.5f : .7f, star % 9 == 0 ? 1.5f : .7f);
    }
    for (int layer = 0; layer < 3; ++layer)
    {
        juce::Path mountain;
        mountain.startNewSubPath(0, 72);
        for (int point = 0; point <= 18; ++point)
        {
            const float x = point * 170.f / 18;
            const float y = 55 + layer * 4 + std::sin(point * 2.3f + layer) * (8 - layer * 2);
            mountain.lineTo(x, y);
        }
        mountain.lineTo(170, 72);
        mountain.closeSubPath();
        g.setColour(juce::Colour(0xff090b20).interpolatedWith(violet, .24f - layer * .07f));
        g.fillPath(mountain);
        g.setColour(violet.withAlpha(.4f - layer * .08f));
        g.strokePath(mountain, juce::PathStrokeType(.65f));
    }
    juce::Path dome;
    dome.startNewSubPath(53, 44);
    dome.cubicTo(53, 1, 113, 1, 113, 44);
    dome.closeSubPath();
    g.setGradientFill(juce::ColourGradient(violet.brighter(.12f), 51, 30,
                                          juce::Colour(0xff080b1e), 99, 31, false));
    g.fillPath(dome);
    g.setColour(violet.brighter(.6f).withAlpha(.85f));
    g.strokePath(dome, juce::PathStrokeType(1.2f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1d1642), 54, 43,
                                          juce::Colour(0xff090b1e), 54, 66, false));
    g.fillRect(54.f, 43.f, 58.f, 22.f);
    g.setColour(violet.withAlpha(.6f));
    g.drawRect(54.f, 43.f, 58.f, 22.f, .8f);
    for (float x : {65.f, 80.f, 96.f, 106.f})
    {
        juce::Path seam;
        seam.startNewSubPath(x, 43);
        seam.quadraticTo(80 + (x - 80) * .35f, 7, 83, 9);
        g.setColour(juce::Colour(0xff030612).withAlpha(.9f));
        g.strokePath(seam, juce::PathStrokeType(2));
        g.setColour(violet.withAlpha(.5f));
        g.strokePath(seam, juce::PathStrokeType(.55f));
    }
    g.setColour(juce::Colour(0xff07091a));
    g.fillRect(77.f, 49.f, 14.f, 17.f);
    g.setColour(violet.withAlpha(.65f));
    g.drawLine(77, 50, 77, 66, .6f);
    g.drawLine(54, 48, 112, 48, .6f);
}
} // namespace Surge::GUI::Reference
#endif
