/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MiniCurveDisplay.h"
#include "../../ipc/InstanceRegistry.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * Scrollable panel showing all active plugin instances with mini curve displays.
 * Updates at 5 Hz to show real-time EQ curves from all instances.
 */
class InstanceListPanel : public juce::Component,
                          public juce::Timer
{
public:
    explicit InstanceListPanel (InstanceRegistry& registry)
        : instanceRegistry (registry)
    {
        for (int i = 0; i < InstanceRegistry::kMaxInstances; ++i)
        {
            auto* mini = new MiniCurveDisplay();
            miniDisplays.add (mini);
            addChildComponent (mini);
        }

        closeButton.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeButton);

        startTimerHz (5);
    }

    ~InstanceListPanel() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColourPalette::background().withAlpha (0.97f));

        g.setColour (ColourPalette::bevelLight());
        g.drawRect (getLocalBounds(), 1);

        g.setColour (ColourPalette::textBright());
        g.setFont (14.0f);
        g.drawText ("Active Instances", 10, 6, 200, 20, juce::Justification::centredLeft);

        if (activeCount == 0)
        {
            g.setColour (ColourPalette::textDim());
            g.setFont (11.0f);
            g.drawText ("No other instances detected", getLocalBounds(),
                         juce::Justification::centred);
        }
    }

    void resized() override
    {
        closeButton.setBounds (getWidth() - 30, 6, 22, 22);
        layoutMiniDisplays();
    }

    void timerCallback() override
    {
        updateFromRegistry();
    }

private:
    void layoutMiniDisplays()
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (28);

        int itemW = 200;
        int itemH = 100;
        int spacing = 8;
        int cols = juce::jmax (1, bounds.getWidth() / (itemW + spacing));

        for (int i = 0; i < activeCount; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            int x = bounds.getX() + col * (itemW + spacing);
            int y = bounds.getY() + row * (itemH + spacing);
            miniDisplays[i]->setBounds (x, y, itemW, itemH);
            miniDisplays[i]->setVisible (true);
        }

        for (int i = activeCount; i < InstanceRegistry::kMaxInstances; ++i)
            miniDisplays[i]->setVisible (false);
    }

    void updateFromRegistry()
    {
        // Read active slots from registry's collision data
        // We show each instance that has recent heartbeat data
        auto& collisions = instanceRegistry.getCollisions();

        // Count active instances by examining the registry's other slots
        // For now, display count based on collision data presence
        int prevCount = activeCount;
        activeCount = 0;

        // Use a simple approach: show a mini display for each detected external instance
        // plus our own instance
        juce::StringArray instanceNames;
        instanceNames.add ("This Instance");
        activeCount = 1;

        // Add instances from collision sources
        std::set<int> seenIds;
        for (auto& c : collisions)
        {
            if (seenIds.find (c.otherInstanceId) == seenIds.end())
            {
                seenIds.insert (c.otherInstanceId);
                instanceNames.add ("Instance " + juce::String (activeCount + 1));
                ++activeCount;
                if (activeCount >= InstanceRegistry::kMaxInstances)
                    break;
            }
        }

        // Update mini displays
        for (int i = 0; i < activeCount && i < miniDisplays.size(); ++i)
        {
            miniDisplays[i]->setInstanceName (instanceNames[i]);
            miniDisplays[i]->setHighlighted (i == 0);
        }

        if (activeCount != prevCount)
        {
            layoutMiniDisplays();
            repaint();
        }
    }

    InstanceRegistry& instanceRegistry;
    juce::OwnedArray<MiniCurveDisplay> miniDisplays;
    juce::TextButton closeButton { "X" };
    int activeCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstanceListPanel)
};
