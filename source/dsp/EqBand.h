/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "CascadedFilter.h"
#include "FilterTypes.h"
#include "EnvelopeFollower.h"
#include "DynamicProcessor.h"
#include "SpectralDynamicsProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <complex>

struct BandParameters
{
    bool active = false;
    FilterType type = FilterType::Bell;
    float frequency = 1000.0f;
    float gain = 0.0f;
    float q = 1.0f;
    int slopeDBPerOct = 12;
    StereoPlacement stereoPlacement = StereoPlacement::Stereo;

    // Dynamic EQ
    bool dynamicEnabled = false;
    float dynamicThreshold = -20.0f;
    float dynamicRange = 0.0f;
    float dynamicAttack = 10.0f;
    float dynamicRelease = 100.0f;
};

class EqBand
{
public:
    EqBand() = default;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void updateParameters (const BandParameters& params);
    void setAnalogMode (bool enabled);
    void setCharacterMode (int mode);
    void setSpectralDynamics (bool enabled, float threshold, float range, float ratio);

    void process (juce::AudioBuffer<float>& buffer, int numSamples);
    void processSingleChannel (float* channelData, int numSamples);

    /** Process a specific channel by index (for surround routing). */
    void processChannel (float* channelData, int numSamples, int channelIndex);

    void computeMagnitudeResponse (const double* frequencies, double* magnitudes,
                                   int numPoints) const;

    bool isActive() const { return active; }
    FilterType getFilterType() const { return filterType; }
    StereoPlacement getStereoPlacement() const { return stereoPlacement; }
    const BandParameters& getCurrentParams() const { return currentParams; }

    /** Returns the current dynamic gain reduction/boost in dB (for UI metering). */
    float getCurrentDynamicGainReduction() const
    {
        return dynamicGainReduction.load (std::memory_order_relaxed);
    }

    static constexpr int kMaxChannels = 16; // 9.1.6 Atmos = 16 channels

private:
    std::array<CascadedFilter, kMaxChannels> filters;
    // Legacy accessors for internal use
    CascadedFilter& filterL = filters[0];
    CascadedFilter& filterR = filters[1];

    BandParameters currentParams;
    bool active = false;
    FilterType filterType = FilterType::Bell;
    StereoPlacement stereoPlacement = StereoPlacement::Stereo;

    // Smoothed parameters for zipper-free changes
    juce::SmoothedValue<float> smoothedFreq { 1000.0f };
    juce::SmoothedValue<float> smoothedGain { 0.0f };
    juce::SmoothedValue<float> smoothedQ { 1.0f };

    double sampleRate = 44100.0;
    bool needsUpdate = true;
    bool analogMode = false;
    int characterMode = 0; // 0=Clean, 1=Smooth, 2=Vintage, 3=Aggressive

    // Dynamic EQ (only L/R used for stereo; index 0/1 for surround)
    std::array<EnvelopeFollower, kMaxChannels> envFollowers;
    std::array<DynamicProcessor, kMaxChannels> dynamicProcs;
    EnvelopeFollower& envFollowerL = envFollowers[0];
    EnvelopeFollower& envFollowerR = envFollowers[1];
    DynamicProcessor& dynamicProcL = dynamicProcs[0];
    DynamicProcessor& dynamicProcR = dynamicProcs[1];
    bool dynamicEnabled = false;
    std::atomic<float> dynamicGainReduction { 0.0f };

    static float saturate (float x);
    static float aggressiveSaturate (float x);

    // Spectral dynamics
    bool spectralEnabled = false;
    float spectralThreshold = -20.0f;
    float spectralRange = -12.0f;
    float spectralRatio = 4.0f;
    std::array<SpectralDynamicsProcessor, kMaxChannels> spectralProcs;
};
