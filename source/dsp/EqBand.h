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
 * @file EqBand.h
 * @brief Single parametric EQ band with dual cascaded filters and parameter smoothing.
 *
 * Each EqBand owns two CascadedFilter instances (filterL and filterR) for
 * independent left/right processing, plus JUCE SmoothedValue objects for
 * zipper-free parameter transitions. The band may also operate on a single
 * channel when used in Left-only, Right-only, Mid, or Side routing modes
 * within ProcessingChain's four-pass architecture.
 *
 * When analog mode is enabled, a soft-saturation waveshaper is applied
 * per-band to emulate the subtle harmonic coloration of analog equalizer
 * circuits.
 *
 * @see CascadedFilter, ProcessingChain
 */

#include "CascadedFilter.h"
#include "FilterTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <complex>

/**
 * @brief Plain-data structure holding all user-facing parameters for a single EQ band.
 *
 * Instances of this struct are populated by BandParameterAttachment from
 * the AudioProcessorValueTreeState and passed to EqBand::updateParameters()
 * on each processing block.
 */
struct BandParameters
{
    bool active = false;                            ///< Whether this band is enabled (bypassed when false).
    FilterType type = FilterType::Bell;             ///< Filter shape / transfer function type.
    float frequency = 1000.0f;                      ///< Center or cutoff frequency in Hz (20-20000).
    float gain = 0.0f;                              ///< Boost/cut in dB (used by Bell and Shelf types).
    float q = 1.0f;                                 ///< Quality factor / resonance (higher = narrower).
    int slopeDBPerOct = 12;                         ///< Filter slope in dB per octave (6-96, or 0 for brickwall).
    StereoPlacement stereoPlacement = StereoPlacement::Stereo; ///< Stereo routing mode for this band.

    // Dynamic EQ (Phase 8)
    bool dynamicEnabled = false;                    ///< Enable dynamic (sidechain-driven) gain modulation.
    float dynamicThreshold = -20.0f;                ///< Threshold level in dBFS for dynamic EQ engagement.
    float dynamicRange = 0.0f;                      ///< Maximum dynamic gain range in dB (0 = no dynamic action).
    float dynamicAttack = 10.0f;                    ///< Attack time in milliseconds for the dynamic envelope follower.
    float dynamicRelease = 100.0f;                  ///< Release time in milliseconds for the dynamic envelope follower.
};

/**
 * @brief A single parametric equalizer band with stereo CascadedFilter pair and parameter smoothing.
 *
 * Signal flow:
 *   1. Parameters are smoothed (frequency, gain, Q) via juce::SmoothedValue.
 *   2. CascadedFilter instances (filterL, filterR) are updated with the
 *      smoothed values each block.
 *   3. Audio is processed through filterL and/or filterR depending on the
 *      StereoPlacement routing mode.
 *   4. If analog mode is active, a soft-saturation waveshaper is applied
 *      to introduce subtle harmonic content.
 *
 * @see CascadedFilter, ProcessingChain, BandParameters
 */
class EqBand
{
public:
    EqBand() = default;

    /**
     * @brief Initialize the band for a given sample rate and maximum block size.
     *
     * Prepares both CascadedFilter instances and configures SmoothedValue
     * ramp lengths for the given block size.
     *
     * @param sampleRate    Host sample rate in Hz.
     * @param maxBlockSize  Maximum expected samples per processing block.
     */
    void prepare (double sampleRate, int maxBlockSize);

    /**
     * @brief Reset all filter states and smoothing ramps.
     *
     * Call on transport restart or audio discontinuity.
     */
    void reset();

    /**
     * @brief Update all band parameters from a BandParameters snapshot.
     *
     * Sets smoothing targets for frequency, gain, and Q. Filter type, slope,
     * and stereo placement changes take effect immediately (no smoothing).
     *
     * @param params  New parameter values for this band.
     */
    void updateParameters (const BandParameters& params);

    /**
     * @brief Enable or disable per-band analog saturation.
     *
     * When enabled, a soft-clipping waveshaper (tanh-based) is applied to the
     * output of each filter section, emulating the nonlinear behavior of
     * analog equalizer circuits.
     *
     * @param enabled  True to enable analog saturation.
     */
    void setAnalogMode (bool enabled);

    // Process stereo buffer (both L and R channels)

    /**
     * @brief Process a stereo audio buffer through both left and right filters.
     *
     * Reads channels 0 (left) and 1 (right) from the buffer, applies the
     * cascaded filter to each, and writes the results back in-place.
     *
     * @param buffer      Stereo juce::AudioBuffer (must have at least 2 channels).
     * @param numSamples  Number of samples to process in this block.
     */
    void process (juce::AudioBuffer<float>& buffer, int numSamples);

    // Process a single channel (uses filterL only). Used for L/R/Mid/Side routing.

    /**
     * @brief Process a single channel of audio through filterL only.
     *
     * Used by ProcessingChain for Left, Right, Mid, and Side routing modes
     * where only one filter instance is needed.
     *
     * @param channelData  Pointer to the mono sample array (modified in-place).
     * @param numSamples   Number of samples to process.
     */
    void processSingleChannel (float* channelData, int numSamples);

    // For UI: compute magnitude response at given frequencies

    /**
     * @brief Compute the magnitude response curve for UI display.
     *
     * Evaluates the composite frequency response of the left filter (filterL)
     * at each specified frequency and writes the magnitude in dB to the
     * output array. Uses CascadedFilter::getResponse() internally.
     *
     * @param frequencies  Array of frequencies in Hz at which to evaluate.
     * @param magnitudes   Output array for magnitude values in dB (same length as frequencies).
     * @param numPoints    Number of frequency points to evaluate.
     */
    void computeMagnitudeResponse (const double* frequencies, double* magnitudes,
                                   int numPoints) const;

    /**
     * @brief Check whether this band is active (processing audio).
     * @return True if the band is enabled.
     */
    bool isActive() const { return active; }

    /**
     * @brief Get the current filter type of this band.
     * @return The active FilterType enum value.
     */
    FilterType getFilterType() const { return filterType; }

    /**
     * @brief Get the current stereo placement routing mode.
     * @return The active StereoPlacement enum value.
     */
    StereoPlacement getStereoPlacement() const { return stereoPlacement; }

    /**
     * @brief Get a const reference to the current BandParameters snapshot.
     * @return The most recently applied BandParameters.
     */
    const BandParameters& getCurrentParams() const { return currentParams; }

private:
    CascadedFilter filterL;
    CascadedFilter filterR;

    BandParameters currentParams;
    bool active = false;
    FilterType filterType = FilterType::Bell;
    StereoPlacement stereoPlacement = StereoPlacement::Stereo;

    // Smoothed parameters for zipper-free changes
    juce::SmoothedValue<float> smoothedFreq { 1000.0f };
    juce::SmoothedValue<float> smoothedGain { 0.0f };
    juce::SmoothedValue<float> smoothedQ { 1.0f };

    double sampleRate = 44100.0;
    bool needsUpdate = true;
    bool analogMode = false;

    static float saturate (float x);
};
