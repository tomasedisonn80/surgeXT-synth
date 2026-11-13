/*
 * Surge XT - a free and open source hybrid synthesizer,
 * built by Surge Synth Team
 *
 * Learn more at https://surge-synthesizer.github.io/
 *
 * Copyright 2018-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * Surge XT is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Surge was a commercial product from 2004-2018, copyright and ownership
 * held by Claes Johanson at Vember Audio during that period.
 * Claes made Surge open source in September 2018.
 *
 * All source for Surge XT is available at
 * https://github.com/surge-synthesizer/surge
 */

#include "SurgeSynthEditor.h"
#include "SurgeSynthProcessor.h"
#include "SurgeImageStore.h"
#include "SurgeImage.h"
#include "SurgeGUIEditor.h"
#include "SurgeJUCELookAndFeel.h"
#include "RuntimeFont.h"
#include "AccessibleHelpers.h"
#include <version.h>
#include "gui/widgets/CurrentFxDisplay.h"
#include "gui/widgets/MainFrame.h"
#include "gui/reference/ParameterBridge.h"
#include "gui/reference/Workspace.h"
#include "gui/reference/WavetableSelection.h"

#include <algorithm>
#include <filesystem>
#include <limits>

struct VKeyboardWheel : public juce::Component
{
    std::function<void(int)> onValueChanged = [](int f) {};
    bool snapBack{false};
    bool unipolar{true};
    int range{127};
    int value{0};
    void paint(juce::Graphics &g) override
    {
        auto wheelSz = getLocalBounds().reduced(2, 3);

        g.setColour(findColour(SurgeJUCELookAndFeel::SurgeColourIds::wheelBgId));
        g.fillRect(wheelSz);
        g.setColour(findColour(SurgeJUCELookAndFeel::SurgeColourIds::wheelBorderId));
        g.drawRect(wheelSz.expanded(1, 1));

        float p = 1.0 * value / range;

        if (!unipolar)
        {
            p = 1.0 * (value + range) / (2 * range);
        }

        // y direction is flipped
        p = 1 - p;

        float cp = wheelSz.getY() + p * (wheelSz.getHeight() - 4);
        auto r = wheelSz.withHeight(2).translated(0, cp - 2).reduced(1, 0);

        g.setColour(findColour(SurgeJUCELookAndFeel::SurgeColourIds::wheelValueId));
        g.fillRect(r);
    }

    void valueFromY(float y)
    {
        auto wheelSz = getLocalBounds().reduced(1, 2);
        auto py = std::clamp(y, 1.f * wheelSz.getY(), 1.f * wheelSz.getY() + wheelSz.getHeight());
        py = (py - wheelSz.getY()) / wheelSz.getHeight();
        py = 1 - py;

        if (unipolar)
            value = py * range;
        else
            value = 2 * py * range - range;
        onValueChanged(value);
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        valueFromY(event.position.y);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override
    {
        valueFromY(event.position.y);
        repaint();
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        if (snapBack)
        {
            value = 0;
            onValueChanged(value);
        }
        repaint();
    }
};

struct VKeyboardSus : public juce::Component
{
    std::function<void(int)> onValueChanged = [](int f) {};
    bool isOn{false};

