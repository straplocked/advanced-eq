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

#include "CascadedFilter.h"
#include <cmath>
#include <cstring>
#include <memory>

static constexpr double kPi = 3.14159265358979323846;

void CascadedFilter::prepare (double sr)
{
    sampleRate = sr;
    for (auto& section : sections)
        section.prepare (sr);
    for (auto& section : sectionsB)
        section.prepare (sr);
}

void CascadedFilter::reset()
{
    for (auto& section : sections)
        section.reset();
    for (auto& section : sectionsB)
        section.reset();
}

void CascadedFilter::setParameters (FilterType type, double freq, double Q,
                                     double gainDB, int slopeDBPerOct)
{
    auto slopeInfo = getSlopeInfo (slopeDBPerOct);

    switch (type)
    {
        case FilterType::Bell:
        case FilterType::Notch:
        case FilterType::BandPass:
        case FilterType::TiltShelf:
        case FilterType::FlatTilt:
        case FilterType::AllPass:
        {
            // These types always use a single section (slope doesn't apply)
            numActiveSections = 1;
            sections[0].setFirstOrder (false);
            sections[0].setParameters (type, freq, Q, gainDB);
            break;
        }

        case FilterType::LowShelf:
        case FilterType::HighShelf:
        {
            numActiveSections = slopeInfo.numSections;
            if (numActiveSections <= 0)
                numActiveSections = 1;

            // Distribute gain across sections for steeper shelves
            double perSectionGain = gainDB / numActiveSections;

            for (int i = 0; i < numActiveSections; ++i)
            {
                bool isLast = (i == numActiveSections - 1);
                bool useFirstOrder = isLast && slopeInfo.lastIsFirstOrder;
                sections[i].setFirstOrder (useFirstOrder);
                sections[i].setParameters (type, freq, Q, perSectionGain);
            }
            break;
        }

        case FilterType::HighPass:
        case FilterType::LowPass:
        {
            numActiveSections = slopeInfo.numSections;
            if (numActiveSections <= 0)
                numActiveSections = 1;

            // Total filter order
            int order = (slopeDBPerOct == 0) ? 16 : slopeDBPerOct / 6;

            // Number of second-order sections (excluding first-order if present)
            int numSecondOrder = slopeInfo.lastIsFirstOrder
                                     ? numActiveSections - 1
                                     : numActiveSections;

            // Butterworth Q values for each second-order section
            // Q_k = 1 / (2 * cos(pi * (2k + 1) / (2 * N))) for k = 0..numSecondOrder-1
            for (int i = 0; i < numSecondOrder; ++i)
            {
                double angle = kPi * (2.0 * i + 1.0) / (2.0 * order);
                double butterworthQ = 1.0 / (2.0 * std::cos (angle));
                sections[i].setFirstOrder (false);
                sections[i].setParameters (type, freq, butterworthQ, 0.0);
            }

            // If odd order, the last section is a first-order filter (real pole)
            if (slopeInfo.lastIsFirstOrder)
            {
                int lastIdx = numActiveSections - 1;
                sections[lastIdx].setFirstOrder (true);
                sections[lastIdx].setParameters (type, freq, 0.707, 0.0);
            }
            break;
        }
    }
}

float CascadedFilter::processSample (float input)
{
    float output = input;
    for (int i = 0; i < numActiveSections; ++i)
        output = sections[i].processSample (output);
    return output;
}

void CascadedFilter::process (float* buffer, int numSamples)
{
    for (int s = 0; s < numActiveSections; ++s)
        sections[s].process (buffer, numSamples);
}

std::complex<double> CascadedFilter::getResponse (double freq) const
{
    std::complex<double> composite (1.0, 0.0);
    for (int i = 0; i < numActiveSections; ++i)
        composite *= sections[i].getResponse (freq);
    return composite;
}

double CascadedFilter::getMagnitudeAtFrequency (double freq) const
{
    auto response = getResponse (freq);
    return 20.0 * std::log10 (std::max (std::abs (response), 1e-10));
}

