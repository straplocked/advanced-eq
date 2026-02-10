#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../dsp/FilterTypes.h"
#include "../LookAndFeel/ColourPalette.h"

class BottomControlStrip : public juce::Component,
                           public juce::Timer
{
public:
    explicit BottomControlStrip (AdvancedEQProcessor& processor);
    ~BottomControlStrip() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setSelectedBand (int bandIndex);
    int getSelectedBand() const { return selectedBand; }

private:
    AdvancedEQProcessor& processor;
    int selectedBand = -1;

    // Band indicator
    juce::Label bandLabel;

    // Selectors
    juce::ComboBox typeSelector;
    juce::ComboBox slopeSelector;
    juce::ComboBox stereoSelector;

    // Rotary knobs
    juce::Slider freqSlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;

    // Value labels under knobs
    juce::Label freqLabel;
    juce::Label gainLabel;
    juce::Label qLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;

    void updateAttachments();
    void updateSlopeEnabled();
    void updateValueLabels();
    void setControlsVisible (bool visible);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomControlStrip)
};
