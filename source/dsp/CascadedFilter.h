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
 * @file CascadedFilter.h
 * @brief Higher-order filter implementation via cascaded SVFFilter sections.
 *
 * Achieves steep filter slopes (up to 96 dB/oct) by chaining multiple
 * second-order SVFFilter sections in series. For Butterworth alignment the
 * Q of each section is set according to the standard pole-placement formula:
 *
 *     Q_k = 1 / (2 * cos(pi * (2k + 1) / (2N)))
 *
 * where N is the total filter order and k is the zero-based section index.
 * This yields maximally-flat magnitude response in the passband.
 *
 * For odd-order slopes (6, 18, 36, 72 dB/oct) the last section is switched
 * to first-order mode via SVFFilter::setFirstOrder(true).
 *
 * @see SVFFilter
 */

#include "SVFFilter.h"
#include "FilterTypes.h"
#include <array>
#include <complex>

// Cascades multiple SVF sections for higher-order filter slopes.
// 12 dB/oct = 1 section, 24 = 2, 48 = 4, 96 = 8, etc.

/**
 * @brief Cascaded second-order SVF sections forming a higher-order filter.
 *
 * Chains up to kMaxSections (8) SVFFilter instances in series. Each section
 * is configured with the appropriate Butterworth Q for its position in the
 * cascade, and the composite frequency response is the product of all
 * individual section responses.
 *
 * Slope mapping:
 *   - 12 dB/oct = 1 section (order 2)
 *   - 24 dB/oct = 2 sections (order 4)
 *   - 48 dB/oct = 4 sections (order 8)
 *   - 96 dB/oct = 8 sections (order 16)
 *   - Brickwall  = 8 sections (maximum attenuation)
 *
 * For Bell, Shelf, Notch, BandPass, TiltShelf, and FlatTilt types each
 * section receives the same user Q value rather than Butterworth placement,
 * since those shapes have a different design intent.
 *
 * @see SVFFilter, EqBand
 */
class CascadedFilter
{
public:
    /** @brief Maximum number of SVF sections in the cascade (supports up to 96 dB/oct). */
    static constexpr int kMaxSections = 8; // 96 dB/oct max

    CascadedFilter() = default;

    /**
     * @brief Initialize all sections for a given sample rate.
     *
     * Propagates the sample rate to every SVFFilter section and resets state.
     *
     * @param sampleRate  Host sample rate in Hz.
     */
    void prepare (double sampleRate);

    /**
     * @brief Clear the internal state of all SVF sections.
     *
     * Should be called on transport restart or when the audio stream
     * is discontinuous to prevent clicks from stale integrator states.
     */
    void reset();

    /**
     * @brief Configure the cascaded filter with the given parameters and slope.
     *
     * Computes the number of active sections from the slope, assigns
     * Butterworth Q values for HP/LP types (or the user Q for other types),
     * and forwards the configuration to each active SVFFilter section.
     *
     * @param type          Filter shape (Bell, HighPass, LowPass, etc.).
     * @param freq          Center or cutoff frequency in Hz.
     * @param Q             Quality factor (used directly for non-HP/LP types;
     *                      overridden by Butterworth Q for HP/LP cascades).
     * @param gainDB        Gain in dB (applicable to Bell/Shelf types).
     * @param slopeDBPerOct Slope in dB per octave (6, 12, 18, 24, 36, 48, 72, 96, or 0 for brickwall).
     */
    void setParameters (FilterType type, double freq, double Q,
                        double gainDB, int slopeDBPerOct);

    /**
     * @brief Configure the filter with a continuous (fractional) slope value.
     *
     * For slopes between standard integer values (e.g. 14.2 dB/oct between 12 and 18),
     * processes through both adjacent cascades and crossfades the output.
     * When the slope is close to a standard value (within 0.5 dB/oct), snaps to it.
     *
     * @param type          Filter shape.
     * @param freq          Center or cutoff frequency in Hz.
     * @param Q             Quality factor.
     * @param gainDB        Gain in dB.
     * @param slopeDBPerOct Continuous slope in dB per octave (6.0 to 96.0).
     */
    void setParametersFractional (FilterType type, double freq, double Q,
                                   double gainDB, double slopeDBPerOct);

    /**
     * @brief Process a block of audio samples in-place through all active sections.
     *
     * Samples pass through each active SVFFilter section sequentially.
     *
     * @param buffer      Pointer to the audio sample array (modified in-place).
     * @param numSamples  Number of samples in the buffer.
     */
    void process (float* buffer, int numSamples);

    /**
     * @brief Process a single audio sample through all active sections.
     *
     * @param input  Input sample value.
     * @return       Output sample after cascaded filtering.
     */
    float processSample (float input);

    /**
     * @brief Evaluate the composite complex frequency response at a given frequency.
     *
     * Returns the product of getResponse() from all active SVFFilter sections,
     * representing the total magnitude and phase of the cascade.
     *
     * @param freq  Frequency in Hz at which to evaluate.
     * @return      Composite complex frequency response H_total(f) = H_1(f) * H_2(f) * ... * H_N(f).
     */
    std::complex<double> getResponse (double freq) const;

    /**
     * @brief Get the composite magnitude response in decibels at a given frequency.
     *
     * Convenience wrapper: returns 20 * log10(abs(getResponse(freq))).
     *
     * @param freq  Frequency in Hz.
     * @return      Composite magnitude in dB.
     */
    double getMagnitudeAtFrequency (double freq) const;

    /**
     * @brief Query the number of SVF sections currently in use.
     * @return Number of active sections (1 to kMaxSections).
     */
    int getNumActiveSections() const { return numActiveSections; }

    /** Returns true if currently in fractional slope crossfade mode. */
    bool isFractionalMode() const { return fractionalMode; }

    /** Process a block through fractional slope crossfade. */
    void processFractional (float* buffer, int numSamples);

    /** Process a single sample through fractional slope crossfade. */
    float processSampleFractional (float input);

private:
    std::array<SVFFilter, kMaxSections> sections;
    int numActiveSections = 1;
    double sampleRate = 44100.0;

    // Fractional slope crossfade
    bool fractionalMode = false;
    float fractionalMix = 0.0f; // 0.0 = lower slope, 1.0 = upper slope
    std::array<SVFFilter, kMaxSections> sectionsB; // Second cascade for upper slope
    int numActiveSectionsB = 1;
};
