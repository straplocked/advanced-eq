#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ColourPalette.h"

class AdvancedEqLookAndFeel : public juce::LookAndFeel_V4
{
public:
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
