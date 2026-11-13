/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_WORKSPACE_DATA_H
#define SURGE_REFERENCE_WORKSPACE_DATA_H

#include <juce_core/juce_core.h>
#include <vector>

struct SurgeSynthProcessor;

namespace Surge::GUI::Reference
{
struct RouteRow
{
    juce::String source, destination, amount;
    // Signed native modulation depth, normalized to the destination's range.
    float normalizedDepth{0};
    bool muted{false};
};

/** Read-only message-thread snapshot of the selected scene and applicable global routes.
    At most 128 rows are returned. A busy routing lock returns no rows rather than blocking audio.
*/
std::vector<RouteRow> modulationRows(SurgeSynthProcessor &, int scene);

struct BrowserRow
{
    juce::String name;
    int patchId{-1};
    bool category{false};
};

/** Read-only message-thread view of the installed native patch catalog.
    Empty queries show real category headings, expanding the current category where possible.
    Searches match installed patch names or category names. Patch IDs are native patch_list indices;
    use the existing dirty-checking preset loader when acting on them, not an unchecked direct load.
*/
std::vector<BrowserRow> browserRows(SurgeSynthProcessor &, const juce::String &query,
                                    int maxRows = 20);
} // namespace Surge::GUI::Reference
#endif
