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
 * @file SpectrumAnalyzer.h
 * @brief UI-thread spectrum visualization component.
 */

#include <juce_graphics/juce_graphics.h>
#include "FFTProcessor.h"
#include "../utils/Constants.h"
#include "../utils/MathUtils.h"
#include <vector>

/**
 * @brief Reads FFT magnitude data and builds a renderable spectrum path on the UI thread.
 *
 * Reads completed FFT magnitude data from the TripleBuffer (via FFTProcessor),
 * resamples the linear-frequency FFT bins to logarithmically-spaced display
 * points, applies asymmetric smoothing for natural decay behavior, and
 * constructs a juce::Path suitable for painting in a Component's paint() method.
 *
 * @see FFTProcessor, TripleBuffer
 */
// Runs on the UI thread. Reads FFT magnitude data from the TripleBuffer,
// resamples from linear-frequency FFT bins to log-frequency display points,
// applies smoothing, and builds a juce::Path for painting.
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    /**
     * @brief Allocates internal buffers for the given number of display points.
     * @param numDisplayPoints Number of horizontal pixels (or log-frequency points) to resample to.
     */
    void prepare (int numDisplayPoints);

    /**
     * @brief Updates the spectrum path from the latest FFT data.
     *
     * Should be called from a timerCallback(). Checks the FFTProcessor's
     * TripleBuffer for new data, resamples to log-frequency spacing,
     * applies smoothing, and rebuilds the internal juce::Path.
     *
     * @param fftProcessor The FFTProcessor whose magnitude buffer will be read.
     * @param displayWidth Width of the display area in pixels.
     * @param displayHeight Height of the display area in pixels.
     * @return True if the path was rebuilt (new data was available); false otherwise.
     */
    // Call from timerCallback. Returns true if path was rebuilt.
    bool update (FFTProcessor& fftProcessor, float displayWidth, float displayHeight);

    /**
     * @brief Returns the spectrum outline path for stroke rendering.
     * @return Const reference to the spectrum outline path.
     */
    const juce::Path& getPath() const { return spectrumPath; }

    /**
     * @brief Returns the spectrum fill path (closed to the bottom edge) for filled rendering.
     * @return Const reference to the spectrum fill path.
     */
    const juce::Path& getFillPath() const { return spectrumFillPath; }

    /**
     * @brief Sets the smoothing coefficient that controls spectrum decay rate.
     *
     * Higher values produce slower decay (more smoothing). The value is
     * clamped to [0.0, 0.99].
     *
     * @param s Smoothing coefficient in the range [0.0, 0.99].
     */
    void setSmoothing (float s) { smoothingCoeff = juce::jlimit (0.0f, 0.99f, s); }

private:
    // Map spectrum dB to Y pixel coordinate.
    // 0 dBFS aligns with the EQ curve's 0 dB gain line (center of display).
    float spectrumDBtoY (float dB, float height) const;

    float smoothingCoeff = Constants::kDefaultAnalyzerSmoothing;

    int numPoints = 0;
    std::vector<float> smoothedMagnitudes;
    std::vector<float> displayFrequencies;

    juce::Path spectrumPath;
    juce::Path spectrumFillPath;
};
