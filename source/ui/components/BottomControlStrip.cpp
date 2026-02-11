/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "BottomControlStrip.h"
#include "../../utils/FrequencyUtils.h"

BottomControlStrip::BottomControlStrip (AdvancedEQProcessor& p)
    : processor (p)
{
    // Band label (colour dot + number)
    bandLabel.setJustificationType (juce::Justification::centred);
    bandLabel.setFont (13.0f);
    bandLabel.setColour (juce::Label::textColourId, ColourPalette::textBright());
    addChildComponent (bandLabel);

    // Type selector
    typeSelector.addItemList (ParamIDs::getFilterTypeNames(), 1);
    typeSelector.setJustificationType (juce::Justification::centred);
    typeSelector.onChange = [this] { updateSlopeEnabled(); };
    addChildComponent (typeSelector);

    // Slope selector
    slopeSelector.addItemList (ParamIDs::getSlopeNames(), 1);
    slopeSelector.setJustificationType (juce::Justification::centred);
    addChildComponent (slopeSelector);

    // Stereo placement selector
    stereoSelector.addItemList (ParamIDs::getStereoPlacementNames(), 1);
    stereoSelector.setJustificationType (juce::Justification::centred);
    addChildComponent (stereoSelector);

    // Frequency knob
    freqSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    freqSlider.onValueChange = [this] { updateValueLabels(); };
    addChildComponent (freqSlider);

    // Gain knob
    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    gainSlider.onValueChange = [this] { updateValueLabels(); };
    addChildComponent (gainSlider);

    // Q knob
    qSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    qSlider.onValueChange = [this] { updateValueLabels(); };
    addChildComponent (qSlider);

    // Value labels
    auto setupLabel = [] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setFont (10.0f);
        label.setColour (juce::Label::textColourId, ColourPalette::axisText());
    };

    setupLabel (freqLabel);
    setupLabel (gainLabel);
    setupLabel (qLabel);

    addChildComponent (freqLabel);
    addChildComponent (gainLabel);
    addChildComponent (qLabel);

    // Dynamic EQ controls
    dynEnableButton.setClickingTogglesState (true);
    dynEnableButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff9e6b).withAlpha (0.3f));
    addChildComponent (dynEnableButton);

    auto setupDynKnob = [] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    };

    setupDynKnob (dynThreshSlider);
    setupDynKnob (dynRangeSlider);
    setupDynKnob (dynAttackSlider);
    setupDynKnob (dynReleaseSlider);

    addChildComponent (dynThreshSlider);
    addChildComponent (dynRangeSlider);
    addChildComponent (dynAttackSlider);
    addChildComponent (dynReleaseSlider);

    setupLabel (dynThreshLabel);
    setupLabel (dynRangeLabel);
    setupLabel (dynAttackLabel);
    setupLabel (dynReleaseLabel);

    addChildComponent (dynThreshLabel);
    addChildComponent (dynRangeLabel);
    addChildComponent (dynAttackLabel);
    addChildComponent (dynReleaseLabel);

    // Component IDs for help system (band controls)
    typeSelector.setComponentID ("bottom.typeSelector");
    slopeSelector.setComponentID ("bottom.slopeSelector");
    stereoSelector.setComponentID ("bottom.stereoSelector");
    freqSlider.setComponentID ("bottom.freqKnob");
    gainSlider.setComponentID ("bottom.gainKnob");
    qSlider.setComponentID ("bottom.qKnob");
    dynEnableButton.setComponentID ("bottom.dynEnable");
    dynThreshSlider.setComponentID ("bottom.dynThreshKnob");
    dynRangeSlider.setComponentID ("bottom.dynRangeKnob");
    dynAttackSlider.setComponentID ("bottom.dynAttackKnob");
    dynReleaseSlider.setComponentID ("bottom.dynReleaseKnob");

    // --- Global tools (right zone, always visible) ---
    auto& apvts = processor.getAPVTS();

    // Phase invert
    phaseInvertButton.setClickingTogglesState (true);
    phaseInvertButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff6b6b).withAlpha (0.3f));
    addAndMakeVisible (phaseInvertButton);
    phaseInvertAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kPhaseInvert, phaseInvertButton);

    // Character mode selector
    characterSelector.addItemList (ParamIDs::getCharacterModeNames(), 1);
    characterSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (characterSelector);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kAnalogMode, characterSelector);

    // Analyzer toggle
    analyzerButton.setClickingTogglesState (true);
    analyzerButton.setColour (juce::TextButton::buttonOnColourId, ColourPalette::compositeCurve().withAlpha (0.3f));
    addAndMakeVisible (analyzerButton);
    analyzerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kAnalyzerOn, analyzerButton);

    // Freeze toggle
    freezeButton.setClickingTogglesState (true);
    freezeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4488ff).withAlpha (0.3f));
    freezeButton.onClick = [this]
    {
        if (onFreezeToggled)
            onFreezeToggled (freezeButton.getToggleState());
    };
    addAndMakeVisible (freezeButton);

    // Match button
    matchButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    matchButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    matchButton.onClick = [this]
    {
        if (onMatchClicked)
            onMatchClicked();
    };
    addAndMakeVisible (matchButton);

    // Sketch toggle
    sketchButton.setClickingTogglesState (true);
    sketchButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffffcc00).withAlpha (0.3f));
    sketchButton.onClick = [this]
    {
        if (onSketchToggled)
            onSketchToggled (sketchButton.getToggleState());
    };
    addAndMakeVisible (sketchButton);

    // Browse button
    browseButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    browseButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    browseButton.onClick = [this]
    {
        if (onBrowseClicked)
            onBrowseClicked();
    };
    addAndMakeVisible (browseButton);

    // MIDI Program Change toggle
    midiPCButton.setClickingTogglesState (true);
    midiPCButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff66ccff).withAlpha (0.3f));
    addAndMakeVisible (midiPCButton);
    midiPCAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kMidiProgramChange, midiPCButton);

    // Help mode toggle
    helpButton.setClickingTogglesState (true);
    helpButton.setColour (juce::TextButton::buttonOnColourId, ColourPalette::compositeCurve().withAlpha (0.3f));
    helpButton.onClick = [this]
    {
        if (onHelpToggled)
            onHelpToggled (helpButton.getToggleState());
    };
    addAndMakeVisible (helpButton);

    // Process mode selector
    processModeSelector.addItemList (ParamIDs::getProcessingModeNames(), 1);
    processModeSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (processModeSelector);
    processModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kProcessMode, processModeSelector);

    // Linear phase quality selector
    lpQualitySelector.addItemList (ParamIDs::getLinearPhaseQualityNames(), 1);
    lpQualitySelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lpQualitySelector);
    lpQualityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kLinearPhaseQuality, lpQualitySelector);

    // Component IDs for help system (global tools)
    phaseInvertButton.setComponentID ("bottom.phaseInvert");
    characterSelector.setComponentID ("bottom.character");
    analyzerButton.setComponentID ("bottom.analyzer");
    freezeButton.setComponentID ("bottom.freeze");
    matchButton.setComponentID ("bottom.match");
    sketchButton.setComponentID ("bottom.sketch");
    browseButton.setComponentID ("bottom.browse");
    midiPCButton.setComponentID ("bottom.midiPC");
    helpButton.setComponentID ("bottom.help");
    processModeSelector.setComponentID ("bottom.processMode");
    lpQualitySelector.setComponentID ("bottom.lpQuality");

    startTimerHz (10);
}

