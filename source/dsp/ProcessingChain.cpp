#include "ProcessingChain.h"
#include "MidSideProcessor.h"
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
    bool isStereo = buffer.getNumChannels() >= 2;

    // --- Pass 1: Stereo bands (process both L and R) ---
    for (auto& band : bands)
    {
        if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Stereo)
            band.process (buffer, numSamples);
    }

    if (isStereo)
    {
        float* dataL = buffer.getWritePointer (0);
        float* dataR = buffer.getWritePointer (1);

        // --- Pass 2: Left-only bands ---
        for (auto& band : bands)
        {
            if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Left)
                band.processSingleChannel (dataL, numSamples);
        }

        // --- Pass 3: Right-only bands ---
        for (auto& band : bands)
        {
            if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Right)
                band.processSingleChannel (dataR, numSamples);
        }

        // --- Pass 4: Mid/Side bands ---
        bool hasMid = false, hasSide = false;
        for (const auto& band : bands)
        {
            if (! band.isActive()) continue;
            if (band.getStereoPlacement() == StereoPlacement::Mid)  hasMid = true;
            if (band.getStereoPlacement() == StereoPlacement::Side) hasSide = true;
        }

        if (hasMid || hasSide)
        {
            MidSideProcessor::encode (dataL, dataR, numSamples);

            for (auto& band : bands)
            {
                if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Mid)
                    band.processSingleChannel (dataL, numSamples);
            }

            for (auto& band : bands)
            {
                if (band.isActive() && band.getStereoPlacement() == StereoPlacement::Side)
                    band.processSingleChannel (dataR, numSamples);
            }

            MidSideProcessor::decode (dataL, dataR, numSamples);
        }
    }

    // Analog mode: crosstalk + noise floor
    if (analogMode && isStereo)
    {
        float* dataL = buffer.getWritePointer (0);
        float* dataR = buffer.getWritePointer (1);

        constexpr float crosstalk = 0.002f; // ~-54 dB channel bleed
        constexpr float noiseLevel = 1e-5f; // ~-100 dBFS

        for (int i = 0; i < numSamples; ++i)
        {
            // Channel crosstalk
            float l = dataL[i];
            float r = dataR[i];
            dataL[i] = l + crosstalk * r;
            dataR[i] = r + crosstalk * l;

            // Pink-ish noise floor (one-pole filtered white noise per channel)
            float whiteL = noiseRng.nextFloat() * 2.0f - 1.0f;
            float whiteR = noiseRng.nextFloat() * 2.0f - 1.0f;
            pinkState[0] = 0.99f * pinkState[0] + 0.01f * whiteL;
            pinkState[1] = 0.99f * pinkState[1] + 0.01f * whiteR;
            dataL[i] += pinkState[0] * noiseLevel;
            dataR[i] += pinkState[1] * noiseLevel;
        }
    }
    else if (analogMode && ! isStereo)
    {
        float* data = buffer.getWritePointer (0);
        constexpr float noiseLevel = 1e-5f;

        for (int i = 0; i < numSamples; ++i)
        {
            float white = noiseRng.nextFloat() * 2.0f - 1.0f;
            pinkState[0] = 0.99f * pinkState[0] + 0.01f * white;
            data[i] += pinkState[0] * noiseLevel;
        }
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

void ProcessingChain::setAnalogMode (bool enabled)
{
    analogMode = enabled;
    for (auto& band : bands)
        band.setAnalogMode (enabled);
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
