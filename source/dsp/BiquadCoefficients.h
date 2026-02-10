#pragma once

#include <cmath>
#include <complex>

namespace BiquadCoefficients
{
    struct Coefficients
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0; // a0 normalized to 1
    };

    // Robert Bristow-Johnson Audio EQ Cookbook implementations
    Coefficients makeBell (double sampleRate, double freq, double Q, double gainDB);
    Coefficients makeLowShelf (double sampleRate, double freq, double Q, double gainDB);
    Coefficients makeHighShelf (double sampleRate, double freq, double Q, double gainDB);
    Coefficients makeHighPass (double sampleRate, double freq, double Q);
    Coefficients makeLowPass (double sampleRate, double freq, double Q);
    Coefficients makeNotch (double sampleRate, double freq, double Q);
    Coefficients makeBandPass (double sampleRate, double freq, double Q);
    Coefficients makeAllPass (double sampleRate, double freq, double Q);

    // Evaluate transfer function at a given frequency (for UI curve rendering)
    std::complex<double> evaluateTransferFunction (
        const Coefficients& c, double freq, double sampleRate);

    // Magnitude in dB at a frequency
    double magnitudeAtFrequency (
        const Coefficients& c, double freq, double sampleRate);

} // namespace BiquadCoefficients
