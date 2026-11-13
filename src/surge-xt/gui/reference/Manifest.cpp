/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Copyright 2026, contributors, as described in the GitHub transaction log.
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 */
#include "Manifest.h"

#include <cmath>
#include <set>

namespace Surge::GUI::Reference
{
namespace
{
constexpr int maxBytes = 1024 * 1024;
constexpr int maxDepth = 12;

bool fail(juce::String &error, const juce::String &message)
{
    error = message;
    return false;
}

bool number(const juce::var &v)
{
    return (v.isInt() || v.isInt64() || v.isDouble()) && std::isfinite(static_cast<double>(v));
}

bool identifier(const juce::String &value)
{
    return value.isNotEmpty() && value.length() <= 80 &&
           value.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-");
}

bool colour(const juce::String &value)
{
    return (value.length() == 7 || value.length() == 9) && value.startsWithChar('#') &&
           value.substring(1).containsOnly("0123456789abcdefABCDEF");
}

bool oneOf(const juce::String &value, const char *options)
{
    return juce::StringArray::fromTokens(options, "|", "").contains(value);
}

bool stringField(const juce::DynamicObject &object, const char *key, int length,
                 const juce::String &where, juce::String &error, bool allowEmpty = false)
{
    const auto &v = object.getProperty(key);
    if (!v.isString() || v.toString().length() > length ||
        (!allowEmpty && v.toString().trim().isEmpty()))
        return fail(error, where + "." + key + " must be " +
                               (allowEmpty ? "a string" : "a nonempty string") + " of at most " +
                               juce::String(length) + " characters");
    return true;
}

bool boundedNumber(const juce::DynamicObject &object, const char *key, double lo, double hi,
                   const juce::String &where, juce::String &error)
{
    const auto &v = object.getProperty(key);
    if (!number(v) || static_cast<double>(v) < lo || static_cast<double>(v) > hi)
        return fail(error, where + "." + key + " must be a finite number in [" + juce::String(lo) +
                               ", " + juce::String(hi) + "]");
    return true;
}

// Keep unknown extension data bounded as well as the fields the renderer uses.
bool boundedTree(const juce::var &v, int depth, juce::String &error)
{
    if (depth > maxDepth)
        return fail(error, "Manifest nesting exceeds 12 levels");
    if (auto *object = v.getDynamicObject())
    {
        const auto &properties = object->getProperties();
        if (properties.size() > 128)
            return fail(error, "A manifest object exceeds 128 properties");
        for (int i = 0; i < properties.size(); ++i)
        {
            if (properties.getName(i).toString().length() > 80)
                return fail(error, "A manifest property name exceeds 80 characters");
            if (!boundedTree(properties.getValueAt(i), depth + 1, error))
                return false;
        }
        return true;
    }
    if (auto *array = v.getArray())
    {
        if (array->size() > 512)
            return fail(error, "A manifest array exceeds 512 items");
        for (const auto &element : *array)
            if (!boundedTree(element, depth + 1, error))
                return false;
        return true;
    }
    if (v.isString())
        return v.toString().length() <= 4096 ||
               fail(error, "A manifest string exceeds 4096 characters");
    if (v.isInt() || v.isInt64() || v.isDouble())
        return number(v) || fail(error, "Manifest numbers must be finite");
    return v.isBool() || v.isVoid() || fail(error, "Unsupported manifest value type");
}

bool rectangle(const juce::DynamicObject &object, const juce::String &where, juce::String &error)
{
    if (!boundedNumber(object, "x", 0, 1586, where, error) ||
        !boundedNumber(object, "y", 0, 992, where, error) ||
        !boundedNumber(object, "w", 1, 1586, where, error) ||
        !boundedNumber(object, "h", 1, 992, where, error))
        return false;
    if (static_cast<double>(object.getProperty("x")) +
                static_cast<double>(object.getProperty("w")) >
            1586.001 ||
        static_cast<double>(object.getProperty("y")) +
                static_cast<double>(object.getProperty("h")) >
            992.001)
        return fail(error, where + " extends outside the 1586 x 992 logical canvas");
    return true;
}

bool plotRectangle(const juce::var &v, const juce::String &where, juce::String &error)
{
    const auto *values = v.getArray();
    if (values == nullptr || values->size() != 4)
        return fail(error, where + " must be [x, y, width, height]");
    for (const auto &value : *values)
        if (!number(value))
            return fail(error, where + " must contain finite numbers");
    const auto x = static_cast<double>(values->getReference(0));
    const auto y = static_cast<double>(values->getReference(1));
    const auto w = static_cast<double>(values->getReference(2));
    const auto h = static_cast<double>(values->getReference(3));
    return (x >= 0 && y >= 0 && w >= 1 && h >= 1 && x + w <= 1586.001 && y + h <= 992.001) ||
           fail(error, where + " must be positive and inside the 1586 x 992 canvas");
}

// Bound JSON nesting before invoking the recursive parser. Also reject a second
// root/trailing text: JUCE's parse API otherwise accepts a completed first root.
bool boundedJSON(const juce::String &text, juce::String &error)
{
    juce::String stack;
    bool quoted = false, escaped = false, started = false, ended = false;
    for (auto c : text)
    {
        if (quoted)
        {
            if (escaped)
                escaped = false;
            else if (c == '\\')
                escaped = true;
            else if (c == '"')
                quoted = false;
            continue;
        }
        if (juce::CharacterFunctions::isWhitespace(c))
            continue;
        if (ended)
            return fail(error, "Unexpected text after the manifest JSON object");
        if (!started && c != '{')
            return fail(error, "Manifest must be a JSON object");
        started = true;
        if (c == '"')
            quoted = true;
        else if (c == '{' || c == '[')
        {
            stack += juce::String::charToString(c);
            if (stack.length() > maxDepth)
                return fail(error, "Manifest JSON nesting exceeds 12 levels");
        }
        else if (c == '}' || c == ']')
        {
            if (stack.isEmpty() || (c == '}' && !stack.endsWithChar('{')) ||
                (c == ']' && !stack.endsWithChar('[')))
                return fail(error, "Unbalanced manifest JSON delimiters");
            stack = stack.dropLastCharacters(1);
            ended = stack.isEmpty();
        }
    }
    return (started && ended && !quoted) || fail(error, "Incomplete manifest JSON object");
}
} // namespace

bool validateManifest(const juce::var &data, juce::String &error)
{
    error.clear();
    if (!boundedTree(data, 0, error))
        return false;
    auto *object = data.getDynamicObject();
    if (object == nullptr)
        return fail(error, "Manifest must be an object");
    if (!number(object->getProperty("schema")) ||
        static_cast<double>(object->getProperty("schema")) != 1.0)
        return fail(error, "Manifest schema must be 1");
    for (const auto *field : {"id", "name", "reference", "style", "font"})
        if (!stringField(*object, field, 256, "manifest", error))
            return false;
    if (!identifier(object->getProperty("id").toString()))
        return fail(error, "Manifest id must be a short alphanumeric identifier");
    if (!oneOf(object->getProperty("style").toString(),
               "precision|ivory|spectral|bauhaus|space|tokyo|aurora|clear|flow|live"))
        return fail(error, "Unknown workspace style");
    if (!oneOf(object->getProperty("font").toString(), "sans|serif|mono"))
        return fail(error, "Unknown workspace font family");
    if (!boundedNumber(*object, "width", 1586, 1586, "manifest", error) ||
        !boundedNumber(*object, "height", 992, 992, "manifest", error))
        return false;
    if (object->hasProperty("headerHeight") &&
        !boundedNumber(*object, "headerHeight", 32, 120, "manifest", error))
        return false;
    if (object->hasProperty("radius") &&
        !boundedNumber(*object, "radius", 0, 48, "manifest", error))
        return false;
    if (object->hasProperty("glow") && !object->getProperty("glow").isBool() &&
        !boundedNumber(*object, "glow", 0, 32, "manifest", error))
        return false;

    const auto &palette = object->getProperty("palette");
    auto *paletteObject = palette.getDynamicObject();
    if (paletteObject == nullptr)
        return fail(error, "Manifest palette must be an object");
    for (const auto *key : {"background", "panel", "border", "text", "muted", "accent", "secondary",
                            "display", "knob", "red", "yellow", "blue"})
        if (!paletteObject->getProperty(key).isString() ||
            !colour(paletteObject->getProperty(key).toString()))
            return fail(error, juce::String("palette.") + key + " must be #RRGGBB or #RRGGBBAA");
    const auto &paletteProperties = paletteObject->getProperties();
    for (int i = 0; i < paletteProperties.size(); ++i)
        if (!paletteProperties.getValueAt(i).isString() ||
            !colour(paletteProperties.getValueAt(i).toString()))
            return fail(error, "Every palette entry must be a hex colour");

    std::set<juce::String> ids;
    for (const auto *collection : {"panels", "controls"})
    {
        const auto &value = object->getProperty(collection);
        auto *items = value.getArray();
        const bool panels = juce::String(collection) == "panels";
        if (items == nullptr || items->isEmpty() || items->size() > (panels ? 80 : 256))
            return fail(error, juce::String(collection) + " must contain 1 to " +
                                   juce::String(panels ? 80 : 256) + " objects");
        for (int i = 0; i < items->size(); ++i)
        {
            const auto where = juce::String(collection) + "[" + juce::String(i) + "]";
            auto *item = items->getReference(i).getDynamicObject();
            if (item == nullptr)
                return fail(error, where + " must be an object");
            if (!stringField(*item, "id", 80, where, error) ||
                !identifier(item->getProperty("id").toString()))
                return fail(error, where + ".id must be a short alphanumeric identifier");
            if (!ids.insert(item->getProperty("id").toString()).second)
                return fail(error, where + ".id duplicates another panel/control id");
            if (!rectangle(*item, where, error))
                return false;
            if (panels)
            {
                if (!stringField(*item, "title", 160, where, error, true) ||
                    !stringField(*item, "kind", 40, where, error))
                    return false;
                if (!oneOf(item->getProperty("kind").toString(),
                           "oscillator|filter|mixer|envelope|lfo|macros|matrix|effects|analyzer|"
                           "waveform|browser|routing|scene|output|xy|keyboard|inspector|amp|tabs"))
                    return fail(error, where + ".kind is unknown");
                if (item->hasProperty("variant") &&
                    (!item->getProperty("variant").isString() ||
                     !oneOf(
                         item->getProperty("variant").toString(),
                         "compact|horizontal|vertical|large|combined|terrain|flow|faders|knobs")))
                    return fail(error, where + ".variant is unknown");
                if (item->hasProperty("index"))
                {
                    const auto &index = item->getProperty("index");
                    if (!number(index) || static_cast<double>(index) < 1 ||
                        static_cast<double>(index) > 16 ||
                        std::floor(static_cast<double>(index)) != static_cast<double>(index))
                        return fail(error, where + ".index must be an integer in 1..16");
                }
                for (const auto *key : {"plot", "plot2"})
                    if (item->hasProperty(key) &&
                        !plotRectangle(item->getProperty(key), where + "." + key, error))
                        return false;
                if (item->hasProperty("titleHeight") &&
                    !boundedNumber(*item, "titleHeight", 0, 120, where, error))
                    return false;
            }
            else
            {
                if (!stringField(*item, "binding", 80, where, error) ||
                    !identifier(item->getProperty("binding").toString()))
                    return fail(error, where + ".binding must be a short parameter identifier");
                if (item->hasProperty("kind") && (!item->getProperty("kind").isString() ||
                                                  !oneOf(item->getProperty("kind").toString(),
                                                         "knob|fader|horizontal|menu|toggle")))
                    return fail(error, where + ".kind is unknown");
                if (item->hasProperty("label") &&
                    !stringField(*item, "label", 160, where, error, true))
                    return false;
                if (item->hasProperty("fontSize") &&
                    !boundedNumber(*item, "fontSize", 8, 24, where, error))
                    return false;
            }
            const auto *colourKey = panels ? "color" : "accent";
            if (item->hasProperty(colourKey))
            {
                const auto &c = item->getProperty(colourKey);
                if (!c.isString() || c.toString().isEmpty() ||
                    (!colour(c.toString()) && !paletteObject->hasProperty(c.toString())))
                    return fail(error,
                                where + "." + colourKey + " must be a hex colour or palette key");
            }
        }
    }
    if (object->hasProperty("decorations"))
    {
        const auto *decorations = object->getProperty("decorations").getArray();
        if (decorations == nullptr || decorations->size() > 128)
            return fail(error, "decorations must be an array of at most 128 objects");
        for (const auto &decoration : *decorations)
            if (decoration.getDynamicObject() == nullptr)
                return fail(error, "Each decoration must be an object");
    }
    const auto *adaptations = object->getProperty("adaptations").getArray();
    if (adaptations == nullptr || adaptations->size() > 32)
        return fail(error, "adaptations must be an array of at most 32 strings");
    for (const auto &adaptation : *adaptations)
        if (!adaptation.isString() || adaptation.toString().length() > 1024)
            return fail(error, "Each adaptation must be a string of at most 1024 characters");
    return true;
}

bool loadManifest(const juce::File &file, juce::var &data, juce::String &error)
{
    data = juce::var{};
    error.clear();
    if (!file.existsAsFile())
        return fail(error, "Workspace manifest does not exist");
    if (file.getSize() <= 0 || file.getSize() > maxBytes)
        return fail(error, "Workspace manifest must be nonempty and no larger than one MiB");
    auto stream = file.createInputStream();
    if (stream == nullptr || !stream->openedOk())
        return fail(error, "Could not open workspace manifest");
    juce::MemoryBlock bytes;
    stream->readIntoMemoryBlock(bytes, maxBytes + 1);
    if (stream->getStatus().failed() || bytes.isEmpty() || bytes.getSize() > maxBytes)
        return fail(error, "Could not read workspace manifest within the one MiB limit");
    const auto *utf8 = static_cast<const char *>(bytes.getData());
    for (size_t i = 0; i < bytes.getSize(); ++i)
        if (utf8[i] == 0)
            return fail(error, "Workspace manifest contains an embedded NUL byte");
    if (!juce::CharPointer_UTF8::isValidString(utf8, static_cast<int>(bytes.getSize())))
        return fail(error, "Workspace manifest must be valid UTF-8");
    const auto text = juce::String::fromUTF8(utf8, static_cast<int>(bytes.getSize()));
    if (!boundedJSON(text, error))
        return false;
    juce::var parsed;
    const auto parsedResult = juce::JSON::parse(text, parsed);
    if (parsedResult.failed())
        return fail(error, "Invalid workspace JSON: " + parsedResult.getErrorMessage());
    if (!validateManifest(parsed, error))
        return false;
    data = std::move(parsed);
    return true;
}
} // namespace Surge::GUI::Reference
