#include "HeaderToolbar.h"

HeaderToolbar::HeaderToolbar (AdvancedEQProcessor& p)
    : processor (p)
{
    auto& apvts = processor.getAPVTS();

    // Analyzer on/off toggle
    analyzerButton.setClickingTogglesState (true);
    analyzerButton.setColour (juce::TextButton::buttonOnColourId, ColourPalette::compositeCurve().withAlpha (0.3f));
    addAndMakeVisible (analyzerButton);
    analyzerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kAnalyzerOn, analyzerButton);

    // Freeze toggle (UI-only, not a parameter)
    freezeButton.setClickingTogglesState (true);
    freezeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4488ff).withAlpha (0.3f));
    freezeButton.onClick = [this]
    {
        if (onFreezeToggled)
            onFreezeToggled (freezeButton.getToggleState());
    };
    addAndMakeVisible (freezeButton);

    // Processing mode selector
    processModeSelector.addItemList (ParamIDs::getProcessingModeNames(), 1);
    processModeSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (processModeSelector);
    processModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kProcessMode, processModeSelector);

    // Output gain knob
    outputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (outputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::kOutputGain, outputGainSlider);

    // Output gain label
    outputGainLabel.setText ("Out", juce::dontSendNotification);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    outputGainLabel.setFont (10.0f);
    outputGainLabel.setColour (juce::Label::textColourId, ColourPalette::axisText());
    addAndMakeVisible (outputGainLabel);
}

HeaderToolbar::~HeaderToolbar()
{
    analyzerAttachment.reset();
    processModeAttachment.reset();
    outputGainAttachment.reset();
}

void HeaderToolbar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Vertical gradient background
    juce::ColourGradient grad (ColourPalette::panelTop(), 0, bounds.getY(),
                               ColourPalette::panelBottom(), 0, bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (bounds);

    // Bottom edge bevel groove
    float bottomY = bounds.getBottom();
    g.setColour (ColourPalette::bevelLight());
    g.drawLine (0.0f, bottomY - 1.5f, bounds.getRight(), bottomY - 1.5f, 1.0f);
    g.setColour (ColourPalette::bevelDark());
    g.drawLine (0.0f, bottomY - 0.5f, bounds.getRight(), bottomY - 0.5f, 1.0f);

    // Title with drop shadow
    g.setFont (16.0f);
    g.setColour (juce::Colour (0x40000000));
    g.drawText ("Advanced EQ", 13, 1, 160, getHeight(), juce::Justification::centredLeft, true);
    g.setColour (ColourPalette::textBright());
    g.drawText ("Advanced EQ", 12, 0, 160, getHeight(), juce::Justification::centredLeft, true);
}

void HeaderToolbar::resized()
{
    auto bounds = getLocalBounds().reduced (8, 4);

    // Skip title area on left
    bounds.removeFromLeft (160);

    // Right side controls (laid out right-to-left)
    auto rightBounds = bounds;

    // Output gain knob + label (rightmost)
    auto outArea = rightBounds.removeFromRight (50);
    auto outKnob = outArea.removeFromTop (outArea.getHeight() - 2);
    outputGainSlider.setBounds (outKnob.withSizeKeepingCentre (28, 28));
    outputGainLabel.setBounds (outArea);

    rightBounds.removeFromRight (6);

    // Process mode selector
    auto modeArea = rightBounds.removeFromRight (110);
    processModeSelector.setBounds (modeArea.withHeight (22).withCentre (modeArea.getCentre()));

    rightBounds.removeFromRight (6);

    // Freeze button
    auto freezeArea = rightBounds.removeFromRight (48);
    freezeButton.setBounds (freezeArea.withHeight (22).withCentre (freezeArea.getCentre()));

    rightBounds.removeFromRight (4);

    // Analyzer on/off button
    auto analyzerArea = rightBounds.removeFromRight (62);
    analyzerButton.setBounds (analyzerArea.withHeight (22).withCentre (analyzerArea.getCentre()));
}
