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
 * @file Constants.h
 * @brief Global compile-time constants for the Advanced EQ plugin.
 */

/**
 * @namespace Constants
 * @brief Compile-time constants for band configuration, frequency/gain/Q ranges,
 *        dynamic EQ ranges, editor dimensions, FFT configuration, UI timer rate,
 *        and curve resolution.
 */
namespace Constants
{
    /** @brief Maximum number of simultaneous EQ bands. */
    constexpr int kMaxBands = 24;

    // Frequency range
    /** @brief Minimum filter frequency in Hz. */
    constexpr float kMinFrequency = 10.0f;      // Hz
    /** @brief Maximum filter frequency in Hz. */
    constexpr float kMaxFrequency = 30000.0f;    // Hz
    /** @brief Default filter frequency in Hz. */
    constexpr float kDefaultFrequency = 1000.0f; // Hz

    // Gain range
    /** @brief Minimum gain in dB. */
    constexpr float kMinGain = -30.0f;   // dB
    /** @brief Maximum gain in dB. */
    constexpr float kMaxGain = 30.0f;    // dB
    /** @brief Default gain in dB (unity). */
    constexpr float kDefaultGain = 0.0f; // dB

    // Q range
    /** @brief Minimum Q factor (bandwidth). */
    constexpr float kMinQ = 0.1f;
    /** @brief Maximum Q factor (bandwidth). */
    constexpr float kMaxQ = 30.0f;
    /** @brief Default Q factor. */
    constexpr float kDefaultQ = 1.0f;

    // Dynamic EQ
    /** @brief Minimum dynamic EQ threshold in dB. */
    constexpr float kMinThreshold = -60.0f;  // dB
    /** @brief Maximum dynamic EQ threshold in dB. */
    constexpr float kMaxThreshold = 0.0f;    // dB
    /** @brief Minimum dynamic EQ range in dB. */
    constexpr float kMinRange = -30.0f;      // dB
    /** @brief Maximum dynamic EQ range in dB. */
    constexpr float kMaxRange = 30.0f;       // dB
    /** @brief Minimum attack time in milliseconds. */
    constexpr float kMinAttack = 0.1f;       // ms
    /** @brief Maximum attack time in milliseconds. */
    constexpr float kMaxAttack = 500.0f;     // ms
    /** @brief Minimum release time in milliseconds. */
    constexpr float kMinRelease = 1.0f;      // ms
    /** @brief Maximum release time in milliseconds. */
    constexpr float kMaxRelease = 5000.0f;   // ms

    // Editor
    /** @brief Default editor window width in pixels. */
    constexpr int kDefaultEditorWidth = 1100;
    /** @brief Default editor window height in pixels. */
    constexpr int kDefaultEditorHeight = 660;

    // Analyzer
    /** @brief FFT order (2^12 = 4096 bins). */
    constexpr int kFFTOrder = 12;                    // 2^12 = 4096
    /** @brief FFT size derived from kFFTOrder. */
    constexpr int kFFTSize = 1 << kFFTOrder;         // 4096
    /** @brief Maximum supported FFT size. */
    constexpr int kMaxFFTSize = 8192;
    /** @brief Minimum dB value displayed in the spectrum analyzer. */
    constexpr float kAnalyzerMinDB = -100.0f;
    /** @brief Maximum dB value displayed in the spectrum analyzer. */
    constexpr float kAnalyzerMaxDB = 6.0f;
    /** @brief Default spectrum analyzer smoothing factor (0.0 = none, 1.0 = full). */
    constexpr float kDefaultAnalyzerSmoothing = 0.7f;

    // UI update rate
    /** @brief UI timer frequency in Hz (repaints per second). */
    constexpr int kUITimerHz = 30;

    // Curve resolution (number of points for magnitude response)
    /** @brief Number of points used to render magnitude response curves. */
    constexpr int kCurveResolution = 512;
}
