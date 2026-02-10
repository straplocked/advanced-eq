#pragma once

#include "FilterTypes.h"
#include <complex>

// Cytomic TPT (Topology-Preserving Transform) State Variable Filter
// Based on Andy Simper's technical papers: https://cytomic.com/technical-papers/
// Handles fast parameter modulation without zipper noise or instability.
class SVFFilter
{
public:
    SVFFilter() = default;

    void prepare (double sampleRate);
    void reset();

    void setParameters (FilterType type, double freq, double Q, double gainDB);

    float processSample (float input);
    void process (float* buffer, int numSamples);

    // For UI: evaluate complex frequency response at a given frequency
    std::complex<double> getResponse (double freq) const;

    // Get magnitude in dB at a frequency
    double getMagnitudeAtFrequency (double freq) const;

    // First-order mode (for 6 dB/oct slopes and odd-order Butterworth)
    void setFirstOrder (bool firstOrder);
    bool getIsFirstOrder() const { return isFirstOrder; }

private:
    void updateCoefficients();

    // SVF state variables (integrator states)
    double ic1eq = 0.0;
    double ic2eq = 0.0;

    // Target parameters
    FilterType type = FilterType::Bell;
    double frequency = 1000.0;
    double q = 1.0;
    double gain = 0.0; // dB
    bool isFirstOrder = false;

    // Computed coefficients
    double g = 0.0;   // tan(pi * freq / sampleRate)
    double k = 0.0;   // 1/Q (or damping)
    double a1 = 0.0;
    double a2 = 0.0;
    double a3 = 0.0;
    double m0 = 0.0, m1 = 0.0, m2 = 0.0; // Mix coefficients

    double currentSampleRate = 44100.0;
};
