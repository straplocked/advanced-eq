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

#include "ProcessingChain.h"
#include "MidSideProcessor.h"
#include "../utils/MathUtils.h"

void ProcessingChain::prepare (double sr, int maxBlock, int numCh)
{
    sampleRate = sr;
    maxBlockSize = maxBlock;
    numChannels = numCh;

    for (auto& band : bands)
        band.prepare (sr, maxBlock);

    outputGain.reset (sr, 0.02); // 20ms smoothing

    // Pre-create oversampling objects (2x and 4x)
    oversampler2x = std::make_unique<juce::dsp::Oversampling<float>> (
        numCh, 1, // factor 1 = 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampler2x->initProcessing ((size_t) maxBlock);

    oversampler4x = std::make_unique<juce::dsp::Oversampling<float>> (
        numCh, 2, // factor 2 = 4x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampler4x->initProcessing ((size_t) maxBlock);

    currentOversamplingIndex = 0;

    // Phase engines
    linearPhaseEngine.prepare (sr, maxBlock, numCh);
    naturalPhaseEngine.prepare (sr, maxBlock, numCh);

    auto magCallback = [this] (const double* frequencies, double* magnitudes, int numPoints)
    {
        computeCompositeMagnitude (frequencies, magnitudes, numPoints);
    };

    linearPhaseEngine.setMagnitudeCallback (magCallback);
    naturalPhaseEngine.setMagnitudeCallback (magCallback);
}

void ProcessingChain::reset()
{
    for (auto& band : bands)
        band.reset();

    if (oversampler2x) oversampler2x->reset();
    if (oversampler4x) oversampler4x->reset();
}

void ProcessingChain::setOversamplingFactor (int factorIndex)
{
    factorIndex = juce::jlimit (0, 2, factorIndex);

    if (factorIndex == currentOversamplingIndex)
        return;

    currentOversamplingIndex = factorIndex;

    // Re-prepare bands at the oversampled rate
    int multiplier = 1 << factorIndex;
    double effectiveRate = sampleRate * multiplier;
    int effectiveBlockSize = maxBlockSize * multiplier;

    for (auto& band : bands)
        band.prepare (effectiveRate, effectiveBlockSize);
}

float ProcessingChain::computeAutoGainCompensation() const
{
    constexpr int numFreqs = 31;
    double frequencies[numFreqs];
    double magnitudes[numFreqs];

    // Log-spaced frequencies from 20 Hz to 20 kHz
    for (int i = 0; i < numFreqs; ++i)
    {
        double t = (double) i / (double) (numFreqs - 1);
        frequencies[i] = 20.0 * std::pow (1000.0, t); // 20 Hz to 20 kHz
    }

    computeCompositeMagnitude (frequencies, magnitudes, numFreqs);

    // Compute RMS energy offset: convert dB magnitudes to power, average, back to dB
    double sumPower = 0.0;
    for (int i = 0; i < numFreqs; ++i)
    {
        double linearGain = std::pow (10.0, magnitudes[i] / 20.0);
        sumPower += linearGain * linearGain;
    }

    double avgPower = sumPower / numFreqs;
    double rmsGainDB = 10.0 * std::log10 (std::max (avgPower, 1e-10));

    return (float) -rmsGainDB;
}

void ProcessingChain::setProcessingMode (int mode)
{
    mode = juce::jlimit (0, 2, mode);
    if (mode == currentProcessingMode)
        return;

    currentProcessingMode = mode;

    if (mode == 1)
        naturalPhaseEngine.requestKernelUpdate();
    else if (mode == 2)
        linearPhaseEngine.requestKernelUpdate();
}

int ProcessingChain::getLatencyInSamples() const
{
    if (currentProcessingMode == 1)
        return naturalPhaseEngine.getLatencyInSamples();
    if (currentProcessingMode == 2)
        return linearPhaseEngine.getLatencyInSamples();

    // IIR mode: oversampling latency only
    if (currentOversamplingIndex == 1 && oversampler2x)
        return (int) oversampler2x->getLatencyInSamples();
    if (currentOversamplingIndex == 2 && oversampler4x)
        return (int) oversampler4x->getLatencyInSamples();
    return 0;
}

void ProcessingChain::processBands (juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    bool isStereo = buffer.getNumChannels() >= 2;

    // --- Pass 1: Stereo bands (process both L and R) ---
    for (auto& band : bands)
    {
        if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Stereo)
            band.process (buffer, numSamples);
    }

    if (isStereo)
    {
        float* dataL = buffer.getWritePointer (0);
        float* dataR = buffer.getWritePointer (1);

        // --- Pass 2: Left-only bands ---
        for (auto& band : bands)
        {
            if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Left)
                band.processSingleChannel (dataL, numSamples);
        }

        // --- Pass 3: Right-only bands ---
        for (auto& band : bands)
        {
            if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Right)
                band.processSingleChannel (dataR, numSamples);
        }

        // --- Pass 4: Mid/Side bands ---
        bool hasMid = false, hasSide = false;
        for (const auto& band : bands)
        {
            if (! band.isActive()) continue;
            if (band.getStereoPlacement() == StereoPlacement::Mid)  hasMid = true;
            if (band.getStereoPlacement() == StereoPlacement::Side) hasSide = true;
        }

        if (hasMid || hasSide)
        {
            MidSideProcessor::encode (dataL, dataR, numSamples);

            for (auto& band : bands)
            {
                if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Mid)
                    band.processSingleChannel (dataL, numSamples);
            }

            for (auto& band : bands)
            {
                if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Side)
                    band.processSingleChannel (dataR, numSamples);
            }

            MidSideProcessor::decode (dataL, dataR, numSamples);
        }
    }

    // Band solo: apply monitor bandpass after normal processing
    if (soloBandIndex >= 0 && soloBandIndex < Constants::kMaxBands)
    {
        auto& band = bands[(size_t) soloBandIndex];
        if (band.isActive())
        {
            auto params = band.getCurrentParams();
            soloFilterL.setParameters (FilterType::BandPass, params.frequency, params.q, 0.0, 12);
            soloFilterR.setParameters (FilterType::BandPass, params.frequency, params.q, 0.0, 12);

            if (buffer.getNumChannels() > 0)
                soloFilterL.process (buffer.getWritePointer (0), numSamples);
            if (buffer.getNumChannels() > 1)
                soloFilterR.process (buffer.getWritePointer (1), numSamples);
        }
    }

    // Crosstalk + noise floor (Vintage and Aggressive modes)
    if (characterMode >= 2 && isStereo)
    {
        float* dataL = buffer.getWritePointer (0);
        float* dataR = buffer.getWritePointer (1);

        // Aggressive: stronger crosstalk and noise than Vintage
        float crosstalk = (characterMode == 3) ? 0.006f : 0.002f; // ~-44 dB vs ~-54 dB
        float noiseLevel = (characterMode == 3) ? 3e-5f : 1e-5f;  // ~-90 dBFS vs ~-100 dBFS

        for (int i = 0; i < numSamples; ++i)
        {
            // Channel crosstalk
            float l = dataL[i];
            float r = dataR[i];
            dataL[i] = l + crosstalk * r;
            dataR[i] = r + crosstalk * l;

            // Pink-ish noise floor (one-pole filtered white noise per channel)
            float whiteL = noiseRng.nextFloat() * 2.0f - 1.0f;
            float whiteR = noiseRng.nextFloat() * 2.0f - 1.0f;
            pinkState[0] = 0.99f * pinkState[0] + 0.01f * whiteL;
            pinkState[1] = 0.99f * pinkState[1] + 0.01f * whiteR;
            dataL[i] += pinkState[0] * noiseLevel;
            dataR[i] += pinkState[1] * noiseLevel;
        }
    }
    else if (characterMode >= 2 && ! isStereo)
    {
        float* data = buffer.getWritePointer (0);
        float noiseLevel = (characterMode == 3) ? 3e-5f : 1e-5f;

        for (int i = 0; i < numSamples; ++i)
        {
            float white = noiseRng.nextFloat() * 2.0f - 1.0f;
            pinkState[0] = 0.99f * pinkState[0] + 0.01f * white;
            data[i] += pinkState[0] * noiseLevel;
        }
    }
}

