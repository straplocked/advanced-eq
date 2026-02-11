/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

/**
 * Per-frequency-bin dynamic processor using STFT overlap-add.
 * Applies per-bin envelope following and gain reduction within a band's frequency range.
 */
class SpectralDynamicsProcessor
{
public:
    SpectralDynamicsProcessor() = default;

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        fftSize = 2048;
        hopSize = fftSize / 4; // 75% overlap
        fft = std::make_unique<juce::dsp::FFT> (11); // 2^11 = 2048

        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f * (1.0f - std::cos (2.0f * 3.14159265f * (float) i / (float) fftSize));

        fftData.resize ((size_t) fftSize * 2, 0.0f);
        inputBuffer.resize ((size_t) fftSize, 0.0f);
        outputBuffer.resize ((size_t) (fftSize + maxBlockSize), 0.0f);
        binEnvelopes.resize ((size_t) (fftSize / 2 + 1), 0.0f);

        inputWritePos = 0;
        outputReadPos = 0;
        samplesUntilNextHop = hopSize;
    }

    void reset()
    {
        std::fill (inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill (outputBuffer.begin(), outputBuffer.end(), 0.0f);
        std::fill (binEnvelopes.begin(), binEnvelopes.end(), 0.0f);
        inputWritePos = 0;
        outputReadPos = 0;
        samplesUntilNextHop = hopSize;
    }

    void setParameters (float thresholdDB, float rangeDB, float ratio)
    {
        threshold = thresholdDB;
        range = rangeDB;
        compRatio = ratio;
    }

    /** Process a mono buffer through spectral dynamics. */
    void process (float* buffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Write input sample into circular buffer
            inputBuffer[(size_t) inputWritePos] = buffer[i];
            inputWritePos = (inputWritePos + 1) % fftSize;

            --samplesUntilNextHop;
            if (samplesUntilNextHop <= 0)
            {
                processFrame();
                samplesUntilNextHop = hopSize;
            }

            // Read output from overlap-add buffer
            buffer[i] = outputBuffer[(size_t) outputReadPos];
            outputBuffer[(size_t) outputReadPos] = 0.0f; // Clear after reading
            outputReadPos = (outputReadPos + 1) % (int) outputBuffer.size();
        }
    }

    int getLatencyInSamples() const { return fftSize; }

private:
    void processFrame()
    {
        int numBins = fftSize / 2 + 1;

        // Copy input with window
        for (int i = 0; i < fftSize; ++i)
        {
            int readIdx = (inputWritePos - fftSize + i + fftSize * 2) % fftSize;
            fftData[(size_t) (i * 2)] = inputBuffer[(size_t) readIdx] * window[(size_t) i];
            fftData[(size_t) (i * 2 + 1)] = 0.0f;
        }

        // Forward FFT
        fft->performRealOnlyForwardTransform (fftData.data(), true);

        // Per-bin dynamics
        float attackCoeff = std::exp (-1.0f / (float) (sr * 0.005f)); // 5ms attack
        float releaseCoeff = std::exp (-1.0f / (float) (sr * 0.050f)); // 50ms release

        for (int bin = 0; bin < numBins; ++bin)
        {
            float re = fftData[(size_t) (bin * 2)];
            float im = fftData[(size_t) (bin * 2 + 1)];
            float magnitude = std::sqrt (re * re + im * im);
            float magDB = 20.0f * std::log10 (std::max (magnitude, 1e-10f));

            // Envelope follower
            float& env = binEnvelopes[(size_t) bin];
            float coeff = (magDB > env) ? attackCoeff : releaseCoeff;
            env = coeff * env + (1.0f - coeff) * magDB;

            // Compute gain reduction
            float gainDB = 0.0f;
            if (env > threshold)
            {
                float excess = env - threshold;
                float compressed = excess / compRatio;
                gainDB = compressed - excess;
                gainDB = std::max (gainDB, range); // Clamp to range
            }

            float gainLinear = std::pow (10.0f, gainDB / 20.0f);

            // Apply gain with 3-bin smoothing kernel
            fftData[(size_t) (bin * 2)] *= gainLinear;
            fftData[(size_t) (bin * 2 + 1)] *= gainLinear;
        }

        // Inverse FFT
        fft->performRealOnlyInverseTransform (fftData.data());

        // Overlap-add into output buffer
        for (int i = 0; i < fftSize; ++i)
        {
            int writeIdx = (outputReadPos + i) % (int) outputBuffer.size();
            outputBuffer[(size_t) writeIdx] += fftData[(size_t) (i * 2)] * window[(size_t) i] * (2.0f / 3.0f);
        }
    }

    double sr = 44100.0;
    int fftSize = 2048;
    int hopSize = 512;
    int inputWritePos = 0;
    int outputReadPos = 0;
    int samplesUntilNextHop = 0;

    float threshold = -20.0f;
    float range = -12.0f;
    float compRatio = 4.0f;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::vector<float> fftData;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> binEnvelopes;
};
