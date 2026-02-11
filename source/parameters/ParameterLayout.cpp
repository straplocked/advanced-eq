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

#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "../utils/Constants.h"

namespace ParameterLayout
{

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- Per-band parameters (24 bands x 12 params = 288 parameters) ----
    for (int i = 0; i < Constants::kMaxBands; ++i)
    {
        auto bandName = "Band " + juce::String (i + 1);

        // Active (bool) - only first band active by default
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kActive), 1 },
            bandName + " Active",
            false));

        // Filter Type (choice: 0=Bell, 1=LowShelf, 2=HighShelf, ...)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kType), 1 },
            bandName + " Type",
            ParamIDs::getFilterTypeNames(),
            0)); // Default: Bell

        // Frequency (10 Hz - 30 kHz, logarithmic)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kFrequency), 1 },
            bandName + " Frequency",
            juce::NormalisableRange<float> (
                Constants::kMinFrequency, Constants::kMaxFrequency, 0.0f, 0.3f),
            Constants::kDefaultFrequency));

        // Gain (-30 to +30 dB)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kGain), 1 },
            bandName + " Gain",
            juce::NormalisableRange<float> (
                Constants::kMinGain, Constants::kMaxGain, 0.01f),
            Constants::kDefaultGain));

        // Q (0.1 - 30.0, logarithmic)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kQ), 1 },
            bandName + " Q",
            juce::NormalisableRange<float> (
                Constants::kMinQ, Constants::kMaxQ, 0.0f, 0.4f),
            Constants::kDefaultQ));

        // Slope (choice: 6/12/18/24/36/48/72/96/Brickwall)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kSlope), 1 },
            bandName + " Slope",
            ParamIDs::getSlopeNames(),
            1)); // Default: 12 dB/oct

        // Stereo Placement
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kStereo), 1 },
            bandName + " Stereo",
            ParamIDs::getStereoPlacementNames(),
            0)); // Default: Stereo

        // Dynamic EQ: Enabled
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kDynEnabled), 1 },
            bandName + " Dynamic",
            false));

        // Dynamic EQ: Threshold (-60 to 0 dB)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kDynThresh), 1 },
            bandName + " Threshold",
            juce::NormalisableRange<float> (
                Constants::kMinThreshold, Constants::kMaxThreshold, 0.1f),
            -20.0f));

        // Dynamic EQ: Range (-30 to +30 dB)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kDynRange), 1 },
            bandName + " Range",
            juce::NormalisableRange<float> (
                Constants::kMinRange, Constants::kMaxRange, 0.1f),
            0.0f));

        // Dynamic EQ: Attack (0.1 - 500 ms, logarithmic)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kDynAttack), 1 },
            bandName + " Attack",
            juce::NormalisableRange<float> (
                Constants::kMinAttack, Constants::kMaxAttack, 0.0f, 0.35f),
            10.0f));

        // Dynamic EQ: Release (1 - 5000 ms, logarithmic)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bandParam (i, ParamIDs::kDynRelease), 1 },
            bandName + " Release",
            juce::NormalisableRange<float> (
                Constants::kMinRelease, Constants::kMaxRelease, 0.0f, 0.35f),
            100.0f));
    }

    // ---- Global Parameters ----

    // Output Gain (-30 to +30 dB)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::kOutputGain, 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-30.0f, 30.0f, 0.01f),
        0.0f));

    // Output Pan (-1 to +1)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::kOutputPan, 1 },
        "Output Pan",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f),
        0.0f));

    // Processing Mode
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::kProcessMode, 1 },
        "Processing Mode",
        ParamIDs::getProcessingModeNames(),
        0)); // Default: Zero Latency

    // Oversampling
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::kOversample, 1 },
        "Oversampling",
        ParamIDs::getOversamplingNames(),
        0)); // Default: 1x

    // Auto Gain
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::kAutoGain, 1 },
        "Auto Gain",
        false));

    // Analyzer On
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::kAnalyzerOn, 1 },
        "Analyzer",
        true));

    // Analyzer Pre/Post
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::kAnalyzerPre, 1 },
        "Analyzer Pre",
        false));

    // Analyzer Speed (smoothing: 0.0 = no smoothing, 1.0 = max smoothing)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::kAnalyzerSpd, 1 },
        "Analyzer Speed",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        Constants::kDefaultAnalyzerSmoothing));

    // Analyzer Resolution (reserved for future FFT size switching)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::kAnalyzerRes, 1 },
        "Analyzer Resolution",
        juce::StringArray { "Low", "Normal", "High", "Max" },
        2)); // Default: High

    // Analog Mode
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::kAnalogMode, 1 },
        "Analog Mode",
        false));

    // Phase Invert
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::kPhaseInvert, 1 },
        "Phase Invert",
        false));

    return layout;
}

} // namespace ParameterLayout
