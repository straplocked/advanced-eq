#pragma once

#include <cmath>
#include "Constants.h"

namespace MathUtils
{
    // dB <-> linear gain conversions
    inline float dbToGain (float dB) { return std::pow (10.0f, dB / 20.0f); }
    inline float gainToDb (float gain) { return 20.0f * std::log10 (std::max (gain, 1e-10f)); }

    // Frequency <-> normalized X coordinate (logarithmic mapping)
    inline float frequencyToNormalized (float freq)
    {
        return std::log (freq / Constants::kMinFrequency)
             / std::log (Constants::kMaxFrequency / Constants::kMinFrequency);
    }

    inline float normalizedToFrequency (float normalized)
    {
        return Constants::kMinFrequency
             * std::pow (Constants::kMaxFrequency / Constants::kMinFrequency, normalized);
    }

    // Map frequency to pixel X coordinate
    inline float frequencyToX (float freq, float width)
    {
        return frequencyToNormalized (freq) * width;
    }

    inline float xToFrequency (float x, float width)
    {
        return normalizedToFrequency (x / width);
    }

    // Map gain (dB) to pixel Y coordinate
    inline float gainToY (float gainDB, float height, float minDB, float maxDB)
    {
        return height * (1.0f - (gainDB - minDB) / (maxDB - minDB));
    }

    inline float yToGain (float y, float height, float minDB, float maxDB)
    {
        return minDB + (1.0f - y / height) * (maxDB - minDB);
    }
}