    void paint(juce::Graphics &g) override
    {
        auto wheelSz = getLocalBounds().reduced(1, 2);

        g.setColour(findColour(SurgeJUCELookAndFeel::SurgeColourIds::wheelBgId));
        g.fillRect(wheelSz);

        if (isOn)
        {
            g.setColour(findColour(SurgeJUCELookAndFeel::SurgeColourIds::wheelValueId));
            g.fillRect(wheelSz.reduced(1, 1));
        }

        g.setColour(findColour(SurgeJUCELookAndFeel::SurgeColourIds::wheelBorderId));
        g.drawRect(wheelSz.expanded(1, 1));
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        isOn = !isOn;
        onValueChanged(isOn * 127);
        repaint();
    }
};

static std::weak_ptr<SurgeJUCELookAndFeel> surgeLookAndFeelWeakPointer;
static std::mutex surgeLookAndFeelSetupMutex;

//==============================================================================

SurgeVirtualKeyboard::~SurgeVirtualKeyboard()
{
    clearAllLatches();
    // otherwise closing the editor with a key held leaves the note on
    releaseAllHeldKeys();
}

float SurgeVirtualKeyboard::getCurrentVelocity() const { return editor->midiKeyboardVelocity; }

//==============================================================================

SurgeSynthEditor::SurgeSynthEditor(SurgeSynthProcessor &p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    // Marks this editor as the popup-scale anchor for the SURGE PATCH in
    // juce_TextEditor.cpp so TextEditor right-click menus inherit only the
    // host scale factor and not the UI zoom transform applied to frame.
    getProperties().set("SSTPopupAnchor", true);

    {
        std::lock_guard<std::mutex> grd(surgeLookAndFeelSetupMutex);
        if (auto sp = surgeLookAndFeelWeakPointer.lock())
        {
            surgeLF = sp;
        }
        else
        {
            surgeLF = std::make_shared<SurgeJUCELookAndFeel>();
            surgeLookAndFeelWeakPointer = surgeLF;

            juce::LookAndFeel::setDefaultLookAndFeel(surgeLF.get());
        }

        surgeLF->addStorage(&(processor.surge->storage));
    }

    addKeyListener(this);
    // lets the editor itself hold keyboard focus, so that takeInitialKeyboardFocus()
    // doesn't push focus onto the first focusable widget instead
    setWantsKeyboardFocus(true);

    topLevelContainer = std::make_unique<juce::Component>();
    addAndMakeVisible(*topLevelContainer);

    sge = std::make_unique<SurgeGUIEditor>(this, processor.surge.get());

    auto mcValue = Surge::Storage::getUserDefaultValue(&(this->processor.surge->storage),
                                                       Surge::Storage::MiddleC, 1);

    keyboard = std::make_unique<SurgeVirtualKeyboard>(
        this, processor.midiKeyboardState,
        juce::MidiKeyboardComponent::Orientation::horizontalKeyboard);
    keyboard->setVelocity(midiKeyboardVelocity, true);
    keyboard->setOctaveForMiddleC(5 - mcValue);
    keyboard->setKeyPressBaseOctave(midiKeyboardOctave);
    keyboard->setLowestVisibleKey(24);
    // this makes VKB always receive keyboard input (except when we focus on any typeins, of course)
    keyboard->setWantsKeyboardFocus(false);

    // Load the saved VKB click velocity mode setting
    bool virtualKeyboardClickSetsVelocity = Surge::Storage::getUserDefaultValue(
        &(this->processor.surge->storage), Surge::Storage::VirtualKeyboardClickSetsVelocity, false);
    if (auto vkb = dynamic_cast<SurgeVirtualKeyboard *>(keyboard.get()))
    {
        vkb->useClickPositionForOverallVelocity = virtualKeyboardClickSetsVelocity;
        // Set up callback to update midiKeyboardVelocity when mode is enabled
        vkb->onVelocityChanged = [this](float vel) { midiKeyboardVelocity = vel; };
    }

    auto vkbLayout = Surge::Storage::getUserDefaultValue(
        &(this->processor.surge->storage), Surge::Storage::VirtualKeyboardLayout, "QWERTY");
    setVKBLayout(vkbLayout);

    auto w = std::make_unique<VKeyboardWheel>();
    w->snapBack = true;
    w->unipolar = false;
    w->range = 8196;
    w->onValueChanged = [this](auto f) {
        processor.queueMidiController(SurgeSynthProcessor::midiR::PITCHWHEEL, f);
    };
    pitchwheel = std::move(w);

    auto m = std::make_unique<VKeyboardWheel>();
    m->onValueChanged = [this](auto f) {
        processor.queueMidiController(SurgeSynthProcessor::midiR::MODWHEEL, f);
    };
    modwheel = std::move(m);

    auto sp = std::make_unique<VKeyboardSus>();
    sp->onValueChanged = [this](auto f) {
        processor.queueMidiController(SurgeSynthProcessor::midiR::SUSPEDAL, f);
    };
    suspedal = std::move(sp);

    tempoTypein = std::make_unique<juce::TextEditor>("Tempo");
    tempoTypein->setFont(sge->currentSkin->fontManager->getLatoAtSize(9));
    tempoTypein->setInputRestrictions(3, "0123456789");
    tempoTypein->setSelectAllWhenFocused(true);
    tempoTypein->onReturnKey = [this]() {
        // this is thread sloppy
        float newT = std::atof(tempoTypein->getText().toRawUTF8());

        processor.standaloneTempo = newT;
        processor.surge->storage.unstreamedTempo = newT;
        tempoTypein->giveAwayKeyboardFocus();
    };

    tempoLabel = std::make_unique<juce::Label>("Tempo", "Tempo");
    sustainLabel = std::make_unique<juce::Label>("Sustain", "Sustain");

    topLevelContainer->addChildComponent(*keyboard);
    topLevelContainer->addChildComponent(*pitchwheel);
    topLevelContainer->addChildComponent(*modwheel);
    topLevelContainer->addChildComponent(*suspedal);
    topLevelContainer->addChildComponent(*tempoLabel);
    topLevelContainer->addChildComponent(*sustainLabel);
    topLevelContainer->addChildComponent(*tempoTypein);

    drawExtendedControls = sge->getShowVirtualKeyboard();

    bool addTempo = processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
    int yExtra = 0;

    if (drawExtendedControls)
    {
        keyboard->setVisible(true);
        tempoLabel->setVisible(addTempo);
        tempoTypein->setVisible(addTempo);

        yExtra = extraYSpaceForVirtualKeyboard;
    }
    else
    {
        keyboard->setVisible(false);
        tempoLabel->setVisible(false);
        tempoTypein->setVisible(false);

        yExtra = 0;
    }

    auto rg = BlockRezoom(this);
    setSize(BASE_WINDOW_SIZE_X, BASE_WINDOW_SIZE_Y + yExtra);
    // add the bottom right corner resizer only for VST2
    setResizable(true, processor.wrapperType == juce::AudioProcessor::wrapperType_VST);

    sge->audioLatencyNotified = processor.inputIsLatent;
    if (juce::Desktop::getInstance().isHeadless() == false)
        sge->open(nullptr);

    updateReferenceWorkspaceFromSkin(true);

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 60);
}

SurgeSynthEditor::~SurgeSynthEditor()
{
    idleTimer->stopTimer();
    // End any live parameter gestures before the processor adapters or the stock
    // editor's look-and-feel are released. Callbacks hold SafePointers, not raw this.
    returnToReferenceWorkspace.reset();
    referenceWorkspace.reset();
    sge->close();

    if (sge->bitmapStore)
    {
        sge->bitmapStore->clearAllLoadedBitmaps();
    }

    surgeLF->removeStorage(&(processor.surge->storage));

    sge.reset(nullptr);
}

void SurgeSynthEditor::setVKBLayout(const std::string layout)
{
    auto searchTerm = [&layout](const auto &x) { return x.first == layout; };
    auto search = std::find_if(vkbLayouts.begin(), vkbLayouts.end(), searchTerm);

    if (search != vkbLayouts.end())
    {
        keyboard->clearKeyMappings();

        unsigned int n = 0;

        for (auto i : search->second)
        {
            // Don't bind accessible action keys to the keyboard
            if (Surge::GUI::allowKeyboardEdits(&processor.surge->storage))
            {
                // Don't know why we have high bit set on the keys? Do both to be sure
                auto b1 = Surge::Widgets::isAccessibleKey((juce::KeyPress)i);
                auto b2 =
                    (i > 128) ? Surge::Widgets::isAccessibleKey((juce::KeyPress)(i - 128)) : false;

                if (b1 || b2)
                {
                    continue;
                }
            }
#if JUCE_LINUX
            // See issue #7604
            if (i < 128)
            {
                keyboard->setKeyPressForNote((juce::KeyPress)i, n);
            }
#else
            keyboard->setKeyPressForNote((juce::KeyPress)i, n);
#endif

            n++;
        }
    }
}

