#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/EqBand.h"
#include "../dsp/FilterTypes.h"
#include "ParameterIDs.h"

// Bridges APVTS parameter values to DSP EqBand.
// Caches getRawParameterValue() pointers for lock-free audio thread reads.
class BandParameterAttachment
{
public:
    BandParameterAttachment() = default;

    void init (int bandIndex, juce::AudioProcessorValueTreeState& apvts)
    {
        rawActive     = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kActive));
        rawType       = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kType));
        rawFreq       = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kFrequency));
        rawGain       = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kGain));
        rawQ          = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kQ));
        rawSlope      = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kSlope));
        rawStereo     = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kStereo));
        rawDynEnabled = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynEnabled));
        rawDynThresh  = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynThresh));
        rawDynRange   = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynRange));
        rawDynAttack  = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynAttack));
        rawDynRelease = apvts.getRawParameterValue (ParamIDs::bandParam (bandIndex, ParamIDs::kDynRelease));
    }

    // Read current parameter values (lock-free, safe for audio thread)
    BandParameters getCurrentParameters() const
    {
        BandParameters params;
        params.active           = rawActive->load() > 0.5f;
        params.type             = static_cast<FilterType> ((int) rawType->load());
        params.frequency        = rawFreq->load();
        params.gain             = rawGain->load();
        params.q                = rawQ->load();
        params.slopeDBPerOct    = slopeIndexToDBPerOct ((int) rawSlope->load());
        params.stereoPlacement  = static_cast<StereoPlacement> ((int) rawStereo->load());
        params.dynamicEnabled   = rawDynEnabled->load() > 0.5f;
        params.dynamicThreshold = rawDynThresh->load();
        params.dynamicRange     = rawDynRange->load();
        params.dynamicAttack    = rawDynAttack->load();
        params.dynamicRelease   = rawDynRelease->load();
        return params;
    }

private:
    std::atomic<float>* rawActive     = nullptr;
    std::atomic<float>* rawType       = nullptr;
    std::atomic<float>* rawFreq       = nullptr;
    std::atomic<float>* rawGain       = nullptr;
    std::atomic<float>* rawQ          = nullptr;
    std::atomic<float>* rawSlope      = nullptr;
    std::atomic<float>* rawStereo     = nullptr;
    std::atomic<float>* rawDynEnabled = nullptr;
    std::atomic<float>* rawDynThresh  = nullptr;
    std::atomic<float>* rawDynRange   = nullptr;
    std::atomic<float>* rawDynAttack  = nullptr;
    std::atomic<float>* rawDynRelease = nullptr;
};
