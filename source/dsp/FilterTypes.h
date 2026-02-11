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
 * @file FilterTypes.h
 * @brief Core enumeration types and slope utility functions for the Advanced EQ DSP engine.
 *
 * This header defines the vocabulary types shared across the entire plugin:
 * filter shapes, stereo routing modes, processing latency modes, and slope
 * configuration helpers. Every DSP class and most UI classes depend on these
 * definitions.
 */

/**
 * @brief Identifies the transfer-function shape of an equalizer band.
 *
 * Each value selects a distinct second-order (or first-order, depending on
 * slope) filter topology implemented by SVFFilter and CascadedFilter.
 *
 * @see SVFFilter::setParameters, CascadedFilter::setParameters
 */
enum class FilterType
{
    Bell = 0,       ///< Parametric peaking/bell filter (constant-Q by default).
    LowShelf,       ///< Low-frequency shelving filter; boosts or cuts below the corner frequency.
    HighShelf,      ///< High-frequency shelving filter; boosts or cuts above the corner frequency.
    HighPass,       ///< High-pass (low-cut) filter; attenuates frequencies below the cutoff.
    LowPass,        ///< Low-pass (high-cut) filter; attenuates frequencies above the cutoff.
    Notch,          ///< Band-reject (notch) filter; deep null at the center frequency.
    BandPass,       ///< Band-pass filter; passes a narrow band around the center frequency.
    TiltShelf,      ///< Tilt shelf: simultaneous low-shelf cut and high-shelf boost (or vice versa) pivoted at the center frequency.
    FlatTilt,       ///< Flat tilt: gentle broadband spectral tilt across the full spectrum.
    AllPass         ///< All-pass: flat magnitude response with frequency-dependent phase rotation.
};

/**
 * @brief Stereo routing mode for an individual EQ band.
 *
 * Determines which channel(s) or encoding domain a band operates on
 * within the four-pass processing model of ProcessingChain.
 *
 * @see ProcessingChain, EqBand::processSingleChannel
 */
enum class StereoPlacement
{
    Stereo = 0,     ///< Band processes both left and right channels identically (Pass 1).
    Left,           ///< Band processes only the left channel (Pass 2).
    Right,          ///< Band processes only the right channel (Pass 3).
    Mid,            ///< Band processes the mid (sum) component after M/S encoding (Pass 4).
    Side            ///< Band processes the side (difference) component after M/S encoding (Pass 4).
};

/**
 * @brief Global processing latency mode for the plugin.
 *
 * Selects the DSP engine variant that trades latency for phase accuracy.
 * Currently only ZeroLatency is implemented; NaturalPhase and LinearPhase
 * are reserved for future phases of development.
 */
enum class ProcessingMode
{
    ZeroLatency = 0, ///< Minimum-phase IIR processing with zero added latency.
    NaturalPhase,    ///< Mixed-phase mode (planned); reduced pre-ringing compared to linear phase.
    LinearPhase      ///< FIR-based linear-phase mode (planned); zero phase distortion at the cost of latency.
};

// Slope in dB/oct to number of biquad sections

/**
 * @brief Convert a slope menu index (0-8) to the corresponding dB/octave value.
 *
 * The mapping is: 0 -> 6, 1 -> 12, 2 -> 18, 3 -> 24, 4 -> 36, 5 -> 48,
 * 6 -> 72, 7 -> 96, 8 -> 0 (brickwall). Out-of-range indices default to 12 dB/oct.
 *
 * @param index  Zero-based slope menu index.
 * @return       Slope value in dB per octave (0 denotes brickwall).
 */
inline int slopeIndexToDBPerOct (int index)
{
    constexpr int slopes[] = { 6, 12, 18, 24, 36, 48, 72, 96, 0 }; // 0 = brickwall
    return (index >= 0 && index < 9) ? slopes[index] : 12;
}

/**
 * @brief Information about how many SVF sections are required for a given slope
 *        and whether the final section must operate in first-order mode.
 */
struct SlopeInfo
{
    int numSections;          // Total SVF sections needed
    bool lastIsFirstOrder;    // True if the last section should be first-order (odd order)
};

/**
 * @brief Determine the SVF section count and first-order tail flag for a given slope.
 *
 * For slopes that correspond to an even filter order (12, 24, 48, 96 dB/oct),
 * all sections are second-order. For odd orders (6, 18, 36, 72 dB/oct),
 * the final section is switched to first-order mode to achieve the correct
 * overall slope. The special value 0 (brickwall) maps to 8 second-order
 * sections (96 dB/oct equivalent).
 *
 * Filter order N = dbPerOct / 6. Number of sections = ceil(N / 2).
 *
 * @param dbPerOct  Slope in dB per octave (6, 12, 18, 24, 36, 48, 72, 96, or 0 for brickwall).
 * @return          SlopeInfo with section count and first-order flag.
 */
inline SlopeInfo getSlopeInfo (int dbPerOct)
{
    if (dbPerOct == 0) return { 8, false };    // Brickwall: 8 second-order sections
    int order = dbPerOct / 6;                  // 6->1, 12->2, 18->3, 24->4, etc.
    bool isOdd = (order % 2) != 0;
    int sections = (order + 1) / 2;            // Ceiling division by 2
    return { sections, isOdd };
}
