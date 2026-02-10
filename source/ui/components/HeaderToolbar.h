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

/**
 * @file HeaderToolbar.h
 * @brief Top toolbar with preset navigation, mode selection, and global controls.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../presets/PresetManager.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * @class HeaderToolbar
 * @brief Top toolbar providing preset navigation, processing mode selection, and global controls.
 *
 * Implements PresetManager::Listener to stay synchronized with preset name changes.
 * Contains preset prev/next/save buttons, a process mode selector (stereo, left/right,
 * mid/side), an analog mode toggle, an analyzer toggle, a freeze toggle, and an
 * output gain knob.
 *
 * @see PresetManager, AdvancedEQProcessor
 */
class HeaderToolbar : public juce::Component,
                      public PresetManager::Listener
{
public:
    /** @brief Constructs the toolbar, connecting to the processor's APVTS and preset manager.
     *  @param processor The audio processor that owns parameters and preset state.
     */
    explicit HeaderToolbar (AdvancedEQProcessor& processor);
    ~HeaderToolbar() override;

    /** @brief Paints the toolbar background with a gradient panel surface. */
    void paint (juce::Graphics& g) override;

    /** @brief Lays out preset buttons on the left and control widgets on the right. */
    void resized() override;

    // PresetManager::Listener
    /** @brief Called when the active preset changes; updates the displayed preset name. */
    void presetChanged() override;

private:
    AdvancedEQProcessor& processor;
    PresetManager& presetManager;

    // Preset controls
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton presetNameButton { "Init" };
    juce::TextButton saveButton { "Save" };

    // Right-side controls
    juce::TextButton analogButton { "Analog" };
    juce::TextButton analyzerButton { "Analyzer" };
    juce::TextButton freezeButton { "Freeze" };
    juce::ComboBox processModeSelector;
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    // Callback for freeze toggle
    public:
    /** @brief Callback invoked when the freeze button is toggled.
     *
     * The parameter is true when freeze is enabled, false when disabled.
     */
    std::function<void (bool)> onFreezeToggled;
    private:

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analogAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analyzerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    void showPresetMenu();
    void showSaveDialog();
    void updatePresetName();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderToolbar)
};
