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
 * @file ParameterLayout.h
 * @brief APVTS parameter layout factory for the entire plugin.
 */

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * @brief Namespace providing the factory function that builds the complete APVTS parameter layout.
 *
 * The layout comprises 298 parameters total: 24 bands x 12 per-band parameters
 * plus 10 global parameters (output gain, pan, processing mode, oversampling,
 * auto-gain, analyzer on/pre/resolution/speed, and analog mode).
 */
namespace ParameterLayout
{
    /**
     * @brief Creates the complete parameter layout for AudioProcessorValueTreeState construction.
     *
     * Builds all 298 parameters (24 bands x 12 per-band + 10 global) with
     * appropriate ranges, defaults, and string-from-value functions.
     *
     * @return Complete parameter layout ready to be passed to the APVTS constructor.
     */
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