// The VKB keeps QWERTY the whole time it is shown; only a text or code editor takes it away.
bool SurgeSynthEditor::vkbShouldTakeKeys() const
{
    // vkbForward covers the typein overlays and the formula editor, the cast the plain
    // TextEditors such as the BPM field next to the keyboard
    return !(referenceWorkspace && referenceWorkspace->isVisible()) &&
           sge->getShowVirtualKeyboard() && sge->shouldForwardKeysToVKB() &&
           dynamic_cast<juce::TextEditor *>(juce::Component::getCurrentlyFocusedComponent()) ==
               nullptr;
}

/*
 * A menu takes the key events without taking the keyboard focus, since PopupMenu's window sets
 * wantsKeyboardFocus false and runs off modal state and a temporary peer. Opening one is not
 * observable through the focus system, which is also why JUCE's own focusLost net never covered
 * menus. So while one is open, read the physical key state instead: keyStateChanged starts and
 * ends notes from it, so QWERTY keeps playing across the menu.
 *
 * Ownership is settled before the menu opens rather than while it is up. The key state is shared
 * by every instance in the process, and on X11 clicking a menu item moves the focus to the menu
 * window, so asking once the menu is up answers for the wrong editor.
 */
void SurgeSynthEditor::updateVKBFromKeyState()
{
    auto *vkb = dynamic_cast<SurgeVirtualKeyboard *>(keyboard.get());

    if (!vkb)
    {
        return;
    }

    // always false on JUCE 7, which cannot tell a menu from any other modal - see JuceAPICope.h
    const auto menuActive = SST_JUCE_ANY_MENU_ACTIVE();

    if (menuActive)
    {
        if (vkbMenuOwnedKeys)
        {
            vkb->keyStateChanged(false);
        }
    }
    else
    {
        // one more rescan on the way out, since the key up can land in the menu with no tick
        // left to notice it before it closes
        if (vkbMenuWasActive && vkbMenuOwnedKeys)
        {
            vkb->keyStateChanged(false);
        }

        // with no menu up the ordinary key events drive the VKB, so all that is left is to drop
        // held notes when something takes the keys away or we stop being frontmost
        const auto takingKeys = vkbShouldTakeKeys() && juce::Process::isForegroundProcess();

        if (vkbWasTakingKeys && !takingKeys)
        {
            vkb->releaseAllHeldKeys();
        }

        vkbWasTakingKeys = takingKeys;
        vkbMenuOwnedKeys = takingKeys && hasKeyboardFocus(true);
    }

    vkbMenuWasActive = menuActive;
}

void SurgeSynthEditor::handleAsyncUpdate() {}

void SurgeSynthEditor::paint(juce::Graphics &g)
{
    g.fillAll(findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoBackgroundId));

#if DEBUG_FULLSCREENBOUNDS
    /* For debugging fullscreen */
    g.setColour(juce::Colours::red);
    for (int x = 100; x < getWidth(); x += 100)
    {
        g.drawLine(x, 0, x, getHeight(), 1);
    }
    for (int y = 100; y < getWidth(); y += 100)
    {
        g.drawLine(0, y, getWidth(), y, 1);
    }
#endif
}

void SurgeSynthEditor::idle()
{
    takeInitialKeyboardFocus();

    updateVKBFromKeyState();

    sge->idle();

    // Skin selection happens in the existing asynchronous Settings menu. Check
    // its identity each tick; stat/reload unchanged packages at most once a second.
    const auto skin = sge->currentSkin;
    const auto key = skin ? juce::String(skin->root + skin->name) : juce::String();
    if (key != referenceSkinKey || --referenceSkinPollCountdown <= 0)
    {
        referenceSkinPollCountdown = 60;
        updateReferenceWorkspaceFromSkin();
    }

    if (referenceWorkspace && referenceWorkspace->isVisible() &&
        referenceWorkspace->bridge().getScene() != sge->getCurrentScene())
        referenceWorkspace->selectScene(sge->getCurrentScene(), false);

    if (processor.surge->refresh_vkb)
    {
        const int curTypeinBPM = std::atoi(tempoTypein->getText().toStdString().c_str());
        const int curBPM = std::round(processor.surge->time_data.tempo);

        if (curTypeinBPM != curBPM)
        {
            tempoTypein->setText(fmt::format("{}", curBPM));
        }

        processor.surge->refresh_vkb = false;
    }
}

/*
 * Keyboard shortcuts are key listeners on this editor. When nothing has keyboard focus,
 * JUCE delivers key presses to the window's top-level component and walks up from there,
 * which in the standalone is our parent, so shortcuts never arrive until something inside
 * the editor is clicked. So once the window is showing and focused, take focus once.
 *
 * This is polled from idle because the window isn't focused yet when the editor is
 * attached, and JUCE silently drops a focus grab made while the peer isn't focused.
 * Requiring the peer to already be focused means we never take OS focus away from a
 * host; in plugins where the view only gets focus on click, this simply doesn't fire.
 */
void SurgeSynthEditor::takeInitialKeyboardFocus()
{
    if (initialKeyboardFocusTaken)
        return;

    auto *peer = getPeer();

    if (!isShowing() || !peer || !peer->isFocused())
        return;

    initialKeyboardFocusTaken = true;

    if (!hasKeyboardFocus(true))
        Surge::GUI::grabKeyboardFocusIfAllowed(this);
}

