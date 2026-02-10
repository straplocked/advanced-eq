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
 * @file PluginEditor.h
 * @brief Plugin editor window -- top-level UI container.
 */

#include "PluginProcessor.h"
#include "ui/components/EqCurveDisplay.h"
#include "ui/components/BottomControlStrip.h"
#include "ui/components/HeaderToolbar.h"
#include "ui/LookAndFeel/AdvancedEqLookAndFeel.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

/**
 * @class AdvancedEQEditor
 * @brief Top-level editor component providing the three-panel plugin UI.
 *
 * Extends juce::AudioProcessorEditor with a three-panel layout:
 * - HeaderToolbar (36 px, top)
 * - EqCurveDisplay (center, fills remaining space)
 * - BottomControlStrip (90 px, bottom)
 *
 * Default window size is 1000 x 600 pixels, resizable from 750 x 450 to 1920 x 1080.
 * Owns the AdvancedEqLookAndFeel instance applied to the entire component hierarchy.
 *
 * @see HeaderToolbar, EqCurveDisplay, BottomControlStrip
 */
class AdvancedEQEditor : public juce::AudioProcessorEditor
{
public:
    /** @brief Constructs the editor, creating child components and setting the initial window size.
     *  @param processor The audio processor this editor is attached to.
     */
    explicit AdvancedEQEditor (AdvancedEQProcessor&);
    ~AdvancedEQEditor() override;

    /** @brief Fills the background with the window background color. */
    void paint (juce::Graphics&) override;

    /** @brief Lays out the three-panel UI: header at top, control strip at bottom, curve display in center. */
    void resized() override;

private:
    AdvancedEqLookAndFeel lookAndFeel;

    // Main components
    HeaderToolbar headerToolbar;
    EqCurveDisplay curveDisplay;
    BottomControlStrip controlStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQEditor)
};
