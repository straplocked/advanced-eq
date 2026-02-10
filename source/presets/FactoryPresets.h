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
 * @file FactoryPresets.h
 * @brief Built-in factory preset library.
 */

#include "PresetData.h"
#include <vector>

/**
 * @brief Namespace providing 57 built-in factory presets organized into three categories.
 */
namespace FactoryPresets
{
    /**
     * @brief Returns instrument EQ curve presets (e.g., vocal presence, guitar body).
     * @return Vector of instrument-category Preset objects.
     */
    std::vector<Preset> getInstrumentPresets();

    /**
     * @brief Returns mixing bus EQ presets (e.g., drum bus, vocal bus, mix bus).
     * @return Vector of bus-category Preset objects.
     */
    std::vector<Preset> getBusPresets();

    /**
     * @brief Returns mastering chain presets (e.g., gentle master, loudness curves).
     * @return Vector of mastering-category Preset objects.
     */
    std::vector<Preset> getMasteringPresets();

    /**
     * @brief Returns the combined list of all 57 factory presets across all categories.
     * @return Vector containing all factory Preset objects.
     */
    std::vector<Preset> getAll();
}
