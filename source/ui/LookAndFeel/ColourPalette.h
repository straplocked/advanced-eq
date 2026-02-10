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
 * @file ColourPalette.h
 * @brief Centralized color definitions for the plugin UI.
 */

#include <juce_graphics/juce_graphics.h>

/**
 * @namespace ColourPalette
 * @brief Provides all UI colors as inline functions returning juce::Colour.
 *
 * Colors are organized into the following categories:
 * - **Background / grid:** background, curveArea, gridLine, gridLineStrong, axisText
 * - **Curve colors:** compositeCurve, compositeFill, curveFillAbove, curveFillBelow
 * - **Per-band colors:** bandColour() cycles through a 12-color palette (modulo bandIndex)
 * - **Stereo placement:** stereoColour, leftColour, rightColour, midColour, sideColour
 * - **UI elements:** controlBar, presetBar, knobBackground, knobForeground, textBright, textDim
 * - **Panel gradients:** panelTop, panelBottom
 * - **Bevels & edges:** bevelLight, bevelDark, screenShadow, screenHighlight
 * - **Knob metallic:** knobEdge, knobShadow, knobIndicator
 * - **Spectrum:** spectrumPre, spectrumPost
 * - **Analog accent:** analogAccent
 */
namespace ColourPalette
{
    //----------------------------------------------------------------------
    /** @name Background and Grid
     *  @{ */
    /** @brief Main window background color. */
    inline juce::Colour background()       { return juce::Colour (0xff121228); }
    /** @brief EQ curve area background color. */
    inline juce::Colour curveArea()         { return juce::Colour (0xff1a1a36); }
    /** @brief Standard grid line color. */
    inline juce::Colour gridLine()          { return juce::Colour (0xff2a2a4a); }
    /** @brief Emphasized grid line color for major divisions. */
    inline juce::Colour gridLineStrong()    { return juce::Colour (0xff3a3a5a); }
    /** @brief Axis label text color. */
    inline juce::Colour axisText()          { return juce::Colour (0xff888899); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Curve Colors
     *  @{ */
    /** @brief Composite EQ response curve color. */
    inline juce::Colour compositeCurve()    { return juce::Colour (0xffffd93d); }
    /** @brief Semi-transparent fill beneath the composite curve. */
    inline juce::Colour compositeFill()     { return juce::Colour (0x18ffd93d); }
    /** @brief Fill color for regions above the 0 dB line. */
    inline juce::Colour curveFillAbove()    { return juce::Colour (0x20ffffff); }
    /** @brief Fill color for regions below the 0 dB line. */
    inline juce::Colour curveFillBelow()    { return juce::Colour (0x20ffffff); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Per-Band Colors
     *  @{ */
    /** @brief Returns the color for a given band, cycling through 12 distinct colors.
     *  @param bandIndex The zero-based band index (modulo 12 is applied internally).
     *  @return The juce::Colour assigned to the band.
     */
    inline juce::Colour bandColour (int bandIndex)
    {
        static const juce::Colour colours[] = {
            juce::Colour (0xffff6b6b),  // Red
            juce::Colour (0xffffa06b),  // Orange
            juce::Colour (0xffffd93d),  // Yellow
            juce::Colour (0xff6bff6b),  // Green
            juce::Colour (0xff6bffd9),  // Teal
            juce::Colour (0xff6bcfff),  // Light Blue
            juce::Colour (0xff6b7fff),  // Blue
            juce::Colour (0xffa06bff),  // Purple
            juce::Colour (0xffff6bd9),  // Pink
            juce::Colour (0xffff9e6b),  // Peach
            juce::Colour (0xffc8ff6b),  // Lime
            juce::Colour (0xff6bffb0),  // Mint
        };
        return colours[bandIndex % 12];
    }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Stereo Placement Colors
     *  @{ */
    /** @brief Color for stereo (L+R) mode. */
    inline juce::Colour stereoColour()    { return juce::Colour (0xffffd93d); } // Yellow
    /** @brief Color for left-only channel mode. */
    inline juce::Colour leftColour()      { return juce::Colour (0xffffffff); } // White
    /** @brief Color for right-only channel mode. */
    inline juce::Colour rightColour()     { return juce::Colour (0xffff6b6b); } // Red
    /** @brief Color for mid channel mode. */
    inline juce::Colour midColour()       { return juce::Colour (0xff6bff6b); } // Green
    /** @brief Color for side channel mode. */
    inline juce::Colour sideColour()      { return juce::Colour (0xff6bcfff); } // Blue
    /** @} */

    //----------------------------------------------------------------------
    /** @name UI Element Colors
     *  @{ */
    /** @brief Bottom control bar background. */
    inline juce::Colour controlBar()       { return juce::Colour (0xff0e0e1e); }
    /** @brief Preset bar background. */
    inline juce::Colour presetBar()        { return juce::Colour (0xff161630); }
    /** @brief Knob body background color. */
    inline juce::Colour knobBackground()   { return juce::Colour (0xff2a2a4a); }
    /** @brief Knob foreground / fill color. */
    inline juce::Colour knobForeground()   { return juce::Colour (0xffccccdd); }
    /** @brief Bright text color for primary labels. */
    inline juce::Colour textBright()       { return juce::Colour (0xffeeeeee); }
    /** @brief Dim text color for secondary labels. */
    inline juce::Colour textDim()          { return juce::Colour (0xff777788); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Panel Surface Gradients
     *  @{ */
    /** @brief Top edge color for vertical panel gradients. */
    inline juce::Colour panelTop()         { return juce::Colour (0xff1e1e3a); }
    /** @brief Bottom edge color for vertical panel gradients. */
    inline juce::Colour panelBottom()      { return juce::Colour (0xff0a0a1a); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Bevels and Edges
     *  @{ */
    /** @brief Light bevel highlight color. */
    inline juce::Colour bevelLight()       { return juce::Colour (0x18ffffff); }
    /** @brief Dark bevel shadow color. */
    inline juce::Colour bevelDark()        { return juce::Colour (0x30000000); }
    /** @brief Inner shadow color for recessed screen areas. */
    inline juce::Colour screenShadow()     { return juce::Colour (0x50000000); }
    /** @brief Inner highlight color for recessed screen areas. */
    inline juce::Colour screenHighlight()  { return juce::Colour (0x08ffffff); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Knob Metallic Colors
     *  @{ */
    /** @brief Metallic knob edge highlight. */
    inline juce::Colour knobEdge()         { return juce::Colour (0xff3a3a5c); }
    /** @brief Knob drop shadow color. */
    inline juce::Colour knobShadow()       { return juce::Colour (0x400a0a1a); }
    /** @brief Knob value indicator / accent color. */
    inline juce::Colour knobIndicator()    { return juce::Colour (0xffffd93d); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Spectrum Analyzer Colors
     *  @{ */
    /** @brief Pre-EQ spectrum fill color. */
    inline juce::Colour spectrumPre()      { return juce::Colour (0x40888899); }
    /** @brief Post-EQ spectrum fill color. */
    inline juce::Colour spectrumPost()     { return juce::Colour (0x606bcfff); }
    /** @} */

    //----------------------------------------------------------------------
    /** @name Analog Mode
     *  @{ */
    /** @brief Warm accent color used when analog mode is enabled. */
    inline juce::Colour analogAccent()     { return juce::Colour (0xffcc8844); }
    /** @} */
}