void SurgeSynthEditor::reapplySurgeComponentColours()
{
    tempoLabel->setColour(juce::Label::textColourId,
                          findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoLabelId));
    sustainLabel->setColour(juce::Label::textColourId,
                            findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoLabelId));

    tempoTypein->setColour(
        juce::TextEditor::backgroundColourId,
        findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinBackgroundId));
    tempoTypein->setColour(juce::TextEditor::outlineColourId,
                           findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinBorderId));
    tempoTypein->setColour(juce::TextEditor::focusedOutlineColourId,
                           findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinBorderId));
    tempoTypein->setColour(
        juce::TextEditor::highlightColourId,
        findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinHighlightId));
    tempoTypein->setColour(juce::TextEditor::highlightedTextColourId,
                           findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinTextId));
    tempoTypein->setColour(juce::TextEditor::textColourId,
                           findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinTextId));
    tempoTypein->applyColourToAllText(
        findColour(SurgeJUCELookAndFeel::SurgeColourIds::tempoTypeinTextId), true);

    for (auto *p = getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (auto dw = dynamic_cast<juce::DocumentWindow *>(p))
        {
            dw->setName("Surge XT");

            if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
            {
                dw->setColour(juce::DocumentWindow::backgroundColourId,
                              findColour(SurgeJUCELookAndFeel::SurgeColourIds::topWindowBorderId));
            }
        }
    }

    repaint();
}

void SurgeSynthEditor::resetWindowFromSkin()
{
    updateReferenceWorkspaceFromSkin(true);
    resized();
}

void SurgeSynthEditor::selectSkin(const Surge::GUI::SkinDB::Entry &entry)
{
    if (!sge || !processor.surge)
        return;
    sge->selectSkinFromReferenceWorkspace(entry);
    processor.surge->refresh_editor = true;
    resetWindowFromSkin();
}

void SurgeSynthEditor::updateReferenceWorkspaceFromSkin(bool force)
{
    if (!sge || !sge->currentSkin)
        return;

    const auto skin = sge->currentSkin;
    const auto key = juce::String(skin->root + skin->name);
    const auto xmlFile = skin->useInMemorySkin
                             ? juce::File()
                             : juce::File(juce::String(skin->resourceName("skin.xml")));
    const auto xmlTime = xmlFile.getLastModificationTime().toMilliseconds();
    const auto layoutTime = referenceManifest.getLastModificationTime().toMilliseconds();
    if (!force && key == referenceSkinKey && xmlTime == referenceSkinModificationTime &&
        layoutTime == referenceLayoutModificationTime)
        return;

    const bool keepFullEditor = referenceWorkspace && !referenceWorkspace->isVisible() &&
                                key == referenceSkinKey;
    referenceSkinKey = key;
    referenceSkinModificationTime = xmlTime;
    referenceLayoutModificationTime = layoutTime;
    referenceWorkspaceError.clear();

    const auto useStockEditor = [this]() {
        referenceWorkspace.reset();
        returnToReferenceWorkspace.reset();
        referenceManifest = juce::File();
        referenceLayoutModificationTime = 0;
        topLevelContainer->setVisible(true);
        resized();
    };

    // In-memory and conventional skins use the original editor unchanged. Keep
    // this small metadata read independent of the private skin-parser internals.
    if (skin->useInMemorySkin || !xmlFile.existsAsFile())
    {
        useStockEditor();
        return;
    }

    if (xmlFile.getSize() > 1024 * 1024)
    {
        referenceWorkspaceError = "Skin metadata exceeds the workspace size limit.";
        useStockEditor();
        return;
    }

    const auto xmlText = xmlFile.loadFileAsString();
    if (xmlText.containsIgnoreCase("<!DOCTYPE") || xmlText.containsIgnoreCase("<!ENTITY"))
    {
        referenceWorkspaceError = "Reference workspace metadata must not contain external entities.";
        useStockEditor();
        return;
    }

    auto xml = juce::parseXML(xmlText);
    auto *globals = xml ? xml->getChildByName("globals") : nullptr;
    auto *workspaceTag = globals ? globals->getChildByName("reference-workspace") : nullptr;
    if (!workspaceTag)
    {
        useStockEditor();
        return;
    }

    const auto layout = workspaceTag->getStringAttribute("layout");
    // Version 1 packages use a single adjacent JSON file. Disallow absolute,
    // traversing and nested paths; canonical checks also reject symlink escapes.
    if (layout.isEmpty() || layout.containsAnyOf("/\\:") || layout == "." || layout == "..")
    {
        referenceWorkspaceError = "The workspace layout must be a file inside its skin package.";
        useStockEditor();
        return;
    }

    const auto file = xmlFile.getSiblingFile(layout);
    const auto nativePath = [](const juce::File &source) {
#if JUCE_WINDOWS
        return std::filesystem::path(source.getFullPathName().toWideCharPointer());
#else
        // POSIX native paths are UTF-8 byte strings; avoid the deprecated C++20 u8path helper.
        return std::filesystem::path(source.getFullPathName().toStdString());
#endif
    };
    std::error_code ec;
    const auto canonicalPackage = std::filesystem::canonical(
        nativePath(xmlFile.getParentDirectory()), ec);
    if (ec)
    {
        referenceWorkspaceError = "The reference skin package could not be resolved.";
        useStockEditor();
        return;
    }
    const auto canonicalFile = std::filesystem::canonical(nativePath(file), ec);
    if (ec || canonicalFile.parent_path() != canonicalPackage)
    {
        referenceWorkspaceError = "The reference layout is missing or outside its skin package.";
        useStockEditor();
        // Detect a subsequently created/repaired file without requiring a restart.
        referenceManifest = file;
        referenceLayoutModificationTime = file.getLastModificationTime().toMilliseconds();
        return;
    }

    // Cache failed files too: an invalid package must not repeatedly interrupt
    // the user or allocate a replacement component on every timer tick.
    referenceManifest = file;
    referenceLayoutModificationTime = file.getLastModificationTime().toMilliseconds();
    auto next = std::make_unique<Surge::GUI::Reference::Workspace>(processor);
    if (!next->load(file, referenceWorkspaceError))
    {
        referenceWorkspace.reset();
        returnToReferenceWorkspace.reset();
        topLevelContainer->setVisible(true);
        resized();
        processor.surge->storage.reportError(referenceWorkspaceError.toStdString(),
                                             "Reference Workspace Load Error");
        return;
    }

    // Initialising a skin must not dispatch a scene-change action: the native
    // scene selector also releases latched notes, even when selecting the same B scene.
    next->selectScene(sge->getCurrentScene(), false);
    const juce::Component::SafePointer<SurgeSynthEditor> safe(this);
    next->onFullEditor = [safe]() {
        if (safe)
            safe->showReferenceWorkspace(false);
    };
    next->onChooseSkin = [safe]() {
        if (safe)
            safe->showReferenceSkinMenu();
    };
    next->onAction = [safe](const juce::String &action) {
        if (safe)
            safe->handleReferenceWorkspaceAction(action);
    };
    next->onSelectWavetable =
        [safe](const Surge::GUI::Reference::WavetableSelectionRequest &request) {
            return safe && safe->handleReferenceWavetable(request);
        };
    referenceWorkspace = std::move(next);
    addChildComponent(*referenceWorkspace);

    returnToReferenceWorkspace = std::make_unique<juce::TextButton>("Return to reference workspace");
    returnToReferenceWorkspace->setTitle("Return to reference workspace");
    returnToReferenceWorkspace->setTooltip("Return to the selected reference skin (Ctrl/Command+Alt+R)");
    returnToReferenceWorkspace->setWantsKeyboardFocus(true);
    returnToReferenceWorkspace->onClick = [safe]() {
        if (safe)
            safe->showReferenceWorkspace(true);
    };
    addChildComponent(*returnToReferenceWorkspace);
    showReferenceWorkspace(!keepFullEditor);
}

