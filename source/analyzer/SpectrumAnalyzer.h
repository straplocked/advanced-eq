#pragma once

#include <juce_graphics/juce_graphics.h>
#include "FFTProcessor.h"
#include "../utils/Constants.h"
#include "../utils/MathUtils.h"
#include <vector>

// Runs on the UI thread. Reads FFT magnitude data from the TripleBuffer,
// resamples from linear-frequency FFT bins to log-frequency display points,
// applies smoothing, and builds a juce::Path for painting.
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    void prepare (int numDisplayPoints);

    // Call from timerCallback. Returns true if path was rebuilt.
    bool update (FFTProcessor& fftProcessor, float displayWidth, float displayHeight);

    const juce::Path& getPath() const { return spectrumPath; }
    const juce::Path& getFillPath() const { return spectrumFillPath; }

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