BottomControlStrip::~BottomControlStrip()
{
    // Global tool attachments
    phaseInvertAttachment.reset();
    characterAttachment.reset();
    analyzerAttachment.reset();
    processModeAttachment.reset();
    lpQualityAttachment.reset();
    midiPCAttachment.reset();

    // Band-specific attachments
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();
    typeAttachment.reset();
    slopeAttachment.reset();
    stereoAttachment.reset();
    dynEnableAttachment.reset();
    dynThreshAttachment.reset();
    dynRangeAttachment.reset();
    dynAttackAttachment.reset();
    dynReleaseAttachment.reset();
}

void BottomControlStrip::setSelectedBand (int bandIndex)
{
    if (selectedBand == bandIndex)
        return;

    selectedBand = bandIndex;

    if (selectedBand < 0)
    {
        setControlsVisible (false);
        freqAttachment.reset();
        gainAttachment.reset();
        qAttachment.reset();
        typeAttachment.reset();
        slopeAttachment.reset();
        stereoAttachment.reset();
        dynEnableAttachment.reset();
        dynThreshAttachment.reset();
        dynRangeAttachment.reset();
        dynAttackAttachment.reset();
        dynReleaseAttachment.reset();
    }
    else
    {
        updateAttachments();
        updateSlopeEnabled();
        updateValueLabels();
        setControlsVisible (true);
        bandLabel.setText (juce::String (selectedBand + 1), juce::dontSendNotification);
        resized();
    }

    repaint();
}

