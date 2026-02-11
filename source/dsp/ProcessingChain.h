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
 * @file ProcessingChain.h
 * @brief Main DSP processing pipeline orchestrating all 24 parametric EQ bands.
 *
 * ProcessingChain is the top-level audio processor called from
 * PluginProcessor::processBlock(). It implements a four-pass stereo
 * processing architecture:
 *
 *   - **Pass 1 (Stereo):** Bands with StereoPlacement::Stereo are applied
 *     to both L and R channels identically.
 *   - **Pass 2 (Left):** Bands with StereoPlacement::Left process only the
 *     left channel via processSingleChannel().
 *   - **Pass 3 (Right):** Bands with StereoPlacement::Right process only
 *     the right channel.
 *   - **Pass 4 (Mid/Side):** The stereo buffer is M/S encoded, Mid and Side
 *     bands are applied, and the buffer is decoded back to L/R.
 *
 * When analog mode is enabled, the chain also applies:
 *   - Per-band soft saturation (delegated to EqBand::setAnalogMode).
 *   - Inter-channel crosstalk at approximately -54 dB to emulate analog
 *     channel bleed.
 *   - A pink-weighted noise floor injected at approximately -100 dBFS to
 *     simulate the self-noise of analog circuitry.
 *
 * Output gain is applied with smoothing after all band processing.
 *
 * @see EqBand, PluginProcessor, MidSideProcessor
 */

#include "EqBand.h"
#include "FilterTypes.h"
#include "LinearPhaseEngine.h"
#include "NaturalPhaseEngine.h"
#include "../utils/Constants.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <juce_core/juce_core.h>

/**
 * @brief Top-level DSP chain that owns and orchestrates all EQ bands.
 *
 * Manages an array of Constants::kMaxBands (24) EqBand instances, routes
 * audio through the four-pass stereo model, and applies output gain with
 * optional analog-mode coloration.
 *
 * @see EqBand, PluginProcessor
 */
class ProcessingChain
{
public:
    ProcessingChain() = default;

    /**
     * @brief Initialize all bands and internal state for the given audio configuration.
     *
     * Propagates the sample rate and block size to every EqBand and
     * configures the output gain smoother.
     *
     * @param sampleRate   Host sample rate in Hz.
     * @param maxBlockSize Maximum expected samples per processing block.
     * @param numChannels  Number of audio channels (expected to be 2 for stereo).
     */
    void prepare (double sampleRate, int maxBlockSize, int numChannels);

    /**
     * @brief Reset all band filters and internal state.
     *
     * Call on transport restart or audio discontinuity.
     */
    void reset();

    // Main audio processing entry point

    /**
     * @brief Process a stereo audio buffer through the entire EQ chain.
     *
     * This is the main audio entry point, called from
     * PluginProcessor::processBlock(). Executes the four-pass routing model
     * (Stereo, Left, Right, Mid/Side), applies analog-mode effects if
     * enabled (saturation, crosstalk, noise floor), and applies smoothed
     * output gain.
     *
     * @param buffer  Stereo juce::AudioBuffer to process in-place.
     */
    void process (juce::AudioBuffer<float>& buffer);

    // Parameter updates

    /**
     * @brief Update the parameters of a specific EQ band.
     *
     * @param bandIndex  Zero-based band index (0 to Constants::kMaxBands - 1).
     * @param params     New parameter values for the band.
     */
    void updateBand (int bandIndex, const BandParameters& params);

    /**
     * @brief Set the output gain applied after all band processing.
     *
     * The gain is smoothed to avoid clicks during adjustment.
     *
     * @param gainDB  Output gain in decibels (0 dB = unity).
     */
    void setOutputGain (float gainDB);

    /**
     * @brief Set the oversampling factor.
     *
     * @param factorIndex  0 = 1x (off), 1 = 2x, 2 = 4x.
     */
    void setOversamplingFactor (int factorIndex);

    /** Set the processing mode: 0=ZeroLatency (IIR), 1=NaturalPhase, 2=LinearPhase */
    void setProcessingMode (int mode);

    /**
     * @brief Compute auto gain compensation in dB.
     *
     * Evaluates the composite magnitude response at log-spaced frequencies
     * across the audible range, computes the RMS energy change, and returns
     * its inverse in dB so that the overall perceived loudness stays constant.
     *
     * @return Compensation gain in dB (negative if EQ is boosting overall).
     */
    float computeAutoGainCompensation() const;

    /**
     * @brief Enable or disable analog-mode processing globally.
     *
     * When enabled, activates per-band saturation, inter-channel crosstalk
     * (approximately -54 dB), and a pink noise floor (approximately -100 dBFS)
     * to emulate the character of analog equalizer hardware.
     *
     * @param enabled  True to enable analog mode.
     */
    void setAnalogMode (bool enabled);

    /** Set the solo band index (-1 = off, 0-23 = solo that band). */
    void setSoloBand (int bandIndex);
    int getSoloBand() const { return soloBandIndex; }

    // For UI: compute magnitude responses

    /**
     * @brief Compute the composite magnitude response of all active bands.
     *
     * Sums (in dB) the magnitude responses of every active band to produce
     * the overall EQ curve for UI display.
     *
     * @param frequencies  Array of frequencies in Hz.
     * @param magnitudes   Output array for composite magnitude in dB.
     * @param numPoints    Number of frequency points.
     */
    void computeCompositeMagnitude (const double* frequencies,
                                    double* magnitudes, int numPoints) const;

    /**
     * @brief Compute the magnitude response of a single band for UI display.
     *
     * @param bandIndex    Zero-based band index.
     * @param frequencies  Array of frequencies in Hz.
     * @param magnitudes   Output array for magnitude values in dB.
     * @param numPoints    Number of frequency points.
     */
    void computeBandMagnitude (int bandIndex, const double* frequencies,
                               double* magnitudes, int numPoints) const;

    /**
     * @brief Get a pointer to a specific EqBand instance.
     *
     * @param index  Zero-based band index.
     * @return       Pointer to the EqBand, or nullptr if the index is out of range.
     */
    EqBand* getBand (int index);

    /**
     * @brief Report the processing latency introduced by oversampling and/or phase mode.
     * @return Latency in samples.
     */
    int getLatencyInSamples() const;

private:
    void processBands (juce::AudioBuffer<float>& buffer);

    std::array<EqBand, Constants::kMaxBands> bands;

    juce::SmoothedValue<float> outputGain { 1.0f };

    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    int numChannels = 2;

    // Analog mode state
    bool analogMode = false;
    juce::Random noiseRng;
    float pinkState[2] = { 0.0f, 0.0f };

    // Band solo
    int soloBandIndex = -1;
    CascadedFilter soloFilterL;
    CascadedFilter soloFilterR;

    // Oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler4x;
    int currentOversamplingIndex = 0; // 0=1x, 1=2x, 2=4x

    // Phase modes
    int currentProcessingMode = 0; // 0=IIR, 1=Natural, 2=Linear
    LinearPhaseEngine linearPhaseEngine;
    NaturalPhaseEngine naturalPhaseEngine;
};
