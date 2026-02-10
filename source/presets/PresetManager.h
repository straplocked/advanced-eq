#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PresetData.h"
#include <vector>

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    const std::vector<Preset>& getAllPresets() const { return allPresets; }

    std::vector<juce::String> getCategories() const;
    std::vector<const Preset*> getPresetsInCategory (const juce::String& category) const;

    void applyPreset (int presetIndex);
    void nextPreset();
    void previousPreset();

    int getCurrentPresetIndex() const { return currentIndex; }
    juce::String getCurrentPresetName() const;

    // User preset management
    bool saveCurrentAsPreset (const juce::String& name);
    bool deleteUserPreset (int presetIndex);
    void rescanUserPresets();
    juce::File getUserPresetsDirectory() const;

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void presetChanged() = 0;
    };

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> allPresets;
    int currentIndex = -1;

    juce::ListenerList<Listener> listeners;

    void rebuildPresetList();
    std::vector<Preset> loadUserPresetsFromDisk() const;
    void resetAllBands();
    void setBandFromPreset (const BandPreset& bp);
    void applyUserPresetFromFile (const juce::File& file);
    void notifyListeners();
};
