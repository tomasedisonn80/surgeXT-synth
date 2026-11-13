/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_MATRIX_LAYOUT_H
#define SURGE_REFERENCE_MATRIX_LAYOUT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>

namespace Surge::GUI::Reference
{
/** The matrix artwork and its native row/depth components share these bounds.

    Coordinates are measured in the reference's logical panel, then scaled to
    that panel's actual width. Source/destination/curve/range are read-only
    fields; only slider is the native depth control's hit rectangle. Its value
    caption is deliberately separate from the track and native amount tooltip.
*/
struct MatrixLayout
{
    using Rect = juce::Rectangle<float>;
    struct Cells
    {
        Rect row, number, source, arrow, destination, amount, slider, value, curve, polarity;
    };
    enum class ValuePosition
    {
        left,
        right,
        above
    };

    Rect panel;
    int columns{1}, rowsPerColumn{6};
    float columnGap{0}, tableTop{35}, headerHeight{26}, rowHeight{32};
    float referenceWidth{523}, rowFont{13.5f}, headerFont{10.5f};
    float valueWidth{39}, trackHeight{8};
    ValuePosition valuePosition{ValuePosition::right};
    // Number, source, arrow, destination, amount, linear curve, source range.
    std::array<std::array<float, 2>, 7> spans{
        {{{9, 27}}, {{35, 155}}, {{0, 0}}, {{302, 435}}, {{165, 292}}, {{445, 481}}, {{0, 0}}}};

    static MatrixLayout make(Rect panel, const juce::String &style,
                             const juce::String &variant, float titleHeight = 24)
    {
        MatrixLayout layout;
        layout.panel = panel;
        if (style == "ivory")
        {
            layout.referenceWidth = 541;
            layout.tableTop = 36;
            layout.headerHeight = 30;
            layout.rowHeight = 34;
            layout.rowFont = 14;
            layout.headerFont = 11;
            layout.valuePosition = ValuePosition::above;
            layout.trackHeight = 5;
            layout.spans = {{{{16, 31}}, {{41, 142}}, {{248, 271}}, {{278, 432}},
                             {{154, 243}}, {{449, 484}}, {{0, 0}}}};
        }
        else if (style == "spectral")
        {
            layout.referenceWidth = 658;
            layout.tableTop = 36;
            layout.headerHeight = 28;
            layout.rowsPerColumn = 5;
            layout.rowHeight = 30.5f;
            layout.rowFont = 12.8f;
            layout.headerFont = 11.5f;
            layout.valuePosition = ValuePosition::left;
            layout.valueWidth = 41;
            layout.trackHeight = 13;
            layout.spans = {{{{13, 30}}, {{44, 154}}, {{158, 182}}, {{188, 321}},
                             {{324, 446}}, {{455, 493}}, {{513, 604}}}};
        }
        else if (style == "bauhaus")
        {
            layout.referenceWidth = 524;
            layout.tableTop = 40;
            layout.headerHeight = 27;
            layout.rowHeight = 27.5f;
            layout.rowFont = 13.5f;
            layout.headerFont = 12;
            layout.valuePosition = ValuePosition::left;
            layout.valueWidth = 44;
            layout.trackHeight = 10;
            layout.spans = {{{{15, 34}}, {{48, 149}}, {{0, 0}}, {{326, 461}},
                             {{158, 312}}, {{472, 511}}, {{0, 0}}}};
        }
        else if (style == "space")
        {
            layout.referenceWidth = 495;
            layout.tableTop = 42;
            layout.headerHeight = 23;
            layout.rowsPerColumn = 8;
            layout.rowHeight = 27;
            layout.rowFont = 13;
            layout.headerFont = 11.5f;
            layout.valueWidth = 44;
            layout.trackHeight = 9;
            layout.spans = {{{{17, 36}}, {{45, 145}}, {{0, 0}}, {{300, 443}},
                             {{157, 288}}, {{0, 0}}, {{456, 480}}}};
        }
        else if (style == "tokyo" && variant == "horizontal")
        {
            layout.columns = 2;
            layout.columnGap = 18;
            layout.referenceWidth = 333.5f;
            layout.tableTop = 35;
            layout.headerHeight = 25;
            layout.rowsPerColumn = 4;
            layout.rowHeight = 27;
            layout.rowFont = 12.5f;
            layout.headerFont = 11.5f;
            layout.valuePosition = ValuePosition::above;
            layout.trackHeight = 2;
            layout.spans = {{{{12, 30}}, {{39, 117}}, {{163, 180}}, {{184, 293}},
                             {{122, 161}}, {{306, 328}}, {{0, 0}}}};
        }
        else if (style == "aurora")
        {
            layout.referenceWidth = 599;
            layout.tableTop = 34;
            layout.headerHeight = 22;
            layout.rowHeight = 23;
            layout.rowFont = 12.5f;
            layout.headerFont = 11.5f;
            layout.valueWidth = 44;
            layout.trackHeight = 4;
            layout.spans = {{{{12, 29}}, {{45, 138}}, {{0, 0}}, {{308, 465}},
                             {{150, 292}}, {{491, 524}}, {{552, 574}}}};
        }
        else if (style == "clear")
        {
            layout.referenceWidth = 391;
            layout.tableTop = 39;
            layout.headerHeight = 29;
            layout.rowsPerColumn = 8;
            layout.rowHeight = 27.5f;
            layout.rowFont = 14;
            layout.headerFont = 12.5f;
            layout.valuePosition = ValuePosition::left;
            layout.valueWidth = 43;
            layout.trackHeight = 9;
            layout.spans = {{{{9, 30}}, {{38, 116}}, {{121, 146}}, {{152, 257}},
                             {{268, 375}}, {{0, 0}}, {{0, 0}}}};
        }
        else if (style == "flow")
        {
            layout.referenceWidth = 708;
            layout.tableTop = 33;
            layout.headerHeight = 24;
            layout.rowHeight = 28;
            layout.rowFont = 13;
            layout.headerFont = 11.5f;
            layout.valuePosition = ValuePosition::left;
            layout.valueWidth = 47;
            layout.trackHeight = 4;
            layout.spans = {{{{12, 34}}, {{51, 173}}, {{0, 0}}, {{378, 537}},
                             {{191, 361}}, {{555, 589}}, {{612, 685}}}};
        }
        else if (style == "live")
        {
            layout.referenceWidth = 443;
            layout.tableTop = 28;
            layout.headerHeight = 21;
            layout.rowsPerColumn = 5;
            layout.rowHeight = 19.8f;
            layout.rowFont = 11.5f;
            layout.headerFont = 11;
            layout.valuePosition = ValuePosition::left;
            layout.valueWidth = 39;
            layout.trackHeight = 8;
            layout.spans = {{{{12, 33}}, {{52, 154}}, {{157, 176}}, {{183, 292}},
                             {{307, 416}}, {{0, 0}}, {{0, 0}}}};
        }
        layout.tableTop += std::max(0.f, titleHeight - 24.f);
        layout.rowHeight = std::min(layout.rowHeight,
                                    std::max(0.f, panel.getHeight() - layout.tableTop -
                                                      layout.headerHeight - 1.f) /
                                        static_cast<float>(layout.rowsPerColumn));
        return layout;
    }