void SurgeSynthEditor::showReferenceWorkspace(bool show)
{
    if (!referenceWorkspace)
        return;

    // Release any held virtual keyboard notes as their owning surface is hidden.
    if (auto *vkb = dynamic_cast<SurgeVirtualKeyboard *>(keyboard.get()))
        vkb->releaseAllHeldKeys();
    if (!show && referenceWorkspace->isVisible())
        referenceWorkspaceSize = {getWidth(), getHeight()};

    referenceWorkspace->setVisible(show);
    topLevelContainer->setVisible(!show);
    returnToReferenceWorkspace->setVisible(!show);
    auto rezoom = BlockRezoom(this);
    if (show)
    {
        referenceWorkspace->selectScene(sge->getCurrentScene(), false);
        auto width = referenceWorkspaceSize.x;
        auto height = referenceWorkspaceSize.y;
        if (auto *display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds()))
        {
            const auto scale = std::min(1.0, std::min(0.95 * display->userArea.getWidth() / width,
                                                     0.90 * display->userArea.getHeight() / height));
            width = juce::jmax(1, juce::roundToInt(width * scale));
            height = juce::jmax(1, juce::roundToInt(height * scale));
        }
        setSize(width, height);
        referenceWorkspace->toFront(false);
    }
    else
    {
        const auto scale = sge->getZoomFactor() * 0.01;
        setSize(juce::roundToInt(sge->getWindowSizeX() * scale),
                juce::roundToInt((sge->getWindowSizeY() +
                                 (sge->getShowVirtualKeyboard() ? extraYSpaceForVirtualKeyboard : 0)) *
                                scale) +
                    28);
        returnToReferenceWorkspace->toFront(false);
    }
    resized();
    repaint();
}

void SurgeSynthEditor::showReferenceSkinMenu()
{
    if (sge)
        sge->showSettingsMenu({20, 30}, nullptr);
}

bool SurgeSynthEditor::handleReferenceWavetable(
    const Surge::GUI::Reference::WavetableSelectionRequest &request)
{
    if (!sge || !processor.surge || !referenceWorkspace || !referenceWorkspace->isVisible() ||
        referenceWorkspace->bridge().getScene() != request.target.scene)
        return false;
    return Surge::GUI::Reference::applyWavetableSelection(
        processor, request,
        [this](int scene, int oscillator, int id, const fs::path &path) {
            auto &storage = processor.surge->storage;
            sge->loadWavetableScript(id, path, &storage,
                                     &storage.getPatch().scene[scene].osc[oscillator]);
        },
        [this](const std::string &announcement) {
            sge->enqueueAccessibleAnnouncement(announcement);
        });
}

void SurgeSynthEditor::handleReferenceWorkspaceAction(const juce::String &action)
{
    if (!sge || !processor.surge)
        return;
    if (action == "settings" || action == "skins")
    {
        showReferenceSkinMenu();
    }
    else if (action == "scene-a" || action == "scene-b")
    {
        const auto scene = action == "scene-a" ? 0 : 1;
        if (sge->getCurrentScene() != scene)
            sge->selectSceneFromReferenceWorkspace(scene);
        if (referenceWorkspace)
            referenceWorkspace->selectScene(scene, false);
    }
    else if (action.startsWith("load-preset:"))
    {
        const auto suffix = action.substring(12);
        // String::getIntValue accepts trailing junk and can overflow. Only a
        // complete, non-negative decimal catalog ID is an actionable selection.
        if (suffix.isEmpty() || suffix.length() > 10 || !suffix.containsOnly("0123456789"))
            return;
        const auto index = suffix.getLargeIntValue();
        auto &catalog = processor.surge->storage.patch_list;
        if (index > std::numeric_limits<int>::max() ||
            static_cast<uint64_t>(index) >= catalog.size())
            return;
        const auto path = catalog[static_cast<size_t>(index)].path;
        if (path.empty())
            return;

        // The stock editor owns the dirty-patch confirmation. Never hide its
        // prompt underneath the alternative workspace.
        if (processor.surge->storage.getPatch().isDirty)
            showReferenceWorkspace(false);
        const juce::Component::SafePointer<SurgeSynthEditor> safe(this);
        sge->loadPatchWithDirtyCheck([safe, index, path]() {
            if (!safe || !safe->sge || !safe->processor.surge)
                return;
            auto &synth = *safe->processor.surge;
            auto &currentCatalog = synth.storage.patch_list;
            auto resolved = static_cast<int>(index);
            // A rescan while the confirmation is open can move catalog indices.
            // Keep the user's selected file, never a new patch occupying its row.
            if (static_cast<size_t>(resolved) >= currentCatalog.size() ||
                currentCatalog[static_cast<size_t>(resolved)].path != path)
            {
                auto found =
                    std::find_if(currentCatalog.begin(), currentCatalog.end(),
                                 [&path](const auto &patch) { return patch.path == path; });
                if (found == currentCatalog.end())
                    return;
                resolved = static_cast<int>(std::distance(currentCatalog.begin(), found));
            }
            if (safe->referenceWorkspace)
                safe->referenceWorkspace->bridge().endAll();
            safe->sge->closeOverlay(SurgeGUIEditor::SAVE_PATCH);
            safe->sge->undoManager()->pushPatch();
            synth.patchSelectedFromFavorites = false;
            safe->sge->enqueuePatchId = resolved;
            safe->sge->flushEnqueuedPatchId();
        });
    }
    else if (action == "next-preset" || action == "previous-preset")
    {
        // The native dirty-patch confirmation belongs to the full editor.
        if (processor.surge->storage.getPatch().isDirty)
            showReferenceWorkspace(false);
        const juce::Component::SafePointer<SurgeSynthEditor> safe(this);
        const bool increment = action == "next-preset";
        sge->loadPatchWithDirtyCheck([safe, increment]() {
            if (!safe || !safe->sge || !safe->processor.surge)
                return;
            if (safe->referenceWorkspace)
                safe->referenceWorkspace->bridge().endAll();
            safe->sge->undoManager()->pushPatch();
            safe->sge->closeOverlay(SurgeGUIEditor::SAVE_PATCH);
            safe->processor.surge->jogPatchOrCategory(increment, false);
        });
    }
    else if (action == "browser" || action == "save" || action == "modulation" ||
             action == "matrix" || action == "analyzer" || action == "effects" ||
             action == "full-editor")
    {
        showReferenceWorkspace(false);
        if (action == "browser")
            sge->showOverlay(SurgeGUIEditor::PATCH_BROWSER);
        else if (action == "save")
            sge->showOverlay(SurgeGUIEditor::SAVE_PATCH);
        else if (action == "modulation" || action == "matrix")
            sge->showOverlay(SurgeGUIEditor::MODULATION_EDITOR);
        else if (action == "analyzer")
            sge->showOverlay(SurgeGUIEditor::OSCILLOSCOPE);
    }
}

