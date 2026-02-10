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
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Background circle
        g.setColour (ColourPalette::knobBackground());
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Arc
        juce::Path arc;
        arc.addCentredArc (centreX, centreY, radius - 2.0f, radius - 2.0f,
                          0.0f, rotaryStartAngle, angle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (arc, juce::PathStrokeType (2.5f));

        // Pointer
        juce::Path pointer;
        auto pointerLength = radius * 0.6f;
        pointer.addRoundedRectangle (-1.5f, -pointerLength, 3.0f, pointerLength, 1.0f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (ColourPalette::textBright());
        g.fillPath (pointer);
    }
};
