/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "LinearPhaseEngine.h"

/**
 * Natural phase EQ engine.
 *
 * Mixed-phase approach: compute magnitude response, apply cepstral
 * minimum-phase reconstruction, blend between minimum-phase (at low
 * frequencies) and linear-phase (at high frequencies).
 *
 * Uses a shorter kernel than full linear phase → less latency, less pre-ringing.
 */
class NaturalPhaseEngine
{
public:
    NaturalPhaseEngine();
    ~NaturalPhaseEngine();

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void process (juce::AudioBuffer<float>& buffer);

    int getLatencyInSamples() const { return kernelSize / 2; }

    using MagnitudeCallback = LinearPhaseEngine::MagnitudeCallback;
    void setMagnitudeCallback (MagnitudeCallback callback);
    void requestKernelUpdate();

private:
    void generateKernel();
    void computeMinimumPhase (std::vector<float>& impulseResponse);

    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    int numChannels = 2;
    int kernelSize = 2048; // Shorter than linear phase

    juce::dsp::Convolution convolver;
    std::mutex kernelMutex;
    MagnitudeCallback magnitudeCallback;
    std::atomic<bool> kernelUpdateRequested { false };

    class KernelThread : public juce::Thread
    {
    public:
        KernelThread (NaturalPhaseEngine& owner) : juce::Thread ("NaturalPhaseKernel"), engine (owner) {}
        void run() override;
    private:
        NaturalPhaseEngine& engine;
    };

    std::unique_ptr<KernelThread> kernelThread;
    bool prepared = false;
};
