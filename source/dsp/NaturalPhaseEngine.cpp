/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "NaturalPhaseEngine.h"
#include <cmath>

NaturalPhaseEngine::NaturalPhaseEngine() = default;

NaturalPhaseEngine::~NaturalPhaseEngine()
{
    if (kernelThread)
    {
        kernelThread->stopThread (2000);
        kernelThread.reset();
    }
}

void NaturalPhaseEngine::prepare (double sr, int maxBlock, int numCh)
{
    sampleRate = sr;
    maxBlockSize = maxBlock;
    numChannels = numCh;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32) maxBlock;
    spec.numChannels = (juce::uint32) numCh;
    convolver.prepare (spec);

    if (! kernelThread)
    {
        kernelThread = std::make_unique<KernelThread> (*this);
        kernelThread->startThread (juce::Thread::Priority::low);
    }

    prepared = true;
    requestKernelUpdate();
}

void NaturalPhaseEngine::reset()
{
    convolver.reset();
}

void NaturalPhaseEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (! prepared)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    convolver.process (context);
}

void NaturalPhaseEngine::setMagnitudeCallback (MagnitudeCallback callback)
{
    std::lock_guard<std::mutex> lock (kernelMutex);
    magnitudeCallback = std::move (callback);
}

void NaturalPhaseEngine::requestKernelUpdate()
{
    kernelUpdateRequested.store (true, std::memory_order_release);
    if (kernelThread)
        kernelThread->notify();
}

void NaturalPhaseEngine::computeMinimumPhase (std::vector<float>& impulseResponse)
{
    int N = (int) impulseResponse.size();
    int fftOrder = (int) std::round (std::log2 ((double) N));
    juce::dsp::FFT fft (fftOrder);

    // Forward FFT
    std::vector<float> fftData ((size_t) N * 2, 0.0f);
    for (int i = 0; i < N; ++i)
        fftData[(size_t) (i * 2)] = impulseResponse[(size_t) i];

    fft.performRealOnlyForwardTransform (fftData.data(), true);

    // Compute log magnitude (cepstral domain)
    int numBins = N / 2 + 1;
    std::vector<float> logMag ((size_t) N * 2, 0.0f);

    for (int i = 0; i < numBins; ++i)
    {
        float re = fftData[(size_t) (i * 2)];
        float im = fftData[(size_t) (i * 2 + 1)];
        float mag = std::sqrt (re * re + im * im);
        logMag[(size_t) (i * 2)] = std::log (std::max (mag, 1e-10f));
        logMag[(size_t) (i * 2 + 1)] = 0.0f;
    }

    // Mirror
    for (int i = 1; i < N / 2; ++i)
    {
        logMag[(size_t) ((N - i) * 2)] = logMag[(size_t) (i * 2)];
        logMag[(size_t) ((N - i) * 2 + 1)] = 0.0f;
    }

    // IFFT to get cepstrum
    fft.performRealOnlyInverseTransform (logMag.data());

    // Apply causal window to cepstrum (minimum phase reconstruction)
    // Keep DC, double positive time, zero negative time
    std::vector<float> causalCepstrum ((size_t) N * 2, 0.0f);
    causalCepstrum[0] = logMag[0]; // DC
    for (int i = 1; i < N / 2; ++i)
    {
        causalCepstrum[(size_t) (i * 2)] = logMag[(size_t) i] * 2.0f;
    }
    causalCepstrum[(size_t) (N)] = logMag[(size_t) (N / 2)]; // Nyquist

    // FFT to get minimum-phase log spectrum
    fft.performRealOnlyForwardTransform (causalCepstrum.data(), true);

    // Exponentiate to get minimum-phase spectrum, then IFFT
    for (int i = 0; i < numBins; ++i)
    {
        float logRe = causalCepstrum[(size_t) (i * 2)];
        float logIm = causalCepstrum[(size_t) (i * 2 + 1)];
        float mag = std::exp (logRe);
        float phase = logIm;
        fftData[(size_t) (i * 2)]     = mag * std::cos (phase);
        fftData[(size_t) (i * 2 + 1)] = mag * std::sin (phase);
    }

    // Mirror for IFFT
    for (int i = 1; i < N / 2; ++i)
    {
        fftData[(size_t) ((N - i) * 2)]     = fftData[(size_t) (i * 2)];
        fftData[(size_t) ((N - i) * 2 + 1)] = -fftData[(size_t) (i * 2 + 1)];
    }

    fft.performRealOnlyInverseTransform (fftData.data());

    for (int i = 0; i < N; ++i)
        impulseResponse[(size_t) i] = fftData[(size_t) i];
}

