/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../PluginProcessor.h"
#include "../LookAndFeel/ColourPalette.h"

class LevelMeter : public juce::Component,
                    public juce::Timer
{
public:
    explicit LevelMeter (AdvancedEQProcessor& p)
        : processor (p)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float w = bounds.getWidth();
        float h = bounds.getHeight();

        // Background
        g.setColour (ColourPalette::controlBar());
        g.fillRect (bounds);

        // Draw two vertical meter bars (L and R)
        float barWidth = (w - 3.0f) / 2.0f;
        float barInset = 1.0f;

        drawMeterBar (g, barInset, 0.0f, barWidth, h, peakL, rmsL);
        drawMeterBar (g, barInset + barWidth + 1.0f, 0.0f, barWidth, h, peakR, rmsR);

        // Labels
        g.setColour (ColourPalette::axisText());
        g.setFont (8.0f);
        g.drawText ("L", (int) barInset, (int) h - 10, (int) barWidth, 10,
                     juce::Justification::centred, false);
        g.drawText ("R", (int) (barInset + barWidth + 1.0f), (int) h - 10, (int) barWidth, 10,
                     juce::Justification::centred, false);
    }

    void timerCallback() override
    {
        peakL = processor.getPeakLevelL();
        peakR = processor.getPeakLevelR();
        rmsL  = processor.getRmsLevelL();
        rmsR  = processor.getRmsLevelR();
        repaint();
    }

private:
    void drawMeterBar (juce::Graphics& g, float x, float y, float w, float h,
                        float peakDB, float rmsDB)
    {
        // Meter range: -60 dB to +6 dB
        constexpr float minDB = -60.0f;
        constexpr float maxDB = 6.0f;
        constexpr float range = maxDB - minDB;

        float peakNorm = juce::jlimit (0.0f, 1.0f, (peakDB - minDB) / range);
        float rmsNorm  = juce::jlimit (0.0f, 1.0f, (rmsDB - minDB) / range);

        float meterH = h - 12.0f; // Leave space for label
        float peakY = y + meterH * (1.0f - peakNorm);
        float rmsY  = y + meterH * (1.0f - rmsNorm);

        // RMS bar with gradient (green -> yellow -> red)
        if (rmsNorm > 0.0f)
        {
            juce::ColourGradient grad (
                juce::Colour (0xff22cc44), x, y + meterH,       // Green at bottom
                juce::Colour (0xffcc2222), x, y, false);         // Red at top
            grad.addColour (0.7, juce::Colour (0xffcccc22));    // Yellow at 70%

            g.setGradientFill (grad);
            g.fillRect (x, rmsY, w, meterH - rmsY + y);
        }

        // Peak indicator (thin line)
        if (peakNorm > 0.01f)
        {
            juce::Colour peakColour = (peakDB > 0.0f) ? juce::Colour (0xffff3333)
                                                        : juce::Colour (0xffcccccc);
            g.setColour (peakColour);
            g.fillRect (x, peakY, w, 1.5f);
        }

        // Meter outline
        g.setColour (ColourPalette::gridLine());
        g.drawRect (x, y, w, meterH, 0.5f);
    }

    AdvancedEQProcessor& processor;
    float peakL = -100.0f;
    float peakR = -100.0f;
    float rmsL  = -100.0f;
    float rmsR  = -100.0f;
};
