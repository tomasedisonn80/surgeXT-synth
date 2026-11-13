/*
 * Surge XT - a free and open source hybrid synthesizer.
 * Copyright 2026, contributors, as described in the GitHub transaction log.
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 */
#ifndef SURGE_SRC_SURGE_XT_GUI_REFERENCE_MANIFEST_H
#define SURGE_SRC_SURGE_XT_GUI_REFERENCE_MANIFEST_H

#include "juce_core/juce_core.h"

namespace Surge::GUI::Reference
{
/** Validate a version-one native workspace description without changing it.
 *
 * This validates data and layout geometry, not whether a binding exists in a
 * particular processor. Workspace must additionally resolve every binding with
 * ParameterBridge before exposing its controls. Panel plots and flow-node
 * rectangles must remain inside their owning panels. Control IDs are unique;
 * repeated parameter bindings are permitted for intentional alternate views.
 */
bool validateManifest(const juce::var &data, juce::String &error);

/** Read at most one MiB of UTF-8 JSON, then validate it.
 *
 * On failure data is cleared and error explains the first rejected field.
 * Input is strict JSON: duplicate property names (including escaped aliases),
 * invalid Unicode, excessive depth/collections, and unsafe numeric tokens are
 * rejected before JUCE's parser. Integer literals are restricted to the exact
 * JSON range +/- (2^53 - 1); numeric tokens are <=128 characters with at most
 * four exponent digits. The Python package validator applies the same limits.
 * This function does not load any referenced resource. The caller is responsible
 * for confining the manifest path to the selected skin package.
 */
bool loadManifest(const juce::File &file, juce::var &data, juce::String &error);
} // namespace Surge::GUI::Reference

#endif
