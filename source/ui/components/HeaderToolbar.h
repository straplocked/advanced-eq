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
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../presets/PresetManager.h"
#include "../LookAndFeel/ColourPalette.h"

class HeaderToolbar : public juce::Component,
                      public PresetManager::Listener
{
public:
    explicit HeaderToolbar (AdvancedEQProcessor& processor);
    ~HeaderToolbar() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // PresetManager::Listener
    void presetChanged() override;

    std::function<void (bool)> onFreezeToggled;
    std::function<void()> onMatchClicked;

private:
    AdvancedEQProcessor& processor;
    PresetManager& presetManager;

    // Preset controls
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton presetNameButton { "Init" };
    juce::TextButton saveButton { "Save" };

    // Undo/Redo and A/B
    juce::TextButton undoButton { "Undo" };
    juce::TextButton redoButton { "Redo" };
    juce::TextButton abToggleButton { "A" };
    juce::TextButton copyABButton { "A>B" };

    // Right-side controls
    juce::TextButton phaseInvertButton { juce::String::charToString (0x00D8) }; // Ø symbol
    juce::TextButton analogButton { "Analog" };
    juce::TextButton analyzerButton { "Analyzer" };
    juce::TextButton freezeButton { "Freeze" };
    juce::TextButton matchButton { "Match" };
    juce::ComboBox processModeSelector;
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseInvertAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analogAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analyzerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    void showPresetMenu();
    void showSaveDialog();
    void updatePresetName();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderToolbar)
};
