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
 * @file PresetManager.h
 * @brief Preset loading, saving, navigation, and listener notification.
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include "PresetData.h"
#include <vector>

/**
 * @brief Manages factory and user presets, applies them by modifying APVTS parameters.
 *
 * Maintains a combined list of factory presets (built-in) and user presets
 * (loaded from XML files in a platform-specific application data directory).
 * Provides navigation (next/previous), saving, deletion, and a Listener
 * interface for preset change notifications.
 */
class PresetManager
{
public:
    /**
     * @brief Constructs the PresetManager and loads all factory and user presets.
     * @param apvts The AudioProcessorValueTreeState whose parameters will be modified when applying presets.
     */
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    /**
     * @brief Returns a const reference to the full list of all loaded presets.
     * @return Vector containing all factory and user presets.
     */
    const std::vector<Preset>& getAllPresets() const { return allPresets; }

    /**
     * @brief Returns a list of unique category names across all presets.
     * @return Vector of category name strings.
     */
    std::vector<juce::String> getCategories() const;

    /**
     * @brief Returns pointers to all presets belonging to the given category.
     * @param category The category name to filter by.
     * @return Vector of const pointers to matching Preset objects.
     */
    std::vector<const Preset*> getPresetsInCategory (const juce::String& category) const;

    /**
     * @brief Applies the preset at the given index to the APVTS.
     *
     * Resets all bands to defaults, then configures each band specified
     * in the preset and sets the output gain.
     *
     * @param presetIndex Zero-based index into the allPresets vector.
     */
    void applyPreset (int presetIndex);

    /**
     * @brief Advances to and applies the next preset in the list, wrapping around at the end.
     */
    void nextPreset();

    /**
     * @brief Steps back to and applies the previous preset in the list, wrapping around at the beginning.
     */
    void previousPreset();

    /**
     * @brief Returns the index of the currently active preset.
     * @return Zero-based index, or -1 if no preset is active.
     */
    int getCurrentPresetIndex() const { return currentIndex; }

    /**
     * @brief Returns the display name of the currently active preset.
     * @return Preset name string, or an empty string if no preset is active.
     */
    juce::String getCurrentPresetName() const;

    // User preset management

    /**
     * @brief Saves the current APVTS parameter state as a user preset.
     *
     * Writes an XML file to the user presets directory and refreshes the preset list.
     *
     * @param name Display name for the new preset.
     * @return True if saving succeeded; false on I/O error.
     */
    bool saveCurrentAsPreset (const juce::String& name);

    /**
     * @brief Deletes a user preset by index.
     *
     * Removes the preset file from disk and refreshes the preset list.
     * Factory presets cannot be deleted.
     *
     * @param presetIndex Zero-based index into the allPresets vector.
     * @return True if the preset was a user preset and was successfully deleted.
     */
    bool deleteUserPreset (int presetIndex);

    /**
     * @brief Re-scans the user presets directory and rebuilds the preset list.
     */
    void rescanUserPresets();

    /**
     * @brief Returns the platform-specific directory where user presets are stored.
     * @return juce::File pointing to the user presets directory.
     */
    juce::File getUserPresetsDirectory() const;

    /**
     * @brief Listener interface for receiving preset change notifications.
     */
    struct Listener
    {
        virtual ~Listener() = default;

        /** @brief Called when the active preset changes. */
        virtual void presetChanged() = 0;
    };

    /**
     * @brief Registers a listener for preset change notifications.
     * @param l Listener to add.
     */
    void addListener (Listener* l)    { listeners.add (l); }

    /**
     * @brief Removes a previously registered listener.
     * @param l Listener to remove.
     */
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> allPresets;
    int currentIndex = -1;

    juce::ListenerList<Listener> listeners;

    void rebuildPresetList();
    std::vector<Preset> loadUserPresetsFromDisk() const;
    void resetAllBands();
    void setBandFromPreset (const BandPreset& bp);
    void applyUserPresetFromFile (const juce::File& file);
    void notifyListeners();
};
