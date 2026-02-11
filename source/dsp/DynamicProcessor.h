/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <cmath>
#include <algorithm>

class DynamicProcessor
{
public:
    DynamicProcessor() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        smoothedGain = 0.0f;
        // One-pole smoother: ~5ms transition time
        smoothCoeff = std::exp (-1.0f / (float) (sr * 0.005));
    }

    void reset()
    {
        smoothedGain = 0.0f;
    }

    void setThreshold (float threshDB) { threshold = threshDB; }
    void setRange (float rangeDB) { range = rangeDB; }

    /**
     * Compute the dynamic gain offset in dB from the envelope level.
     *
     * Bidirectional behavior:
     * - Positive range: expand (boost when signal exceeds threshold)
     * - Negative range: compress (cut when signal exceeds threshold)
     *
     * @param envelopeLevel  Linear envelope level from EnvelopeFollower.
     * @return Gain offset in dB to apply to the band's static gain.
     */
    float computeGainDB (float envelopeLevel)
    {
        if (std::abs (range) < 0.01f)
        {
            smoothedGain = smoothCoeff * smoothedGain;
            return smoothedGain;
        }

        // Convert envelope to dB
        float envDB = 20.0f * std::log10 (std::max (envelopeLevel, 1e-10f));

        // How far above threshold (positive = above)
        float excess = envDB - threshold;

        float targetGainDB = 0.0f;

        if (excess > 0.0f)
        {
            // Soft knee: gradually ramp up the dynamic action
            float ratio = std::min (excess / 20.0f, 1.0f); // Full action at 20 dB above threshold
            targetGainDB = range * ratio;
        }

        // Smooth gain transitions
        smoothedGain = smoothCoeff * smoothedGain + (1.0f - smoothCoeff) * targetGainDB;

        return smoothedGain;
    }

    float getCurrentGainReduction() const { return smoothedGain; }

private:
    double sr = 44100.0;
    float threshold = -20.0f;  // dBFS
    float range = 0.0f;       // dB (positive = expand, negative = compress)
    float smoothedGain = 0.0f;
    float smoothCoeff = 0.0f;
};
