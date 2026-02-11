/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "FilterTypes.h"
#include "EqBand.h"
#include "../utils/Constants.h"
#include <vector>
#include <cmath>
#include <algorithm>

/**
 * EQ Match algorithm.
 *
 * Computes the difference between a reference and current spectrum,
 * then iteratively picks peaks/valleys and fits Bell bands to match
 * the reference curve.
 */
class EQMatcher
{
public:
    struct MatchBand
    {
        float frequency = 1000.0f;
        float gain = 0.0f;
        float q = 1.0f;
    };

    /**
     * Compute match bands from reference and current average spectra.
     *
     * @param referenceSpectrum  Averaged dB magnitude spectrum (from FFTProcessor capture).
     * @param currentSpectrum    Averaged dB magnitude spectrum (from FFTProcessor capture).
     * @param numBins            Number of FFT bins in each spectrum.
     * @param sampleRate         Sample rate used during capture.
     * @param fftSize            FFT size used during capture.
     * @param maxBands           Maximum number of match bands to generate (default 8).
     * @return Vector of MatchBand results.
     */
    static std::vector<MatchBand> computeMatch (const float* referenceSpectrum,
                                                 const float* currentSpectrum,
                                                 int numBins,
                                                 double sampleRate,
                                                 int fftSize,
                                                 int maxBands = 8)
    {
        std::vector<MatchBand> results;

        // Compute difference curve (in dB)
        std::vector<float> diff ((size_t) numBins);
        for (int i = 0; i < numBins; ++i)
            diff[(size_t) i] = referenceSpectrum[i] - currentSpectrum[i];

        // Smooth the diff curve to avoid matching noise
        std::vector<float> smoothDiff = smooth (diff, 5);

        // Working copy that we subtract matched bands from
        std::vector<float> residual = smoothDiff;

        for (int bandIdx = 0; bandIdx < maxBands; ++bandIdx)
        {
            // Find the largest peak or valley in the residual
            int peakBin = 0;
            float peakAbsVal = 0.0f;

            // Skip DC and near-Nyquist bins
            for (int i = 2; i < numBins - 2; ++i)
            {
                float absVal = std::abs (residual[(size_t) i]);
                if (absVal > peakAbsVal)
                {
                    peakAbsVal = absVal;
                    peakBin = i;
                }
            }

            // Stop if remaining residual is small
            if (peakAbsVal < 1.0f) // Less than 1 dB difference
                break;

            // Convert bin to frequency
            float freq = (float) peakBin * (float) sampleRate / (float) fftSize;

            // Clamp to useful range
            freq = std::max (20.0f, std::min (freq, (float) sampleRate * 0.45f));

            float gain = residual[(size_t) peakBin];

            // Estimate Q from peak width (find -3 dB points relative to peak)
            float halfGain = peakAbsVal * 0.5f;
            int leftBin = peakBin;
            int rightBin = peakBin;

            for (int i = peakBin - 1; i >= 1; --i)
            {
                if (std::abs (residual[(size_t) i]) < halfGain)
                {
                    leftBin = i;
                    break;
                }
                leftBin = i;
            }

            for (int i = peakBin + 1; i < numBins - 1; ++i)
            {
                if (std::abs (residual[(size_t) i]) < halfGain)
                {
                    rightBin = i;
                    break;
                }
                rightBin = i;
            }

            float leftFreq = (float) leftBin * (float) sampleRate / (float) fftSize;
            float rightFreq = (float) rightBin * (float) sampleRate / (float) fftSize;
            float bandwidth = rightFreq - leftFreq;

            float q = 1.0f;
            if (bandwidth > 0.0f)
                q = freq / bandwidth;

            q = std::max (0.1f, std::min (q, 10.0f));

            // Clamp gain to reasonable range
            gain = std::max (-15.0f, std::min (gain, 15.0f));

            results.push_back ({ freq, gain, q });

            // Subtract this band's response from the residual
            subtractBellResponse (residual, freq, gain, q, sampleRate, fftSize);
        }

        return results;
    }

    /**
     * Get the difference curve between two spectra (for UI display).
     */
    static std::vector<float> computeDiffCurve (const float* referenceSpectrum,
                                                  const float* currentSpectrum,
                                                  int numBins)
    {
        std::vector<float> diff ((size_t) numBins);
        for (int i = 0; i < numBins; ++i)
            diff[(size_t) i] = referenceSpectrum[i] - currentSpectrum[i];
        return diff;
    }

private:
    static std::vector<float> smooth (const std::vector<float>& data, int windowSize)
    {
        std::vector<float> result (data.size());
        int half = windowSize / 2;

        for (int i = 0; i < (int) data.size(); ++i)
        {
            float sum = 0.0f;
            int count = 0;

            for (int j = i - half; j <= i + half; ++j)
            {
                if (j >= 0 && j < (int) data.size())
                {
                    sum += data[(size_t) j];
                    ++count;
                }
            }

            result[(size_t) i] = (count > 0) ? sum / (float) count : data[(size_t) i];
        }

        return result;
    }

    static void subtractBellResponse (std::vector<float>& residual,
                                       float freq, float gain, float q,
                                       double sampleRate, int fftSize)
    {
        int numBins = (int) residual.size();

        for (int i = 0; i < numBins; ++i)
        {
            float binFreq = (float) i * (float) sampleRate / (float) fftSize;
            if (binFreq < 1.0f)
                continue;

            // Compute bell filter magnitude response at this frequency
            float ratio = binFreq / freq;
            float logRatio = std::log2 (ratio);
            float bellMag = gain * std::exp (-0.5f * (logRatio * logRatio) * q * q * 4.0f);

            residual[(size_t) i] -= bellMag;
        }
    }
};
