/* Native decorative engraving for reference skins. GPL-3.0-or-later.
   These bounded vector ornaments represent no parameter, meter or signal. */
#ifndef SURGE_REFERENCE_ORNAMENTS_H
#define SURGE_REFERENCE_ORNAMENTS_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace Surge::GUI::Reference
{
enum class ClearSectionIdentity
{
    none,
    oscillators,
    filters,
    macros,
    modulation,
    inspector,
    mixer,
    envelope,
    lfo,
    effects,
    analyzer,
    output
};

/** Small caption pictograms, drawn from fixed vector geometry. These identify
    sections only: they have no control, selection, routing or signal input. */
inline void clearSectionIdentityGlyph(juce::Graphics &g, juce::Rectangle<float> area,
                                      juce::Colour ink, ClearSectionIdentity identity)
{
    if (area.isEmpty() || identity == ClearSectionIdentity::none)
        return;
    const juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(area.toNearestInt());
    const auto size = std::min(area.getWidth(), area.getHeight());
    const auto square = juce::Rectangle<float>(size, size).withCentre(area.getCentre());
    g.addTransform(juce::AffineTransform::scale(size / 16.f).translated(square.getX(),
                                                                       square.getY()));
    g.setColour(ink);
    juce::Path path;
    switch (identity)
    {
    case ClearSectionIdentity::oscillators:
        path.addRoundedRectangle(juce::Rectangle<float>(2, 2, 12, 12), 1.1f);
        path.startNewSubPath(4, 8);
        path.lineTo(6, 5);
        path.lineTo(9, 11);
        path.lineTo(11, 8);
        path.lineTo(12, 8);
        break;
    case ClearSectionIdentity::filters:
        path.startNewSubPath(2, 3);
        path.lineTo(14, 3);
        path.lineTo(9, 8);
        path.lineTo(9, 12);
        path.lineTo(7, 14);
        path.lineTo(7, 8);
        path.closeSubPath();
        break;
    case ClearSectionIdentity::macros:
        for (int column = 0; column < 3; ++column)
            for (int row = 0; row < 3; ++row)
                g.fillEllipse(1.6f + column * 5.f, 1.6f + row * 5.f, 2.8f, 2.8f);
        break;
    case ClearSectionIdentity::modulation:
        path.startNewSubPath(2, 13);
        path.lineTo(2, 8);
        path.lineTo(5, 8);
        path.lineTo(5, 13);
        path.lineTo(8, 13);
        path.lineTo(8, 3);
        path.lineTo(11, 3);
        path.lineTo(11, 13);
        path.lineTo(14, 13);
        path.lineTo(14, 6);
        break;
    case ClearSectionIdentity::inspector:
        path.addEllipse(1.5f, 1.5f, 9.f, 9.f);
        path.startNewSubPath(9.5f, 9.5f);
        path.lineTo(14, 14);
        path.startNewSubPath(4, 6);
        path.lineTo(8, 6);
        break;
    case ClearSectionIdentity::mixer:
        for (int column = 0; column < 3; ++column)
        {
            const auto x = 3.f + column * 5.f;
            path.startNewSubPath(x, 2);
            path.lineTo(x, 14);
            g.fillRoundedRectangle(juce::Rectangle<float>(x - 2, column == 1 ? 4.f : 9.f,
                                                          4, 2.5f), .6f);
        }
        break;
    case ClearSectionIdentity::envelope:
        path.addRoundedRectangle(juce::Rectangle<float>(1.5f, 2, 13, 12), 1.f);
        path.startNewSubPath(3, 11);
        path.lineTo(5.5f, 5);
        path.lineTo(9, 8);
        path.lineTo(11, 8);
        path.lineTo(13, 11);
        break;
    case ClearSectionIdentity::lfo:
        path.startNewSubPath(1.5f, 8);
        path.cubicTo(4, 1, 5.5f, 1, 8, 8);
        path.cubicTo(10.5f, 15, 12, 15, 14.5f, 8);
        break;
    case ClearSectionIdentity::effects:
        for (int corner = 0; corner < 32; ++corner)
        {
            const auto angle = juce::MathConstants<float>::twoPi * corner / 32.f;
            const auto radius = corner % 4 == 0 || corner % 4 == 3 ? 5.2f : 6.7f;
            const juce::Point<float> point(8.f + std::sin(angle) * radius,
                                           8.f - std::cos(angle) * radius);
            if (corner == 0)
                path.startNewSubPath(point);
            else
                path.lineTo(point);
        }
        path.closeSubPath();
        path.addEllipse(5.6f, 5.6f, 4.8f, 4.8f);
        break;
    case ClearSectionIdentity::analyzer:
        path.addRectangle(juce::Rectangle<float>(2, 9, 2.5f, 5));
        path.addRectangle(juce::Rectangle<float>(6.75f, 5.5f, 2.5f, 8.5f));
        path.addRectangle(juce::Rectangle<float>(11.5f, 2, 2.5f, 12));
        break;
    case ClearSectionIdentity::output:
        path.startNewSubPath(1.5f, 6);
        path.lineTo(4, 6);
        path.lineTo(8, 3);
        path.lineTo(8, 13);
        path.lineTo(4, 10);
        path.lineTo(1.5f, 10);
        path.closeSubPath();
        path.addCentredArc(6.5f, 8, 5, 5, 0, .75f, 2.4f, true);
        path.addCentredArc(6.5f, 8, 7.2f, 7.2f, 0, .75f, 2.4f, true);
        break;
    case ClearSectionIdentity::none:
        break;
    }
    g.strokePath(path, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

/** Static brand artwork: folded violet ribbons or an ascending aurora fan.
    These hand-built curves have no signal, parameter, time or random input. */
inline void luminousRibbonLogo(juce::Graphics &g, juce::Rectangle<float> area,
                               juce::Colour violet, juce::Colour cyan, bool ascending)
{
    if (area.isEmpty())
        return;
    juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(area.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(area.getWidth() / 100.f,
                                               area.getHeight() / 60.f)
                       .translated(area.getX(), area.getY()));
    const auto pink = juce::Colour(0xffef91ec);
    juce::Path strands[5];
    if (ascending)
    {
        // Independent bends make the strands cross instead of repeating one
        // translated waveform. Both tips taper into the same small fan.
        static constexpr float curves[5][12] = {
            {26, 4, 34, 10, 54, 19, 76, 27, 86, 13, 97, 3},
            {23, 17, 38, 29, 56, 20, 76, 13, 87, 16, 97, 3},
            {24, 29, 37, 10, 57, 29, 76, 43, 86, 20, 97, 3},
            {23, 20, 34, 35, 57, 37, 76, 55, 86, 23, 97, 3},
            {24, 22, 33, 24, 56, 43, 75, 63, 84, 23, 97, 3}};
        for (int strand = 0; strand < 5; ++strand)
        {
            const auto &c = curves[strand];
            strands[strand].startNewSubPath(3, 56);
            strands[strand].cubicTo(c[0], c[1], c[2], c[3], c[4], c[5]);
            strands[strand].cubicTo(c[6], c[7], c[8], c[9], c[10], c[11]);
        }
        auto veil = strands[0];
        veil.cubicTo(84, 23, 75, 63, 56, 43);
        veil.cubicTo(33, 24, 24, 22, 3, 56);
        veil.closeSubPath();
        g.setGradientFill(juce::ColourGradient(pink.withAlpha(.16f), 5, 44,
                                              cyan.withAlpha(.08f), 94, 13, false));
        g.fillPath(veil);
    }
    else
    {
        static constexpr float curves[5][18] = {
            {20, 49, 31, -3, 46, 9, 59, 18, 54, 57, 72, 43, 83, 35, 86, 24, 97, 34},
            {20, 49, 27, 16, 40, 23, 53, 22, 55, 53, 72, 43, 84, 32, 88, 28, 97, 34},
            {18, 44, 28, 24, 40, 23, 47, 33, 48, 72, 67, 56, 80, 48, 83, 29, 97, 34},
            {21, 47, 28, 30, 40, 30, 53, 29, 61, 44, 73, 43, 84, 42, 88, 29, 97, 34},
            {21, 43, 29, 34, 41, 35, 54, 36, 61, 40, 74, 40, 85, 39, 89, 29, 97, 34}};
        for (int strand = 0; strand < 5; ++strand)
        {
            const auto &c = curves[strand];
            strands[strand].startNewSubPath(3, 37);
            for (int bend = 0; bend < 3; ++bend)
                strands[strand].cubicTo(c[bend * 6], c[bend * 6 + 1], c[bend * 6 + 2],
                                        c[bend * 6 + 3], c[bend * 6 + 4], c[bend * 6 + 5]);
        }
        auto fold = strands[0];
        fold.cubicTo(83, 29, 80, 48, 67, 56);
        fold.cubicTo(48, 72, 47, 33, 40, 23);
        fold.cubicTo(28, 24, 18, 44, 3, 37);
        fold.closeSubPath();
        g.setGradientFill(juce::ColourGradient(violet.brighter(.2f).withAlpha(.72f), 45, 9,
                                              violet.darker(.2f).withAlpha(.42f), 64, 56, false));
        g.fillPath(fold);
    }
    for (int strand = 0; strand < 5; ++strand)
    {
        const auto &path = strands[strand];
        g.setColour((ascending ? pink : violet).withAlpha(.10f));
        g.strokePath(path, juce::PathStrokeType(5.f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        auto colour = ascending ? pink : violet.interpolatedWith(pink, .35f);
        const auto tip = ascending ? cyan.interpolatedWith(juce::Colours::white, .35f)
                                   : violet.interpolatedWith(juce::Colours::white, .55f);
        g.setGradientFill(juce::ColourGradient(colour.withAlpha(strand == 0 ? .98f : .72f),
                                              4, 39, tip.withAlpha(.88f), 96, 17, false));
        g.strokePath(path, juce::PathStrokeType(strand == 0 || strand == 2 ? 1.6f : .95f,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }
}

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