void BottomControlStrip::setControlsVisible (bool visible)
{
    bandLabel.setVisible (visible);
    typeSelector.setVisible (visible);
    slopeSelector.setVisible (visible);
    stereoSelector.setVisible (visible);
    freqSlider.setVisible (visible);
    gainSlider.setVisible (visible);
    qSlider.setVisible (visible);
    freqLabel.setVisible (visible);
    gainLabel.setVisible (visible);
    qLabel.setVisible (visible);

    // Dynamic EQ controls
    dynEnableButton.setVisible (visible);
    dynThreshSlider.setVisible (visible);
    dynRangeSlider.setVisible (visible);
    dynAttackSlider.setVisible (visible);
    dynReleaseSlider.setVisible (visible);
    dynThreshLabel.setVisible (visible);
    dynRangeLabel.setVisible (visible);
    dynAttackLabel.setVisible (visible);
    dynReleaseLabel.setVisible (visible);
}

void BottomControlStrip::updateAttachments()
{
    auto& apvts = processor.getAPVTS();

    // Must reset old attachments before creating new ones
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();
    typeAttachment.reset();
    slopeAttachment.reset();
    stereoAttachment.reset();
    dynEnableAttachment.reset();
    dynThreshAttachment.reset();
    dynRangeAttachment.reset();
    dynAttackAttachment.reset();
    dynReleaseAttachment.reset();

    if (selectedBand < 0)
        return;

    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kType), typeSelector);

    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kSlope), slopeSelector);

    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kStereo), stereoSelector);

    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kFrequency), freqSlider);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kGain), gainSlider);

    qAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kQ), qSlider);

    // Dynamic EQ
    dynEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kDynEnabled), dynEnableButton);

    dynThreshAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kDynThresh), dynThreshSlider);

    dynRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kDynRange), dynRangeSlider);

    dynAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kDynAttack), dynAttackSlider);

    dynReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kDynRelease), dynReleaseSlider);
}

void BottomControlStrip::updateSlopeEnabled()
{
    if (selectedBand < 0)
        return;

    auto* typeParam = processor.getAPVTS().getRawParameterValue (
        ParamIDs::bandParam (selectedBand, ParamIDs::kType));
    auto type = static_cast<FilterType> ((int) typeParam->load());

    bool slopeApplies = (type == FilterType::LowPass
                      || type == FilterType::HighPass
                      || type == FilterType::LowShelf
                      || type == FilterType::HighShelf)
                    && type != FilterType::AllPass;

    slopeSelector.setEnabled (slopeApplies);
    slopeSelector.setAlpha (slopeApplies ? 1.0f : 0.4f);
}

void BottomControlStrip::updateValueLabels()
{
    freqLabel.setText (FrequencyUtils::formatFrequency ((float) freqSlider.getValue()),
                       juce::dontSendNotification);
    gainLabel.setText (juce::String (gainSlider.getValue(), 1) + " dB",
                       juce::dontSendNotification);
    qLabel.setText ("Q " + juce::String (qSlider.getValue(), 2),
                     juce::dontSendNotification);

    // Dynamic EQ labels
    dynThreshLabel.setText (juce::String (dynThreshSlider.getValue(), 1) + " dB",
                            juce::dontSendNotification);
    dynRangeLabel.setText (juce::String (dynRangeSlider.getValue(), 1) + " dB",
                           juce::dontSendNotification);
    dynAttackLabel.setText (juce::String (dynAttackSlider.getValue(), 1) + " ms",
                            juce::dontSendNotification);
    dynReleaseLabel.setText (juce::String (dynReleaseSlider.getValue(), 0) + " ms",
                             juce::dontSendNotification);
}

void BottomControlStrip::timerCallback()
{
    if (selectedBand >= 0)
    {
        updateSlopeEnabled();
        updateValueLabels();
    }
}

