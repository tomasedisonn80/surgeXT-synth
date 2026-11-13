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
constexpr juce::int64 maxExactInteger = 9007199254740991LL;

bool fail(juce::String &error, const juce::String &message)
{
    error = message;
    return false;
}

bool number(const juce::var &v)
{
    if (v.isInt64())
        return static_cast<juce::int64>(v) >= -maxExactInteger &&
               static_cast<juce::int64>(v) <= maxExactInteger;
    return (v.isInt() || v.isDouble()) && std::isfinite(static_cast<double>(v));
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

bool unicodeText(const juce::String &value)
{
    for (const auto c : value)
        if (c == 0 || (c >= 0xd800 && c <= 0xdfff) || c > 0x10ffff)
            return false;
    return true;
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
            const auto key = properties.getName(i).toString();
            if (key.isEmpty() || key.length() > 80 || !unicodeText(key))
                return fail(error, "Manifest property names must be 1..80 Unicode characters");
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
        return (v.toString().length() <= 4096 && unicodeText(v.toString())) ||
               fail(error, "Manifest strings must be valid Unicode of at most 4096 characters");
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

bool insidePanel(const juce::var &v, const juce::DynamicObject &panel,
                 const juce::String &where, juce::String &error)
{
    const auto *values = v.getArray(); // Call only after plotRectangle succeeds.
    const auto x = static_cast<double>(values->getReference(0));
    const auto y = static_cast<double>(values->getReference(1));
    const auto w = static_cast<double>(values->getReference(2));
    const auto h = static_cast<double>(values->getReference(3));
    const auto px = static_cast<double>(panel.getProperty("x"));
    const auto py = static_cast<double>(panel.getProperty("y"));
    return (x >= px - 0.001 && y >= py - 0.001 &&
            x + w <= px + static_cast<double>(panel.getProperty("w")) + 0.001 &&
            y + h <= py + static_cast<double>(panel.getProperty("h")) + 0.001) ||
           fail(error, where + " must be inside its panel");
}

// JUCE's general-purpose parser accepts some non-JSON escapes/trailing commas,
// overwrites duplicate properties, and accumulates integer digits in int64.
// Preflight the bounded input before invoking it. This also prevents integer
// overflow before schema validation and agrees with the Python package checker.
class JSONPreflight
{
  public:
    JSONPreflight(const juce::String &text, juce::String &error)
        : cursor(text.getCharPointer()), error(error)
    {
    }
    bool run()
    {
        whitespace();
        if (*cursor != '{')
            return reject("Manifest must be a JSON object");
        if (!value(0))
            return false;
        whitespace();
        return *cursor == 0 || reject("Unexpected text after the manifest JSON object");
    }

  private:
    juce::String::CharPointerType cursor;
    juce::String &error;
    bool reject(const juce::String &message) { return fail(error, message); }
    void whitespace()
    {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
            ++cursor;
    }
    bool take(juce::juce_wchar c)
    {
        if (*cursor != c)
            return false;
        ++cursor;
        return true;
    }
    bool hex4(juce::juce_wchar &code)
    {
        code = 0;
        for (int i = 0; i < 4; ++i)
        {
            const auto c = *cursor;
            const int digit = c >= '0' && c <= '9' ? c - '0'
                              : c >= 'a' && c <= 'f' ? c - 'a' + 10
                              : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                                      : -1;
            if (digit < 0)
                return reject("Invalid JSON Unicode escape");
            ++cursor;
            code = code * 16 + static_cast<juce::juce_wchar>(digit);
        }
        return true;
    }
    bool string(juce::String *key = nullptr)
    {
        if (!take('"'))
            return reject("Expected a JSON string");
        int length = 0;
        while (*cursor != '"')
        {
            auto c = *cursor;
            if (c < 0x20)
                return reject("Unterminated JSON string or unescaped control character");
            ++cursor;
            if (c == '\\')
            {
                c = *cursor;
                if (c == 0)
                    return reject("Incomplete JSON escape");
                ++cursor;
                switch (c)
                {
                case '"': case '\\': case '/': break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'u':
                    if (!hex4(c))
                        return false;
                    if (c >= 0xd800 && c <= 0xdbff)
                    {
                        juce::juce_wchar low;
                        if (!take('\\') || !take('u') || !hex4(low) || low < 0xdc00 ||
                            low > 0xdfff)
                            return reject("Unpaired JSON Unicode surrogate");
                        c = 0x10000 + (c - 0xd800) * 0x400 + (low - 0xdc00);
                    }
                    break;
                default: return reject("Invalid JSON escape");
                }
            }
            if (c == 0 || (c >= 0xd800 && c <= 0xdfff) || c > 0x10ffff)
                return reject("JSON strings must contain valid non-NUL Unicode scalars");
            if (++length > (key == nullptr ? 4096 : 80))
                return reject("JSON string or property name exceeds its length limit");
            if (key != nullptr)
                *key += juce::String::charToString(c);
        }
        ++cursor;
        return true;
    }
    bool numericToken()
    {
        const auto start = cursor;
        take('-');
        const auto digits = cursor;
        auto digit = [&] { return *cursor >= '0' && *cursor <= '9'; };
        if (!take('0'))
        {
            if (*cursor < '1' || *cursor > '9')
                return reject("Invalid JSON number");
            while (digit())
                ++cursor;
        }
        const auto integerEnd = cursor;
        bool integral = true;
        if (take('.'))
        {
            integral = false;
            if (!digit())
                return reject("JSON fraction requires a digit");
            while (digit())
                ++cursor;
        }
        if (*cursor == 'e' || *cursor == 'E')
        {
            integral = false;
            ++cursor;
            if (*cursor == '+' || *cursor == '-')
                ++cursor;
            if (!digit())
                return reject("JSON exponent requires a digit");
            int exponentDigits = 0;
            while (digit())
            {
                if (++exponentDigits > 4)
                    return reject("JSON exponent exceeds its four-digit limit");
                ++cursor;
            }
        }
        const juce::String token(start, cursor);
        if (token.length() > 128)
            return reject("JSON number exceeds its 128-character limit");
        const juce::String magnitude(digits, integerEnd);
        // parseNumber accumulates even a floating token's leading digits in
        // int64 before reaching its decimal point or exponent.
        if (magnitude.length() > 19 ||
            (magnitude.length() == 19 && magnitude.compare("9223372036854775807") > 0))
            return reject("JSON number's integer part exceeds the parser's safe range");
        if (!std::isfinite(token.getDoubleValue()))
            return reject("Manifest numbers must be finite");
        if (integral)
        {
            if (magnitude.length() > 16 ||
                (magnitude.length() == 16 && magnitude.compare("9007199254740991") > 0))
                return reject("Manifest integer exceeds the exact JSON integer range");
        }
        return true;
    }
    bool value(int depth)
    {
        if (depth > maxDepth)
            return reject("Manifest JSON nesting exceeds 12 levels");
        whitespace();
        if (take('{'))
        {
            whitespace();
            if (take('}'))
                return true;
            std::set<juce::String> keys;
            do
            {
                whitespace();
                juce::String key;
                if (!string(&key))
                    return false;
                if (key.isEmpty() || !keys.insert(key).second || keys.size() > 128)
                    return reject("Empty/duplicate JSON property or more than 128 properties");
                whitespace();
                if (!take(':') || !value(depth + 1))
                    return error.isNotEmpty() ? false : reject("Expected ':' after JSON property");
                whitespace();
                if (take('}'))
                    return true;
            } while (take(','));
            return reject("Expected ',' or '}' after JSON property");
        }
        if (take('['))
        {
            whitespace();
            if (take(']'))
                return true;
            int count = 0;
            do
            {
                if (++count > 512)
                    return reject("JSON array exceeds 512 items");
                if (!value(depth + 1))
                    return false;
                whitespace();
                if (take(']'))
                    return true;
            } while (take(','));
            return reject("Expected ',' or ']' after JSON value");
        }
        if (*cursor == '"')
            return string();
        for (const char *literal : {"true", "false", "null"})
            if (*cursor == *literal)
            {
                for (auto p = literal; *p != 0; ++p)
                    if (!take(*p))
                        return reject("Invalid JSON literal");
                return true;
            }
        return numericToken();
    }
};

bool boundedJSON(const juce::String &text, juce::String &error)
{
    return JSONPreflight(text, error).run();
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
                        (!plotRectangle(item->getProperty(key), where + "." + key, error) ||
                         !insidePanel(item->getProperty(key), *item, where + "." + key, error)))
                        return false;
                if (item->hasProperty("flowNodes"))
                {
                    auto *nodes = item->getProperty("flowNodes").getDynamicObject();
                    if (nodes == nullptr || item->getProperty("kind").toString() != "routing" ||
                        nodes->getProperties().size() == 0 || nodes->getProperties().size() > 11)
                        return fail(error, where + ".flowNodes requires 1..11 routing node rectangles");
                    const auto &properties = nodes->getProperties();
                    for (int n = 0; n < properties.size(); ++n)
                    {
                        const auto key = properties.getName(n).toString();
                        const auto nodeWhere = where + ".flowNodes." + key;
                        if (!oneOf(key, "osc1|osc2|osc3|mixer|filter1|filter2|amp|fx|output|modsources|modbus"))
                            return fail(error, nodeWhere + " is not a supported flow node");
                        const auto &rect = properties.getValueAt(n);
                        if (!plotRectangle(rect, nodeWhere, error) ||
                            !insidePanel(rect, *item, nodeWhere, error))
                            return false;
                    }
                }
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
                if (item->hasProperty("labelPosition") &&
                    (!item->getProperty("labelPosition").isString() ||
                     !oneOf(item->getProperty("labelPosition").toString(), "left|right") ||
                     (item->hasProperty("kind") && item->getProperty("kind").toString() != "knob")))
                    return fail(error, where + ".labelPosition must be left/right on a knob");
                const auto kind = item->getProperty("kind").toString();
                if ((kind == "fader" &&
                     (static_cast<double>(item->getProperty("w")) < 24 ||
                      static_cast<double>(item->getProperty("h")) < 46)) ||
                    (kind == "horizontal" &&
                     (static_cast<double>(item->getProperty("w")) < 24 ||
                      static_cast<double>(item->getProperty("h")) < 40)))
                    return fail(error, where + " is too small for its labelled slider and track");
            }
            if (item->hasProperty("ink") &&
                (!item->getProperty("ink").isString() || !colour(item->getProperty("ink").toString())))
                return fail(error, where + ".ink must be #RRGGBB or #RRGGBBAA");
            if (panels && item->hasProperty("fontSize") &&
                !boundedNumber(*item, "fontSize", 8, 24, where, error))
                return false;
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
