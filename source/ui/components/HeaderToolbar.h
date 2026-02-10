#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../presets/PresetManager.h"
#include "../LookAndFeel/ColourPalette.h"

class HeaderToolbar : public juce::Component,
                      public PresetManager::Listener
{
public:
    explicit HeaderToolbar (AdvancedEQProcessor& processor);
    ~HeaderToolbar() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // PresetManager::Listener
    void presetChanged() override;

private:
    AdvancedEQProcessor& processor;
    PresetManager& presetManager;

    // Preset controls
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton presetNameButton { "Init" };
    juce::TextButton saveButton { "Save" };

    // Right-side controls
    juce::TextButton analyzerButton { "Analyzer" };
    juce::TextButton freezeButton { "Freeze" };
    juce::ComboBox processModeSelector;
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    // Callback for freeze toggle
    public:
    std::function<void (bool)> onFreezeToggled;
    private:

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analyzerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    void showPresetMenu();
    void showSaveDialog();
    void updatePresetName();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderToolbar)
};
