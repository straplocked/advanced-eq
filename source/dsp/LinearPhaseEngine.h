/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>

/**
 * Linear phase EQ engine using FFT convolution.
 *
 * Generates a zero-phase FIR kernel from the composite magnitude response,
 * then convolves audio via overlap-add. The kernel is generated on a
 * background thread and double-buffered for click-free swapping.
 */
class LinearPhaseEngine
{
public:
    LinearPhaseEngine();
    ~LinearPhaseEngine();

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    /** Process a stereo buffer through the linear-phase FIR convolution. */
    void process (juce::AudioBuffer<float>& buffer);

    /** Set the kernel size (number of FIR taps). Default: 4096. */
    void setKernelSize (int size);

    /** Get latency in samples (kernelSize / 2). */
    int getLatencyInSamples() const { return kernelSize / 2; }

    using MagnitudeCallback = std::function<void (const double* frequencies, double* magnitudes, int numPoints)>;

    /** Set the callback used to query the current composite magnitude response. */
    void setMagnitudeCallback (MagnitudeCallback callback);

    /** Request a kernel rebuild (call when EQ parameters change). */
    void requestKernelUpdate();

private:
    void generateKernel();
    void applyKaiserWindow (std::vector<float>& kernel, float beta);

    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    int numChannels = 2;
    int kernelSize = 4096;

    // Convolution engine (JUCE's built-in uniform partitioned convolver)
    juce::dsp::Convolution convolver;
    std::mutex kernelMutex;

    // Kernel generation
    MagnitudeCallback magnitudeCallback;
    std::atomic<bool> kernelUpdateRequested { false };

    // Background thread for kernel generation
    class KernelThread : public juce::Thread
    {
    public:
        KernelThread (LinearPhaseEngine& owner) : juce::Thread ("LinearPhaseKernel"), engine (owner) {}
        void run() override;
    private:
        LinearPhaseEngine& engine;
    };

    std::unique_ptr<KernelThread> kernelThread;
    bool prepared = false;
};
