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

    // --- Right-side controls ---

    analogButton.setClickingTogglesState (true);
    analogButton.setColour (juce::TextButton::buttonOnColourId, ColourPalette::analogAccent().withAlpha (0.3f));
    addAndMakeVisible (analogButton);
    analogAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParamIDs::kAnalogMode, analogButton);

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
}

HeaderToolbar::~HeaderToolbar()
{
    presetManager.removeListener (this);
    analogAttachment.reset();
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

    rightBounds.removeFromRight (4);

    // Analog mode button
    auto analogArea = rightBounds.removeFromRight (52);
    analogButton.setBounds (analogArea.withHeight (22).withCentre (analogArea.getCentre()));

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
