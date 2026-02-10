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

static constexpr double kPi = 3.14159265358979323846;

void CascadedFilter::prepare (double sr)
{
    sampleRate = sr;
    for (auto& section : sections)
        section.prepare (sr);
}

void CascadedFilter::reset()
{
    for (auto& section : sections)
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
