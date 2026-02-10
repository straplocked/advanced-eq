#pragma once

#include <juce_core/juce_core.h>
#include "../dsp/FilterTypes.h"
#include <vector>

struct BandPreset
{
    int         bandIndex;
    FilterType  type       = FilterType::Bell;
    float       frequency  = 1000.0f;
    float       gain       = 0.0f;
    float       q          = 1.0f;
    int         slopeIndex = 1;   // 0=6dB, 1=12dB, 2=18dB, 3=24dB, 4=36dB, 5=48dB, 6=72dB, 7=96dB, 8=Brickwall
    int         stereoIndex = 0;  // 0=Stereo, 1=Left, 2=Right, 3=Mid, 4=Side
};

struct Preset
{
    juce::String                name;
    juce::String                category;
    std::vector<BandPreset>     bands;
    float                       outputGain = 0.0f;
    bool                        isFactory  = true;
    juce::File                  sourceFile;  // only set for user presets
};
