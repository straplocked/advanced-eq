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
 * @file FrequencyUtils.h
 * @brief MIDI note and frequency conversion utilities.
 */

#include <juce_core/juce_core.h>
#include <cmath>

/**
 * @namespace FrequencyUtils
 * @brief Conversion utilities between MIDI note numbers, frequencies, note names,
 *        and human-readable frequency strings.
 *
 * Uses the A4 = 440 Hz, MIDI note 69 convention.
 */
namespace FrequencyUtils
{
    // MIDI note number to frequency
    /** @brief Converts a MIDI note number to its frequency in Hz.
     *  @param note MIDI note number (e.g., 69 = A4).
     *  @return Frequency in Hz (A4 = 440 Hz convention).
     */
    inline float midiNoteToFrequency (int note)
    {
        return 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
    }

    // Frequency to nearest MIDI note
    /** @brief Converts a frequency in Hz to the nearest MIDI note number.
     *  @param freq Frequency in Hz.
     *  @return The closest MIDI note number (A4 = 69 convention).
     */
    inline int frequencyToMidiNote (float freq)
    {
        return juce::roundToInt (69.0f + 12.0f * std::log2 (freq / 440.0f));
    }

    // Note name from MIDI note number
    /** @brief Converts a MIDI note number to a note name string (e.g., "A4", "C#3").
     *  @param note MIDI note number.
     *  @return Note name with octave (e.g., "C4", "F#5").
     */
    inline juce::String midiNoteToName (int note)
    {
        static const char* noteNames[] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        int octave = (note / 12) - 1;
        int nameIndex = note % 12;
        return juce::String (noteNames[nameIndex]) + juce::String (octave);
    }

    // Frequency to note name string
    /** @brief Converts a frequency in Hz to its nearest note name string.
     *  @param freq Frequency in Hz.
     *  @return The note name of the nearest MIDI note (e.g., "A4").
     */
    inline juce::String frequencyToNoteName (float freq)
    {
        int note = frequencyToMidiNote (freq);
        return midiNoteToName (note);
    }

    // Format frequency for display
    /** @brief Formats a frequency value as a human-readable string.
     *
     * Returns Hz for values below 1 kHz, and kHz for values at or above 1 kHz.
     * Precision varies by range: one decimal for < 100 Hz, no decimals for
     * 100-999 Hz, two decimals for 1-9.99 kHz, one decimal for >= 10 kHz.
     *
     *  @param freq Frequency in Hz.
     *  @return Formatted string (e.g., "440 Hz", "1.00 kHz", "12.5 kHz").
     */
    inline juce::String formatFrequency (float freq)
    {
        if (freq < 100.0f)
            return juce::String (freq, 1) + " Hz";
        if (freq < 1000.0f)
            return juce::String (freq, 0) + " Hz";
        if (freq < 10000.0f)
            return juce::String (freq / 1000.0f, 2) + " kHz";
        return juce::String (freq / 1000.0f, 1) + " kHz";
    }
}
