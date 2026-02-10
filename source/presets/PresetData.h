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
 * @file PresetData.h
 * @brief Data structures for preset storage and serialization.
 */

#include <juce_core/juce_core.h>
#include "../dsp/FilterTypes.h"
#include <vector>

/**
 * @brief Per-band preset data describing one EQ band's configuration.
 *
 * Stores the filter type, frequency, gain, Q, slope, and stereo placement
 * for a single band within a preset.
 *
 * Slope index mapping: 0 = 6 dB/oct, 1 = 12 dB/oct, 2 = 18 dB/oct,
 * 3 = 24 dB/oct, 4 = 36 dB/oct, 5 = 48 dB/oct, 6 = 72 dB/oct,
 * 7 = 96 dB/oct, 8 = Brickwall.
 *
 * Stereo index mapping: 0 = Stereo, 1 = Left, 2 = Right, 3 = Mid, 4 = Side.
 */
struct BandPreset
{
    int         bandIndex;
    FilterType  type       = FilterType::Bell;
    float       frequency  = 1000.0f;
    float       gain       = 0.0f;
    float       q          = 1.0f;
    int         slopeIndex = 1;   // 0=6dB, 1=12dB, 2=18dB, 3=24dB, 4=36dB, 5=48dB, 6=72dB, 7=96dB, 8=Brickwall
    int         stereoIndex = 0;  // 0=Stereo, 1=Left, 2=Right, 3=Mid, 4=Side
};

/**
 * @brief Complete preset containing all band configurations and global settings.
 *
 * Holds the preset name, category, a vector of BandPreset entries for each
 * active band, the output gain, a factory/user flag, and (for user presets)
 * the source file on disk.
 */
struct Preset
{
    juce::String                name;
    juce::String                category;
    std::vector<BandPreset>     bands;
    float                       outputGain = 0.0f;
    bool                        isFactory  = true;
    juce::File                  sourceFile;  // only set for user presets
};
