#pragma once

#include <juce_core/juce_core.h>

namespace ParamIDs
{
    // Generates band-specific parameter IDs: "band_00_freq", "band_01_gain", etc.
    inline juce::String bandParam (int bandIndex, const juce::String& param)
    {
        return "band_" + juce::String (bandIndex).paddedLeft ('0', 2) + "_" + param;
    }

    // Per-band parameter suffixes
    constexpr const char* kActive     = "active";
    constexpr const char* kType       = "type";
    constexpr const char* kFrequency  = "freq";
    constexpr const char* kGain       = "gain";
    constexpr const char* kQ          = "q";
    constexpr const char* kSlope      = "slope";
    constexpr const char* kStereo     = "stereo";
    constexpr const char* kDynEnabled = "dyn_on";
    constexpr const char* kDynThresh  = "dyn_thresh";
    constexpr const char* kDynRange   = "dyn_range";
    constexpr const char* kDynAttack  = "dyn_attack";
    constexpr const char* kDynRelease = "dyn_release";

    // Global parameters
    constexpr const char* kOutputGain  = "output_gain";
    constexpr const char* kOutputPan   = "output_pan";
    constexpr const char* kProcessMode = "process_mode";
    constexpr const char* kOversample  = "oversample";
    constexpr const char* kAutoGain    = "auto_gain";
    constexpr const char* kAnalyzerOn  = "analyzer_on";
    constexpr const char* kAnalyzerPre = "analyzer_pre";
    constexpr const char* kAnalyzerRes = "analyzer_res";
    constexpr const char* kAnalyzerSpd = "analyzer_speed";

    // Filter type names (matching FilterType enum order)
    inline juce::StringArray getFilterTypeNames()
    {
        return { "Bell", "Low Shelf", "High Shelf", "High Pass",
                 "Low Pass", "Notch", "Band Pass", "Tilt Shelf", "Flat Tilt" };
    }

    // Slope names
    inline juce::StringArray getSlopeNames()
    {
        return { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct",
                 "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct", "Brickwall" };
    }

    // Stereo placement names
    inline juce::StringArray getStereoPlacementNames()
    {
        return { "Stereo", "Left", "Right", "Mid", "Side" };
    }

    // Processing mode names
    inline juce::StringArray getProcessingModeNames()
    {
        return { "Zero Latency", "Natural Phase", "Linear Phase" };
    }

    // Oversampling names
    inline juce::StringArray getOversamplingNames()
    {
        return { "1x", "2x", "4x" };
    }
}
