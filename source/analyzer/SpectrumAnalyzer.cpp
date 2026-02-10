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
    float binWidth = (float) (sampleRate / (double) Constants::kFFTSize);

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

float SpectrumAnalyzer::spectrumDBtoY (float dB, float height) const
{
    // Same pixels-per-dB as the EQ display (60 dB range over full height).
    // 0 dBFS aligns with the EQ curve's 0 dB gain line (center of display).
    return height * 0.5f - dB * (height / 60.0f);
}
