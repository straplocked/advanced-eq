#pragma once

#include <juce_dsp/juce_dsp.h>
#include "TripleBuffer.h"
#include "../utils/Constants.h"
#include <vector>

// Accumulates audio samples into a ring buffer and performs windowed FFT
// when enough samples are collected. Writes magnitude spectrum (in dB)
// to a lock-free TripleBuffer for the UI thread to read.
// Called entirely from the audio thread.
class FFTProcessor
{
public:
    FFTProcessor();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Push samples from a stereo (or mono) buffer. Averages channels to mono.
    void pushSamples (const juce::AudioBuffer<float>& buffer, int numSamples);

    TripleBuffer<float>& getMagnitudeBuffer() { return magnitudeBuffer; }
    const TripleBuffer<float>& getMagnitudeBuffer() const { return magnitudeBuffer; }

    int getNumBins() const { return fftSize / 2 + 1; }
    double getSampleRate() const { return currentSampleRate; }

private:
    void performFFT();

    static constexpr int fftOrder = Constants::kFFTOrder;  // 12
    static constexpr int fftSize  = Constants::kFFTSize;   // 4096
    static constexpr int hopSize  = fftSize / 2;           // 2048 (50% overlap)

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        (size_t) fftSize,
        juce::dsp::WindowingFunction<float>::hann
    };

    std::vector<float> ringBuffer;
    int ringWritePos = 0;

    // Work buffer for FFT (must be 2 * fftSize)
    std::vector<float> fftData;

    // Output magnitudes in dB per bin
    TripleBuffer<float> magnitudeBuffer;

    double currentSampleRate = 44100.0;
};