void ProcessingChain::process (juce::AudioBuffer<float>& buffer)
{
    // Linear/Natural phase modes use FIR convolution instead of IIR bands
    if (currentProcessingMode == 2) // Linear Phase
    {
        linearPhaseEngine.process (buffer);

        // Apply output gain after convolution
        int numSamples = buffer.getNumSamples();
        if (outputGain.isSmoothing())
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                for (int i = 0; i < numSamples; ++i)
                    data[i] *= outputGain.getNextValue();
            }
        }
        else
        {
            float gainValue = outputGain.getCurrentValue();
            if (std::abs (gainValue - 1.0f) > 1e-6f)
                buffer.applyGain (gainValue);
        }
        return;
    }

    if (currentProcessingMode == 1) // Natural Phase
    {
        naturalPhaseEngine.process (buffer);

        int numSamples = buffer.getNumSamples();
        if (outputGain.isSmoothing())
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                for (int i = 0; i < numSamples; ++i)
                    data[i] *= outputGain.getNextValue();
            }
        }
        else
        {
            float gainValue = outputGain.getCurrentValue();
            if (std::abs (gainValue - 1.0f) > 1e-6f)
                buffer.applyGain (gainValue);
        }
        return;
    }

    // Zero-latency IIR mode with optional oversampling
    if (currentOversamplingIndex > 0)
    {
        auto* oversampler = (currentOversamplingIndex == 1) ? oversampler2x.get()
                                                            : oversampler4x.get();
        juce::dsp::AudioBlock<float> inputBlock (buffer);
        auto oversampledBlock = oversampler->processSamplesUp (inputBlock);

        int osChannels = (int) oversampledBlock.getNumChannels();
        int osSamples = (int) oversampledBlock.getNumSamples();

        // Wrap the oversampled block data in an AudioBuffer (zero-copy)
        float* channelPtrs[2] = { nullptr, nullptr };
        for (int ch = 0; ch < juce::jmin (osChannels, 2); ++ch)
            channelPtrs[ch] = oversampledBlock.getChannelPointer ((size_t) ch);

        juce::AudioBuffer<float> osBuffer (channelPtrs, osChannels, osSamples);

        processBands (osBuffer);

        oversampler->processSamplesDown (inputBlock);
    }
    else
    {
        processBands (buffer);
    }

    // Apply output gain (always at original sample rate, after downsampling)
    int numSamples = buffer.getNumSamples();

    if (outputGain.isSmoothing())
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] *= outputGain.getNextValue();
        }
    }
    else
    {
        float gainValue = outputGain.getCurrentValue();
        if (std::abs (gainValue - 1.0f) > 1e-6f)
            buffer.applyGain (gainValue);
    }
}

