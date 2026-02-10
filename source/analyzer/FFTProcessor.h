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
 * @file FFTProcessor.h
 * @brief Audio-thread FFT processor with ring buffer and Hann windowing.
 */

#include <juce_dsp/juce_dsp.h>
#include "TripleBuffer.h"
#include "../utils/Constants.h"
#include <vector>

/**
 * @brief Accumulates audio samples and performs windowed FFT on the audio thread.
 *
 * Incoming samples are averaged to mono and written into an internal ring buffer.
 * When enough samples have accumulated (hopSize = 2048, giving 50% overlap),
 * a 4096-point FFT is performed with a Hann window. The resulting magnitude
 * spectrum (in dB) is written to a lock-free TripleBuffer so the UI thread
 * can read it without blocking.
 *
 * All methods except getMagnitudeBuffer() are called exclusively from the
 * audio thread.
 *
 * @see TripleBuffer, SpectrumAnalyzer
 */
// Accumulates audio samples into a ring buffer and performs windowed FFT
// when enough samples are collected. Writes magnitude spectrum (in dB)
// to a lock-free TripleBuffer for the UI thread to read.
// Called entirely from the audio thread.
class FFTProcessor
{
public:
    FFTProcessor();

    /**
     * @brief Prepares the processor for a new playback session.
     *
     * Allocates internal buffers sized for the current sample rate and
     * block size. Must be called from prepareToPlay().
     *
     * @param sampleRate The host sample rate in Hz.
     * @param maxBlockSize Maximum number of samples per processBlock() call.
     */
    void prepare (double sampleRate, int maxBlockSize);

    /**
     * @brief Resets the ring buffer write position and clears internal state.
     */
    void reset();

    /**
     * @brief Pushes audio samples into the ring buffer for FFT processing.
     *
     * Channels are averaged to mono before being written. When enough
     * samples accumulate to form a complete FFT frame (with 50% overlap),
     * the FFT is performed automatically.
     *
     * @param buffer The audio buffer containing one or more channels of samples.
     * @param numSamples Number of samples to process from the buffer.
     */
    // Push samples from a stereo (or mono) buffer. Averages channels to mono.
    void pushSamples (const juce::AudioBuffer<float>& buffer, int numSamples);

    /**
     * @brief Returns a mutable reference to the magnitude TripleBuffer.
     *
     * The UI thread uses this reference to call swapRead() and getReadPointer()
     * for spectrum visualization.
     *
     * @return Reference to the internal TripleBuffer containing dB magnitudes per FFT bin.
     */
    TripleBuffer<float>& getMagnitudeBuffer() { return magnitudeBuffer; }

    /**
     * @brief Returns a const reference to the magnitude TripleBuffer.
     * @return Const reference to the internal TripleBuffer containing dB magnitudes per FFT bin.
     */
    const TripleBuffer<float>& getMagnitudeBuffer() const { return magnitudeBuffer; }

    /**
     * @brief Returns the number of frequency bins produced by the FFT.
     * @return Number of bins (fftSize / 2 + 1 = 2049).
     */
    int getNumBins() const { return fftSize / 2 + 1; }

    /**
     * @brief Returns the sample rate used during the last prepare() call.
     * @return Sample rate in Hz.
     */
    double getSampleRate() const { return currentSampleRate; }

private:
    void performFFT();

    static constexpr int fftOrder = Constants::kFFTOrder;  // 12
    static constexpr int fftSize  = Constants::kFFTSize;   // 4096
    static constexpr int hopSize  = fftSize / 2;           // 2048 (50% overlap)

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        (size_t) fftSize,
        juce::dsp::WindowingFunction<float>::hann
    };

    std::vector<float> ringBuffer;
    int ringWritePos = 0;

    // Work buffer for FFT (must be 2 * fftSize)
    std::vector<float> fftData;

    // Output magnitudes in dB per bin
    TripleBuffer<float> magnitudeBuffer;

    double currentSampleRate = 44100.0;
};
