/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "PluginProcessor.h"
#include "ui/components/EqCurveDisplay.h"
#include "ui/components/BottomControlStrip.h"
#include "ui/components/HeaderToolbar.h"
#include "ui/components/LevelMeter.h"
#include "ui/components/EQMatchPanel.h"
#include "ui/help/HelpOverlay.h"
#include "ui/components/PresetBrowserPanel.h"
#include "ui/components/InstanceListPanel.h"
#include "ui/LookAndFeel/AdvancedEqLookAndFeel.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

class AdvancedEQEditor : public juce::AudioProcessorEditor,
                         public juce::KeyListener
{
public:
    explicit AdvancedEQEditor (AdvancedEQProcessor&);
    ~AdvancedEQEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // KeyListener
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;

    // Window size presets
    enum WindowSizePreset { Mini = 0, Small, Medium, Large, XL };
    void setWindowSizePreset (WindowSizePreset preset);

private:
    AdvancedEQProcessor& processorRef;
    AdvancedEqLookAndFeel lookAndFeel;

    // Main components
    HeaderToolbar headerToolbar;
    EqCurveDisplay curveDisplay;
    BottomControlStrip controlStrip;
    LevelMeter levelMeter;
    EQMatchPanel matchPanel;
    HelpOverlay helpOverlay;
    PresetBrowserPanel presetBrowser;
    InstanceListPanel instanceList;
    juce::TooltipWindow tooltipWindow;

    // Fullscreen
    bool isFullScreen = false;
    juce::Rectangle<int> previousBounds;
    void toggleFullScreen();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQEditor)
};