void CascadedFilter::setParametersFractional (FilterType type, double freq, double Q,
                                                double gainDB, double slopeDBPerOct)
{
    // Standard slope values
    static const int standardSlopes[] = { 6, 12, 18, 24, 36, 48, 72, 96 };
    static const int numStandard = 8;

    // Only HP/LP/Shelf types support fractional slopes
    if (type != FilterType::HighPass && type != FilterType::LowPass
        && type != FilterType::LowShelf && type != FilterType::HighShelf)
    {
        fractionalMode = false;
        setParameters (type, freq, Q, gainDB, 12);
        return;
    }

    // Clamp to range
    double slope = std::max (6.0, std::min (slopeDBPerOct, 96.0));

    // Find bracketing standard slopes
    int lowerSlope = 6;
    int upperSlope = 12;

    for (int i = 0; i < numStandard - 1; ++i)
    {
        if (slope >= standardSlopes[i] && slope <= standardSlopes[i + 1])
        {
            lowerSlope = standardSlopes[i];
            upperSlope = standardSlopes[i + 1];
            break;
        }
    }

    // Snap to nearest standard if close (within 0.5 dB/oct)
    double distToLower = std::abs (slope - lowerSlope);
    double distToUpper = std::abs (slope - upperSlope);

    if (distToLower <= 0.5)
    {
        fractionalMode = false;
        setParameters (type, freq, Q, gainDB, lowerSlope);
        return;
    }

    if (distToUpper <= 0.5)
    {
        fractionalMode = false;
        setParameters (type, freq, Q, gainDB, upperSlope);
        return;
    }

    // Fractional mode: crossfade between lower and upper
    fractionalMode = true;
    fractionalMix = (float) ((slope - lowerSlope) / (double) (upperSlope - lowerSlope));

    // Configure primary cascade (lower slope)
    setParameters (type, freq, Q, gainDB, lowerSlope);

    // Configure secondary cascade (upper slope)
    auto slopeInfoB = getSlopeInfo (upperSlope);
    numActiveSectionsB = slopeInfoB.numSections;

    if (type == FilterType::HighPass || type == FilterType::LowPass)
    {
        int orderB = upperSlope / 6;
        int numSecondOrderB = slopeInfoB.lastIsFirstOrder
                                  ? numActiveSectionsB - 1
                                  : numActiveSectionsB;

        for (int i = 0; i < numSecondOrderB; ++i)
        {
            double angle = kPi * (2.0 * i + 1.0) / (2.0 * orderB);
            double butterworthQ = 1.0 / (2.0 * std::cos (angle));
            sectionsB[i].setFirstOrder (false);
            sectionsB[i].setParameters (type, freq, butterworthQ, 0.0);
        }

        if (slopeInfoB.lastIsFirstOrder)
        {
            int lastIdx = numActiveSectionsB - 1;
            sectionsB[lastIdx].setFirstOrder (true);
            sectionsB[lastIdx].setParameters (type, freq, 0.707, 0.0);
        }
    }
    else // Shelves
    {
        double perSectionGain = gainDB / numActiveSectionsB;
        for (int i = 0; i < numActiveSectionsB; ++i)
        {
            bool isLast = (i == numActiveSectionsB - 1);
            bool useFirstOrder = isLast && slopeInfoB.lastIsFirstOrder;
            sectionsB[i].setFirstOrder (useFirstOrder);
            sectionsB[i].setParameters (type, freq, Q, perSectionGain);
        }
    }
}

void CascadedFilter::processFractional (float* buffer, int numSamples)
{
    if (! fractionalMode)
    {
        process (buffer, numSamples);
        return;
    }

    // Process through both cascades and crossfade
    // Use stack buffer for small blocks, heap for large
    constexpr int kStackLimit = 2048;
    float stackBuf[kStackLimit];
    std::unique_ptr<float[]> heapBuf;
    float* bufferB;
    if (numSamples <= kStackLimit)
    {
        bufferB = stackBuf;
    }
    else
    {
        heapBuf.reset (new float[(size_t) numSamples]);
        bufferB = heapBuf.get();
    }
    std::memcpy (bufferB, buffer, sizeof (float) * (size_t) numSamples);

    // Process primary (lower slope)
    for (int s = 0; s < numActiveSections; ++s)
        sections[s].process (buffer, numSamples);

    // Process secondary (upper slope)
    for (int s = 0; s < numActiveSectionsB; ++s)
        sectionsB[s].process (bufferB, numSamples);

    // Crossfade
    float mixB = fractionalMix;
    float mixA = 1.0f - mixB;
    for (int i = 0; i < numSamples; ++i)
        buffer[i] = buffer[i] * mixA + bufferB[i] * mixB;
}

float CascadedFilter::processSampleFractional (float input)
{
    if (! fractionalMode)
        return processSample (input);

    // Process through both cascades
    float outputA = input;
    for (int i = 0; i < numActiveSections; ++i)
        outputA = sections[i].processSample (outputA);

    float outputB = input;
    for (int i = 0; i < numActiveSectionsB; ++i)
        outputB = sectionsB[i].processSample (outputB);

    return outputA * (1.0f - fractionalMix) + outputB * fractionalMix;
}
