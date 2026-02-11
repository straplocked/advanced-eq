/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel/ColourPalette.h"
#include "HelpTextDatabase.h"

class HelpOverlay : public juce::Component,
                    public juce::Timer
{
public:
    HelpOverlay();
    ~HelpOverlay() override = default;

    void paint (juce::Graphics& g) override;
    void timerCallback() override;

    void setHelpModeActive (bool active);
    bool isHelpModeActive() const { return helpModeActive; }

private:
    bool helpModeActive = false;
    juce::String currentHelpText;
    juce::Rectangle<int> currentTargetBounds;
    juce::String lastHoveredID;

    void drawHelpTooltip (juce::Graphics& g, const juce::String& text,
                          juce::Rectangle<int> targetBounds);

    juce::Component* findDeepestComponentWithID (juce::Component* root,
                                                  juce::Point<int> pos);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelpOverlay)
};
