#include "EqBand.h"

void EqBand::prepare (double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    filterL.prepare (sr);
    filterR.prepare (sr);

    smoothedFreq.reset (sr, 0.02);  // 20ms smoothing
    smoothedGain.reset (sr, 0.02);
    smoothedQ.reset (sr, 0.02);
}

void EqBand::reset()
{
    filterL.reset();
    filterR.reset();
}

void EqBand::updateParameters (const BandParameters& params)
{
    currentParams = params;
    active = params.active;
    filterType = params.type;
    stereoPlacement = params.stereoPlacement;

    if (! active)
        return;

    smoothedFreq.setTargetValue (params.frequency);
    smoothedGain.setTargetValue (params.gain);
    smoothedQ.setTargetValue (params.q);

    // Update filters with current (possibly smoothed) values
    float freq = smoothedFreq.getCurrentValue();
    float gain = smoothedGain.getCurrentValue();
    float q = smoothedQ.getCurrentValue();

    filterL.setParameters (params.type, freq, q, gain, params.slopeDBPerOct);
    filterR.setParameters (params.type, freq, q, gain, params.slopeDBPerOct);

    needsUpdate = false;
}

void EqBand::process (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (! active)
        return;

    // Check if parameters are still smoothing
    if (smoothedFreq.isSmoothing() || smoothedGain.isSmoothing() || smoothedQ.isSmoothing())
    {
        // Process sample by sample for smooth transitions
        for (int i = 0; i < numSamples; ++i)
        {
            float freq = smoothedFreq.getNextValue();
            float gain = smoothedGain.getNextValue();
            float q = smoothedQ.getNextValue();

            filterL.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);
            filterR.setParameters (filterType, freq, q, gain, currentParams.slopeDBPerOct);

            if (buffer.getNumChannels() > 0)
            {
                float* dataL = buffer.getWritePointer (0);
                dataL[i] = filterL.processSample (dataL[i]);
            }
            if (buffer.getNumChannels() > 1)
            {
                float* dataR = buffer.getWritePointer (1);
                dataR[i] = filterR.processSample (dataR[i]);
            }
        }
    }
    else
    {
        // Block processing (more efficient when parameters are stable)
        if (buffer.getNumChannels() > 0)
            filterL.process (buffer.getWritePointer (0), numSamples);
        if (buffer.getNumChannels() > 1)
            filterR.process (buffer.getWritePointer (1), numSamples);
    }
}

void EqBand::computeMagnitudeResponse (const double* frequencies, double* magnitudes,
                                        int numPoints) const
{
    if (! active)
    {
        for (int i = 0; i < numPoints; ++i)
            magnitudes[i] = 0.0; // 0 dB = unity gain
        return;
    }

    for (int i = 0; i < numPoints; ++i)
    {
        auto response = filterL.getResponse (frequencies[i]);
        double mag = std::abs (response);
        magnitudes[i] = 20.0 * std::log10 (std::max (mag, 1e-10));
    }
}
