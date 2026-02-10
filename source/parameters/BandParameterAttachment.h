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
 * @file BandParameterAttachment.h
 * @brief Lock-free parameter bridge between APVTS and DSP engine.
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/EqBand.h"
#include "../dsp/FilterTypes.h"
#include "ParameterIDs.h"

/**
 * @brief Caches atomic parameter pointers from APVTS for zero-allocation, lock-free audio-thread reads.
 *
 * On construction (or after init()), stores the std::atomic<float>* pointers
 * returned by getRawParameterValue() for all 12 per-band parameters. The
 * getCurrentParameters() method then reads these atomics to build a
 * BandParameters struct without any heap allocation or locking, making it
 * safe to call on the audio thread every processBlock().
 *
 * @note Thread safety relies on JUCE's atomic parameter storage; no additional
 *       locks are needed.
 *
 * @see ParameterIDs, BandParameters
 */
// Bridges APVTS parameter values to DSP EqBand.
// Caches getRawParameterValue() pointers for lock-free audio thread reads.
class BandParameterAttachment
{
public:
    BandParameterAttachment() = default;

    /**
     * @brief Initializes atomic parameter pointers for the specified band.
     *
     * Must be called once after APVTS construction, before any audio processing
     * begins. Caches pointers to all 12 per-band parameters for the given band.
     *
     * @param bandIndex Zero-based band index (0..23).
     * @param apvts The AudioProcessorValueTreeState that owns the parameters.
     */
    void init (int bandIndex, juce::AudioProcessorValueTreeState& apvts)
    {
        rawActive     = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kActive));
        rawType       = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kType));
        rawFreq       = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kFrequency));
        rawGain       = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kGain));
        rawQ          = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kQ));
        rawSlope      = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kSlope));
        rawStereo     = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kStereo));
        rawDynEnabled = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynEnabled));
        rawDynThresh  = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynThresh));
        rawDynRange   = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynRange));
        rawDynAttack  = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynAttack));
        rawDynRelease = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynRelease));
    }

    /**
     * @brief Reads current parameter values into a BandParameters struct.
     *
     * Performs lock-free atomic loads on all cached parameter pointers.
     * Safe to call from the audio thread with zero allocation overhead.
     *
     * @return A BandParameters struct populated with the current values of all
     *         12 per-band parameters.
     */
    // Read current parameter values (lock-free, safe for audio thread)
    BandParameters getCurrentParameters() const
    {
        BandParameters params;
        params.active           = rawActive->load() > 0.5f;
        params.type             = static_cast<FilterType> ((int) rawType->load());
        params.frequency        = rawFreq->load();
        params.gain             = rawGain->load();
        params.q                = rawQ->load();
        params.slopeDBPerOct    = slopeIndexToDBPerOct ((int) rawSlope->load());
        params.stereoPlacement  = static_cast<StereoPlacement> ((int) rawStereo->load());
        params.dynamicEnabled   = rawDynEnabled->load() > 0.5f;
        params.dynamicThreshold = rawDynThresh->load();
        params.dynamicRange     = rawDynRange->load();
        params.dynamicAttack    = rawDynAttack->load();
        params.dynamicRelease   = rawDynRelease->load();
        return params;
    }

private:
    std::atomic<float>* rawActive     = nullptr;
    std::atomic<float>* rawType       = nullptr;
    std::atomic<float>* rawFreq       = nullptr;
    std::atomic<float>* rawGain       = nullptr;
    std::atomic<float>* rawQ          = nullptr;
    std::atomic<float>* rawSlope      = nullptr;
    std::atomic<float>* rawStereo     = nullptr;
    std::atomic<float>* rawDynEnabled = nullptr;
    std::atomic<float>* rawDynThresh  = nullptr;
    std::atomic<float>* rawDynRange   = nullptr;
    std::atomic<float>* rawDynAttack  = nullptr;
    std::atomic<float>* rawDynRelease = nullptr;
};