void SurgeSynthEditor::resized()
{
    if (referenceWorkspace && referenceWorkspace->isVisible())
    {
        referenceWorkspace->setBounds(getLocalBounds());
        // Skin reload may temporarily size the hidden original editor. Do not
        // mistake that for a user resize of the currently displayed workspace.
        if (rezoomGuard == 0 && sge->currentSkin &&
            referenceSkinKey == juce::String(sge->currentSkin->root + sge->currentSkin->name))
            referenceWorkspaceSize = {getWidth(), getHeight()};
        if (getConstrainer())
            getConstrainer()->setFixedAspectRatio(1586.0 / 992.0);
        return;
    }

    // The return action occupies its own footer, never an invisible hit target
    // over a stock control. Normal skins have no footer and follow the old path.
    const auto footer = referenceWorkspace && returnToReferenceWorkspace ? 28 : 0;
    const auto contentBounds = getLocalBounds().withTrimmedBottom(footer);
    if (returnToReferenceWorkspace)
        returnToReferenceWorkspace->setBounds(getLocalBounds().removeFromBottom(footer).reduced(4, 2));
    topLevelContainer->setBounds(contentBounds);
    drawExtendedControls = sge->getShowVirtualKeyboard();

    auto w = contentBounds.getWidth();
    auto h =
        contentBounds.getHeight() -
        (drawExtendedControls ? 0.01 * sge->getZoomFactor() * extraYSpaceForVirtualKeyboard : 0);

    if (Surge::GUI::getIsStandalone())
    {
        juce::Component *comp = this;
        while (comp)
        {
            auto *cdw = dynamic_cast<juce::ResizableWindow *>(comp);
            if (cdw)
            {
                if (cdw->isFullScreen())
                {
                    auto b = contentBounds;
                    auto xw = 1.f * sge->getWindowSizeX() / b.getWidth();
                    auto xh = 1.f *
                              (sge->getWindowSizeY() +
                               (drawExtendedControls ? extraYSpaceForVirtualKeyboard : 0)) /
                              b.getHeight();

                    auto nz = std::min(1.0 / xw, 1.0 / xh);
                    auto snz = nz / sge->getZoomFactor() * 100.f;

                    topLevelContainer->setTransform(juce::AffineTransform().scaled(snz));

                    // target width
                    auto tw = sge->getWindowSizeX() * sge->getZoomFactor() * 0.01 * snz;
                    auto th = (sge->getWindowSizeY() +
                               (drawExtendedControls ? extraYSpaceForVirtualKeyboard : 0)) *
                              sge->getZoomFactor() * 0.01 * snz;

                    auto pw = (b.getWidth() - tw) / 2.0;
                    auto ph = (b.getHeight() - th) / 2.0;

                    // turn off aspect ratio
                    if (getConstrainer())
                        getConstrainer()->setFixedAspectRatio(0.f);

                    sge->moveTopLeftTo(std::round(pw / snz), std::round(ph / snz));
                    return;
                }
                comp = nullptr;
            }
            else
            {
                comp = comp->getParentComponent();
            }
        }

        sge->moveTopLeftTo(0, 0);
    }

    topLevelContainer->setTransform(juce::AffineTransform());

    auto b = contentBounds;
    auto wR = 1.0 * w / sge->getWindowSizeX();
    auto hR = 1.0 * h / sge->getWindowSizeY();

    auto ar = 1.f * sge->getWindowSizeX() /
              (sge->getWindowSizeY() + (drawExtendedControls ? extraYSpaceForVirtualKeyboard : 0));
    if (getConstrainer())
        getConstrainer()->setFixedAspectRatio(footer > 0 ? w / (w / ar + footer) : ar);

    auto zfn = std::min(wR, hR);
    if (wR < 1 && hR < 1)
        zfn = std::max(wR, hR);
    if ((wR - 1) * (hR - 1) < 0)
        zfn = std::min(zfn, 1.0);

    zfn = 100.0 * zfn / sge->getZoomFactor();

    float applyZoomFactor = sge->getZoomFactor() * 0.01;
    if (!rezoomGuard)
        applyZoomFactor *= zfn;

    bool addTempo = processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;

    if (drawExtendedControls)
    {
        auto y = sge->getWindowSizeY();
        auto x = addTempo ? 50 : 0;
        auto wheels = 32;
        auto margin = 6;
        int noTempoSusYOffset = -16;
        int tempoHeight = 10, typeinHeight = 14;
        int tempoBlockHeight = tempoHeight + typeinHeight;

        auto xf = juce::AffineTransform().scaled(applyZoomFactor);
        auto r = juce::Rectangle<int>(x + wheels + margin, y,
                                      sge->getWindowSizeX() - x - wheels - margin,
                                      extraYSpaceForVirtualKeyboard);

        keyboard->setBounds(r);
        keyboard->setTransform(xf);
        keyboard->setVisible(true);

        auto pmr = juce::Rectangle<int>(x, y, wheels / 2, extraYSpaceForVirtualKeyboard);
        pitchwheel->setBounds(pmr);
        pitchwheel->setTransform(xf);
        pitchwheel->setVisible(true);
        pmr = pmr.translated((wheels / 2) + margin / 3, 0);
        modwheel->setBounds(pmr);
        modwheel->setTransform(xf);
        modwheel->setVisible(true);

        if (addTempo)
        {
            tempoLabel->setBounds(4, y, x - 8, tempoHeight);
            tempoLabel->setFont(sge->currentSkin->fontManager->getLatoAtSize(8, juce::Font::bold));
            tempoLabel->setJustificationType(juce::Justification::centred);
            tempoLabel->setTransform(xf);
            tempoLabel->setVisible(addTempo);

            tempoTypein->setBounds(4, y + tempoHeight, x - 8, typeinHeight);
            tempoTypein->setText(
                std::to_string((int)(processor.surge->storage.temposyncratio * 120)));
            tempoTypein->setFont(sge->currentSkin->fontManager->getLatoAtSize(9));
            tempoTypein->setIndents(4, -1);
            tempoTypein->setJustification(juce::Justification::centred);
            tempoTypein->setTransform(xf);
            tempoTypein->setVisible(addTempo);
        }

        auto sml = juce::Rectangle<int>(4, y + tempoBlockHeight, x - 8, tempoHeight);
        sml.translate(0, addTempo ? 0 : noTempoSusYOffset);
        sustainLabel->setBounds(sml);
        sustainLabel->setFont(sge->currentSkin->fontManager->getLatoAtSize(8, juce::Font::bold));
        sustainLabel->setJustificationType(juce::Justification::centred);
        sustainLabel->setTransform(xf);
        sustainLabel->setVisible(true);

        auto smr =
            juce::Rectangle<int>(4, y + tempoBlockHeight + tempoHeight, x - 8, typeinHeight / 2);
        smr = smr.withBottom(pmr.getBottom() - 1).translated(0, addTempo ? 0 : noTempoSusYOffset);
        suspedal->setBounds(smr);
        suspedal->setTransform(xf);
        suspedal->setVisible(true);
    }
    else
    {
        keyboard->setVisible(false);
        tempoLabel->setVisible(false);
        tempoTypein->setVisible(false);
    }

    if (zfn != 1.0 && rezoomGuard == 0)
    {
        auto br = BlockRezoom(this);
        sge->setZoomFactor(round(sge->getZoomFactor() * zfn), false);
    }
}

