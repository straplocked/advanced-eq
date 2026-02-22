/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "MidiMappingsPanel.h"

MidiMappingsPanel::MidiMappingsPanel (MidiLearnManager& ml)
    : midiLearn (ml)
{
    titleLabel.setText ("MIDI Mappings", juce::dontSendNotification);
    titleLabel.setFont (14.0f);
    titleLabel.setColour (juce::Label::textColourId, ColourPalette::textBright());
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    clearAllButton.setColour (juce::TextButton::buttonColourId, ColourPalette::knobBackground());
    clearAllButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
    clearAllButton.onClick = [this]
    {
        midiLearn.clearAllMappings();
        refreshMappings();
    };
    addAndMakeVisible (clearAllButton);

    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
    closeButton.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closeButton);

    startTimerHz (4);
}

MidiMappingsPanel::~MidiMappingsPanel()
{
    stopTimer();
}

void MidiMappingsPanel::timerCallback()
{
    refreshMappings();
}

void MidiMappingsPanel::refreshMappings()
{
    auto mappings = midiLearn.getMappings();

    // Build a fingerprint to detect content changes (not just count)
    juce::String fingerprint;
    for (const auto& [cc, mapping] : mappings)
        fingerprint += juce::String (cc) + ":" + mapping.parameterID + ";";

    if (fingerprint == lastFingerprint)
        return;

    lastFingerprint = fingerprint;

    // Remove old row child components before clearing
    for (auto* row : rows)
    {
        removeChildComponent (&row->ccLabel);
        removeChildComponent (&row->paramLabel);
        removeChildComponent (&row->clearButton);
    }
    rows.clear();

    for (const auto& [cc, mapping] : mappings)
    {
        auto* row = rows.add (new MappingRow());

        row->ccLabel.setText ("CC " + juce::String (cc), juce::dontSendNotification);
        row->ccLabel.setFont (12.0f);
        row->ccLabel.setColour (juce::Label::textColourId, juce::Colour (0xff66ccff));
        row->ccLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (row->ccLabel);

        row->paramLabel.setText (mapping.parameterID, juce::dontSendNotification);
        row->paramLabel.setFont (12.0f);
        row->paramLabel.setColour (juce::Label::textColourId, ColourPalette::textBright());
        row->paramLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (row->paramLabel);

        juce::String paramID = mapping.parameterID;
        row->clearButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        row->clearButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
        row->clearButton.onClick = [this, paramID]
        {
            midiLearn.removeMapping (paramID);
            refreshMappings();
        };
        addAndMakeVisible (row->clearButton);
    }

    resized();
    repaint();
}

void MidiMappingsPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Panel background
    g.setColour (ColourPalette::panelBottom().withAlpha (0.95f));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (ColourPalette::gridLine());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    if (rows.isEmpty())
    {
        g.setFont (12.0f);
        g.setColour (ColourPalette::textDim());
        g.drawText ("No MIDI mappings.\nRight-click a knob to learn.",
                     getLocalBounds().reduced (12).withTrimmedTop (28),
                     juce::Justification::centredTop, true);
    }
}

void MidiMappingsPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    // Title row
    auto titleRow = bounds.removeFromTop (24);
    titleLabel.setBounds (titleRow.removeFromLeft (titleRow.getWidth() - 60));
    closeButton.setBounds (getWidth() - 24, 2, 22, 22);

    bounds.removeFromTop (4);

    // Mapping rows
    for (auto* row : rows)
    {
        auto rowBounds = bounds.removeFromTop (22);
        row->ccLabel.setBounds (rowBounds.removeFromLeft (48));
        rowBounds.removeFromLeft (4);
        row->clearButton.setBounds (rowBounds.removeFromRight (22));
        rowBounds.removeFromRight (4);
        row->paramLabel.setBounds (rowBounds);
        bounds.removeFromTop (2);
    }

    // Clear All button at bottom
    if (! rows.isEmpty())
    {
        bounds.removeFromTop (4);
        auto btnRow = bounds.removeFromTop (24);
        clearAllButton.setBounds (btnRow.withSizeKeepingCentre (80, 22));
    }
}
