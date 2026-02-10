#include "PluginEditor.h"
#include "utils/Constants.h"
#include "utils/MathUtils.h"
#include "parameters/ParameterIDs.h"
#include "ui/LookAndFeel/ColourPalette.h"

AdvancedEQEditor::AdvancedEQEditor (AdvancedEQProcessor& p)
    : AudioProcessorEditor (&p),
      headerToolbar (p),
      curveDisplay (p),
      controlStrip (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (headerToolbar);
    addAndMakeVisible (curveDisplay);
    addAndMakeVisible (controlStrip);

    // Wire band selection callback to bottom control strip
    curveDisplay.onBandSelected = [this] (int band)
    {
        controlStrip.setSelectedBand (band);
    };

    // Wire freeze toggle
    headerToolbar.onFreezeToggled = [this] (bool frozen)
    {
        curveDisplay.setAnalyzerFrozen (frozen);
    };

    setSize (Constants::kDefaultEditorWidth, Constants::kDefaultEditorHeight);
    setResizable (true, true);
    setResizeLimits (750, 450, 1920, 1080);
}

AdvancedEQEditor::~AdvancedEQEditor()
{
    setLookAndFeel (nullptr);
}

void AdvancedEQEditor::paint (juce::Graphics& g)
{
    g.fillAll (ColourPalette::background());
}

void AdvancedEQEditor::resized()
{
    auto bounds = getLocalBounds();

    headerToolbar.setBounds (bounds.removeFromTop (36));
    controlStrip.setBounds (bounds.removeFromBottom (90));
    curveDisplay.setBounds (bounds);
}
