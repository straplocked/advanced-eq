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
 * @file ParameterIDs.h
 * @brief Centralized parameter ID definitions for AudioProcessorValueTreeState.
 */

#include <juce_core/juce_core.h>

/**
 * @brief Namespace containing all parameter ID constants and helper functions for APVTS.
 *
 * Provides band-specific parameter ID generation via bandParam(), per-band
 * parameter suffix constants (kActive through kDynRelease), global parameter
 * ID constants (kOutputGain through kAnalogMode), and name-list accessors
 * that return StringArrays matching their respective enum orders.
 */
namespace ParamIDs
{
    /**
     * @brief Generates a band-specific parameter ID string.
     *
     * Produces IDs in the format "band_XX_suffix", where XX is the zero-padded
     * band index (e.g., "band_00_freq", "band_11_gain").
     *
     * @param bandIndex Zero-based band index (0..23).
     * @param param Parameter suffix string (e.g., ParamIDs::kFrequency).
     * @return Formatted parameter ID string.
     */
    // Generates band-specific parameter IDs: "band_00_freq", "band_01_gain", etc.
    inline juce::String bandParam (int bandIndex, const juce::String& param)
    {
        return "band_" + juce::String (bandIndex).paddedLeft ('0', 2) + "_" + param;
    }

    // Per-band parameter suffixes
    constexpr const char* kActive     = "active";
    constexpr const char* kType       = "type";
    constexpr const char* kFrequency  = "freq";
    constexpr const char* kGain       = "gain";
    constexpr const char* kQ          = "q";
    constexpr const char* kSlope      = "slope";
    constexpr const char* kStereo     = "stereo";
    constexpr const char* kDynEnabled = "dyn_on";
    constexpr const char* kDynThresh  = "dyn_thresh";
    constexpr const char* kDynRange   = "dyn_range";
    constexpr const char* kDynAttack  = "dyn_attack";
    constexpr const char* kDynRelease = "dyn_release";

    // Global parameters
    constexpr const char* kOutputGain  = "output_gain";
    constexpr const char* kOutputPan   = "output_pan";
    constexpr const char* kProcessMode = "process_mode";
    constexpr const char* kOversample  = "oversample";
    constexpr const char* kAutoGain    = "auto_gain";
    constexpr const char* kAnalyzerOn  = "analyzer_on";
    constexpr const char* kAnalyzerPre = "analyzer_pre";
    constexpr const char* kAnalyzerRes = "analyzer_res";
    constexpr const char* kAnalyzerSpd = "analyzer_speed";
    constexpr const char* kAnalogMode  = "analog_mode";
    constexpr const char* kPhaseInvert = "phase_invert";
    constexpr const char* kMidiProgramChange = "midi_pc";
    constexpr const char* kOutputWidth = "output_width";
    constexpr const char* kLinearPhaseQuality = "lp_quality";
    constexpr const char* kDynSidechain = "dyn_sc";

    // Per-band spectral dynamics suffixes
    constexpr const char* kSpectralOn     = "spectral_on";
    constexpr const char* kSpectralThresh = "spectral_thresh";
    constexpr const char* kSpectralRange  = "spectral_range";
    constexpr const char* kSpectralRatio  = "spectral_ratio";

    inline juce::StringArray getCharacterModeNames()
    {
        return { "Clean", "Gentle", "Warm" };
    }

    inline juce::StringArray getLinearPhaseQualityNames()
    {
        return { "Low", "Medium", "High" };
    }

    /**
     * @brief Returns the display names for all filter types.
     *
     * The order matches the FilterType enum: Bell, Low Shelf, High Shelf,
     * High Pass, Low Pass, Notch, Band Pass, Tilt Shelf, Flat Tilt.
     *
     * @return StringArray of filter type display names.
     */
    // Filter type names (matching FilterType enum order)
    inline juce::StringArray getFilterTypeNames()
    {
        return { "Bell", "Low Shelf", "High Shelf", "High Pass",
                 "Low Pass", "Notch", "Band Pass", "Tilt Shelf", "Flat Tilt", "All Pass" };
    }

    /**
     * @brief Returns the display names for all filter slope options.
     *
     * Indices 0..8 map to: 6, 12, 18, 24, 36, 48, 72, 96 dB/oct, and Brickwall.
     *
     * @return StringArray of slope display names.
     */
    // Slope names
    inline juce::StringArray getSlopeNames()
    {
        return { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct",
                 "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct", "Brickwall" };
    }

    /**
     * @brief Returns the display names for stereo placement options.
     *
     * Indices 0..4 map to: Stereo, Left, Right, Mid, Side.
     *
     * @return StringArray of stereo placement display names.
     */
    // Stereo placement names
    inline juce::StringArray getStereoPlacementNames()
    {
        return { "Stereo", "Left", "Right", "Mid", "Side" };
    }

    /**
     * @brief Returns the display names for processing mode options.
     *
     * The order matches the ProcessingMode enum: Zero Latency, Natural Phase,
     * Linear Phase.
     *
     * @return StringArray of processing mode display names.
     */
    // Processing mode names
    inline juce::StringArray getProcessingModeNames()
    {
        return { "Zero Latency", "Natural Phase", "Linear Phase" };
    }

    /**
     * @brief Returns the display names for oversampling factor options.
     * @return StringArray of oversampling display names (1x, 2x, 4x).
     */
    // Oversampling names
    inline juce::StringArray getOversamplingNames()
    {
        return { "1x", "2x", "4x" };
    }
}
