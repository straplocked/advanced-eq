#pragma once

#include "PluginProcessor.h"
#include "ui/components/EqCurveDisplay.h"
#include "ui/components/BandControlPanel.h"
#include "ui/LookAndFeel/AdvancedEqLookAndFeel.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

class AdvancedEQEditor : public juce::AudioProcessorEditor
{
public:
    explicit AdvancedEQEditor (AdvancedEQProcessor&);
    ~AdvancedEQEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AdvancedEQProcessor& processorRef;
    AdvancedEqLookAndFeel lookAndFeel;

    // Main components
    EqCurveDisplay curveDisplay;
    BandControlPanel bandControlPanel;

    void updateBandControlPanelPosition();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQEditor)
};
