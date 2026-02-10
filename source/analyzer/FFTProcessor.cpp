#include "FFTProcessor.h"
#include <cmath>
#include <algorithm>

FFTProcessor::FFTProcessor() = default;

void FFTProcessor::prepare (double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    ringBuffer.assign ((size_t) fftSize, 0.0f);
    ringWritePos = 0;

    fftData.assign ((size_t) fftSize * 2, 0.0f);

    magnitudeBuffer.resize ((size_t) getNumBins());
}

void FFTProcessor::reset()
{
    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    ringWritePos = 0;
}

void FFTProcessor::pushSamples (const juce::AudioBuffer<float>& buffer,
                                 int numSamples)
{
    int numChannels = buffer.getNumChannels();
    if (numChannels == 0)
        return;

    float invChannels = 1.0f / (float) numChannels;

    for (int i = 0; i < numSamples; ++i)
    {
        // Average all channels to mono
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getReadPointer (ch)[i];
        sample *= invChannels;

        ringBuffer[(size_t) ringWritePos] = sample;
        ++ringWritePos;

        if (ringWritePos >= fftSize)
        {
            performFFT();

            // Shift second half to first half for 50% overlap
            std::copy (ringBuffer.begin() + hopSize,
                       ringBuffer.begin() + fftSize,
                       ringBuffer.begin());
            ringWritePos = hopSize;
        }
    }
}

void FFTProcessor::performFFT()
{
    // Copy ring buffer into FFT work buffer
    std::copy (ringBuffer.begin(), ringBuffer.begin() + fftSize,
               fftData.begin());

    // Zero the second half (required by JUCE FFT)
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    // Apply Hann window
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);

    // Forward FFT → magnitude values directly
    // With onlyCalculateNonNegativeFrequencies = true, returns
    // at least fftSize/2 + 1 magnitude values.
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    // Convert magnitudes to dB and write to triple buffer
    int numBins = getNumBins();
    float* dest = magnitudeBuffer.getWritePointer();
    float fftSizeF = (float) fftSize;

    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = fftData[(size_t) bin];

        // Normalize: divide by fftSize, x2 for single-sided (except DC and Nyquist)
        mag /= fftSizeF;
        if (bin > 0 && bin < numBins - 1)
            mag *= 2.0f;

        // Convert to dB, clamp to analyzer floor
        float dB = 20.0f * std::log10 (std::max (mag, 1e-10f));
        dest[bin] = std::max (dB, Constants::kAnalyzerMinDB);
    }

    magnitudeBuffer.finishWrite();
}
