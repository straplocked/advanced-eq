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

#include "FFTProcessor.h"
#include <cmath>
#include <algorithm>

FFTProcessor::FFTProcessor()
{
    // Pre-create FFT objects for all 4 resolutions
    for (int i = 0; i < kNumResolutions; ++i)
    {
        int order = kFFTOrders[i];
        int size = 1 << order;
        configs[i].fft = std::make_unique<juce::dsp::FFT> (order);
        configs[i].window = std::make_unique<juce::dsp::WindowingFunction<float>> (
            (size_t) size, juce::dsp::WindowingFunction<float>::hann);
        configs[i].fftSize = size;
        configs[i].hopSize = size / 2;
        configs[i].numBins = size / 2 + 1;
    }
}

void FFTProcessor::prepare (double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    // Allocate at maximum size
    ringBuffer.assign ((size_t) kMaxFFTSizeInternal, 0.0f);
    ringWritePos = 0;

    fftData.assign ((size_t) kMaxFFTSizeInternal * 2, 0.0f);

    // Magnitude buffer sized for max bins (8192/2 + 1 = 4097)
    magnitudeBuffer.resize ((size_t) (kMaxFFTSizeInternal / 2 + 1));

    // Capture buffers
    captureAccumulator.assign ((size_t) (kMaxFFTSizeInternal / 2 + 1), 0.0f);
    averageSpectrum.assign ((size_t) (kMaxFFTSizeInternal / 2 + 1), Constants::kAnalyzerMinDB);
    capturing = false;
    captureFrameCount = 0;
}

void FFTProcessor::reset()
{
    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    ringWritePos = 0;
}

int FFTProcessor::getCurrentFFTSize() const
{
    return configs[activeResolution].fftSize;
}

void FFTProcessor::setResolution (int resolutionIndex)
{
    int newRes = juce::jlimit (0, kNumResolutions - 1, resolutionIndex);
    if (newRes != activeResolution)
    {
        activeResolution = newRes;
        ringWritePos = 0; // Reset ring buffer position for clean start
    }
}

void FFTProcessor::startCapture()
{
    std::fill (captureAccumulator.begin(), captureAccumulator.end(), 0.0f);
    captureFrameCount = 0;
    capturing = true;
}

void FFTProcessor::stopCapture()
{
    capturing = false;

    if (captureFrameCount > 0)
    {
        int numBins = getNumBins();
        float invCount = 1.0f / (float) captureFrameCount;
        for (int i = 0; i < numBins; ++i)
            averageSpectrum[(size_t) i] = captureAccumulator[(size_t) i] * invCount;
    }
}

void FFTProcessor::pushSamples (const juce::AudioBuffer<float>& buffer,
                                 int numSamples)
{
    int numChannels = buffer.getNumChannels();
    if (numChannels == 0)
        return;

    auto& config = configs[activeResolution];
    int currentFFTSize = config.fftSize;

    float invChannels = 1.0f / (float) numChannels;

    for (int i = 0; i < numSamples; ++i)
    {
        // Average all channels to mono
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getReadPointer (ch)[i];
        sample *= invChannels;

        ringBuffer[(size_t) ringWritePos] = sample;
        ++ringWritePos;

        if (ringWritePos >= currentFFTSize)
        {
            performFFT();

            // Shift second half to first half for 50% overlap
            int hopSize = config.hopSize;
            std::copy (ringBuffer.begin() + hopSize,
                       ringBuffer.begin() + currentFFTSize,
                       ringBuffer.begin());
            ringWritePos = hopSize;
        }
    }
}

void FFTProcessor::performFFT()
{
    auto& config = configs[activeResolution];
    int fftSize = config.fftSize;
    int numBins = config.numBins;

    // Copy ring buffer into FFT work buffer
    std::copy (ringBuffer.begin(), ringBuffer.begin() + fftSize,
               fftData.begin());

    // Zero the second half (required by JUCE FFT)
    std::fill (fftData.begin() + fftSize, fftData.begin() + fftSize * 2, 0.0f);

    // Apply Hann window
    config.window->multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);

    // Forward FFT
    config.fft->performFrequencyOnlyForwardTransform (fftData.data(), true);

    // Convert magnitudes to dB and write to triple buffer
    float* dest = magnitudeBuffer.getWritePointer();
    float fftSizeF = (float) fftSize;

    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = fftData[(size_t) bin];

        // Normalize: divide by fftSize, x2 for single-sided (except DC and Nyquist)
        mag /= fftSizeF;
        if (bin > 0 && bin < numBins - 1)
            mag *= 2.0f;

        // Convert to dB, clamp to analyzer floor
        float dB = 20.0f * std::log10 (std::max (mag, 1e-10f));
        dest[bin] = std::max (dB, Constants::kAnalyzerMinDB);
    }

    // Zero out remaining bins beyond current resolution (buffer is max-sized)
    int maxBins = kMaxFFTSizeInternal / 2 + 1;
    for (int bin = numBins; bin < maxBins; ++bin)
        dest[bin] = Constants::kAnalyzerMinDB;

    magnitudeBuffer.finishWrite();

    // Accumulate for spectrum capture
    if (capturing)
    {
        for (int bin = 0; bin < numBins; ++bin)
            captureAccumulator[(size_t) bin] += dest[bin];
        ++captureFrameCount;
    }
}
