/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "HeaderToolbar.h"

HeaderToolbar::HeaderToolbar (AdvancedEQProcessor& p)
    : processor (p),
      presetManager (p.getPresetManager())
{
    presetManager.addListener (this);

    auto& apvts = processor.getAPVTS();

    // --- Preset controls ---

    prevButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    prevButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    prevButton.onClick = [this] { presetManager.previousPreset(); };
    addAndMakeVisible (prevButton);

    nextButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    nextButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    nextButton.onClick = [this] { presetManager.nextPreset(); };
    addAndMakeVisible (nextButton);

    presetNameButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetNameButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
    presetNameButton.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetNameButton);

    saveButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    saveButton.onClick = [this] { showSaveDialog(); };
    addAndMakeVisible (saveButton);

    updatePresetName();

    // --- Undo/Redo ---
    undoButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    undoButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    undoButton.onClick = [this] { processor.getUndoManager().undo(); };
    addAndMakeVisible (undoButton);

    redoButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    redoButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    redoButton.onClick = [this] { processor.getUndoManager().redo(); };
    addAndMakeVisible (redoButton);

    // --- A/B Comparison ---
    abToggleButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    abToggleButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
    abToggleButton.onClick = [this]
    {
        processor.toggleAB();
        abToggleButton.setButtonText (processor.isSlotA() ? "A" : "B");
    };
    addAndMakeVisible (abToggleButton);

    copyABButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    copyABButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    copyABButton.onClick = [this]
    {
        if (processor.isSlotA())
            processor.copyAtoB();
        else
            processor.copyBtoA();
    };
    addAndMakeVisible (copyABButton);

    // --- Right-side controls ---

    // Phase invert button
    phaseInvertButton.setClickingTogglesState (true);
    phaseInvertButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff6b6b).withAlpha (0.3f));
    addAndMakeVisible (phaseInvertButton);
    phaseInvertAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kPhaseInvert, phaseInvertButton);

    // Character mode selector (replaces analog button)
    characterSelector.addItemList (ParamIDs::getCharacterModeNames(), 1);
    characterSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (characterSelector);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kAnalogMode, characterSelector);

    analyzerButton.setClickingTogglesState (true);
    analyzerButton.setColour (juce::TextButton::buttonOnColourId, ColourPalette::compositeCurve().withAlpha (0.3f));
    addAndMakeVisible (analyzerButton);
    analyzerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kAnalyzerOn, analyzerButton);

    freezeButton.setClickingTogglesState (true);
    freezeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4488ff).withAlpha (0.3f));
    freezeButton.onClick = [this]
    {
        if (onFreezeToggled)
            onFreezeToggled (freezeButton.getToggleState());
    };
    addAndMakeVisible (freezeButton);

    matchButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    matchButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    matchButton.onClick = [this]
    {
        if (onMatchClicked)
            onMatchClicked();
    };
    addAndMakeVisible (matchButton);

    // Sketch button
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

    // Fullscreen button
    fullscreenButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    fullscreenButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    fullscreenButton.onClick = [this]
    {
        if (onFullscreenClicked)
            onFullscreenClicked();
    };
    addAndMakeVisible (fullscreenButton);

    // Linear phase quality selector (hidden until LP mode selected)
    lpQualitySelector.addItemList (ParamIDs::getLinearPhaseQualityNames(), 1);
    lpQualitySelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lpQualitySelector);
    lpQualityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kLinearPhaseQuality, lpQualitySelector);

    // Output width knob
    outputWidthSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputWidthSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (outputWidthSlider);
    outputWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::kOutputWidth, outputWidthSlider);

    outputWidthLabel.setText ("W", juce::dontSendNotification);
    outputWidthLabel.setJustificationType (juce::Justification::centred);
    outputWidthLabel.setFont (10.0f);
    outputWidthLabel.setColour (juce::Label::textColourId, ColourPalette::axisText());
    addAndMakeVisible (outputWidthLabel);

    // Help mode toggle
    helpButton.setClickingTogglesState (true);
    helpButton.setColour (juce::TextButton::buttonOnColourId,
                          ColourPalette::compositeCurve().withAlpha (0.3f));
    helpButton.onClick = [this]
    {
        if (onHelpToggled)
            onHelpToggled (helpButton.getToggleState());
    };
    addAndMakeVisible (helpButton);

    processModeSelector.addItemList (ParamIDs::getProcessingModeNames(), 1);
    processModeSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (processModeSelector);
    processModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::kProcessMode, processModeSelector);

    outputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (outputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::kOutputGain, outputGainSlider);

    outputGainLabel.setText ("Out", juce::dontSendNotification);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    outputGainLabel.setFont (10.0f);
    outputGainLabel.setColour (juce::Label::textColourId, ColourPalette::axisText());
    addAndMakeVisible (outputGainLabel);

    // Component IDs for help system
    prevButton.setComponentID ("header.prevPreset");
    nextButton.setComponentID ("header.nextPreset");
    presetNameButton.setComponentID ("header.presetName");
    saveButton.setComponentID ("header.savePreset");
    undoButton.setComponentID ("header.undo");
    redoButton.setComponentID ("header.redo");
    abToggleButton.setComponentID ("header.abToggle");
    copyABButton.setComponentID ("header.copyAB");
    phaseInvertButton.setComponentID ("header.phaseInvert");
    characterSelector.setComponentID ("header.character");
    analyzerButton.setComponentID ("header.analyzer");
    freezeButton.setComponentID ("header.freeze");
    matchButton.setComponentID ("header.match");
    sketchButton.setComponentID ("header.sketch");
    browseButton.setComponentID ("header.browse");
    midiPCButton.setComponentID ("header.midiPC");
    fullscreenButton.setComponentID ("header.fullscreen");
    helpButton.setComponentID ("header.help");
    processModeSelector.setComponentID ("header.processMode");
    lpQualitySelector.setComponentID ("header.lpQuality");
    outputGainSlider.setComponentID ("header.outputGain");
    outputWidthSlider.setComponentID ("header.outputWidth");
}

