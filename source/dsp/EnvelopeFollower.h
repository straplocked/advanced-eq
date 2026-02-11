/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <cmath>
#include <algorithm>

class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        updateCoefficients();
    }

    void reset()
    {
        envelope = 0.0f;
    }

    void setAttack (float attackMs)
    {
        attackTime = attackMs;
        updateCoefficients();
    }

    void setRelease (float releaseMs)
    {
        releaseTime = releaseMs;
        updateCoefficients();
    }

    void setBandFrequency (float freq)
    {
        // Frequency-adaptive time constants: scale inversely with band frequency
        // to prevent modulation artifacts at low frequencies
        bandFreq = std::max (freq, 20.0f);
        updateCoefficients();
    }

    float processSample (float input)
    {
        float rectified = std::abs (input);

        if (rectified > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * rectified;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * rectified;

        return envelope;
    }

    float getCurrentLevel() const { return envelope; }

private:
    void updateCoefficients()
    {
        if (sr <= 0.0) return;

        // Frequency-adaptive scaling: slower at low frequencies
        float freqScale = std::max (0.5f, std::min (2.0f, 1000.0f / bandFreq));

        float effectiveAttack = attackTime * freqScale;
        float effectiveRelease = releaseTime * freqScale;

        attackCoeff  = std::exp (-1.0f / (float) (sr * effectiveAttack * 0.001f));
        releaseCoeff = std::exp (-1.0f / (float) (sr * effectiveRelease * 0.001f));
    }

    double sr = 44100.0;
    float attackTime = 10.0f;    // ms
    float releaseTime = 100.0f;  // ms
    float bandFreq = 1000.0f;    // Hz
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
};
