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
 * @file BandControlPanel.h
 * @brief Floating band control panel for filter type and slope selection.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../dsp/FilterTypes.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * @class BandControlPanel
 * @brief Compact floating panel with filter type, slope, and stereo placement selectors.
 *
 * Designed to be positioned near a band handle via positionNear(). Uses a timer to
 * keep the displayed selector values synchronized with the underlying APVTS parameters.
 *
 * @see EqCurveDisplay
 */
class BandControlPanel : public juce::Component,
                          public juce::Timer
{
public:
    /** @brief Constructs the panel, referencing the processor for APVTS access.
     *  @param processor The audio processor that owns the parameter tree.
     */
    explicit BandControlPanel (AdvancedEQProcessor& processor);
    ~BandControlPanel() override;

    /** @brief Paints the panel background. */
    void paint (juce::Graphics& g) override;

    /** @brief Lays out the type, slope, and stereo selectors within the panel. */
    void resized() override;

    /** @brief Timer callback that synchronizes selector values with parameter state. */
    void timerCallback() override;

    /** @brief Selects a band and re-creates APVTS attachments for its parameters.
     *  @param bandIndex The band index to select, or -1 to clear the selection.
     */
    void setSelectedBand (int bandIndex);

    /** @brief Returns the currently selected band index, or -1 if none.
     *  @return The selected band index.
     */
    int getSelectedBand() const { return selectedBand; }

    // Position the panel near the given point (band handle location)
    /** @brief Positions this panel near a band handle, clamped within parent bounds.
     *  @param x            The X coordinate of the band handle.
     *  @param y            The Y coordinate of the band handle.
     *  @param parentBounds The bounding rectangle of the parent component.
     */
    void positionNear (float x, float y, juce::Rectangle<int> parentBounds);

private:
    AdvancedEQProcessor& processor;
    int selectedBand = -1;

    juce::ComboBox typeSelector;
    juce::ComboBox slopeSelector;
    juce::ComboBox stereoSelector;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoAttachment;

    void updateAttachments();
    void updateSlopeEnabled();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandControlPanel)
};
