/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel/ColourPalette.h"
#include <array>
#include <cmath>

/**
 * Miniature EQ curve thumbnail for instance list panel.
 * Renders a 512-point composite magnitude curve.
 */
class MiniCurveDisplay : public juce::Component
{
public:
    static constexpr int kNumPoints = 512;

    MiniCurveDisplay() = default;

    void setCurveData (const float* data, int numPoints)
    {
        int count = juce::jmin (numPoints, kNumPoints);
        for (int i = 0; i < count; ++i)
            curveData[(size_t) i] = data[i];
        for (int i = count; i < kNumPoints; ++i)
            curveData[(size_t) i] = 0.0f;
        repaint();
    }

    void setInstanceName (const juce::String& name) { instanceName = name; repaint(); }
    void setHighlighted (bool h) { highlighted = h; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour (highlighted ? ColourPalette::background().brighter (0.15f) : ColourPalette::background());
        g.fillRoundedRectangle (bounds, 4.0f);

        // Border
        g.setColour (highlighted ? ColourPalette::bandColour (0) : ColourPalette::bevelDark());
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

        // Draw curve
        float w = bounds.getWidth() - 8.0f;
        float h = bounds.getHeight() - 20.0f;
        float midY = bounds.getY() + 14.0f + h * 0.5f;

        juce::Path curvePath;
        for (int i = 0; i < kNumPoints; ++i)
        {
            float x = bounds.getX() + 4.0f + (float) i / (float) (kNumPoints - 1) * w;
            float y = midY - curveData[(size_t) i] / 24.0f * (h * 0.5f);
            y = juce::jlimit (bounds.getY() + 14.0f, bounds.getBottom() - 4.0f, y);

            if (i == 0)
                curvePath.startNewSubPath (x, y);
            else
                curvePath.lineTo (x, y);
        }

        g.setColour (ColourPalette::bandColour (0).withAlpha (0.7f));
        g.strokePath (curvePath, juce::PathStrokeType (1.5f));

        // Zero line
        g.setColour (ColourPalette::gridLine().withAlpha (0.3f));
        g.drawHorizontalLine ((int) midY, bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

        // Instance name
        g.setColour (ColourPalette::textBright());
        g.setFont (10.0f);
        g.drawText (instanceName, bounds.reduced (4.0f, 2.0f).removeFromTop (12),
                     juce::Justification::centredLeft);
    }

private:
    std::array<float, kNumPoints> curveData {};
    juce::String instanceName { "Instance" };
    bool highlighted = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniCurveDisplay)
};
