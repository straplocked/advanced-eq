/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "LinearPhaseEngine.h"
#include <cmath>

LinearPhaseEngine::LinearPhaseEngine() = default;

LinearPhaseEngine::~LinearPhaseEngine()
{
    if (kernelThread)
    {
        kernelThread->stopThread (2000);
        kernelThread.reset();
    }
}

void LinearPhaseEngine::prepare (double sr, int maxBlock, int numCh)
{
    sampleRate = sr;
    maxBlockSize = maxBlock;
    numChannels = numCh;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32) maxBlock;
    spec.numChannels = (juce::uint32) numCh;
    convolver.prepare (spec);

    // Start kernel generation thread
    if (! kernelThread)
    {
        kernelThread = std::make_unique<KernelThread> (*this);
        kernelThread->startThread (juce::Thread::Priority::low);
    }

    prepared = true;
    requestKernelUpdate();
}

void LinearPhaseEngine::reset()
{
    convolver.reset();
}

void LinearPhaseEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (! prepared)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    convolver.process (context);
}

void LinearPhaseEngine::setKernelSize (int size)
{
    kernelSize = juce::jlimit (512, 16384, size);
    requestKernelUpdate();
}

void LinearPhaseEngine::setMagnitudeCallback (MagnitudeCallback callback)
{
    std::lock_guard<std::mutex> lock (kernelMutex);
    magnitudeCallback = std::move (callback);
}

void LinearPhaseEngine::requestKernelUpdate()
{
    kernelUpdateRequested.store (true, std::memory_order_release);
    if (kernelThread)
        kernelThread->notify();
}

void LinearPhaseEngine::applyKaiserWindow (std::vector<float>& kernel, float beta)
{
    int N = (int) kernel.size();
    float halfN = (float) (N - 1) / 2.0f;

    // Approximation of I0 (modified Bessel function of the first kind, order 0)
    auto besselI0 = [] (float x) -> float
    {
        float sum = 1.0f;
        float term = 1.0f;
        for (int k = 1; k < 20; ++k)
        {
            term *= (x * x) / (4.0f * (float) (k * k));
            sum += term;
            if (term < 1e-10f) break;
        }
        return sum;
    };

    float denominator = besselI0 (beta);

    for (int n = 0; n < N; ++n)
    {
        float ratio = ((float) n - halfN) / halfN;
        float arg = beta * std::sqrt (1.0f - ratio * ratio);
        kernel[(size_t) n] *= besselI0 (arg) / denominator;
    }
}

void LinearPhaseEngine::generateKernel()
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

    // Evaluate magnitude response at N/2+1 uniformly spaced frequency bins
    int numBins = halfN + 1;
    std::vector<double> frequencies ((size_t) numBins);
    std::vector<double> magnitudes ((size_t) numBins);

    for (int i = 0; i < numBins; ++i)
        frequencies[(size_t) i] = (double) i * sampleRate / (double) N;

    callback (frequencies.data(), magnitudes.data(), numBins);

    // Convert dB magnitudes to linear and create the frequency-domain representation
    // Magnitudes are real, phase is zero (linear phase = symmetric impulse response)
    std::vector<float> fftData ((size_t) N * 2, 0.0f);

    for (int i = 0; i < numBins; ++i)
    {
        float linearMag = std::pow (10.0f, (float) magnitudes[i] / 20.0f);
        fftData[(size_t) (i * 2)]     = linearMag; // Real
        fftData[(size_t) (i * 2 + 1)] = 0.0f;     // Imaginary (zero phase)
    }

    // Mirror for negative frequencies
    for (int i = 1; i < halfN; ++i)
    {
        fftData[(size_t) ((N - i) * 2)]     = fftData[(size_t) (i * 2)];
        fftData[(size_t) ((N - i) * 2 + 1)] = 0.0f;
    }

    // Inverse FFT to get impulse response
    int fftOrder = (int) std::round (std::log2 ((double) N));
    juce::dsp::FFT ifft (fftOrder);
    ifft.performRealOnlyInverseTransform (fftData.data());

    // Extract the real part and circularly shift to make it causal and symmetric
    std::vector<float> impulseResponse ((size_t) N);
    for (int i = 0; i < N; ++i)
    {
        int shifted = (i + halfN) % N;
        impulseResponse[(size_t) i] = fftData[(size_t) shifted];
    }

    // Apply Kaiser window (beta = 8 gives good sidelobe suppression)
    applyKaiserWindow (impulseResponse, 8.0f);

    // Normalize
    float sum = 0.0f;
    for (auto& s : impulseResponse)
        sum += s;
    if (std::abs (sum) > 1e-10f)
    {
        float invSum = 1.0f / sum;
        for (auto& s : impulseResponse)
            s *= invSum;
    }

    // Load the kernel into the convolver
    juce::AudioBuffer<float> kernelBuffer (1, N);
    kernelBuffer.copyFrom (0, 0, impulseResponse.data(), N);

    convolver.loadImpulseResponse (std::move (kernelBuffer),
                                    sampleRate, juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::no,
                                    juce::dsp::Convolution::Normalise::no);
}

void LinearPhaseEngine::KernelThread::run()
{
    while (! threadShouldExit())
    {
        if (engine.kernelUpdateRequested.exchange (false, std::memory_order_acquire))
        {
            engine.generateKernel();
        }

        wait (100); // Check every 100ms
    }
}
