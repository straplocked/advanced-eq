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
 * @file AdvancedEqLookAndFeel.h
 * @brief Custom JUCE LookAndFeel for studio rack gear visual style.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "ColourPalette.h"

/**
 * @class AdvancedEqLookAndFeel
 * @brief Extends LookAndFeel_V4 with custom rendering for rotary sliders, buttons,
 *        and combo boxes styled after studio rack gear.
 *
 * Rotary sliders are drawn as metallic knobs with a radial gradient body, rim highlight,
 * value arc, and pointer dot. Buttons use a vertical gradient fill with bevel lines and
 * hover/press states. Combo boxes are rendered with a panel gradient and dropdown arrow.
 *
 * All color values are sourced from ColourPalette.
 *
 * @see ColourPalette
 */
class AdvancedEqLookAndFeel : public juce::LookAndFeel_V4
{
public:
    /** @brief Constructs the look-and-feel and sets default JUCE colour IDs from ColourPalette. */
    AdvancedEqLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, ColourPalette::background());
        setColour (juce::Slider::rotarySliderFillColourId, ColourPalette::knobForeground());
        setColour (juce::Slider::rotarySliderOutlineColourId, ColourPalette::knobBackground());
        setColour (juce::Slider::thumbColourId, ColourPalette::textBright());
        setColour (juce::Label::textColourId, ColourPalette::textBright());
        setColour (juce::ComboBox::backgroundColourId, ColourPalette::knobBackground());
        setColour (juce::ComboBox::textColourId, ColourPalette::textBright());
        setColour (juce::ComboBox::outlineColourId, ColourPalette::gridLine());
        setColour (juce::TextButton::buttonColourId, ColourPalette::knobBackground());
        setColour (juce::TextButton::textColourOnId, ColourPalette::textBright());
        setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
        setColour (juce::PopupMenu::backgroundColourId, ColourPalette::controlBar());
        setColour (juce::PopupMenu::textColourId, ColourPalette::textBright());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ColourPalette::knobBackground());
        setColour (juce::PopupMenu::highlightedTextColourId, ColourPalette::textBright());
    }

    /** @brief Draws a metallic rotary knob with a value arc, pointer dot, and notch marks.
     *  @param g                The graphics context.
     *  @param x                Left edge of the knob area.
     *  @param y                Top edge of the knob area.
     *  @param width            Width of the knob area.
     *  @param height           Height of the knob area.
     *  @param sliderPos        Normalized slider position in [0, 1].
     *  @param rotaryStartAngle Start angle of the rotary arc in radians.
     *  @param rotaryEndAngle   End angle of the rotary arc in radians.
     *  @param slider           The slider component being drawn.
     */
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Drop shadow (offset below)
        g.setColour (ColourPalette::knobShadow());
        g.fillEllipse (centreX - radius, centreY - radius + 1.5f, radius * 2.0f, radius * 2.0f);

        // Knob body — radial gradient (lighter centre, darker edge)
        juce::ColourGradient bodyGrad (ColourPalette::knobEdge(), centreX, centreY,
                                       ColourPalette::knobBackground(), centreX, centreY + radius,
                                       true); // radial
        g.setGradientFill (bodyGrad);
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Rim highlight (top-left arc, ~10 o'clock to ~2 o'clock)
        juce::Path rimHighlight;
        rimHighlight.addCentredArc (centreX, centreY, radius - 0.5f, radius - 0.5f,
                                    0.0f, -2.4f, -0.7f, true);
        g.setColour (ColourPalette::bevelLight().withMultipliedAlpha (2.0f));
        g.strokePath (rimHighlight, juce::PathStrokeType (1.0f));

        // Track arc (full sweep, dim)
        juce::Path trackArc;
        trackArc.addCentredArc (centreX, centreY, radius - 3.0f, radius - 3.0f,
                               0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (ColourPalette::knobBackground().darker (0.3f));
        g.strokePath (trackArc, juce::PathStrokeType (3.0f));

        // Value arc (filled portion — accent colour)
        if (std::abs (sliderPos) > 0.001f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centreX, centreY, radius - 3.0f, radius - 3.0f,
                                    0.0f, rotaryStartAngle, angle, true);
            g.setColour (ColourPalette::knobIndicator());
            g.strokePath (valueArc, juce::PathStrokeType (3.0f));
        }

        // Pointer dot at the end of the arc
        float dotRadius = 2.5f;
        float dotDist = radius - 3.0f;
        float dotX = centreX + dotDist * std::sin (angle);
        float dotY = centreY - dotDist * std::cos (angle);
        g.setColour (ColourPalette::textBright());
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);

        // Notch marks at start/end
        g.setColour (ColourPalette::axisText().withAlpha (0.4f));
        for (float a : { rotaryStartAngle, rotaryEndAngle })
        {
            float innerR = radius + 1.0f;
            float outerR = radius + 3.0f;
            float nx1 = centreX + innerR * std::sin (a);
            float ny1 = centreY - innerR * std::cos (a);
            float nx2 = centreX + outerR * std::sin (a);
            float ny2 = centreY - outerR * std::cos (a);
            g.drawLine (nx1, ny1, nx2, ny2, 1.0f);
        }

        (void) slider;
    }

    /** @brief Draws a button background with gradient fill, bevel lines, and hover/press states.
     *  @param g                The graphics context.
     *  @param button           The button component being drawn.
     *  @param backgroundColour The base background colour (unused; ColourPalette is used instead).
     *  @param isMouseOverButton True if the mouse is hovering over the button.
     *  @param isButtonDown      True if the button is currently pressed.
     */
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        float cornerSize = 4.0f;

        bool isOn = button.getToggleState();

        // Gradient fill
        juce::ColourGradient grad (ColourPalette::panelTop(), 0, bounds.getY(),
                                   ColourPalette::panelBottom(), 0, bounds.getBottom(),
                                   false);
        if (isOn)
        {
            auto accent = button.findColour (juce::TextButton::buttonOnColourId);
            grad = juce::ColourGradient (accent.brighter (0.1f), 0, bounds.getY(),
                                         accent.darker (0.2f), 0, bounds.getBottom(),
                                         false);
        }

        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, cornerSize);

        // Bevel lines
        g.setColour (isOn ? ColourPalette::bevelLight().withMultipliedAlpha (1.5f)
                          : ColourPalette::bevelLight());
        g.drawLine (bounds.getX() + cornerSize, bounds.getY(),
                    bounds.getRight() - cornerSize, bounds.getY(), 1.0f);

        g.setColour (ColourPalette::bevelDark());
        g.drawLine (bounds.getX() + cornerSize, bounds.getBottom(),
                    bounds.getRight() - cornerSize, bounds.getBottom(), 1.0f);

        // Hover highlight
        if (isMouseOverButton)
        {
            g.setColour (juce::Colour (0x08ffffff));
            g.fillRoundedRectangle (bounds, cornerSize);
        }

        // Press darkening
        if (isButtonDown)
        {
            g.setColour (juce::Colour (0x10000000));
            g.fillRoundedRectangle (bounds, cornerSize);
        }

        // Subtle outline
        g.setColour (ColourPalette::gridLine());
        g.drawRoundedRectangle (bounds, cornerSize, 0.5f);
    }

    /** @brief Draws a combo box with a panel gradient, bevel lines, and a dropdown arrow.
     *  @param g            The graphics context.
     *  @param width        Width of the combo box area.
     *  @param height       Height of the combo box area.
     *  @param isButtonDown True if the dropdown button is currently pressed.
     *  @param buttonX      Unused button area X.
     *  @param buttonY      Unused button area Y.
     *  @param buttonW      Unused button area width.
     *  @param buttonH      Unused button area height.
     *  @param box          The combo box component being drawn.
     */
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        float cornerSize = 4.0f;

        // Gradient fill
        juce::ColourGradient grad (ColourPalette::panelTop(), 0, 0,
                                   ColourPalette::panelBottom(), 0, (float) height,
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, cornerSize);

        // Bevel
        g.setColour (ColourPalette::bevelLight());
        g.drawLine (bounds.getX() + cornerSize, bounds.getY(),
                    bounds.getRight() - cornerSize, bounds.getY(), 1.0f);
        g.setColour (ColourPalette::bevelDark());
        g.drawLine (bounds.getX() + cornerSize, bounds.getBottom(),
                    bounds.getRight() - cornerSize, bounds.getBottom(), 1.0f);

        // Outline
        g.setColour (ColourPalette::gridLine());
        g.drawRoundedRectangle (bounds, cornerSize, 0.5f);

        // Dropdown arrow
        float arrowSize = 6.0f;
        float arrowX = (float) width - 14.0f;
        float arrowY = (float) height * 0.5f;

        juce::Path arrow;
        arrow.addTriangle (arrowX - arrowSize * 0.5f, arrowY - 2.0f,
                           arrowX + arrowSize * 0.5f, arrowY - 2.0f,
                           arrowX, arrowY + 3.0f);

        g.setColour (isButtonDown ? ColourPalette::textBright() : ColourPalette::axisText());
        g.fillPath (arrow);

        (void) box;
    }
};
