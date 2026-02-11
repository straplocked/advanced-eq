/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "EqBand.h"
#include <cmath>

void EqBand::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;

    for (auto& f : filters)
        f.prepare (sr);

    smoothedFreq.reset (sr, 0.02);  // 20ms smoothing
    smoothedGain.reset (sr, 0.02);
    smoothedQ.reset (sr, 0.02);

    for (auto& e : envFollowers)
        e.prepare (sr);
    for (auto& d : dynamicProcs)
        d.prepare (sr);
    for (auto& sp : spectralProcs)
        sp.prepare (sr, maxBlockSize);
}

void EqBand::reset()
{
    for (auto& f : filters)
        f.reset();
    for (auto& e : envFollowers)
        e.reset();
    for (auto& d : dynamicProcs)
        d.reset();
    for (auto& sp : spectralProcs)
        sp.reset();
}

void EqBand::setAnalogMode (bool enabled)
{
    analogMode = enabled;
}

void EqBand::setCharacterMode (int mode)
{
    characterMode = mode;
    analogMode = (mode >= 1);
}

void EqBand::setSpectralDynamics (bool enabled, float threshold, float range, float ratio)
{
    spectralEnabled = enabled;
    spectralThreshold = threshold;
    spectralRange = range;
    spectralRatio = ratio;

    for (auto& sp : spectralProcs)
        sp.setParameters (threshold, range, ratio);
}

float EqBand::saturate (float x)
{
    constexpr float drive = 1.2f;
    constexpr float asymmetry = 0.05f;
    constexpr float normFactor = 1.0f / 0.8336546f; // 1/tanh(1.2)
    float shaped = x + asymmetry * x * x;
    return std::tanh (shaped * drive) * normFactor;
}

float EqBand::aggressiveSaturate (float x)
{
    constexpr float drive = 2.5f;
    constexpr float asymmetry = 0.12f;
    constexpr float normFactor = 1.0f / 0.9866143f; // 1/tanh(2.5)
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

    // Dynamic EQ parameters
    dynamicEnabled = params.dynamicEnabled;
    if (dynamicEnabled)
    {
        for (auto& e : envFollowers)
        {
            e.setAttack (params.dynamicAttack);
            e.setRelease (params.dynamicRelease);
            e.setBandFrequency (params.frequency);
        }
        for (auto& d : dynamicProcs)
        {
            d.setThreshold (params.dynamicThreshold);
            d.setRange (params.dynamicRange);
        }
    }

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
    // Aggressive mode uses stronger tightening factor
    if (analogMode && params.type == FilterType::Bell)
    {
        float qFactor = (characterMode == 3) ? 0.7f : 0.4f;
        q *= (1.0f + qFactor * std::abs (gain) / 30.0f);
    }

    for (auto& f : filters)
        f.setParameters (params.type, freq, q, gain, params.slopeDBPerOct);

    needsUpdate = false;
}

void EqBand::process (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (! active)
        return;

    // Spectral dynamics path: use STFT overlap-add instead of IIR cascade
    if (spectralEnabled)
    {
        int numChannels = juce::jmin (buffer.getNumChannels(), kMaxChannels);
        for (int ch = 0; ch < numChannels; ++ch)
            spectralProcs[ch].process (buffer.getWritePointer (ch), numSamples);
        return;
    }

    bool isSmoothing = smoothedFreq.isSmoothing() || smoothedGain.isSmoothing() || smoothedQ.isSmoothing();

    int numChannels = juce::jmin (buffer.getNumChannels(), kMaxChannels);

    // Dynamic EQ: per-sample processing when enabled
    if (dynamicEnabled || isSmoothing)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float freq = smoothedFreq.getNextValue();
            float gain = smoothedGain.getNextValue();
            float q = smoothedQ.getNextValue();

            if (analogMode && filterType == FilterType::Bell)
                q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);

            // Dynamic gain modulation
            if (dynamicEnabled)
            {
                // Average envelope across all channels
                float dynGainSum = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float env = envFollowers[ch].processSample (buffer.getReadPointer (ch)[i]);
                    dynGainSum += dynamicProcs[ch].computeGainDB (env);
                }
                float dynGainAvg = dynGainSum / (float) numChannels;
                float modulatedGain = gain + dynGainAvg;

                for (int ch = 0; ch < numChannels; ++ch)
                    filters[ch].setParameters (filterType, freq, q, modulatedGain, currentParams.slopeDBPerOct);

                dynamicGainReduction.store (dynGainAvg, std::memory_order_relaxed);
            }
            else
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    filters[ch].setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                data[i] = filters[ch].processSample (data[i]);
            }
        }
    }
    else
    {
        // Block processing (more efficient when parameters are stable)
        for (int ch = 0; ch < numChannels; ++ch)
            filters[ch].process (buffer.getWritePointer (ch), numSamples);
    }

    // Per-band saturation: Smooth=even-harmonic, Vintage=tanh, Aggressive=hard tanh
    if (characterMode >= 1)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            if (characterMode == 1) // Smooth
            {
                for (int i = 0; i < numSamples; ++i)
                    data[i] = data[i] + 0.15f * data[i] * data[i];
            }
            else if (characterMode == 2) // Vintage
            {
                for (int i = 0; i < numSamples; ++i)
                    data[i] = saturate (data[i]);
            }
            else // Aggressive
            {
                for (int i = 0; i < numSamples; ++i)
                    data[i] = aggressiveSaturate (data[i]);
            }
        }
    }
}

