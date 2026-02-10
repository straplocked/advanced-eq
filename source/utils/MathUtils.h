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
 * @file MathUtils.h
 * @brief Math helpers for DSP and UI coordinate mapping.
 */

#include <cmath>
#include "Constants.h"

/**
 * @namespace MathUtils
 * @brief Provides decibel/gain conversions, logarithmic frequency-to-pixel mapping,
 *        and gain-to-pixel mapping for DSP calculations and UI rendering.
 */
namespace MathUtils
{
    // dB <-> linear gain conversions
    /** @brief Converts a decibel value to a linear gain factor.
     *  @param dB The value in decibels.
     *  @return The equivalent linear gain (e.g., 0 dB -> 1.0, -6 dB -> ~0.5).
     */
    inline float dbToGain (float dB) { return std::pow (10.0f, dB / 20.0f); }

    /** @brief Converts a linear gain factor to decibels.
     *  @param gain The linear gain value (clamped to a minimum of 1e-10 internally).
     *  @return The equivalent value in decibels.
     */
    inline float gainToDb (float gain) { return 20.0f * std::log10 (std::max (gain, 1e-10f)); }

    // Frequency <-> normalized X coordinate (logarithmic mapping)
    /** @brief Maps a frequency to a normalized [0, 1] value using logarithmic scaling.
     *
     * Maps Constants::kMinFrequency to 0.0 and Constants::kMaxFrequency to 1.0.
     *
     *  @param freq Frequency in Hz.
     *  @return Normalized position in [0, 1].
     */
    inline float frequencyToNormalized (float freq)
    {
        return std::log (freq / Constants::kMinFrequency)
             / std::log (Constants::kMaxFrequency / Constants::kMinFrequency);
    }

    /** @brief Maps a normalized [0, 1] value back to a frequency using logarithmic scaling.
     *
     * Inverse of frequencyToNormalized().
     *
     *  @param normalized Position in [0, 1].
     *  @return Frequency in Hz.
     */
    inline float normalizedToFrequency (float normalized)
    {
        return Constants::kMinFrequency
             * std::pow (Constants::kMaxFrequency / Constants::kMinFrequency, normalized);
    }

    // Map frequency to pixel X coordinate
    /** @brief Maps a frequency to a pixel X coordinate.
     *  @param freq  Frequency in Hz.
     *  @param width The total width of the display area in pixels.
     *  @return The X pixel coordinate.
     */
    inline float frequencyToX (float freq, float width)
    {
        return frequencyToNormalized (freq) * width;
    }

    /** @brief Maps a pixel X coordinate back to a frequency.
     *  @param x     The X pixel coordinate.
     *  @param width The total width of the display area in pixels.
     *  @return Frequency in Hz.
     */
    inline float xToFrequency (float x, float width)
    {
        return normalizedToFrequency (x / width);
    }

    // Map gain (dB) to pixel Y coordinate
    /** @brief Maps a gain value in dB to a pixel Y coordinate.
     *
     * The Y axis is inverted: higher gain values map to lower Y coordinates.
     *
     *  @param gainDB Gain in decibels.
     *  @param height The total height of the display area in pixels.
     *  @param minDB  The minimum displayed dB value (bottom of the area).
     *  @param maxDB  The maximum displayed dB value (top of the area).
     *  @return The Y pixel coordinate.
     */
    inline float gainToY (float gainDB, float height, float minDB, float maxDB)
    {
        return height * (1.0f - (gainDB - minDB) / (maxDB - minDB));
    }

    /** @brief Maps a pixel Y coordinate back to a gain value in dB.
     *  @param y      The Y pixel coordinate.
     *  @param height The total height of the display area in pixels.
     *  @param minDB  The minimum displayed dB value (bottom of the area).
     *  @param maxDB  The maximum displayed dB value (top of the area).
     *  @return Gain in decibels.
     */
    inline float yToGain (float y, float height, float minDB, float maxDB)
    {
        return minDB + (1.0f - y / height) * (maxDB - minDB);
    }
}
