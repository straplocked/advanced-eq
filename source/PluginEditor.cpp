#include "PluginEditor.h"
#include "utils/Constants.h"
#include "utils/MathUtils.h"
#include "parameters/ParameterIDs.h"
#include "ui/LookAndFeel/ColourPalette.h"

AdvancedEQEditor::AdvancedEQEditor (AdvancedEQProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      curveDisplay (p),
      bandControlPanel (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (curveDisplay);
    addAndMakeVisible (bandControlPanel);

    // Wire band selection callback to show/hide/reposition control panel
    curveDisplay.onBandSelected = [this] (int band)
    {
        bandControlPanel.setSelectedBand (band);
        updateBandControlPanelPosition();
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
    updateBandControlPanelPosition();
}

void AdvancedEQEditor::updateBandControlPanelPosition()
{
    int band = bandControlPanel.getSelectedBand();
    if (band < 0)
        return;

    auto& apvts = processorRef.getAPVTS();
    auto* freqParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kFrequency));
    auto* gainParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kGain));

    float freq = freqParam->load();
    float gain = gainParam->load();

    // Map to pixel coordinates within the curve display
    auto curveBounds = curveDisplay.getBounds();
    float x = MathUtils::frequencyToX (freq, (float) curveBounds.getWidth()) + curveBounds.getX();
    float y = MathUtils::gainToY (gain, (float) curveBounds.getHeight(), -30.0f, 30.0f) + curveBounds.getY();

    bandControlPanel.positionNear (x, y, curveBounds);
}
