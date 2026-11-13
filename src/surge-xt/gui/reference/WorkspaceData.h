/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_WORKSPACE_DATA_H
#define SURGE_REFERENCE_WORKSPACE_DATA_H

#include <juce_core/juce_core.h>
#include <vector>

struct SurgeSynthProcessor;

namespace Surge::GUI::Reference
{
/** Engine route identity, independent of its current table position.
    sourceScene is the route's stored scene for global targets, including macros.
    targetType guards against writing an oscillator/FX slot after its algorithm changed.
*/
struct RouteKey
{
    int parameterId{-1}, sourceId{0}, sourceScene{0}, sourceIndex{0}, targetType{0};

    bool operator==(const RouteKey &other) const
    {
        return parameterId == other.parameterId && sourceId == other.sourceId &&
               sourceScene == other.sourceScene && sourceIndex == other.sourceIndex &&
               targetType == other.targetType;
    }
    bool operator!=(const RouteKey &other) const { return !(*this == other); }
};

struct RouteRow
{
    juce::String source, destination, amount;
    // Signed native modulation depth, normalized to the destination's range.
    float normalizedDepth{0};
    bool muted{false};
    RouteKey key;
    bool bipolar{false};
};

/** Read-only message-thread snapshot of the selected scene and applicable global routes.
    At most 128 rows are returned. A busy routing lock returns no rows rather than blocking audio.
*/
std::vector<RouteRow> modulationRows(SurgeSynthProcessor &, int scene);

/** Message-thread editing of an existing native modulation route.

    A gesture captures one native undo item and emits the same begin/end edit
    notifications as the original editor. It may cross zero (the engine removes
    zero-depth routes), but it never creates an arbitrary absent route. Busy locks,
    invalid identities, stale target algorithms, and nonfinite values are rejected.

    The processor must outlive this object. Call end() on scene/preset changes,
    editor hiding, and before rebuilding row components. Keep a dragged row alive
    while isEditing() even if a zero-depth route vanishes from a fresh snapshot.
*/
class ModulationEditorModel
{
  public:
    explicit ModulationEditorModel(SurgeSynthProcessor &processor) : processor(processor) {}
    ~ModulationEditorModel();
    ModulationEditorModel(const ModulationEditorModel &) = delete;
    ModulationEditorModel &operator=(const ModulationEditorModel &) = delete;

    bool begin(const RouteRow &);
    bool setDepth(float normalizedDepth);
    void end();
    bool setMuted(const RouteRow &, bool muted);
    bool remove(const RouteRow &);
    bool isEditing() const { return editing; }
    bool isEditing(const RouteKey &key) const { return editing && activeKey == key; }
    float currentDepth() const { return lastDepth; }

  private:
    SurgeSynthProcessor &processor;
    RouteKey activeKey;
    float lastDepth{0};
    bool editing{false}, capturedUndo{false}, changed{false};
    bool originalMuted{false}, removedByZero{false};
    void notifyChanged();
};

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
