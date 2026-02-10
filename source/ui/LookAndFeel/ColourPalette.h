#pragma once

#include <juce_graphics/juce_graphics.h>

namespace ColourPalette
{
    // Background
    inline juce::Colour background()       { return juce::Colour (0xff121228); }
    inline juce::Colour curveArea()         { return juce::Colour (0xff1a1a36); }
    inline juce::Colour gridLine()          { return juce::Colour (0xff2a2a4a); }
    inline juce::Colour gridLineStrong()    { return juce::Colour (0xff3a3a5a); }
    inline juce::Colour axisText()          { return juce::Colour (0xff888899); }

    // Curve
    inline juce::Colour compositeCurve()    { return juce::Colour (0xffffd93d); }
    inline juce::Colour compositeFill()     { return juce::Colour (0x18ffd93d); }
    inline juce::Colour curveFillAbove()    { return juce::Colour (0x20ffffff); }
    inline juce::Colour curveFillBelow()    { return juce::Colour (0x20ffffff); }

    // Per-band colours (24 bands cycle through these)
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

    // Stereo placement colors
    inline juce::Colour stereoColour()    { return juce::Colour (0xffffd93d); } // Yellow
    inline juce::Colour leftColour()      { return juce::Colour (0xffffffff); } // White
    inline juce::Colour rightColour()     { return juce::Colour (0xffff6b6b); } // Red
    inline juce::Colour midColour()       { return juce::Colour (0xff6bff6b); } // Green
    inline juce::Colour sideColour()      { return juce::Colour (0xff6bcfff); } // Blue

    // UI elements
    inline juce::Colour controlBar()       { return juce::Colour (0xff0e0e1e); }
    inline juce::Colour presetBar()        { return juce::Colour (0xff161630); }
    inline juce::Colour knobBackground()   { return juce::Colour (0xff2a2a4a); }
    inline juce::Colour knobForeground()   { return juce::Colour (0xffccccdd); }
    inline juce::Colour textBright()       { return juce::Colour (0xffeeeeee); }
    inline juce::Colour textDim()          { return juce::Colour (0xff777788); }

    // Panel surfaces (gradient top/bottom)
    inline juce::Colour panelTop()         { return juce::Colour (0xff1e1e3a); }
    inline juce::Colour panelBottom()      { return juce::Colour (0xff0a0a1a); }

    // Bevels & edges
    inline juce::Colour bevelLight()       { return juce::Colour (0x18ffffff); }
    inline juce::Colour bevelDark()        { return juce::Colour (0x30000000); }

    // Screen recess (inner shadow)
    inline juce::Colour screenShadow()     { return juce::Colour (0x50000000); }
    inline juce::Colour screenHighlight()  { return juce::Colour (0x08ffffff); }

    // Knob metallic
    inline juce::Colour knobEdge()         { return juce::Colour (0xff3a3a5c); }
    inline juce::Colour knobShadow()       { return juce::Colour (0x400a0a1a); }
    inline juce::Colour knobIndicator()    { return juce::Colour (0xffffd93d); }

    // Spectrum
    inline juce::Colour spectrumPre()      { return juce::Colour (0x40888899); }
    inline juce::Colour spectrumPost()     { return juce::Colour (0x606bcfff); }
}
