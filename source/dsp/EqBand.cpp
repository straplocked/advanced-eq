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

#include "EqBand.h"
#include <cmath>

void EqBand::prepare (double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    filterL.prepare (sr);
    filterR.prepare (sr);

    smoothedFreq.reset (sr, 0.02);  // 20ms smoothing
    smoothedGain.reset (sr, 0.02);
    smoothedQ.reset (sr, 0.02);
}

void EqBand::reset()
{
    filterL.reset();
    filterR.reset();
}

void EqBand::setAnalogMode (bool enabled)
{
    analogMode = enabled;
}

float EqBand::saturate (float x)
{
    constexpr float drive = 1.2f;
    constexpr float asymmetry = 0.05f;
    constexpr float normFactor = 1.0f / 0.8336546f; // 1/tanh(1.2)
    float shaped = x + asymmetry * x * x;
    return std::tanh (shaped * drive) * normFactor;
}

void EqBand::updateParameters (const BandParameters& params)
{
    // Reset filters when stereo placement changes to prevent clicks from stale state
    if (params.stereoPlacement != stereoPlacement && active)
        reset();

    currentParams = params;
    active = params.active;
    filterType = params.type;
    stereoPlacement = params.stereoPlacement;

    if (! active)
        return;

    smoothedFreq.setTargetValue (params.frequency);
    smoothedGain.setTargetValue (params.gain);
    smoothedQ.setTargetValue (params.q);

    // Update filters with current (possibly smoothed) values
    float freq = smoothedFreq.getCurrentValue();
    float gain = smoothedGain.getCurrentValue();
    float q = smoothedQ.getCurrentValue();

    // Proportional Q: bell filters tighten as gain increases (analog behavior)
    if (analogMode && params.type == FilterType::Bell)
        q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);

    filterL.setParameters (params.type, freq, q, gain, params.slopeDBPerOct);
    filterR.setParameters (params.type, freq, q, gain, params.slopeDBPerOct);

    needsUpdate = false;
}

void EqBand::process (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (! active)
        return;

    // Check if parameters are still smoothing
    if (smoothedFreq.isSmoothing() || smoothedGain.isSmoothing() || smoothedQ.isSmoothing())
    {
        // Process sample by sample for smooth transitions
        for (int i = 0; i < numSamples; ++i)
        {
            float freq = smoothedFreq.getNextValue();
            float gain = smoothedGain.getNextValue();
            float q = smoothedQ.getNextValue();

            if (analogMode && filterType == FilterType::Bell)
                q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);

            filterL.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);
            filterR.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);

            if (buffer.getNumChannels() > 0)
            {
                float* dataL = buffer.getWritePointer (0);
                dataL[i] = filterL.processSample (dataL[i]);
            }
            if (buffer.getNumChannels() > 1)
            {
                float* dataR = buffer.getWritePointer (1);
                dataR[i] = filterR.processSample (dataR[i]);
            }
        }
    }
    else
    {
        // Block processing (more efficient when parameters are stable)
        if (buffer.getNumChannels() > 0)
            filterL.process (buffer.getWritePointer (0), numSamples);
        if (buffer.getNumChannels() > 1)
            filterR.process (buffer.getWritePointer (1), numSamples);
    }

    // Per-band analog saturation
    if (analogMode)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = saturate (data[i]);
        }
    }
}

void EqBand::processSingleChannel (float* channelData, int numSamples)
{
    if (! active)
        return;

    if (smoothedFreq.isSmoothing() || smoothedGain.isSmoothing() || smoothedQ.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float freq = smoothedFreq.getNextValue();
            float gain = smoothedGain.getNextValue();
            float q = smoothedQ.getNextValue();

            if (analogMode && filterType == FilterType::Bell)
                q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);

            filterL.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);

            channelData[i] = filterL.processSample (channelData[i]);
        }
    }
    else
    {
        filterL.process (channelData, numSamples);
    }

    // Per-band analog saturation
    if (analogMode)
    {
        for (int i = 0; i < numSamples; ++i)
            channelData[i] = saturate (channelData[i]);
    }
}

void EqBand::computeMagnitudeResponse (const double* frequencies, double* magnitudes,
                                        int numPoints) const
{
    if (! active)
    {
        for (int i = 0; i < numPoints; ++i)
            magnitudes[i] = 0.0; // 0 dB = unity gain
        return;
    }

    for (int i = 0; i < numPoints; ++i)
    {
        auto response = filterL.getResponse (frequencies[i]);
        double mag = std::abs (response);
        magnitudes[i] = 20.0 * std::log10 (std::max (mag, 1e-10));
    }
}
