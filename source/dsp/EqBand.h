#pragma once

#include "CascadedFilter.h"
#include "FilterTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
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

    // Dynamic EQ (Phase 8)
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

    // Process stereo buffer (only processes the channels this band targets)
    void process (juce::AudioBuffer<float>& buffer, int numSamples);

    // For UI: compute magnitude response at given frequencies
    void computeMagnitudeResponse (const double* frequencies, double* magnitudes,
                                   int numPoints) const;

    bool isActive() const { return active; }
    FilterType getFilterType() const { return filterType; }
    StereoPlacement getStereoPlacement() const { return stereoPlacement; }

    const BandParameters& getCurrentParams() const { return currentParams; }

private:
    CascadedFilter filterL;
    CascadedFilter filterR;

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
};
