/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_graphics/juce_graphics.h>
#include "BinaryData.h"

/**
 * @brief Centralized font management for embedded typefaces.
 *
 * Loads Space Grotesk (body) and Audiowide (title) from binary data
 * and provides convenience accessors used throughout the UI.
 */
class Fonts
{
public:
    static juce::Typeface::Ptr getSpaceGroteskTypeface()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::SpaceGroteskRegular_ttf,
            BinaryData::SpaceGroteskRegular_ttfSize);
        return typeface;
    }

    static juce::Typeface::Ptr getAudiowideTypeface()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::AudiowideRegular_ttf,
            BinaryData::AudiowideRegular_ttfSize);
        return typeface;
    }

    /** Body font (Space Grotesk) at the given size. */
    static juce::Font getBody (float height)
    {
        return juce::Font (getSpaceGroteskTypeface()).withHeight (height);
    }

    /** Bold body font (Space Grotesk, synthetic bold). */
    static juce::Font getBold (float height)
    {
        return juce::Font (getSpaceGroteskTypeface()).withHeight (height).boldened();
    }

    /** Title/branding font (Audiowide) at the given size. */
    static juce::Font getTitle (float height)
    {
        return juce::Font (getAudiowideTypeface()).withHeight (height);
    }
};