void ProcessingChain::updateBand (int bandIndex, const BandParameters& params)
{
    if (bandIndex >= 0 && bandIndex < Constants::kMaxBands)
    {
        bands[(size_t) bandIndex].updateParameters (params);

        // Trigger kernel rebuild in phase modes when band parameters change
        if (currentProcessingMode == 1)
            naturalPhaseEngine.requestKernelUpdate();
        else if (currentProcessingMode == 2)
            linearPhaseEngine.requestKernelUpdate();
    }
}

void ProcessingChain::setOutputGain (float gainDB)
{
    outputGain.setTargetValue (MathUtils::dbToGain (gainDB));
}

void ProcessingChain::setAnalogMode (bool enabled)
{
    analogMode = enabled;
    for (auto& band : bands)
        band.setAnalogMode (enabled);
}

void ProcessingChain::setCharacterMode (int mode)
{
    mode = juce::jlimit (0, 3, mode);
    if (mode == characterMode)
        return;

    characterMode = mode;
    analogMode = (mode >= 1); // Smooth, Vintage, Aggressive enable analog-style processing

    for (auto& band : bands)
        band.setCharacterMode (mode);
}

void ProcessingChain::setLinearPhaseQuality (int quality)
{
    quality = juce::jlimit (0, 2, quality);
    static const int kernelSizes[] = { 2048, 4096, 8192 };
    linearPhaseEngine.setKernelSize (kernelSizes[quality]);
}

void ProcessingChain::setSoloBand (int bandIndex)
{
    if (bandIndex == soloBandIndex)
        return;

    soloBandIndex = bandIndex;

    if (soloBandIndex >= 0 && soloBandIndex < Constants::kMaxBands)
    {
        // Configure solo monitor bandpass filter at the soloed band's frequency/Q
        auto& band = bands[(size_t) soloBandIndex];
        auto params = band.getCurrentParams();
        soloFilterL.prepare (sampleRate);
        soloFilterR.prepare (sampleRate);
        soloFilterL.setParameters (FilterType::BandPass, params.frequency, params.q, 0.0, 12);
        soloFilterR.setParameters (FilterType::BandPass, params.frequency, params.q, 0.0, 12);
    }
}

void ProcessingChain::computeCompositeMagnitude (const double* frequencies,
                                                  double* magnitudes,
                                                  int numPoints) const
{
    // Start at 0 dB (unity)
    for (int i = 0; i < numPoints; ++i)
        magnitudes[i] = 0.0;

    // Sum magnitudes from all active bands (in dB, since they cascade)
    std::vector<double> bandMag (numPoints);
    for (const auto& band : bands)
    {
        if (band.isActive())
        {
            band.computeMagnitudeResponse (frequencies, bandMag.data(), numPoints);
            for (int i = 0; i < numPoints; ++i)
                magnitudes[i] += bandMag[i];
        }
    }
}

void ProcessingChain::computeBandMagnitude (int bandIndex, const double* frequencies,
                                             double* magnitudes, int numPoints) const
{
    if (bandIndex >= 0 && bandIndex < Constants::kMaxBands)
        bands[(size_t) bandIndex].computeMagnitudeResponse (frequencies, magnitudes, numPoints);
}

EqBand* ProcessingChain::getBand (int index)
{
    if (index >= 0 && index < Constants::kMaxBands)
        return &bands[(size_t) index];
    return nullptr;
}
