#pragma once

namespace Constants
{
    // Band configuration
    constexpr int kMaxBands = 24;

    // Frequency range
    constexpr float kMinFrequency = 10.0f;      // Hz
    constexpr float kMaxFrequency = 30000.0f;    // Hz
    constexpr float kDefaultFrequency = 1000.0f; // Hz

    // Gain range
    constexpr float kMinGain = -30.0f;   // dB
    constexpr float kMaxGain = 30.0f;    // dB
    constexpr float kDefaultGain = 0.0f; // dB

    // Q range
    constexpr float kMinQ = 0.1f;
    constexpr float kMaxQ = 30.0f;
    constexpr float kDefaultQ = 1.0f;

    // Dynamic EQ
    constexpr float kMinThreshold = -60.0f;  // dB
    constexpr float kMaxThreshold = 0.0f;    // dB
    constexpr float kMinRange = -30.0f;      // dB
    constexpr float kMaxRange = 30.0f;       // dB
    constexpr float kMinAttack = 0.1f;       // ms
    constexpr float kMaxAttack = 500.0f;     // ms
    constexpr float kMinRelease = 1.0f;      // ms
    constexpr float kMaxRelease = 5000.0f;   // ms

    // Editor
    constexpr int kDefaultEditorWidth = 1000;
    constexpr int kDefaultEditorHeight = 600;

    // Analyzer
    constexpr int kFFTOrder = 12;                    // 2^12 = 4096
    constexpr int kFFTSize = 1 << kFFTOrder;         // 4096
    constexpr int kMaxFFTSize = 8192;
    constexpr float kAnalyzerMinDB = -100.0f;
    constexpr float kAnalyzerMaxDB = 6.0f;
    constexpr float kDefaultAnalyzerSmoothing = 0.7f;

    // UI update rate
    constexpr int kUITimerHz = 30;

    // Curve resolution (number of points for magnitude response)
    constexpr int kCurveResolution = 512;
}
