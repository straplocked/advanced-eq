#pragma once

#include "EqBand.h"
#include "FilterTypes.h"
#include "../utils/Constants.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class ProcessingChain
{
public:
    ProcessingChain() = default;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    // Main audio processing entry point
    void process (juce::AudioBuffer<float>& buffer);

    // Parameter updates
    void updateBand (int bandIndex, const BandParameters& params);
    void setOutputGain (float gainDB);

    // For UI: compute magnitude responses
    void computeCompositeMagnitude (const double* frequencies,
                                    double* magnitudes, int numPoints) const;
    void computeBandMagnitude (int bandIndex, const double* frequencies,
                               double* magnitudes, int numPoints) const;

    EqBand* getBand (int index);
    int getLatencyInSamples() const { return 0; } // Zero latency mode for now

private:
    std::array<EqBand, Constants::kMaxBands> bands;

    juce::SmoothedValue<float> outputGain { 1.0f };

    double sampleRate = 44100.0;
    int maxBlockSize = 512;
};
