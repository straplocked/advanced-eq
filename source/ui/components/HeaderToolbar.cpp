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
#include "../LookAndFeel/Fonts.h"

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
    presetNameButton.onClick = [this] { if (onPresetNameClicked) onPresetNameClicked(); };
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

    // Fullscreen button
    fullscreenButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    fullscreenButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    fullscreenButton.onClick = [this]
    {
        if (onFullscreenClicked)
            onFullscreenClicked();
    };
    addAndMakeVisible (fullscreenButton);

    // Output width knob
    outputWidthSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputWidthSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (outputWidthSlider);
    outputWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::kOutputWidth, outputWidthSlider);

    outputWidthLabel.setText ("Width", juce::dontSendNotification);
    outputWidthLabel.setJustificationType (juce::Justification::centred);
    outputWidthLabel.setFont (Fonts::getBody (11.0f));
    outputWidthLabel.setColour (juce::Label::textColourId, ColourPalette::axisText());
    addAndMakeVisible (outputWidthLabel);

    outputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (outputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParamIDs::kOutputGain, outputGainSlider);

    outputGainLabel.setText ("Out", juce::dontSendNotification);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    outputGainLabel.setFont (Fonts::getBody (11.0f));
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
    fullscreenButton.setComponentID ("header.fullscreen");
    outputGainSlider.setComponentID ("header.outputGain");
    outputWidthSlider.setComponentID ("header.outputWidth");

}

HeaderToolbar::~HeaderToolbar()
{
    presetManager.removeListener (this);
    outputGainAttachment.reset();
    outputWidthAttachment.reset();
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

    // Title with drop shadow (Audiowide branding font)
    g.setFont (Fonts::getTitle (17.5f));
    g.setColour (juce::Colour (0x40000000));
    g.drawText ("ADVANCED EQ", 13, 1, 155, getHeight(), juce::Justification::centredLeft, true);
    g.setColour (ColourPalette::textBright());
    g.drawText ("ADVANCED EQ", 12, 0, 155, getHeight(), juce::Justification::centredLeft, true);
}

void HeaderToolbar::resized()
{
    auto bounds = getLocalBounds().reduced (8, 4);

    // --- Left side: title, undo/redo, A/B ---
    bounds.removeFromLeft (160); // title "ADVANCED EQ" (Audiowide font)

    auto undoArea = bounds.removeFromLeft (36);
    undoButton.setBounds (undoArea.withHeight (22).withCentre (undoArea.getCentre()));

    auto redoArea = bounds.removeFromLeft (36);
    redoButton.setBounds (redoArea.withHeight (22).withCentre (redoArea.getCentre()));

    bounds.removeFromLeft (6);

    auto abArea = bounds.removeFromLeft (22);
    abToggleButton.setBounds (abArea.withHeight (22).withCentre (abArea.getCentre()));

    bounds.removeFromLeft (2);

    auto copyArea = bounds.removeFromLeft (30);
    copyABButton.setBounds (copyArea.withHeight (22).withCentre (copyArea.getCentre()));

    bounds.removeFromLeft (8);

    // --- Right side: output knobs, fullscreen ---
    auto widthArea = bounds.removeFromRight (40);
    auto widthKnob = widthArea.removeFromTop (widthArea.getHeight() - 2);
    outputWidthSlider.setBounds (widthKnob.withSizeKeepingCentre (28, 28));
    outputWidthLabel.setBounds (widthArea);

    auto outArea = bounds.removeFromRight (40);
    auto outKnob = outArea.removeFromTop (outArea.getHeight() - 2);
    outputGainSlider.setBounds (outKnob.withSizeKeepingCentre (28, 28));
    outputGainLabel.setBounds (outArea);

    bounds.removeFromRight (4);

    auto fsArea = bounds.removeFromRight (22);
    fullscreenButton.setBounds (fsArea.withHeight (22).withCentre (fsArea.getCentre()));

    bounds.removeFromRight (8);

    // --- Centre: preset controls ---
    auto presetArea = bounds.withSizeKeepingCentre (
        juce::jmin (320, bounds.getWidth()), bounds.getHeight());

    auto prevArea = presetArea.removeFromLeft (24);
    prevButton.setBounds (prevArea.withHeight (22).withCentre (prevArea.getCentre()));

    presetArea.removeFromLeft (4);

    auto saveBtnArea = presetArea.removeFromRight (38);
    saveButton.setBounds (saveBtnArea.withHeight (22).withCentre (saveBtnArea.getCentre()));

    presetArea.removeFromRight (4);

    auto nextBtnArea = presetArea.removeFromRight (24);
    nextButton.setBounds (nextBtnArea.withHeight (22).withCentre (nextBtnArea.getCentre()));

    presetArea.removeFromRight (4);
    presetArea.removeFromLeft (4);
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
