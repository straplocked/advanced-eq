#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../dsp/FilterTypes.h"
#include "../LookAndFeel/ColourPalette.h"

class BandControlPanel : public juce::Component,
                          public juce::Timer
{
public:
    explicit BandControlPanel (AdvancedEQProcessor& processor);
    ~BandControlPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setSelectedBand (int bandIndex);
    int getSelectedBand() const { return selectedBand; }

    // Position the panel near the given point (band handle location)
    void positionNear (float x, float y, juce::Rectangle<int> parentBounds);

private:
    AdvancedEQProcessor& processor;
    int selectedBand = -1;

    juce::ComboBox typeSelector;
    juce::ComboBox slopeSelector;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;

    void updateAttachments();
    void updateSlopeEnabled();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandControlPanel)
};
