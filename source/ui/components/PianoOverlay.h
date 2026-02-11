/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../utils/FrequencyUtils.h"
#include "../../utils/MathUtils.h"
#include "../LookAndFeel/ColourPalette.h"

class PianoOverlay : public juce::Component
{
public:
    PianoOverlay() { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        if (! visible)
            return;

        float w = (float) getWidth();
        float h = (float) getHeight();

        // Draw note markers for C notes (C0 through C10)
        for (int octave = 0; octave <= 10; ++octave)
        {
            int midiNote = (octave + 1) * 12; // C notes: MIDI 12, 24, 36...
            float freq = FrequencyUtils::midiNoteToFrequency (midiNote);
            float x = MathUtils::frequencyToX (freq, w);

            if (x < 0 || x > w)
                continue;

            // Draw key indicator line
            g.setColour (juce::Colour (0x20ffffff));
            g.drawVerticalLine ((int) x, 0.0f, h);

            // Draw note name
            juce::String noteName = "C" + juce::String (octave);
            g.setColour (ColourPalette::axisText().withAlpha (0.6f));
            g.setFont (9.0f);
            g.drawText (noteName, (int) x - 10, (int) h - 14, 20, 12,
                        juce::Justification::centred, false);
        }

        // Draw markers for all white/black keys in visible range (simplified)
        static const bool isBlack[] = { false, true, false, true, false, false, true, false, true, false, true, false };

        for (int note = 24; note <= 120; ++note) // C1 to C9
        {
            float freq = FrequencyUtils::midiNoteToFrequency (note);
            float x = MathUtils::frequencyToX (freq, w);

            if (x < 0 || x > w)
                continue;

            int noteInOctave = note % 12;
            if (isBlack[noteInOctave])
            {
                // Black key: small tick
                g.setColour (juce::Colour (0x10ffffff));
                g.fillRect (x - 0.5f, h - 4.0f, 1.0f, 4.0f);
            }
        }
    }

    void setVisible (bool shouldBeVisible)
    {
        visible = shouldBeVisible;
        repaint();
    }

    bool isOverlayVisible() const { return visible; }

private:
    bool visible = false;
};
