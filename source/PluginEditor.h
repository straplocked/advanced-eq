#pragma once

#include "PluginProcessor.h"
#include "ui/components/EqCurveDisplay.h"
#include "ui/components/BottomControlStrip.h"
#include "ui/components/HeaderToolbar.h"
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
    AdvancedEqLookAndFeel lookAndFeel;

    // Main components
    HeaderToolbar headerToolbar;
    EqCurveDisplay curveDisplay;
    BottomControlStrip controlStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQEditor)
};
