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
#pragma once

/**
 * @file BiquadCoefficients.h
 * @brief Biquad filter coefficient calculation based on the Robert Bristow-Johnson Audio EQ Cookbook.
 *
 * Provides factory functions for computing normalized biquad (second-order IIR)
 * coefficients for standard equalizer filter types (Bell, Shelf, HP, LP, Notch,
 * BandPass, AllPass). The implementation follows the widely-used formulas from:
 *
 *   Robert Bristow-Johnson, "Cookbook formulae for audio EQ biquad filter
 *   coefficients" (Audio-EQ-Cookbook.txt).
 *
 * All coefficients are returned with a0 normalized to 1, so the transfer
 * function is:
 *
 *     H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
 *
 * This namespace also provides utility functions for evaluating the complex
 * transfer function and magnitude response at arbitrary frequencies, used by
 * the UI for curve rendering.
 */

#include <cmath>
#include <complex>

/**
 * @brief Coefficient calculation utilities for standard biquad (second-order IIR) filters.
 *
 * All make*() functions use the bilinear transform with pre-warped frequencies
 * to map the analog prototype to the digital domain.
 */
namespace BiquadCoefficients
{
    /**
     * @brief Normalized biquad filter coefficients.
     *
     * Stores the five independent coefficients of a second-order IIR filter
     * with a0 normalized to 1. The transfer function is:
     *
     *     H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
     */
    struct Coefficients
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;  ///< Numerator (feedforward) coefficients.
        double a1 = 0.0, a2 = 0.0;              ///< Denominator (feedback) coefficients (a0 normalized to 1).
    };

    // Robert Bristow-Johnson Audio EQ Cookbook implementations

    /**
     * @brief Compute coefficients for a parametric peaking (bell) filter.
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Center frequency in Hz.
     * @param Q           Quality factor (bandwidth = freq / Q).
     * @param gainDB      Boost or cut in decibels.
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeBell (double sampleRate, double freq, double Q, double gainDB);

    /**
     * @brief Compute coefficients for a low-frequency shelving filter.
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Shelf transition frequency in Hz.
     * @param Q           Slope steepness factor (0.707 = Butterworth transition).
     * @param gainDB      Shelf gain in decibels.
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeLowShelf (double sampleRate, double freq, double Q, double gainDB);

    /**
     * @brief Compute coefficients for a high-frequency shelving filter.
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Shelf transition frequency in Hz.
     * @param Q           Slope steepness factor (0.707 = Butterworth transition).
     * @param gainDB      Shelf gain in decibels.
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeHighShelf (double sampleRate, double freq, double Q, double gainDB);

    /**
     * @brief Compute coefficients for a second-order high-pass (low-cut) filter.
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Cutoff frequency in Hz (-3 dB point for Q = 0.707).
     * @param Q           Resonance / quality factor.
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeHighPass (double sampleRate, double freq, double Q);

    /**
     * @brief Compute coefficients for a second-order low-pass (high-cut) filter.
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Cutoff frequency in Hz (-3 dB point for Q = 0.707).
     * @param Q           Resonance / quality factor.
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeLowPass (double sampleRate, double freq, double Q);

    /**
     * @brief Compute coefficients for a notch (band-reject) filter.
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Center frequency of the notch in Hz.
     * @param Q           Quality factor (higher = narrower notch).
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeNotch (double sampleRate, double freq, double Q);

    /**
     * @brief Compute coefficients for a band-pass filter (constant-skirt-gain).
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Center frequency in Hz.
     * @param Q           Quality factor (higher = narrower pass band).
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeBandPass (double sampleRate, double freq, double Q);

    /**
     * @brief Compute coefficients for a second-order all-pass filter.
     *
     * Passes all frequencies with unity gain but introduces a frequency-
     * dependent phase shift (180 degrees at the center frequency).
     *
     * @param sampleRate  Host sample rate in Hz.
     * @param freq        Center frequency in Hz (point of 180-degree phase shift).
     * @param Q           Quality factor controlling the steepness of the phase transition.
     * @return            Normalized biquad coefficients.
     */
    Coefficients makeAllPass (double sampleRate, double freq, double Q);

    // Evaluate transfer function at a given frequency (for UI curve rendering)

    /**
     * @brief Evaluate the complex transfer function H(z) at a given frequency.
     *
     * Computes H(e^{j*omega}) where omega = 2*pi*freq/sampleRate by
     * substituting z = e^{j*omega} into the biquad transfer function.
     * The result encodes both magnitude (|H|) and phase (arg(H)).
     *
     * @param c           Biquad coefficients to evaluate.
     * @param freq        Frequency in Hz at which to evaluate.
     * @param sampleRate  Host sample rate in Hz.
     * @return            Complex frequency response at the given frequency.
     */
    std::complex<double> evaluateTransferFunction (
        const Coefficients& c, double freq, double sampleRate);

    // Magnitude in dB at a frequency

    /**
     * @brief Compute the magnitude response in decibels at a given frequency.
     *
     * Convenience wrapper: returns 20 * log10(abs(evaluateTransferFunction(c, freq, sampleRate))).
     *
     * @param c           Biquad coefficients to evaluate.
     * @param freq        Frequency in Hz.
     * @param sampleRate  Host sample rate in Hz.
     * @return            Magnitude in dB at the specified frequency.
     */
    double magnitudeAtFrequency (
        const Coefficients& c, double freq, double sampleRate);

} // namespace BiquadCoefficients