void SurgeSynthEditor::parentHierarchyChanged()
{
    reapplySurgeComponentColours();

#if WINDOWS
    auto swr = Surge::Storage::getUserDefaultValue(&(this->processor.surge->storage),
                                                   Surge::Storage::UseSoftwareRenderer, false);

    if (swr)
    {
        if (auto peer = getPeer())
        {
            // 0 for software mode, 1 for Direct2D mode
            peer->setCurrentRenderingEngine(0);
        }
    }
#endif
}

void SurgeSynthEditor::IdleTimer::timerCallback() { ed->idle(); }

void SurgeSynthEditor::populateForStreaming(SurgeSynthesizer *s)
{
    if (sge)
        sge->populateDawExtraState(s);
}

void SurgeSynthEditor::populateFromStreaming(SurgeSynthesizer *s)
{
    if (sge)
        sge->loadFromDawExtraState(s);
}

bool SurgeSynthEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    if (files.size() != 1)
        return false;

    auto *fxw =
        dynamic_cast<Surge::Widgets::CurrentFxDisplay *>(sge->frame->getControlGroupLayer(cg_FX));

    for (auto i = files.begin(); i != files.end(); ++i)
    {
        if (fxw->canDropTarget(*i))
            return true;

        if (sge->canDropTarget(i->toStdString()))
            return true;
    }
    return false;
}

void SurgeSynthEditor::filesDropped(const juce::StringArray &files, int x, int y)
{
    if (files.size() != 1)
        return;

    auto *fxw =
        dynamic_cast<Surge::Widgets::CurrentFxDisplay *>(sge->frame->getControlGroupLayer(cg_FX));

    for (auto i = files.begin(); i != files.end(); ++i)
    {
        if (fxw->canDropTarget(*i) &&
            fxw->reallyContains(fxw->getLocalPoint(this, juce::Point<int>(x, y)), true))
            fxw->onDrop(*i);
        else if (sge->canDropTarget(i->toStdString()))
            sge->onDrop(i->toStdString());
    }
}

void SurgeSynthEditor::beginParameterEdit(Parameter *p)
{
    // std::cout << "BEGIN EDIT " << p->get_name() << std::endl;
    auto par = processor.paramsByID[processor.surge->idForParameter(p)];
    par->inEditGesture = true;
    par->beginChangeGesture();
}

void SurgeSynthEditor::endParameterEdit(Parameter *p)
{
    auto par = processor.paramsByID[processor.surge->idForParameter(p)];
    par->inEditGesture = false;
    par->endChangeGesture();
    if (fireListenersOnEndEdit)
        processor.paramChangeToListeners(p);
}