void EqBand::processSingleChannel (float* channelData, int numSamples)
{
    if (! active)
        return;

    bool isSmoothing = smoothedFreq.isSmoothing() || smoothedGain.isSmoothing() || smoothedQ.isSmoothing();

    if (dynamicEnabled || isSmoothing)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float freq = smoothedFreq.getNextValue();
            float gain = smoothedGain.getNextValue();
            float q = smoothedQ.getNextValue();

            if (analogMode && filterType == FilterType::Bell)
                q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);

            if (dynamicEnabled)
            {
                float env = envFollowerL.processSample (channelData[i]);
                float dynGain = dynamicProcL.computeGainDB (env);
                float modulatedGain = gain + dynGain;

                filterL.setParameters (filterType, freq, q, modulatedGain, currentParams.slopeDBPerOct);
                dynamicGainReduction.store (dynGain, std::memory_order_relaxed);
            }
            else
            {
                filterL.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);
            }

            channelData[i] = filterL.processSample (channelData[i]);
        }
    }
    else
    {
        filterL.process (channelData, numSamples);
    }

    // Per-band saturation: Smooth=even-harmonic, Vintage=tanh, Aggressive=hard tanh
    if (characterMode >= 1)
    {
        if (characterMode == 1)
        {
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = channelData[i] + 0.15f * channelData[i] * channelData[i];
        }
        else if (characterMode == 2)
        {
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = saturate (channelData[i]);
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = aggressiveSaturate (channelData[i]);
        }
    }
}

void EqBand::processChannel (float* channelData, int numSamples, int channelIndex)
{
    if (! active || channelIndex < 0 || channelIndex >= kMaxChannels)
        return;

    auto& filter = filters[channelIndex];

    bool isSmoothing = smoothedFreq.isSmoothing() || smoothedGain.isSmoothing() || smoothedQ.isSmoothing();

    if (dynamicEnabled || isSmoothing)
    {
        auto& envFollower = envFollowers[channelIndex];
        auto& dynamicProc = dynamicProcs[channelIndex];

        for (int i = 0; i < numSamples; ++i)
        {
            float freq = smoothedFreq.getNextValue();
            float gain = smoothedGain.getNextValue();
            float q = smoothedQ.getNextValue();

            if (analogMode && filterType == FilterType::Bell)
                q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);

            if (dynamicEnabled)
            {
                float env = envFollower.processSample (channelData[i]);
                float dynGain = dynamicProc.computeGainDB (env);
                float modulatedGain = gain + dynGain;
                filter.setParameters (filterType, freq, q, modulatedGain, currentParams.slopeDBPerOct);
                dynamicGainReduction.store (dynGain, std::memory_order_relaxed);
            }
            else
            {
                filter.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);
            }

            channelData[i] = filter.processSample (channelData[i]);
        }
    }
    else
    {
        filter.process (channelData, numSamples);
    }

    if (characterMode >= 1)
    {
        if (characterMode == 1)
        {
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = channelData[i] + 0.15f * channelData[i] * channelData[i];
        }
        else if (characterMode == 2)
        {
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = saturate (channelData[i]);
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = aggressiveSaturate (channelData[i]);
        }
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
