/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_core/juce_core.h>
#include <unordered_map>

namespace HelpText
{

inline juce::String getHelpText (const juce::String& componentID)
{
    static const std::unordered_map<std::string, const char*> database = {
        // HeaderToolbar
        { "header.prevPreset",   "Previous Preset \xe2\x80\x94 cycles backward through the preset list." },
        { "header.nextPreset",   "Next Preset \xe2\x80\x94 cycles forward through the preset list." },
        { "header.presetName",   "Current Preset \xe2\x80\x94 click to browse and select factory or user presets." },
        { "header.savePreset",   "Save Preset \xe2\x80\x94 saves the current EQ settings as a user preset." },
        { "header.undo",         "Undo \xe2\x80\x94 reverses the last parameter change (Cmd+Z)." },
        { "header.redo",         "Redo \xe2\x80\x94 re-applies the last undone change (Cmd+Shift+Z)." },
        { "header.abToggle",     "A/B Compare \xe2\x80\x94 toggles between two independent EQ states for quick comparison." },
        { "header.copyAB",       "Copy A to B (or B to A) \xe2\x80\x94 copies the active EQ state to the inactive slot." },
        { "header.phaseInvert",  "Phase Invert \xe2\x80\x94 flips output polarity (180\xc2\xb0 phase shift)." },
        { "header.analog",       "Analog Mode \xe2\x80\x94 enables saturation, proportional Q, channel crosstalk, and noise floor." },
        { "header.analyzer",     "Spectrum Analyzer \xe2\x80\x94 toggles real-time FFT spectrum display (pre/post EQ)." },
        { "header.freeze",       "Freeze Analyzer \xe2\x80\x94 pauses the spectrum at the current frame for inspection." },
        { "header.match",        "EQ Match \xe2\x80\x94 opens the panel to capture a reference spectrum and auto-fit bands." },
        { "header.processMode",  "Processing Mode \xe2\x80\x94 Zero Latency (IIR), Natural Phase (reduced pre-ringing), or Linear Phase (FIR, zero phase distortion)." },
        { "header.outputGain",   "Output Gain \xe2\x80\x94 adjusts overall output level after EQ processing (-30 to +30 dB)." },
        { "header.help",         "Help Mode \xe2\x80\x94 hover over any control to see what it does." },

        // EqCurveDisplay
        { "curve.display",       "EQ Curve Display \xe2\x80\x94 click to add a band, drag to adjust frequency/gain, scroll for Q, double-click to remove. Right-click for options." },

        // BottomControlStrip
        { "bottom.typeSelector",    "Filter Type \xe2\x80\x94 Bell, Low Shelf, High Shelf, High Pass, Low Pass, Notch, Band Pass, Tilt Shelf, or Flat Tilt." },
        { "bottom.freqKnob",        "Frequency \xe2\x80\x94 center/cutoff frequency of the selected band (10 Hz \xe2\x80\x93 30 kHz)." },
        { "bottom.gainKnob",        "Gain \xe2\x80\x94 boost or cut amount (-30 to +30 dB)." },
        { "bottom.qKnob",           "Q (Bandwidth) \xe2\x80\x94 higher Q = narrower band. Range: 0.1 to 30." },
        { "bottom.slopeSelector",   "Filter Slope \xe2\x80\x94 rolloff steepness for pass/shelf filters (6 to 96 dB/oct + Brickwall)." },
        { "bottom.stereoSelector",  "Stereo Placement \xe2\x80\x94 which channels this band affects: Stereo, Left, Right, Mid, or Side." },
        { "bottom.dynEnable",       "Dynamic EQ \xe2\x80\x94 makes this band's gain respond to the input signal level in real time." },
        { "bottom.dynThreshKnob",   "Threshold \xe2\x80\x94 input level above which the dynamic EQ engages (-60 to 0 dB)." },
        { "bottom.dynRangeKnob",    "Range \xe2\x80\x94 max dynamic gain change. Positive = expand, negative = compress (-30 to +30 dB)." },
        { "bottom.dynAttackKnob",   "Attack \xe2\x80\x94 how quickly the dynamic EQ responds when signal exceeds threshold (0.1 \xe2\x80\x93 500 ms)." },
        { "bottom.dynReleaseKnob",  "Release \xe2\x80\x94 how quickly the dynamic EQ recovers after signal drops below threshold (1 \xe2\x80\x93 5000 ms)." },

        // LevelMeter
        { "levelMeter",          "Output Level Meter \xe2\x80\x94 peak and RMS output levels (L/R). Red indicates clipping." },

        // EQMatchPanel
        { "match.captureRef",    "Capture Reference \xe2\x80\x94 records the spectrum of the target sound to match." },
        { "match.captureCur",    "Capture Current \xe2\x80\x94 records the spectrum of the source to be shaped." },
        { "match.compute",       "Match \xe2\x80\x94 computes EQ bands that transform the current spectrum toward the reference." },
        { "match.apply",         "Apply \xe2\x80\x94 creates new EQ bands with the computed match settings." },
        { "match.close",         "Close \xe2\x80\x94 hides the EQ Match panel." },
    };

    auto it = database.find (componentID.toStdString());
    if (it != database.end())
        return juce::String::fromUTF8 (it->second);

    return {};
}

} // namespace HelpText