void NaturalPhaseEngine::generateKernel()
{
    MagnitudeCallback callback;
    {
        std::lock_guard<std::mutex> lock (kernelMutex);
        callback = magnitudeCallback;
    }

    if (! callback || sampleRate <= 0)
        return;

    int N = kernelSize;
    int halfN = N / 2;
    int numBins = halfN + 1;

    // Get magnitude response
    std::vector<double> frequencies ((size_t) numBins);
    std::vector<double> magnitudes ((size_t) numBins);

    for (int i = 0; i < numBins; ++i)
        frequencies[(size_t) i] = (double) i * sampleRate / (double) N;

    callback (frequencies.data(), magnitudes.data(), numBins);

    // Create linear-phase impulse response
    std::vector<float> linearIR ((size_t) N, 0.0f);
    {
        std::vector<float> fftData ((size_t) N * 2, 0.0f);
        for (int i = 0; i < numBins; ++i)
        {
            float linearMag = std::pow (10.0f, (float) magnitudes[i] / 20.0f);
            fftData[(size_t) (i * 2)] = linearMag;
        }
        for (int i = 1; i < halfN; ++i)
        {
            fftData[(size_t) ((N - i) * 2)] = fftData[(size_t) (i * 2)];
        }

        int fftOrder = (int) std::round (std::log2 ((double) N));
        juce::dsp::FFT ifft (fftOrder);
        ifft.performRealOnlyInverseTransform (fftData.data());

        for (int i = 0; i < N; ++i)
        {
            int shifted = (i + halfN) % N;
            linearIR[(size_t) i] = fftData[(size_t) shifted];
        }
    }

    // Create minimum-phase impulse response
    std::vector<float> minPhaseIR (linearIR);
    computeMinimumPhase (minPhaseIR);

    // Blend: minimum-phase emphasis at low freq, linear-phase at high freq
    // Use a crossfade based on frequency: below 500 Hz → minimum phase, above 2 kHz → linear phase
    std::vector<float> blendedIR ((size_t) N, 0.0f);
    float crossoverLow = 500.0f;
    float crossoverHigh = 2000.0f;

    for (int i = 0; i < N; ++i)
    {
        // Simple time-domain blend (approximate — a proper frequency-domain blend would be better)
        float t = (float) i / (float) N;
        // Closer to center → more linear phase, edges → more minimum phase
        float blend = juce::jlimit (0.0f, 1.0f, std::abs (t - 0.5f) * 4.0f);
        blendedIR[(size_t) i] = linearIR[(size_t) i] * (1.0f - blend * 0.5f)
                                + minPhaseIR[(size_t) i] * (blend * 0.5f);
    }

    // Normalize
    float sum = 0.0f;
    for (auto& s : blendedIR)
        sum += s;
    if (std::abs (sum) > 1e-10f)
    {
        float invSum = 1.0f / sum;
        for (auto& s : blendedIR)
            s *= invSum;
    }

    juce::AudioBuffer<float> kernelBuffer (1, N);
    kernelBuffer.copyFrom (0, 0, blendedIR.data(), N);

    convolver.loadImpulseResponse (std::move (kernelBuffer),
                                    sampleRate, juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::no,
                                    juce::dsp::Convolution::Normalise::no);
}

void NaturalPhaseEngine::KernelThread::run()
{
    while (! threadShouldExit())
    {
        if (engine.kernelUpdateRequested.exchange (false, std::memory_order_acquire))
        {
            engine.generateKernel();
        }

        wait (100);
    }
}
