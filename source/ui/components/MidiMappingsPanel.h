/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../midi/MidiLearnManager.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * Overlay panel that lists all active MIDI CC → parameter mappings.
 * Allows clearing individual mappings or all mappings at once.
 */
class MidiMappingsPanel : public juce::Component,
                          public juce::Timer
{
public:
    explicit MidiMappingsPanel (MidiLearnManager& midiLearn);
    ~MidiMappingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiLearnManager& midiLearn;

    juce::TextButton clearAllButton { "Clear All" };
    juce::TextButton closeButton { "X" };
    juce::Label titleLabel;

    struct MappingRow
    {
        juce::Label ccLabel;
        juce::Label paramLabel;
        juce::TextButton clearButton { "X" };
    };
    juce::OwnedArray<MappingRow> rows;

    void refreshMappings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiMappingsPanel)
};
