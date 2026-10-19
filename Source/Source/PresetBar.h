#pragma once

#include <JuceHeader.h>
#include "BinaryData.h"

/*  Preset manager shown at the top of the editor.

    - Factory presets are compiled into the plugin (BinaryData).
    - User presets are .xml files in the user preset folder:
        macOS    ~/Library/Audio/Presets/Ticco Ross/PitchShifter
        Windows  Documents\Ticco Ross\PitchShifter\Presets
        Linux    ~/.local/share/Ticco Ross/PitchShifter/Presets
    - The current preset name is stored in the plugin state ("presetName"),
      so it is restored with the DAW session.
*/
class PresetBar : public juce::Component,
                  private juce::ValueTree::Listener
{
public:
    explicit PresetBar (juce::AudioProcessorValueTreeState& vts)
        : state (vts)
    {
        for (auto* b : { &prevButton, &nextButton })
        {
            addAndMakeVisible (*b);
            b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff171717));
            b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2a2a2a));
            b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
            b->setColour (juce::ComboBox::outlineColourId, juce::Colours::whitesmoke);
        }
        prevButton.onClick = [this] { step (-1); };
        nextButton.onClick = [this] { step (+1); };
        prevButton.setTooltip ("Previous preset");
        nextButton.setTooltip ("Next preset");

        addAndMakeVisible (presetBox);
        presetBox.setJustificationType (juce::Justification::centred);
        presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff171717));
        presetBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
        presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::whitesmoke);
        presetBox.setColour (juce::ComboBox::arrowColourId, juce::Colours::whitesmoke);
        presetBox.setTextWhenNothingSelected ("Default");
        presetBox.onChange = [this] { comboChanged(); };

        state.state.addListener (this);
        refreshList();
    }

    ~PresetBar() override
    {
        state.state.removeListener (this);
    }

    //==============================================================================
    /** Asks for a name and saves the current settings as a user preset. */
    void saveAs()
    {
        auto current = getCurrentName();
        dialog = std::make_unique<juce::AlertWindow> ("Save preset", "Preset name:",
                                                      juce::MessageBoxIconType::NoIcon, this);
        dialog->addTextEditor ("name", isUser (current) ? current : juce::String(), {});
        dialog->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        juce::Component::SafePointer<PresetBar> safe (this);
        dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe] (int result)
        {
            if (safe == nullptr) return;
            auto name = juce::File::createLegalFileName (safe->dialog->getTextEditorContents ("name").trim());
            safe->dialog.reset();

            if (result == 1 && name.isNotEmpty())
                safe->confirmAndWrite (name);
        }), false);
    }

    /** Lets the user pick a preset file from disk, copies it into the user folder and loads it. */
    void importPreset()
    {
        chooser = std::make_unique<juce::FileChooser> ("Import preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.xml");

        juce::Component::SafePointer<PresetBar> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            auto file = fc.getResult();
            if (! file.existsAsFile()) return;

            juce::MemoryBlock data;
            file.loadFileAsData (data);
            auto xml = parsePreset (data);
            if (xml == nullptr)
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "Import preset",
                                                        "This file is not a PitchShifter preset.");
                return;
            }

            auto name = file.getFileNameWithoutExtension();
            auto dest = userFolder().getChildFile (name + ".xml");
            if (dest != file)
                file.copyFileTo (dest);

            safe->refreshList();
            safe->applyXml (*xml, userPrefix + name);
        });
    }

    static juce::File userFolder()
    {
       #if JUCE_MAC
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                       .getChildFile ("Library/Audio/Presets/Ticco Ross/PitchShifter");
       #elif JUCE_WINDOWS
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                       .getChildFile ("Ticco Ross/PitchShifter/Presets");
       #else
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                       .getChildFile (".local/share/Ticco Ross/PitchShifter/Presets");
       #endif
        dir.createDirectory();
        return dir;
    }

    //==============================================================================
    void resized() override
    {
        auto r = getLocalBounds().reduced (0, 6);
        prevButton.setBounds (r.removeFromLeft (36));
        nextButton.setBounds (r.removeFromRight (36));
        presetBox.setBounds (r.reduced (6, 0));
    }

