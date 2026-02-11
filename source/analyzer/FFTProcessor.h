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

#include <juce_dsp/juce_dsp.h>
#include "TripleBuffer.h"
#include "../utils/Constants.h"
#include <vector>
#include <memory>
#include <atomic>

class FFTProcessor
{
public:
    FFTProcessor();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void pushSamples (const juce::AudioBuffer<float>& buffer, int numSamples);

    /** Set the FFT resolution. 0=Low(1024), 1=Normal(2048), 2=High(4096), 3=Max(8192). */
    void setResolution (int resolutionIndex);

    TripleBuffer<float>& getMagnitudeBuffer() { return magnitudeBuffer; }
    const TripleBuffer<float>& getMagnitudeBuffer() const { return magnitudeBuffer; }

    int getNumBins() const { return getCurrentFFTSize() / 2 + 1; }
    double getSampleRate() const { return currentSampleRate; }
    int getCurrentFFTSize() const;

    // Spectrum capture for EQ Match (Phase 6)
    void startCapture();
    void stopCapture();
    bool isCapturing() const { return capturing; }
    const std::vector<float>& getAverageSpectrum() const { return averageSpectrum; }

private:
    void performFFT();

    static constexpr int kNumResolutions = 4;
    static constexpr int kFFTOrders[kNumResolutions] = { 10, 11, 12, 13 };
    static constexpr int kMaxFFTSizeInternal = 8192;

    struct FFTConfig
    {
        std::unique_ptr<juce::dsp::FFT> fft;
        std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
        int fftSize;
        int hopSize;
        int numBins;
    };

    std::array<FFTConfig, kNumResolutions> configs;
    int activeResolution = 2; // Default: High (4096)

    std::vector<float> ringBuffer;
    int ringWritePos = 0;

    // Work buffer for FFT (sized for max FFT size)
    std::vector<float> fftData;

    // Output magnitudes in dB per bin
    TripleBuffer<float> magnitudeBuffer;

    double currentSampleRate = 44100.0;

    // Spectrum capture state (for EQ Match)
    bool capturing = false;
    int captureFrameCount = 0;
    std::vector<float> captureAccumulator;
    std::vector<float> averageSpectrum;
};
