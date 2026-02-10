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
 * @file BottomControlStrip.h
 * @brief Band parameter editing strip shown when a band is selected.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../dsp/FilterTypes.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * @class BottomControlStrip
 * @brief Displays filter type, slope, and stereo selectors alongside frequency, gain,
 *        and Q rotary knobs for the currently selected EQ band.
 *
 * When a band is selected in the EqCurveDisplay, setSelectedBand() is called to
 * re-attach APVTS parameter attachments to the new band's parameters. A 10 Hz
 * timer drives value label updates so the displayed text stays in sync with
 * the underlying parameter values.
 *
 * @see EqCurveDisplay (provides selection), AdvancedEQProcessor
 */
class BottomControlStrip : public juce::Component,
                           public juce::Timer
{
public:
    /** @brief Constructs the control strip, referencing the processor for APVTS access.
     *  @param processor The audio processor that owns the parameter tree.
     */
    explicit BottomControlStrip (AdvancedEQProcessor& processor);
    ~BottomControlStrip() override;

    /** @brief Paints the strip background with the control bar color. */
    void paint (juce::Graphics& g) override;

    /** @brief Lays out selectors and rotary knobs across the strip. */
    void resized() override;

    /** @brief Timer callback that refreshes the value labels at 10 Hz. */
    void timerCallback() override;

    /** @brief Selects a band and re-creates APVTS attachments to target the new band.
     *  @param bandIndex The band index to select, or -1 to clear the selection.
     */
    void setSelectedBand (int bandIndex);

    /** @brief Returns the currently selected band index, or -1 if none.
     *  @return The selected band index.
     */
    int getSelectedBand() const { return selectedBand; }

private:
    AdvancedEQProcessor& processor;
    int selectedBand = -1;

    // Band indicator
    juce::Label bandLabel;

    // Selectors
    juce::ComboBox typeSelector;
    juce::ComboBox slopeSelector;
    juce::ComboBox stereoSelector;

    // Rotary knobs
    juce::Slider freqSlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;

    // Value labels under knobs
    juce::Label freqLabel;
    juce::Label gainLabel;
    juce::Label qLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;

    void updateAttachments();
    void updateSlopeEnabled();
    void updateValueLabels();
    void setControlsVisible (bool visible);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomControlStrip)
};
