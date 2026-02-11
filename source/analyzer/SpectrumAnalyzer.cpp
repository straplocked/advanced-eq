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

#include "SpectrumAnalyzer.h"
#include <algorithm>

SpectrumAnalyzer::SpectrumAnalyzer() = default;

void SpectrumAnalyzer::prepare (int numDisplayPoints)
{
    numPoints = numDisplayPoints;
    smoothedMagnitudes.assign ((size_t) numPoints, Constants::kAnalyzerMinDB);
    displayFrequencies.resize ((size_t) numPoints);

    for (int i = 0; i < numPoints; ++i)
    {
        float normalized = (float) i / (float) (numPoints - 1);
        displayFrequencies[(size_t) i] = MathUtils::normalizedToFrequency (normalized);
    }
}

bool SpectrumAnalyzer::update (FFTProcessor& fftProcessor,
                                float displayWidth, float displayHeight)
{
    if (numPoints == 0)
        return false;

    auto& magnitudeBuffer = fftProcessor.getMagnitudeBuffer();

    if (! magnitudeBuffer.swapRead())
        return false;

    const float* fftMagnitudes = magnitudeBuffer.getReadPointer();
    int numBins = fftProcessor.getNumBins();
    double sampleRate = fftProcessor.getSampleRate();
    float binWidth = (float) (sampleRate / (double) fftProcessor.getCurrentFFTSize());

    // Resample from linear FFT bins to log-spaced display points with smoothing
    for (int i = 0; i < numPoints; ++i)
    {
        float freq = displayFrequencies[(size_t) i];

        // Find the corresponding FFT bin (floating point)
        float binPos = freq / binWidth;

        // Linear interpolation between surrounding bins
        int binLow = (int) binPos;
        int binHigh = binLow + 1;
        float frac = binPos - (float) binLow;

        float dB;
        if (binLow < 0)
            dB = Constants::kAnalyzerMinDB;
        else if (binHigh >= numBins)
            dB = fftMagnitudes[numBins - 1];
        else
            dB = fftMagnitudes[binLow] * (1.0f - frac) + fftMagnitudes[binHigh] * frac;

        // Apply spectrum tilt compensation
        if (std::abs (tiltSlope) > 0.01f)
            dB += tiltSlope * std::log2 (std::max (freq, 1.0f) / 1000.0f);

        // Asymmetric smoothing: fast attack (0.5x coeff), slow decay (full coeff)
        float current = smoothedMagnitudes[(size_t) i];
        if (dB > current)
            smoothedMagnitudes[(size_t) i] = current + (1.0f - smoothingCoeff * 0.5f) * (dB - current);
        else
            smoothedMagnitudes[(size_t) i] = current + (1.0f - smoothingCoeff) * (dB - current);
    }

    // Build paths
    spectrumPath.clear();
    spectrumFillPath.clear();

    bool started = false;

    for (int i = 0; i < numPoints; ++i)
    {
        float x = (float) i / (float) (numPoints - 1) * displayWidth;
        float y = spectrumDBtoY (smoothedMagnitudes[(size_t) i], displayHeight);
        y = juce::jlimit (0.0f, displayHeight, y);

        if (! started)
        {
            spectrumPath.startNewSubPath (x, y);
            spectrumFillPath.startNewSubPath (x, displayHeight);
            spectrumFillPath.lineTo (x, y);
            started = true;
        }
        else
        {
            spectrumPath.lineTo (x, y);
            spectrumFillPath.lineTo (x, y);
        }
    }

    // Close the fill path along the bottom
    if (started)
    {
        spectrumFillPath.lineTo (displayWidth, displayHeight);
        spectrumFillPath.closeSubPath();
    }

    return true;
}

float SpectrumAnalyzer::findNearestPeak (float targetFreq, float maxOctaves) const
{
    if (numPoints == 0 || smoothedMagnitudes.empty())
        return -1.0f;

    // Find the display index closest to the target frequency
    int centerIdx = -1;
    float closestDist = 1e10f;

    for (int i = 0; i < numPoints; ++i)
    {
        float dist = std::abs (std::log2 (displayFrequencies[(size_t) i] / targetFreq));
        if (dist < closestDist)
        {
            closestDist = dist;
            centerIdx = i;
        }
    }

    if (centerIdx < 0)
        return -1.0f;

    // Search radius in indices (approximate based on octave range)
    float freqLow = targetFreq * std::pow (2.0f, -maxOctaves);
    float freqHigh = targetFreq * std::pow (2.0f, maxOctaves);

    int startIdx = 0;
    int endIdx = numPoints - 1;

    for (int i = 0; i < numPoints; ++i)
    {
        if (displayFrequencies[(size_t) i] >= freqLow)
        {
            startIdx = i;
            break;
        }
    }

    for (int i = numPoints - 1; i >= 0; --i)
    {
        if (displayFrequencies[(size_t) i] <= freqHigh)
        {
            endIdx = i;
            break;
        }
    }

    // Find the local maximum within the search range
    int bestIdx = -1;
    float bestMag = Constants::kAnalyzerMinDB;

    for (int i = startIdx; i <= endIdx; ++i)
    {
        float mag = smoothedMagnitudes[(size_t) i];

        // Must be a local peak: higher than neighbors
        bool isLocalMax = true;
        if (i > 0 && smoothedMagnitudes[(size_t) (i - 1)] > mag)
            isLocalMax = false;
        if (i < numPoints - 1 && smoothedMagnitudes[(size_t) (i + 1)] > mag)
            isLocalMax = false;

        if (isLocalMax && mag > bestMag && mag > Constants::kAnalyzerMinDB + 12.0f) // Must be above noise floor
        {
            bestMag = mag;
            bestIdx = i;
        }
    }

    if (bestIdx < 0)
        return -1.0f;

    return displayFrequencies[(size_t) bestIdx];
}

float SpectrumAnalyzer::spectrumDBtoY (float dB, float height) const
{
    // Map the analyzer's full dB range to the display height.
    // +6 dBFS at top, -100 dBFS at bottom (106 dB visible range).
    float normalized = (dB - Constants::kAnalyzerMinDB)
                     / (Constants::kAnalyzerMaxDB - Constants::kAnalyzerMinDB);
    return height * (1.0f - normalized);
}