void SurgeSynthEditor::beginMacroEdit(long macroNum)
{
    auto par = processor.macrosById[macroNum];
    par->beginChangeGesture();
}

void SurgeSynthEditor::endMacroEdit(long macroNum)
{
    auto par = processor.macrosById[macroNum];
    par->endChangeGesture();
    // echo change to OSC out
    float newval = par->getValue();
    processor.paramChangeToListeners(nullptr, true, processor.SCT_MACRO, (float)macroNum, newval,
                                     .0, "");
}

#if LINUX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

juce::PopupMenu SurgeSynthEditor::modifyHostMenu(juce::PopupMenu menu)
{
    // make things look a bit nicer for our friends from Image-Line
    if (juce::PluginHostType().isFruityLoops())
    {
        auto it = juce::PopupMenu::MenuItemIterator(menu);

        while (it.next())
        {
            auto txt = it.getItem().text;

            if (txt.startsWithChar('-'))
            {
                it.getItem().isSectionHeader = true;
                it.getItem().text = txt.fromFirstOccurrenceOf("-", false, false);
            }
        }

        return menu;
    }

    // we really don't need that parameter name repeated in Reaper...
    if (juce::PluginHostType().isReaper())
    {
        auto newMenu = juce::PopupMenu();
        auto it = juce::PopupMenu::MenuItemIterator(menu);

        while (it.next())
        {
            auto txt = it.getItem().text;
            bool include = true;

            if (txt.startsWithChar('[') && txt.endsWithChar(']'))
            {
                include = it.next();
            }

            if (include)
            {
                newMenu.addItem(it.getItem());
            }
        }

        return newMenu;
    }

    return menu;
}

juce::PopupMenu SurgeSynthEditor::hostMenuFor(Parameter *p)
{
    auto par = processor.paramsByID[processor.surge->idForParameter(p)];

    if (auto *c = getHostContext())
    {
        if (auto menuInfo = c->getContextMenuForParameterIndex(par))
        {
            auto menu = menuInfo->getEquivalentPopupMenu();

            return modifyHostMenu(menu);
        }
    }

    return juce::PopupMenu();
}

juce::PopupMenu SurgeSynthEditor::hostMenuForMacro(int macro)
{
    auto par = processor.macrosById[macro];

    if (auto *c = getHostContext())
    {
        if (auto menuInfo = c->getContextMenuForParameterIndex(par))
        {
            auto menu = menuInfo->getEquivalentPopupMenu();

            return modifyHostMenu(menu);
        }
    }

    return juce::PopupMenu();
}

#if LINUX
#pragma GCC diagnostic pop
#endif

bool SurgeSynthEditor::keyPressed(const juce::KeyPress &key, juce::Component *orig)
{
    if (referenceWorkspace && key.getModifiers().isCommandDown() &&
        key.getModifiers().isAltDown() && key.getKeyCode() == 'R')
    {
        showReferenceWorkspace(!referenceWorkspace->isVisible());
        return true;
    }
    if (referenceWorkspace && referenceWorkspace->isVisible())
        return false;

    /*
     * So, sigh. On Linux/Windows the keyStateChanged event of the parent isn't suppressed
     * when the keyStateChange is false. But the MIDI keyboard rescans on state change.
     * So what we need to do is: forward all keystate trues (which will come before a
     * keypress but only if the keypress is not in a text edit) but only forward keystate
     * false if I have sent at least one key through this fallthrough mechanism. So:
     */
    if (sge->getShowVirtualKeyboard())
    {
        bool shortcutsUsed = sge->getUseKeyboardShortcuts();
        auto mapMatch = sge->keyMapManager->matches(key);

        if (mapMatch.has_value())
        {
            if (shortcutsUsed)
            {
                auto action = *mapMatch;

                switch (action)
                {
                case Surge::GUI::VKB_OCTAVE_DOWN:
                    midiKeyboardOctave = std::clamp(midiKeyboardOctave - 1, 0, 9);
                    keyboard->setKeyPressBaseOctave(midiKeyboardOctave);
                    return true;
                case Surge::GUI::VKB_OCTAVE_UP:
                    midiKeyboardOctave = std::clamp(midiKeyboardOctave + 1, 0, 9);
                    keyboard->setKeyPressBaseOctave(midiKeyboardOctave);
                    return true;
                case Surge::GUI::VKB_VELOCITY_DOWN_10PCT:
                    midiKeyboardVelocity = std::clamp(midiKeyboardVelocity - 0.1f, 0.f, 1.f);
                    keyboard->setVelocity(midiKeyboardVelocity, true);
                    return true;
                case Surge::GUI::VKB_VELOCITY_UP_10PCT:
                    midiKeyboardVelocity = std::clamp(midiKeyboardVelocity + 0.1f, 0.f, 1.f);
                    keyboard->setVelocity(midiKeyboardVelocity, true);
                    return true;
                default:
                    break;
                }
            }
        }

        if (vkbShouldTakeKeys() && orig != keyboard.get())
        {
            return keyboard->keyPressed(key);
        }
    }

    return false;
}

bool SurgeSynthEditor::keyStateChanged(bool isKeyDown, juce::Component *originatingComponent)
{
    // TextEditor::keyStateChanged returns false on key up, so without vkbShouldTakeKeys() here
    // the BPM field's key ups would still bubble up and rescan
    if (vkbShouldTakeKeys() && originatingComponent != keyboard.get())
    {
        return keyboard->keyStateChanged(isKeyDown);
    }

    return false;
}

void SurgeSynthEditor::setPitchModSustainGUI(int pitch, int mod, int sus)
{
    auto pw = dynamic_cast<VKeyboardWheel *>(pitchwheel.get());
    if (pw)
    {
        pw->value = pitch;
        pw->repaint();
    }
    auto mw = dynamic_cast<VKeyboardWheel *>(modwheel.get());
    if (mw)
    {
        mw->value = mod;
        mw->repaint();
    }
    auto sw = dynamic_cast<VKeyboardSus *>(suspedal.get());
    if (sw)
    {
        sw->isOn = sus > 64;
        sw->repaint();
    }
}
