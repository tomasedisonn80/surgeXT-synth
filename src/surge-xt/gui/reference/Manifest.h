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
 * ParameterBridge before exposing its controls.
 */
bool validateManifest(const juce::var &data, juce::String &error);

/** Read at most one MiB of UTF-8 JSON, then validate it.
 *
 * On failure data is cleared and error explains the first rejected field.
 * This function does not load any referenced resource. The caller is responsible
 * for confining the manifest path to the selected skin package.
 */
bool loadManifest(const juce::File &file, juce::var &data, juce::String &error);
} // namespace Surge::GUI::Reference

#endif
