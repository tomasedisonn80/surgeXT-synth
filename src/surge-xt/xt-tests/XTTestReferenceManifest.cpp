/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Released under GPL-3.0-or-later; see LICENSE in the repository root.
 */

#include "catch2/catch_amalgamated.hpp"
#include "gui/reference/Manifest.h"

namespace
{
void applySchemaChange(juce::var &manifest, const juce::var &change)
{
    const auto *path = change["path"].getArray();
    REQUIRE(path != nullptr);
    REQUIRE(!path->isEmpty());
    auto parent = manifest;
    for (int i = 0; i + 1 < path->size(); ++i)
    {
        const auto &key = path->getReference(i);
        if (key.isString())
        {
            REQUIRE(parent.getDynamicObject() != nullptr);
            REQUIRE(parent.hasProperty(juce::Identifier(key.toString())));
            parent = parent[juce::Identifier(key.toString())];
        }
        else
        {
            REQUIRE(key.isInt());
            auto *array = parent.getArray();
            REQUIRE(array != nullptr);
            const int index = static_cast<int>(key);
            REQUIRE(index >= 0);
            REQUIRE(index < array->size());
            parent = array->getReference(index);
        }
    }
    const auto &key = path->getLast();
    if (key.isString())
    {
        auto *object = parent.getDynamicObject();
        REQUIRE(object != nullptr);
        const juce::Identifier field(key.toString());
        if (static_cast<bool>(change["remove"]))
        {
            REQUIRE(object->hasProperty(field));
            object->removeProperty(field);
        }
        else
        {
            // NamedValueSet's equality shortcut compares array elements with var's
            // numeric coercion, so [380] and ["380"] can compare equal. Replace the
            // property outright to preserve the fixture's exact JSON value types.
            object->removeProperty(field);
            object->setProperty(field, change["value"].clone());
        }
    }
    else
    {
        REQUIRE(key.isInt());
        auto *array = parent.getArray();
        REQUIRE(array != nullptr);
        const int index = static_cast<int>(key);
        REQUIRE(index >= 0);
        REQUIRE(index < array->size());
        if (static_cast<bool>(change["remove"]))
            array->remove(index);
        else
            array->set(index, change["value"].clone());
    }
}
} // namespace

TEST_CASE("Reference selection and widget schema matches shared fixtures",
          "[reference][manifest][schema]")
{
    const auto fixtureFile = juce::File(juce::String(SURGE_REFERENCE_SOURCE_DIR))
                                 .getChildFile("scripts/reference-workspaces/fixtures/selection-schema.json");
    REQUIRE(fixtureFile.existsAsFile());
    juce::var fixtures;
    REQUIRE(juce::JSON::parse(fixtureFile.loadFileAsString(), fixtures).wasOk());
    REQUIRE(fixtures["base"].isObject());
    const auto *cases = fixtures["cases"].getArray();
    REQUIRE(cases != nullptr);
    REQUIRE(!cases->isEmpty());
    for (const auto &fixture : *cases)
    {
        DYNAMIC_SECTION(fixture["name"].toString().toStdString())
        {
            auto manifest = fixtures["base"].clone();
            const auto *changes = fixture["changes"].getArray();
            REQUIRE(changes != nullptr);
            for (const auto &change : *changes)
                applySchemaChange(manifest, change);
            REQUIRE(fixture["valid"].isBool());
            const bool expected = static_cast<bool>(fixture["valid"]);
            juce::String error;
            const bool valid = Surge::GUI::Reference::validateManifest(manifest, error);
            INFO("Schema error: " << error.toStdString());
            REQUIRE(valid == expected);
            REQUIRE(error.isEmpty() == expected);
        }
    }
}