void BottomControlStrip::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Vertical gradient background
    juce::ColourGradient grad (ColourPalette::panelTop(), 0, bounds.getY(),
                               ColourPalette::controlBar(), 0, bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (bounds);

    // Top edge bevel (raised ridge)
    g.setColour (ColourPalette::bevelDark());
    g.drawLine (0.0f, 0.5f, bounds.getRight(), 0.5f, 1.0f);
    g.setColour (ColourPalette::bevelLight());
    g.drawLine (0.0f, 1.5f, bounds.getRight(), 1.5f, 1.0f);

    // Horizontal separator between band controls (top) and global toolbar (bottom)
    float sepY = bounds.getBottom() - 30.0f;
    g.setColour (ColourPalette::bevelDark());
    g.drawLine (4.0f, sepY, bounds.getRight() - 4.0f, sepY, 1.0f);
    g.setColour (ColourPalette::bevelLight());
    g.drawLine (4.0f, sepY + 1.0f, bounds.getRight() - 4.0f, sepY + 1.0f, 1.0f);

    if (selectedBand < 0)
    {
        // Show help text when no band selected (band controls zone only)
        auto bandZone = getLocalBounds().withBottom ((int) sepY);
        g.setColour (ColourPalette::textDim());
        g.setFont (11.0f);
        g.drawText ("Click to add band  |  Drag to adjust  |  Scroll for Q  |  Double-click to remove",
                     bandZone, juce::Justification::centred, false);
    }
    else
    {
        // Band colour indicator with glow halo
        auto colour = ColourPalette::bandColour (selectedBand);
        float dotX = 18.0f;
        float dotY = (sepY - bounds.getY()) * 0.5f;

        // Glow
        g.setColour (colour.withAlpha (0.15f));
        g.fillEllipse (dotX - 10.0f, dotY - 10.0f, 20.0f, 20.0f);

        // Dot
        g.setColour (colour);
        g.fillEllipse (dotX - 5.0f, dotY - 5.0f, 10.0f, 10.0f);
    }
}

