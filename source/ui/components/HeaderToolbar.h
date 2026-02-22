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
    void mouseDown (const juce::MouseEvent& e) override;

    // PresetManager::Listener
    void presetChanged() override;

    std::function<void()> onFullscreenClicked;
    std::function<void()> onPresetNameClicked;

private:
    AdvancedEQProcessor& processor;
    PresetManager& presetManager;

    // Preset controls
    juce::TextButton prevButton { juce::String::charToString (0x25C0) };   // ◀
    juce::TextButton nextButton { juce::String::charToString (0x25B6) };   // ▶
    juce::TextButton presetNameButton { "Init" };
    juce::TextButton saveButton { juce::String::charToString (0x2913) };   // ⤓ (save/download)

    // Undo/Redo and A/B
    juce::TextButton undoButton { juce::String::charToString (0x21A9) };   // ↩
    juce::TextButton redoButton { juce::String::charToString (0x21AA) };   // ↪
    juce::TextButton abToggleButton { "A" };
    juce::TextButton copyABButton { "A>B" };

    // Right-side controls
    juce::TextButton fullscreenButton { juce::String::charToString (0x2922) }; // ⤢
    juce::Slider outputGainSlider;
    juce::Slider outputWidthSlider;
    juce::Label outputGainLabel;
    juce::Label outputWidthLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputWidthAttachment;

    void showSaveDialog();
    void updatePresetName();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderToolbar)
};
