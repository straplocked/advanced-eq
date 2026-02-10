#pragma once

#include "SVFFilter.h"
#include "FilterTypes.h"
#include <array>
#include <complex>

// Cascades multiple SVF sections for higher-order filter slopes.
// 12 dB/oct = 1 section, 24 = 2, 48 = 4, 96 = 8, etc.
class CascadedFilter
{
public:
    static constexpr int kMaxSections = 8; // 96 dB/oct max

    CascadedFilter() = default;

    void prepare (double sampleRate);
    void reset();

    void setParameters (FilterType type, double freq, double Q,
                        double gainDB, int slopeDBPerOct);

    void process (float* buffer, int numSamples);
    float processSample (float input);

    // Composite frequency response (product of all sections)
    std::complex<double> getResponse (double freq) const;
    double getMagnitudeAtFrequency (double freq) const;

    int getNumActiveSections() const { return numActiveSections; }

private:
    std::array<SVFFilter, kMaxSections> sections;
    int numActiveSections = 1;
    double sampleRate = 44100.0;
};