    Rect column(int index) const
    {
        const auto width = (panel.getWidth() - columnGap * (columns - 1)) / columns;
        return {panel.getX() + index * (width + columnGap), panel.getY(), width,
                panel.getHeight()};
    }
    Cells header(int columnIndex) const
    {
        const auto area = column(columnIndex);
        return cells({area.getX(), panel.getY() + tableTop, area.getWidth(), headerHeight});
    }
    Cells row(int index) const
    {
        const auto area = column(index / rowsPerColumn);
        return cells({area.getX(), panel.getY() + tableTop + headerHeight +
                                      (index % rowsPerColumn) * rowHeight,
                      area.getWidth(), rowHeight});
    }
    Rect body() const
    {
        return {panel.getX() + 8, panel.getY() + tableTop, panel.getWidth() - 16,
                headerHeight + rowsPerColumn * rowHeight};
    }

  private:
    Cells cells(Rect row) const
    {
        const auto scale = row.getWidth() / referenceWidth;
        auto span = [&](size_t index) {
            const auto &edge = spans[index];
            return Rect(row.getX() + edge[0] * scale, row.getY(),
                        (edge[1] - edge[0]) * scale, row.getHeight());
        };
        Cells result{row.reduced(8, 0), span(0), span(1), span(2), span(3), span(4),
                     {}, {}, span(5), span(6)};
        auto amount = result.amount;
        if (valuePosition == ValuePosition::above)
        {
            result.value = amount.removeFromTop(std::max(12.f, amount.getHeight() * .52f));
            result.slider = amount;
        }
        else if (valuePosition == ValuePosition::left)
        {
            result.value = amount.removeFromLeft(valueWidth * scale);
            result.slider = amount;
        }
        else
        {
            result.value = amount.removeFromRight(valueWidth * scale);
            result.slider = amount;
        }
        return result;
    }
};
} // namespace Surge::GUI::Reference
#endif
