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

#include "FactoryPresets.h"

// Shorthand aliases
using FT = FilterType;

namespace FactoryPresets
{

// ============================================================================
// INSTRUMENTS (32 presets)
// ============================================================================

std::vector<Preset> getInstrumentPresets()
{
    std::vector<Preset> presets;

    // --- Vocals ---

    presets.push_back ({ "Vocal Clarity", "Instruments", {
        { 0, FT::HighPass,  80.0f,   0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,     250.0f,  -2.5f, 1.5f,  1, 0 },
        { 2, FT::Bell,    3000.0f,  +3.0f, 1.2f,  1, 0 },
        { 3, FT::HighShelf,10000.0f,+2.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Vocal Warmth", "Instruments", {
        { 0, FT::HighPass, 100.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,     200.0f, +2.0f, 0.8f,  1, 0 },
        { 2, FT::Bell,     800.0f, -1.5f, 1.0f,  1, 0 },
        { 3, FT::Bell,    5000.0f, -1.0f, 1.5f,  1, 0 },
    }});

    presets.push_back ({ "Vocal De-Mud", "Instruments", {
        { 0, FT::HighPass,  60.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,     200.0f, -4.0f, 1.2f,  1, 0 },
        { 2, FT::Bell,     400.0f, -2.0f, 1.0f,  1, 0 },
    }});

    presets.push_back ({ "Vocal Air", "Instruments", {
        { 0, FT::HighPass,   80.0f,  0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,     6000.0f, +2.0f, 0.8f,  1, 0 },
        { 2, FT::HighShelf,12000.0f,+4.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Vocal Telephone", "Instruments", {
        { 0, FT::HighPass,  400.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::Bell,     1500.0f,+4.0f, 0.7f,  1, 0 },
        { 2, FT::LowPass,  3500.0f, 0.0f, 0.71f, 4, 0 },
    }});

    presets.push_back ({ "Vocal Radio", "Instruments", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::LowShelf,   200.0f,+1.5f, 0.71f, 1, 0 },
        { 2, FT::Bell,      2500.0f,+2.5f, 1.0f,  1, 0 },
        { 3, FT::HighShelf,  8000.0f,+3.0f, 0.71f, 1, 0 },
        { 4, FT::LowPass,  16000.0f, 0.0f, 0.71f, 2, 0 },
    }});

    // --- Guitar ---

    presets.push_back ({ "Acoustic Guitar", "Instruments", {
        { 0, FT::HighPass,   80.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      200.0f,-2.0f, 1.2f,  1, 0 },
        { 2, FT::Bell,     2000.0f,+2.0f, 1.0f,  1, 0 },
        { 3, FT::HighShelf, 8000.0f,+2.5f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Electric Guitar Clean", "Instruments", {
        { 0, FT::HighPass,  100.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      400.0f,+1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,     1500.0f,-1.5f, 1.5f,  1, 0 },
        { 3, FT::Bell,     3500.0f,+2.0f, 1.0f,  1, 0 },
    }});

    presets.push_back ({ "Electric Guitar Crunch", "Instruments", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       800.0f,+2.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,      2500.0f,+1.5f, 1.2f,  1, 0 },
        { 3, FT::LowPass,  10000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Guitar DI Fix", "Instruments", {
        { 0, FT::HighPass,   60.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,      150.0f, -3.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,     1000.0f, +2.0f, 0.8f,  1, 0 },
        { 3, FT::Bell,     4000.0f, +1.5f, 1.2f,  1, 0 },
        { 4, FT::HighShelf, 8000.0f,+2.0f, 0.71f, 1, 0 },
    }});

    // --- Bass ---

    presets.push_back ({ "Bass Finger", "Instruments", {
        { 0, FT::HighPass,  30.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,      80.0f, +2.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,     500.0f, -2.5f, 1.2f,  1, 0 },
        { 3, FT::Bell,    1200.0f, +2.0f, 1.5f,  1, 0 },
    }});

    presets.push_back ({ "Bass Pick", "Instruments", {
        { 0, FT::HighPass,  35.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,     100.0f, +1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,     700.0f, -2.0f, 1.0f,  1, 0 },
        { 3, FT::Bell,    2500.0f, +3.0f, 1.5f,  1, 0 },
    }});

    presets.push_back ({ "Bass Synth Sub", "Instruments", {
        { 0, FT::HighPass,  25.0f,   0.0f, 0.71f, 3, 0 },
        { 1, FT::LowShelf,  60.0f,  +3.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,     200.0f,  -2.0f, 1.0f,  1, 0 },
        { 3, FT::Bell,    1000.0f,  +1.5f, 1.2f,  1, 0 },
    }});

    presets.push_back ({ "Bass DI Clean", "Instruments", {
        { 0, FT::HighPass,  30.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,      60.0f, +1.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,     250.0f, -2.0f, 1.5f,  1, 0 },
        { 3, FT::Bell,     800.0f, +1.0f, 1.0f,  1, 0 },
        { 4, FT::LowPass, 6000.0f,  0.0f, 0.71f, 1, 0 },
    }});

    // --- Drums ---

    presets.push_back ({ "Kick Drum", "Instruments", {
        { 0, FT::HighPass,  25.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,      60.0f, +3.0f, 1.5f,  1, 0 },
        { 2, FT::Bell,     350.0f, -4.0f, 1.5f,  1, 0 },
        { 3, FT::Bell,    3500.0f, +3.5f, 2.0f,  1, 0 },
    }});

    presets.push_back ({ "Snare Drum", "Instruments", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       200.0f,+2.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,       800.0f,-2.0f, 1.5f,  1, 0 },
        { 3, FT::Bell,      4000.0f,+3.0f, 1.5f,  1, 0 },
        { 4, FT::HighShelf, 10000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Hi-Hat", "Instruments", {
        { 0, FT::HighPass,   300.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       800.0f,-2.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,      6000.0f,+2.0f, 1.2f,  1, 0 },
        { 3, FT::HighShelf, 12000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Toms", "Instruments", {
        { 0, FT::HighPass,    50.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       100.0f,+2.0f, 0.8f,  1, 0 },
        { 2, FT::Bell,       400.0f,-3.0f, 1.5f,  1, 0 },
        { 3, FT::Bell,      3000.0f,+2.0f, 1.0f,  1, 0 },
        { 4, FT::LowPass,  10000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Overhead Mics", "Instruments", {
        { 0, FT::HighPass,   200.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       500.0f,-1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,      3000.0f,+1.5f, 0.7f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+2.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Room Mics", "Instruments", {
        { 0, FT::HighPass,  100.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      300.0f,-2.0f, 0.7f,  1, 0 },
        { 2, FT::Bell,     2000.0f,+1.5f, 0.5f,  1, 0 },
        { 3, FT::HighShelf, 8000.0f,+2.0f,0.71f, 1, 0 },
    }});

    // --- Keys & Piano ---

    presets.push_back ({ "Grand Piano", "Instruments", {
        { 0, FT::HighPass,   30.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      200.0f,-1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,     2500.0f,+2.0f, 0.7f,  1, 0 },
        { 3, FT::HighShelf, 8000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Electric Piano", "Instruments", {
        { 0, FT::HighPass,   60.0f,  0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      300.0f, +1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,     1000.0f, -1.0f, 1.0f,  1, 0 },
        { 3, FT::Bell,     3000.0f, +1.5f, 1.0f,  1, 0 },
    }});

    presets.push_back ({ "Synth Pad", "Instruments", {
        { 0, FT::HighPass,    50.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::LowShelf,   150.0f,-2.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,      2000.0f,+1.0f, 0.5f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+3.0f,0.71f, 1, 0 },
    }});

    // --- Strings & Brass ---

    presets.push_back ({ "Strings Ensemble", "Instruments", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       300.0f,-1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,      3000.0f,+2.0f, 0.7f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+2.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Violin / Viola", "Instruments", {
        { 0, FT::HighPass,  150.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      500.0f,+1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,     2000.0f,-1.5f, 1.5f,  1, 0 },
        { 3, FT::Bell,     5000.0f,+2.0f, 1.0f,  1, 0 },
    }});

    presets.push_back ({ "Cello / Double Bass", "Instruments", {
        { 0, FT::HighPass,  35.0f,  0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,     150.0f, +1.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,     600.0f, -2.0f, 1.2f,  1, 0 },
        { 3, FT::Bell,    2500.0f, +1.5f, 0.8f,  1, 0 },
    }});

    presets.push_back ({ "Brass Section", "Instruments", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       300.0f,+1.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,      1500.0f,-2.0f, 1.5f,  1, 0 },
        { 3, FT::Bell,      5000.0f,+2.0f, 1.0f,  1, 0 },
        { 4, FT::LowPass,  12000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    // --- Other Instruments ---

    presets.push_back ({ "Drum Machine", "Instruments", {
        { 0, FT::HighPass,    30.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       100.0f,+2.5f, 1.0f,  1, 0 },
        { 2, FT::Bell,      1000.0f,-1.5f, 0.8f,  1, 0 },
        { 3, FT::Bell,      5000.0f,+2.5f, 1.0f,  1, 0 },
        { 4, FT::HighShelf, 12000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Percussion", "Instruments", {
        { 0, FT::HighPass,  150.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      400.0f,-1.5f, 1.0f,  1, 0 },
        { 2, FT::Bell,     2500.0f,+2.0f, 0.8f,  1, 0 },
        { 3, FT::HighShelf, 8000.0f,+3.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Organ", "Instruments", {
        { 0, FT::HighPass,   40.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      250.0f,-1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,     1200.0f,+2.0f, 0.7f,  1, 0 },
        { 3, FT::Bell,     4000.0f,+1.5f, 1.0f,  1, 0 },
    }});

    presets.push_back ({ "Harmonica", "Instruments", {
        { 0, FT::HighPass,  200.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      500.0f,+2.0f, 0.8f,  1, 0 },
        { 2, FT::Bell,     2000.0f,-2.0f, 1.5f,  1, 0 },
        { 3, FT::Bell,     5000.0f,+1.5f, 1.0f,  1, 0 },
    }});

    presets.push_back ({ "Banjo / Mandolin", "Instruments", {
        { 0, FT::HighPass,  100.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,      400.0f,+1.0f, 0.8f,  1, 0 },
        { 2, FT::Bell,     2000.0f,+2.0f, 1.0f,  1, 0 },
        { 3, FT::HighShelf, 7000.0f,+2.5f,0.71f, 1, 0 },
    }});

    return presets;
}

// ============================================================================
// BUS EQ (13 presets)
// ============================================================================

std::vector<Preset> getBusPresets()
{
    std::vector<Preset> presets;

    presets.push_back ({ "Drum Bus Punch", "Bus EQ", {
        { 0, FT::HighPass,    30.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,        80.0f,+2.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,       400.0f,-2.5f, 0.8f,  1, 0 },
        { 3, FT::Bell,      3500.0f,+2.0f, 0.8f,  1, 0 },
        { 4, FT::HighShelf, 10000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Drum Bus Thick", "Bus EQ", {
        { 0, FT::HighPass,   25.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::LowShelf,  100.0f,+3.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,      300.0f,-2.0f, 1.0f,  1, 0 },
        { 3, FT::Bell,     5000.0f,+1.5f, 0.7f,  1, 0 },
    }});

    presets.push_back ({ "Vocal Bus Glue", "Bus EQ", {
        { 0, FT::HighPass,   100.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       250.0f,-1.5f, 0.8f,  1, 0 },
        { 2, FT::Bell,      2000.0f,+1.5f, 0.7f,  1, 0 },
        { 3, FT::HighShelf, 12000.0f,+2.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Vocal Bus Bright", "Bus EQ", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       300.0f,-2.0f, 1.0f,  1, 0 },
        { 2, FT::Bell,      3500.0f,+3.0f, 0.8f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+3.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Guitar Bus", "Bus EQ", {
        { 0, FT::HighPass,    80.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       400.0f,-1.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,      2000.0f,+1.5f, 0.8f,  1, 0 },
        { 3, FT::LowPass,  12000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Bass Bus", "Bus EQ", {
        { 0, FT::HighPass,  25.0f,  0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,      60.0f, +1.5f, 1.0f,  1, 0 },
        { 2, FT::Bell,     300.0f, -2.0f, 1.0f,  1, 0 },
        { 3, FT::Bell,    1500.0f, +1.5f, 0.8f,  1, 0 },
    }});

    presets.push_back ({ "Keys Bus", "Bus EQ", {
        { 0, FT::HighPass,    60.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       300.0f,-1.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,      2500.0f,+1.5f, 0.7f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+2.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Strings Bus", "Bus EQ", {
        { 0, FT::HighPass,    60.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,       250.0f,-1.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,      3000.0f,+1.5f, 0.5f,  1, 0 },
        { 3, FT::HighShelf, 12000.0f,+2.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Parallel Compression Bus", "Bus EQ", {
        { 0, FT::HighPass,   40.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::LowShelf,  150.0f,+3.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,      500.0f,-2.0f, 0.8f,  1, 0 },
        { 3, FT::Bell,     4000.0f,+2.5f, 0.7f,  1, 0 },
    }});

    presets.push_back ({ "Room / Reverb Bus", "Bus EQ", {
        { 0, FT::HighPass,   200.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       500.0f,-1.5f, 0.7f,  1, 0 },
        { 2, FT::Bell,      3000.0f,+1.0f, 0.5f,  1, 0 },
        { 3, FT::LowPass,  10000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Delay Bus", "Bus EQ", {
        { 0, FT::HighPass,  250.0f, 0.0f, 0.71f, 2, 0 },
        { 1, FT::Bell,     1000.0f,+1.0f, 0.5f,  1, 0 },
        { 2, FT::LowPass,  6000.0f, 0.0f, 0.71f, 2, 0 },
    }});

    presets.push_back ({ "FX Bus Tame", "Bus EQ", {
        { 0, FT::HighPass,   150.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       400.0f,-2.0f, 0.7f,  1, 0 },
        { 2, FT::Bell,      3000.0f,+1.0f, 0.5f,  1, 0 },
        { 3, FT::LowPass,  12000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mix Bus Gentle", "Bus EQ", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::LowShelf,   80.0f,+1.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,       600.0f,-1.0f, 0.5f,  1, 0 },
        { 3, FT::Bell,      3000.0f,+0.5f, 0.5f,  1, 0 },
        { 4, FT::HighShelf, 12000.0f,+1.5f,0.71f, 1, 0 },
    }});

    return presets;
}

// ============================================================================
// MASTERING (12 presets)
// ============================================================================

std::vector<Preset> getMasteringPresets()
{
    std::vector<Preset> presets;

    presets.push_back ({ "Mastering Gentle Smile", "Mastering", {
        { 0, FT::HighPass,   20.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::LowShelf,  60.0f,+1.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,      600.0f,-0.5f, 0.5f,  1, 0 },
        { 3, FT::HighShelf,10000.0f,+1.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Air", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::HighShelf,  8000.0f,+1.5f,0.71f, 1, 0 },
        { 2, FT::Bell,      16000.0f,+1.0f, 0.7f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Warmth", "Mastering", {
        { 0, FT::HighPass,   20.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::LowShelf,  80.0f,+1.5f, 0.71f, 1, 0 },
        { 2, FT::Bell,      200.0f,+0.5f, 0.5f,  1, 0 },
        { 3, FT::LowPass, 18000.0f, 0.0f, 0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Mid Scoop", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::Bell,       400.0f,-1.0f, 0.4f,  1, 0 },
        { 2, FT::Bell,      2500.0f,+0.5f, 0.5f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+1.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Presence", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::Bell,      2500.0f,+1.0f, 0.5f,  1, 0 },
        { 2, FT::Bell,      5000.0f,+0.5f, 0.7f,  1, 0 },
        { 3, FT::HighShelf, 12000.0f,+1.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Low-End Tighten", "Mastering", {
        { 0, FT::HighPass, 25.0f,  0.0f, 0.71f, 5, 0 },
        { 1, FT::Bell,     60.0f, +1.0f, 1.5f,  1, 0 },
        { 2, FT::Bell,    200.0f, -1.0f, 0.7f,  1, 0 },
        { 3, FT::Bell,    400.0f, -0.5f, 0.5f,  1, 0 },
    }});

    presets.push_back ({ "Mastering Rock", "Mastering", {
        { 0, FT::HighPass,    25.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::LowShelf,   80.0f,+1.5f, 0.71f, 1, 0 },
        { 2, FT::Bell,       500.0f,-1.0f, 0.5f,  1, 0 },
        { 3, FT::Bell,      3000.0f,+1.0f, 0.7f,  1, 0 },
        { 4, FT::HighShelf, 10000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Pop", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 4, 0 },
        { 1, FT::LowShelf,   60.0f,+0.5f, 0.71f, 1, 0 },
        { 2, FT::Bell,       300.0f,-0.5f, 0.4f,  1, 0 },
        { 3, FT::Bell,      4000.0f,+1.0f, 0.5f,  1, 0 },
        { 4, FT::HighShelf, 12000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Hip-Hop", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 5, 0 },
        { 1, FT::LowShelf,   60.0f,+2.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,       250.0f,-1.0f, 0.7f,  1, 0 },
        { 3, FT::Bell,      3000.0f,+0.5f, 0.5f,  1, 0 },
        { 4, FT::HighShelf, 10000.0f,+1.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Acoustic / Folk", "Mastering", {
        { 0, FT::HighPass,    25.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       150.0f,+0.5f, 0.5f,  1, 0 },
        { 2, FT::Bell,      2500.0f,+1.0f, 0.4f,  1, 0 },
        { 3, FT::HighShelf, 10000.0f,+1.5f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering EDM", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 5, 0 },
        { 1, FT::LowShelf,   50.0f,+2.0f, 0.71f, 1, 0 },
        { 2, FT::Bell,       300.0f,-1.5f, 0.5f,  1, 0 },
        { 3, FT::Bell,      4000.0f,+1.0f, 0.7f,  1, 0 },
        { 4, FT::HighShelf, 12000.0f,+2.0f,0.71f, 1, 0 },
    }});

    presets.push_back ({ "Mastering Classical", "Mastering", {
        { 0, FT::HighPass,    20.0f, 0.0f, 0.71f, 3, 0 },
        { 1, FT::Bell,       200.0f,-0.5f, 0.4f,  1, 0 },
        { 2, FT::Bell,      3000.0f,+0.5f, 0.3f,  1, 0 },
        { 3, FT::HighShelf, 12000.0f,+0.5f,0.71f, 1, 0 },
    }});

    return presets;
}

// ============================================================================
// ALL PRESETS
// ============================================================================

std::vector<Preset> getAll()
{
    auto all = getInstrumentPresets();
    auto bus = getBusPresets();
    auto master = getMasteringPresets();

    all.insert (all.end(), bus.begin(), bus.end());
    all.insert (all.end(), master.begin(), master.end());
    return all;
}

} // namespace FactoryPresets
