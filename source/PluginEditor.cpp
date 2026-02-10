#include "PluginEditor.h"
#include "utils/Constants.h"
#include "ui/LookAndFeel/ColourPalette.h"

AdvancedEQEditor::AdvancedEQEditor (AdvancedEQProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      curveDisplay (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (curveDisplay);

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

    // Top bar with title
    auto topBar = getLocalBounds().removeFromTop (36);
    g.setColour (ColourPalette::presetBar());
    g.fillRect (topBar);

    g.setColour (ColourPalette::textBright());
    g.setFont (16.0f);
    g.drawText ("Advanced EQ", topBar, juce::Justification::centred, true);

    // Bottom control bar background
    auto bottomBar = getLocalBounds().removeFromBottom (36);
    g.setColour (ColourPalette::controlBar());
    g.fillRect (bottomBar);

    // Bottom bar labels
    g.setColour (ColourPalette::textDim());
    g.setFont (11.0f);
    g.drawText ("Click to add band  |  Drag to adjust  |  Scroll for Q  |  Double-click to remove",
                bottomBar, juce::Justification::centred, false);
}

void AdvancedEQEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (36);    // Top bar
    bounds.removeFromBottom (36); // Bottom bar

    curveDisplay.setBounds (bounds);
}
