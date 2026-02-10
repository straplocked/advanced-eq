#include "ProcessingChain.h"
#include "../utils/MathUtils.h"

void ProcessingChain::prepare (double sr, int maxBlock, int /*numChannels*/)
{
    sampleRate = sr;
    maxBlockSize = maxBlock;

    for (auto& band : bands)
        band.prepare (sr, maxBlock);

    outputGain.reset (sr, 0.02); // 20ms smoothing
}

void ProcessingChain::reset()
{
    for (auto& band : bands)
        band.reset();
}

void ProcessingChain::process (juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();

    // Process each active band
    for (auto& band : bands)
    {
        if (band.isActive())
            band.process (buffer, numSamples);
    }

    // Apply output gain
    if (outputGain.isSmoothing())
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] *= outputGain.getNextValue();
        }
    }
    else
    {
        float gainValue = outputGain.getCurrentValue();
        if (std::abs (gainValue - 1.0f) > 1e-6f)
            buffer.applyGain (gainValue);
    }
}

void ProcessingChain::updateBand (int bandIndex, const BandParameters& params)
{
    if (bandIndex >= 0 && bandIndex < Constants::kMaxBands)
        bands[(size_t) bandIndex].updateParameters (params);
}

void ProcessingChain::setOutputGain (float gainDB)
{
    outputGain.setTargetValue (MathUtils::dbToGain (gainDB));
}

void ProcessingChain::computeCompositeMagnitude (const double* frequencies,
                                                  double* magnitudes,
                                                  int numPoints) const
{
    // Start at 0 dB (unity)
    for (int i = 0; i < numPoints; ++i)
        magnitudes[i] = 0.0;

    // Sum magnitudes from all active bands (in dB, since they cascade)
    std::vector<double> bandMag (numPoints);
    for (const auto& band : bands)
    {
        if (band.isActive())
        {
            band.computeMagnitudeResponse (frequencies, bandMag.data(), numPoints);
            for (int i = 0; i < numPoints; ++i)
                magnitudes[i] += bandMag[i];
        }
    }
}

void ProcessingChain::computeBandMagnitude (int bandIndex, const double* frequencies,
                                             double* magnitudes, int numPoints) const
{
    if (bandIndex >= 0 && bandIndex < Constants::kMaxBands)
        bands[(size_t) bandIndex].computeMagnitudeResponse (frequencies, magnitudes, numPoints);
}

EqBand* ProcessingChain::getBand (int index)
{
    if (index >= 0 && index < Constants::kMaxBands)
        return &bands[(size_t) index];
    return nullptr;
}
