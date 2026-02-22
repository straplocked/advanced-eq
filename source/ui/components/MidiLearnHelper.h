/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../midi/MidiLearnManager.h"

/**
 * Helper for showing MIDI learn context menus on sliders/knobs.
 * Call showMidiLearnMenu() from a mouseDown() handler when isPopupMenu() is true.
 */
namespace MidiLearnHelper
{
    inline void showMidiLearnMenu (juce::Component* parent,
                                   MidiLearnManager& midiLearn,
                                   const juce::String& parameterID,
                                   const juce::String& displayName)
    {
        juce::PopupMenu menu;

        auto mappings = midiLearn.getMappings();
        int existingCC = -1;
        for (const auto& [cc, mapping] : mappings)
        {
            if (mapping.parameterID == parameterID)
            {
                existingCC = cc;
                break;
            }
        }

        if (existingCC >= 0)
        {
            menu.addItem (1, "MIDI Learn (" + displayName + ", CC " + juce::String (existingCC) + ")");
            menu.addItem (2, "Clear MIDI Mapping");
        }
        else
        {
            menu.addItem (1, "MIDI Learn (" + displayName + ")");
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (parent),
            [ml = &midiLearn, parameterID] (int result)
            {
                if (result == 1)
                    ml->startLearn (parameterID);
                else if (result == 2)
                    ml->removeMapping (parameterID);
            });
    }
}