void BottomControlStrip::resized()
{
    auto bounds = getLocalBounds().reduced (8, 2);

    // --- Bottom row: global toolbar (full width, fixed 26px) ---
    auto toolbarRow = bounds.removeFromBottom (26);
    bounds.removeFromBottom (4); // gap between rows

    // Centre the toolbar controls within the full-width row
    int totalToolW = 100 + 2 + 60 + 2 + 60 + 2 + 26  // selectors + phase
                   + 12                                   // gap
                   + 56 + 2 + 44 + 2 + 38 + 2 + 44 + 2 + 44 + 2 + 48 + 2 + 22; // buttons
    auto tb = toolbarRow.withSizeKeepingCentre (juce::jmin (totalToolW, toolbarRow.getWidth()),
                                                 toolbarRow.getHeight());

    // Left group: selectors
    auto pmArea = tb.removeFromLeft (100);
    processModeSelector.setBounds (pmArea.withHeight (22).withCentre (pmArea.getCentre()));
    tb.removeFromLeft (2);
    auto lpArea = tb.removeFromLeft (60);
    lpQualitySelector.setBounds (lpArea.withHeight (22).withCentre (lpArea.getCentre()));
    tb.removeFromLeft (2);
    auto charArea = tb.removeFromLeft (60);
    characterSelector.setBounds (charArea.withHeight (22).withCentre (charArea.getCentre()));
    tb.removeFromLeft (2);
    auto phaseArea = tb.removeFromLeft (26);
    phaseInvertButton.setBounds (phaseArea.withHeight (22).withCentre (phaseArea.getCentre()));

    tb.removeFromLeft (12); // gap between groups

    // Right group: buttons
    auto analyzerArea = tb.removeFromLeft (56);
    analyzerButton.setBounds (analyzerArea.withHeight (22).withCentre (analyzerArea.getCentre()));
    tb.removeFromLeft (2);
    auto freezeArea = tb.removeFromLeft (44);
    freezeButton.setBounds (freezeArea.withHeight (22).withCentre (freezeArea.getCentre()));
    tb.removeFromLeft (2);
    auto matchArea = tb.removeFromLeft (38);
    matchButton.setBounds (matchArea.withHeight (22).withCentre (matchArea.getCentre()));
    tb.removeFromLeft (2);
    auto sketchArea = tb.removeFromLeft (44);
    sketchButton.setBounds (sketchArea.withHeight (22).withCentre (sketchArea.getCentre()));
    tb.removeFromLeft (2);
    auto browseArea = tb.removeFromLeft (44);
    browseButton.setBounds (browseArea.withHeight (22).withCentre (browseArea.getCentre()));
    tb.removeFromLeft (2);
    auto midiArea = tb.removeFromLeft (48);
    midiPCButton.setBounds (midiArea.withHeight (22).withCentre (midiArea.getCentre()));
    tb.removeFromLeft (2);
    auto helpArea = tb.removeFromLeft (22);
    helpButton.setBounds (helpArea.withHeight (22).withCentre (helpArea.getCentre()));

    // --- Top row: band controls (full width) ---

    // Band indicator area
    auto bandArea = bounds.removeFromLeft (30);
    bandLabel.setBounds (bandArea.removeFromBottom (20));

    bounds.removeFromLeft (4); // spacing

    // Type selector
    auto typeArea = bounds.removeFromLeft (90);
    typeSelector.setBounds (typeArea.withHeight (24).withCentre (typeArea.getCentre()));

    bounds.removeFromLeft (8);

    // Knob areas
    int knobSize = 50;
    int knobAreaWidth = 70;

    // Frequency knob
    auto freqArea = bounds.removeFromLeft (knobAreaWidth);
    auto freqKnobArea = freqArea.removeFromTop (freqArea.getHeight() - 16);
    freqSlider.setBounds (freqKnobArea.withSizeKeepingCentre (knobSize, knobSize));
    freqLabel.setBounds (freqArea);

    bounds.removeFromLeft (4);

    // Gain knob
    auto gainArea = bounds.removeFromLeft (knobAreaWidth);
    auto gainKnobArea = gainArea.removeFromTop (gainArea.getHeight() - 16);
    gainSlider.setBounds (gainKnobArea.withSizeKeepingCentre (knobSize, knobSize));
    gainLabel.setBounds (gainArea);

    bounds.removeFromLeft (4);

    // Q knob
    auto qArea = bounds.removeFromLeft (knobAreaWidth);
    auto qKnobArea = qArea.removeFromTop (qArea.getHeight() - 16);
    qSlider.setBounds (qKnobArea.withSizeKeepingCentre (knobSize, knobSize));
    qLabel.setBounds (qArea);

    bounds.removeFromLeft (8);

    // Slope selector
    auto slopeArea = bounds.removeFromLeft (80);
    slopeSelector.setBounds (slopeArea.withHeight (24).withCentre (slopeArea.getCentre()));

    bounds.removeFromLeft (8);

    // Stereo selector
    auto stereoArea = bounds.removeFromLeft (70);
    stereoSelector.setBounds (stereoArea.withHeight (24).withCentre (stereoArea.getCentre()));

    bounds.removeFromLeft (12);

    // Dynamic EQ controls
    int dynKnobSize = 38;
    int dynAreaWidth = 52;

    // Dyn enable button
    auto dynBtnArea = bounds.removeFromLeft (32);
    dynEnableButton.setBounds (dynBtnArea.withHeight (22).withCentre (dynBtnArea.getCentre()));

    bounds.removeFromLeft (4);

    // Threshold knob
    auto threshArea = bounds.removeFromLeft (dynAreaWidth);
    auto threshKnob = threshArea.removeFromTop (threshArea.getHeight() - 16);
    dynThreshSlider.setBounds (threshKnob.withSizeKeepingCentre (dynKnobSize, dynKnobSize));
    dynThreshLabel.setBounds (threshArea);

    bounds.removeFromLeft (2);

    // Range knob
    auto rangeArea = bounds.removeFromLeft (dynAreaWidth);
    auto rangeKnob = rangeArea.removeFromTop (rangeArea.getHeight() - 16);
    dynRangeSlider.setBounds (rangeKnob.withSizeKeepingCentre (dynKnobSize, dynKnobSize));
    dynRangeLabel.setBounds (rangeArea);

    bounds.removeFromLeft (2);

    // Attack knob
    auto attackArea = bounds.removeFromLeft (dynAreaWidth);
    auto attackKnob = attackArea.removeFromTop (attackArea.getHeight() - 16);
    dynAttackSlider.setBounds (attackKnob.withSizeKeepingCentre (dynKnobSize, dynKnobSize));
    dynAttackLabel.setBounds (attackArea);

    bounds.removeFromLeft (2);

    // Release knob
    auto releaseArea = bounds.removeFromLeft (dynAreaWidth);
    auto releaseKnob = releaseArea.removeFromTop (releaseArea.getHeight() - 16);
    dynReleaseSlider.setBounds (releaseKnob.withSizeKeepingCentre (dynKnobSize, dynKnobSize));
    dynReleaseLabel.setBounds (releaseArea);
}
