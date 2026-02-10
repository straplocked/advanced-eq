/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "BiquadCoefficients.h"

namespace BiquadCoefficients
{

static constexpr double kPi = 3.14159265358979323846;

static Coefficients normalize (double b0, double b1, double b2,
                               double a0, double a1, double a2)
{
    double inv = 1.0 / a0;
    return { b0 * inv, b1 * inv, b2 * inv, a1 * inv, a2 * inv };
}

Coefficients makeBell (double sampleRate, double freq, double Q, double gainDB)
{
    double A  = std::pow (10.0, gainDB / 40.0);
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);

    return normalize (
        1.0 + alpha * A,
        -2.0 * cosw0,
        1.0 - alpha * A,
        1.0 + alpha / A,
        -2.0 * cosw0,
        1.0 - alpha / A
    );
}

Coefficients makeLowShelf (double sampleRate, double freq, double Q, double gainDB)
{
    double A  = std::pow (10.0, gainDB / 40.0);
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);
    double sqrtA = std::sqrt (A);
    double twoSqrtAAlpha = 2.0 * sqrtA * alpha;

    return normalize (
        A * ((A + 1.0) - (A - 1.0) * cosw0 + twoSqrtAAlpha),
        2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0),
        A * ((A + 1.0) - (A - 1.0) * cosw0 - twoSqrtAAlpha),
        (A + 1.0) + (A - 1.0) * cosw0 + twoSqrtAAlpha,
        -2.0 * ((A - 1.0) + (A + 1.0) * cosw0),
        (A + 1.0) + (A - 1.0) * cosw0 - twoSqrtAAlpha
    );
}

Coefficients makeHighShelf (double sampleRate, double freq, double Q, double gainDB)
{
    double A  = std::pow (10.0, gainDB / 40.0);
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);
    double sqrtA = std::sqrt (A);
    double twoSqrtAAlpha = 2.0 * sqrtA * alpha;

    return normalize (
        A * ((A + 1.0) + (A - 1.0) * cosw0 + twoSqrtAAlpha),
        -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0),
        A * ((A + 1.0) + (A - 1.0) * cosw0 - twoSqrtAAlpha),
        (A + 1.0) - (A - 1.0) * cosw0 + twoSqrtAAlpha,
        2.0 * ((A - 1.0) - (A + 1.0) * cosw0),
        (A + 1.0) - (A - 1.0) * cosw0 - twoSqrtAAlpha
    );
}

Coefficients makeHighPass (double sampleRate, double freq, double Q)
{
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);

    return normalize (
        (1.0 + cosw0) / 2.0,
        -(1.0 + cosw0),
        (1.0 + cosw0) / 2.0,
        1.0 + alpha,
        -2.0 * cosw0,
        1.0 - alpha
    );
}

Coefficients makeLowPass (double sampleRate, double freq, double Q)
{
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);

    return normalize (
        (1.0 - cosw0) / 2.0,
        1.0 - cosw0,
        (1.0 - cosw0) / 2.0,
        1.0 + alpha,
        -2.0 * cosw0,
        1.0 - alpha
    );
}

Coefficients makeNotch (double sampleRate, double freq, double Q)
{
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);

    return normalize (
        1.0,
        -2.0 * cosw0,
        1.0,
        1.0 + alpha,
        -2.0 * cosw0,
        1.0 - alpha
    );
}

Coefficients makeBandPass (double sampleRate, double freq, double Q)
{
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);

    return normalize (
        alpha,
        0.0,
        -alpha,
        1.0 + alpha,
        -2.0 * cosw0,
        1.0 - alpha
    );
}

Coefficients makeAllPass (double sampleRate, double freq, double Q)
{
    double w0 = 2.0 * kPi * freq / sampleRate;
    double cosw0 = std::cos (w0);
    double sinw0 = std::sin (w0);
    double alpha = sinw0 / (2.0 * Q);

    return normalize (
        1.0 - alpha,
        -2.0 * cosw0,
        1.0 + alpha,
        1.0 + alpha,
        -2.0 * cosw0,
        1.0 - alpha
    );
}

std::complex<double> evaluateTransferFunction (
    const Coefficients& c, double freq, double sampleRate)
{
    double w = 2.0 * kPi * freq / sampleRate;
    std::complex<double> z = std::exp (std::complex<double> (0.0, w));
    std::complex<double> z_1 = 1.0 / z;
    std::complex<double> z_2 = z_1 * z_1;

    std::complex<double> num = c.b0 + c.b1 * z_1 + c.b2 * z_2;
    std::complex<double> den = 1.0  + c.a1 * z_1 + c.a2 * z_2;

    return num / den;
}

double magnitudeAtFrequency (const Coefficients& c, double freq, double sampleRate)
{
    auto response = evaluateTransferFunction (c, freq, sampleRate);
    double mag = std::abs (response);
    return 20.0 * std::log10 (std::max (mag, 1e-10));
}

} // namespace BiquadCoefficients