HeaderToolbar::~HeaderToolbar()
{
    presetManager.removeListener (this);
    phaseInvertAttachment.reset();
    characterAttachment.reset();
    analyzerAttachment.reset();
    processModeAttachment.reset();
    lpQualityAttachment.reset();
    outputGainAttachment.reset();
    outputWidthAttachment.reset();
    midiPCAttachment.reset();
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
    g.drawText ("Advanced EQ", 13, 1, 120, getHeight(), juce::Justification::centredLeft, true);
    g.setColour (ColourPalette::textBright());
    g.drawText ("Advanced EQ", 12, 0, 120, getHeight(), juce::Justification::centredLeft, true);
}

void HeaderToolbar::resized()
{
    auto bounds = getLocalBounds().reduced (8, 4);

    // Title area on left
    bounds.removeFromLeft (124);

    // Right side controls (laid out right-to-left)
    auto rightBounds = bounds;

    // Output gain knob + width knob (rightmost)
    auto widthArea = rightBounds.removeFromRight (40);
    auto widthKnob = widthArea.removeFromTop (widthArea.getHeight() - 2);
    outputWidthSlider.setBounds (widthKnob.withSizeKeepingCentre (28, 28));
    outputWidthLabel.setBounds (widthArea);

    auto outArea = rightBounds.removeFromRight (40);
    auto outKnob = outArea.removeFromTop (outArea.getHeight() - 2);
    outputGainSlider.setBounds (outKnob.withSizeKeepingCentre (28, 28));
    outputGainLabel.setBounds (outArea);

    rightBounds.removeFromRight (4);

    // Fullscreen button
    auto fsArea = rightBounds.removeFromRight (22);
    fullscreenButton.setBounds (fsArea.withHeight (22).withCentre (fsArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Process mode selector + LP quality
    auto modeArea = rightBounds.removeFromRight (100);
    processModeSelector.setBounds (modeArea.withHeight (22).withCentre (modeArea.getCentre()));

    auto lpArea = rightBounds.removeFromRight (60);
    lpQualitySelector.setBounds (lpArea.withHeight (22).withCentre (lpArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Help button
    auto helpArea = rightBounds.removeFromRight (22);
    helpButton.setBounds (helpArea.withHeight (22).withCentre (helpArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Browse button
    auto browseArea = rightBounds.removeFromRight (44);
    browseButton.setBounds (browseArea.withHeight (22).withCentre (browseArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Match button
    auto matchArea = rightBounds.removeFromRight (38);
    matchButton.setBounds (matchArea.withHeight (22).withCentre (matchArea.getCentre()));

    // Sketch button
    auto sketchArea = rightBounds.removeFromRight (44);
    sketchButton.setBounds (sketchArea.withHeight (22).withCentre (sketchArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Freeze button
    auto freezeArea = rightBounds.removeFromRight (44);
    freezeButton.setBounds (freezeArea.withHeight (22).withCentre (freezeArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Analyzer on/off
    auto analyzerArea = rightBounds.removeFromRight (56);
    analyzerButton.setBounds (analyzerArea.withHeight (22).withCentre (analyzerArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Character mode selector (replaces Analog)
    auto charArea = rightBounds.removeFromRight (60);
    characterSelector.setBounds (charArea.withHeight (22).withCentre (charArea.getCentre()));

    rightBounds.removeFromRight (2);

    // MIDI PC toggle
    auto midiArea = rightBounds.removeFromRight (48);
    midiPCButton.setBounds (midiArea.withHeight (22).withCentre (midiArea.getCentre()));

    rightBounds.removeFromRight (2);

    // Phase invert button
    auto phaseArea = rightBounds.removeFromRight (26);
    phaseInvertButton.setBounds (phaseArea.withHeight (22).withCentre (phaseArea.getCentre()));

    // Undo/Redo buttons
    rightBounds.removeFromRight (4);

    auto undoArea = rightBounds.removeFromRight (36);
    undoButton.setBounds (undoArea.withHeight (22).withCentre (undoArea.getCentre()));

    auto redoArea = rightBounds.removeFromRight (36);
    redoButton.setBounds (redoArea.withHeight (22).withCentre (redoArea.getCentre()));

    rightBounds.removeFromRight (6);

    // A/B buttons
    auto abArea = rightBounds.removeFromRight (22);
    abToggleButton.setBounds (abArea.withHeight (22).withCentre (abArea.getCentre()));

    auto copyArea = rightBounds.removeFromRight (30);
    copyABButton.setBounds (copyArea.withHeight (22).withCentre (copyArea.getCentre()));

    // Preset controls in the remaining center space
    auto presetBounds = rightBounds;
    presetBounds.removeFromRight (8);

    // Lay out preset controls centered in remaining space
    auto presetArea = presetBounds.withSizeKeepingCentre (
        juce::jmin (320, presetBounds.getWidth()), presetBounds.getHeight());

    prevButton.setBounds (presetArea.removeFromLeft (24).withHeight (22).withCentre (
        juce::Point<int> (presetArea.getX() - 12, presetArea.getCentreY())));
    // Recalc after removeFromLeft
    auto saveBtnArea = presetArea.removeFromRight (38);
    saveButton.setBounds (saveBtnArea.withHeight (22).withCentre (saveBtnArea.getCentre()));

    presetArea.removeFromRight (4);

    auto nextBtnArea = presetArea.removeFromRight (24);
    nextButton.setBounds (nextBtnArea.withHeight (22).withCentre (nextBtnArea.getCentre()));

    presetArea.removeFromLeft (4);
    presetArea.removeFromRight (4);
    presetNameButton.setBounds (presetArea.withHeight (22).withCentre (presetArea.getCentre()));
}

void HeaderToolbar::presetChanged()
{
    updatePresetName();
}

void HeaderToolbar::updatePresetName()
{
    presetNameButton.setButtonText (presetManager.getCurrentPresetName());
}

void HeaderToolbar::showPresetMenu()
{
    juce::PopupMenu menu;
    auto categories = presetManager.getCategories();
    auto& allPresets = presetManager.getAllPresets();

    for (auto& category : categories)
    {
        juce::PopupMenu subMenu;

        for (int i = 0; i < (int) allPresets.size(); ++i)
        {
            if (allPresets[(size_t) i].category == category)
            {
                bool isCurrent = (i == presetManager.getCurrentPresetIndex());

                if (! allPresets[(size_t) i].isFactory)
                {
                    // User presets get a delete option via right-click submenu
                    juce::PopupMenu itemMenu;
                    itemMenu.addItem (10000 + i, "Delete");
                    subMenu.addItem (i + 1, allPresets[(size_t) i].name, true, isCurrent);
                }
                else
                {
                    subMenu.addItem (i + 1, allPresets[(size_t) i].name, true, isCurrent);
                }
            }
        }

        menu.addSubMenu (category, subMenu);
    }

    // Add save/delete actions at bottom
    menu.addSeparator();
    menu.addItem (-1, "Save Preset...");

    // Add delete option if current preset is a user preset
    auto idx = presetManager.getCurrentPresetIndex();
    if (idx >= 0 && idx < (int) allPresets.size() && ! allPresets[(size_t) idx].isFactory)
        menu.addItem (-2, "Delete \"" + allPresets[(size_t) idx].name + "\"");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetNameButton),
        [this] (int result)
        {
            if (result == -1)
            {
                showSaveDialog();
            }
            else if (result == -2)
            {
                presetManager.deleteUserPreset (presetManager.getCurrentPresetIndex());
            }
            else if (result > 0)
            {
                presetManager.applyPreset (result - 1);
            }
        });
}

void HeaderToolbar::showSaveDialog()
{
    auto* nameEditor = new juce::AlertWindow ("Save Preset", "Enter a name for your preset:", juce::MessageBoxIconType::NoIcon);
    nameEditor->addTextEditor ("name", presetManager.getCurrentPresetName(), "Preset name:");
    nameEditor->addButton ("Save", 1);
    nameEditor->addButton ("Cancel", 0);

    nameEditor->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, nameEditor] (int result)
        {
            if (result == 1)
            {
                auto name = nameEditor->getTextEditorContents ("name");
                presetManager.saveCurrentAsPreset (name);
            }
            delete nameEditor;
        }), false);
}