private:
    // Item IDs: factory presets 1..999, user presets 1000..8999, menu actions 9000+.
    enum { userBase = 1000, actionSave = 9001, actionImport, actionDelete, actionShowFolder };
    static inline const juce::String userPrefix { "user:" };

    struct Entry { juce::String key, label; };   // key: factory name or "user:<name>"

    //==============================================================================
    static std::unique_ptr<juce::XmlElement> parsePreset (const juce::MemoryBlock& data)
    {
        // Accept both plain XML and JUCE's binary state format ("VC2!").
        auto xml = juce::AudioProcessor::getXmlFromBinary (data.getData(), (int) data.getSize());
        if (xml == nullptr)
            xml = juce::parseXML (data.toString());
        if (xml != nullptr && xml->hasTagName ("APVTS"))
            return xml;
        return nullptr;
    }

    static juce::String factoryName (const juce::String& fileName)
    {
        auto base = fileName.upToLastOccurrenceOf (".", false, false).replaceCharacter ('-', ' ');
        juce::String out;
        for (auto word : juce::StringArray::fromTokens (base, " ", ""))
            out << (out.isEmpty() ? "" : " ") << word.substring (0, 1).toUpperCase() << word.substring (1);
        return out;
    }

    void refreshList()
    {
        entries.clear();
        presetBox.clear (juce::dontSendNotification);

        presetBox.addSectionHeading ("Factory");
        juce::Array<Entry> factory;
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            auto file = juce::String (BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]));
            if (file.endsWithIgnoreCase (".xml"))
                factory.add ({ factoryName (file), factoryName (file) });
        }
        std::sort (factory.begin(), factory.end(), [] (const Entry& a, const Entry& b) { return a.label < b.label; });
        for (auto& e : factory) { entries.add (e); presetBox.addItem (e.label, entries.size()); }

        auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*.xml");
        files.sort();
        if (! files.isEmpty())
        {
            presetBox.addSeparator();
            presetBox.addSectionHeading ("User");
            for (auto& f : files)
            {
                entries.add ({ userPrefix + f.getFileNameWithoutExtension(), f.getFileNameWithoutExtension() });
                presetBox.addItem (f.getFileNameWithoutExtension(), userBase + entries.size());
            }
        }

        presetBox.addSeparator();
        presetBox.addItem ("Save As...", actionSave);
        presetBox.addItem ("Import...", actionImport);
        presetBox.addItem ("Delete Current Preset", actionDelete);
        presetBox.setItemEnabled (actionDelete, isUser (getCurrentName()));
        presetBox.addItem ("Show Preset Folder", actionShowFolder);

        showCurrent();
    }

    int idForEntry (int index) const
    {
        return entries[index].key.startsWith (userPrefix) ? userBase + index + 1 : index + 1;
    }

    int indexForId (int id) const { return (id >= userBase ? id - userBase : id) - 1; }

    void showCurrent()
    {
        auto current = getCurrentName();
        for (int i = 0; i < entries.size(); ++i)
            if (entries[i].key == current)
            {
                presetBox.setSelectedId (idForEntry (i), juce::dontSendNotification);
                return;
            }

        presetBox.setSelectedId (0, juce::dontSendNotification);
        presetBox.setText (current.isEmpty() ? "Default" : current.fromFirstOccurrenceOf (userPrefix, false, false),
                           juce::dontSendNotification);
    }

    void comboChanged()
    {
        const int id = presetBox.getSelectedId();
        if (id == 0) return;

        if (id >= actionSave)
        {
            showCurrent();   // put the name back; the actions are not presets
            switch (id)
            {
                case actionSave:       saveAs(); break;
                case actionImport:     importPreset(); break;
                case actionDelete:     deleteCurrent(); break;
                case actionShowFolder: userFolder().revealToUser(); break;
                default: break;
            }
            return;
        }

        loadEntry (indexForId (id));
    }

    void step (int delta)
    {
        if (entries.isEmpty()) return;

        int index = -1;
        for (int i = 0; i < entries.size(); ++i)
            if (entries[i].key == getCurrentName()) index = i;

        index = index < 0 ? (delta > 0 ? 0 : entries.size() - 1)
                          : (index + delta + entries.size()) % entries.size();
        loadEntry (index);
    }

    void loadEntry (int index)
    {
        if (! juce::isPositiveAndBelow (index, entries.size())) return;
        auto& e = entries.getReference (index);

        juce::MemoryBlock data;
        if (e.key.startsWith (userPrefix))
        {
            userFolder().getChildFile (e.label + ".xml").loadFileAsData (data);
        }
        else
        {
            for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            {
                auto file = juce::String (BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]));
                if (factoryName (file) == e.key)
                {
                    int size = 0;
                    if (auto* res = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                        data.append (res, (size_t) size);
                }
            }
        }

        if (auto xml = parsePreset (data))
            applyXml (*xml, e.key);
        else
            refreshList();   // file vanished or is broken
    }

    void applyXml (const juce::XmlElement& xml, const juce::String& key)
    {
        auto tree = juce::ValueTree::fromXml (xml);
        tree.setProperty ("presetName", key, nullptr);
        state.replaceState (tree);
        if (state.undoManager != nullptr)
            state.undoManager->clearUndoHistory();
        showCurrent();
        presetBox.setItemEnabled (actionDelete, isUser (key));
    }

    void confirmAndWrite (const juce::String& name)
    {
        auto file = userFolder().getChildFile (name + ".xml");
        if (! file.existsAsFile())
        {
            write (name);
            return;
        }

        juce::Component::SafePointer<PresetBar> safe (this);
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::NoIcon, "Save preset",
                                            "A preset called \"" + name + "\" already exists. Replace it?",
                                            "Replace", "Cancel", this,
                                            juce::ModalCallbackFunction::create ([safe, name] (int ok)
                                            {
                                                if (safe != nullptr && ok == 1) safe->write (name);
                                            }));
    }

    void write (const juce::String& name)
    {
        auto tree = state.copyState();
        tree.setProperty ("presetName", userPrefix + name, nullptr);
        if (auto xml = tree.createXml())
            xml->writeTo (userFolder().getChildFile (name + ".xml"));

        state.state.setProperty ("presetName", userPrefix + name, nullptr);
        refreshList();
    }

    void deleteCurrent()
    {
        auto current = getCurrentName();
        if (! isUser (current)) return;
        auto name = current.fromFirstOccurrenceOf (userPrefix, false, false);

        juce::Component::SafePointer<PresetBar> safe (this);
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::NoIcon, "Delete preset",
                                            "Delete the preset \"" + name + "\"?",
                                            "Delete", "Cancel", this,
                                            juce::ModalCallbackFunction::create ([safe, name] (int ok)
                                            {
                                                if (safe == nullptr || ok != 1) return;
                                                auto f = userFolder().getChildFile (name + ".xml");
                                                if (! f.moveToTrash())
                                                    f.deleteFile();
                                                safe->state.state.setProperty ("presetName", juce::String(), nullptr);
                                                safe->refreshList();
                                            }));
    }

    juce::String getCurrentName() const { return state.state.getProperty ("presetName").toString(); }
    static bool isUser (const juce::String& key) { return key.startsWith (userPrefix); }

    // The DAW can restore a session (and its preset name) while the editor is open.
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& id) override
    {
        if (id == juce::Identifier ("presetName"))
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<PresetBar> (this)]
                                             { if (safe != nullptr) safe->showCurrent(); });
    }
    void valueTreeRedirected (juce::ValueTree&) override
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<PresetBar> (this)]
                                         { if (safe != nullptr) safe->showCurrent(); });
    }

    juce::AudioProcessorValueTreeState& state;

    juce::TextButton prevButton { juce::CharPointer_UTF8 ("\xe2\x97\x80") },   // ◀
                     nextButton { juce::CharPointer_UTF8 ("\xe2\x96\xb6") };   // ▶
    juce::ComboBox presetBox;
    juce::Array<Entry> entries;

    std::unique_ptr<juce::AlertWindow> dialog;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
