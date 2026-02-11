/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <mutex>

/**
 * MIDI Learn Manager.
 *
 * Maps MIDI CC messages to APVTS parameters. Supports learn mode
 * (capture next CC) and playback mode (apply CC values to parameters).
 * Mappings are serializable for state save/restore.
 */
class MidiLearnManager
{
public:
    struct MidiMapping
    {
        int channel = 0;   // 0 = any channel, 1-16 = specific
        int cc = -1;
        juce::String parameterID;
    };

    explicit MidiLearnManager (juce::AudioProcessorValueTreeState& apvts)
        : apvts (apvts)
    {
    }

    /** Start learning: the next CC received will be mapped to the given parameter. */
    void startLearn (const juce::String& parameterID)
    {
        std::lock_guard<std::mutex> lock (mutex);
        learnTargetParam = parameterID;
        learning = true;
    }

    /** Cancel learning mode. */
    void cancelLearn()
    {
        std::lock_guard<std::mutex> lock (mutex);
        learning = false;
        learnTargetParam = {};
    }

    /** Check if we're currently in learn mode. */
    bool isLearning() const { return learning; }

    /** Get the parameter being learned (if any). */
    juce::String getLearnTarget() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        return learnTargetParam;
    }

    /** Remove the mapping for a specific parameter. */
    void removeMapping (const juce::String& parameterID)
    {
        std::lock_guard<std::mutex> lock (mutex);
        for (auto it = mappings.begin(); it != mappings.end(); ++it)
        {
            if (it->second.parameterID == parameterID)
            {
                mappings.erase (it);
                return;
            }
        }
    }

    /** Clear all MIDI mappings. */
    void clearAllMappings()
    {
        std::lock_guard<std::mutex> lock (mutex);
        mappings.clear();
    }

    /**
     * Process MIDI messages. Call from processBlock().
     * In learn mode, captures the first CC and creates a mapping.
     * In playback mode, applies CC values to mapped parameters.
     */
    void processMidi (const juce::MidiBuffer& midiMessages)
    {
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();

            if (msg.isController())
            {
                int channel = msg.getChannel();
                int cc = msg.getControllerNumber();
                float value = msg.getControllerValue() / 127.0f;

                // Learn mode: capture this CC
                if (learning)
                {
                    std::lock_guard<std::mutex> lock (mutex);
                    if (! learnTargetParam.isEmpty())
                    {
                        // Remove any existing mapping for this CC
                        int key = cc; // Use CC number as key (channel-agnostic for simplicity)
                        mappings[key] = { 0, cc, learnTargetParam };
                        learning = false;
                        learnTargetParam = {};
                    }
                    continue;
                }

                // Playback: apply CC value to mapped parameter
                std::lock_guard<std::mutex> lock (mutex);
                auto it = mappings.find (cc);
                if (it != mappings.end())
                {
                    if (auto* param = apvts.getParameter (it->second.parameterID))
                        param->setValueNotifyingHost (value);
                }
            }
        }
    }

    /** Serialize all mappings to a ValueTree. */
    juce::ValueTree getState() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        juce::ValueTree tree ("MidiMappings");

        for (const auto& [key, mapping] : mappings)
        {
            juce::ValueTree mapTree ("Mapping");
            mapTree.setProperty ("cc", mapping.cc, nullptr);
            mapTree.setProperty ("channel", mapping.channel, nullptr);
            mapTree.setProperty ("param", mapping.parameterID, nullptr);
            tree.appendChild (mapTree, nullptr);
        }

        return tree;
    }

    /** Restore mappings from a ValueTree. */
    void setState (const juce::ValueTree& tree)
    {
        std::lock_guard<std::mutex> lock (mutex);
        mappings.clear();

        if (! tree.isValid() || tree.getType().toString() != "MidiMappings")
            return;

        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            auto mapTree = tree.getChild (i);
            if (mapTree.getType().toString() == "Mapping")
            {
                int cc = mapTree.getProperty ("cc", -1);
                int channel = mapTree.getProperty ("channel", 0);
                juce::String paramID = mapTree.getProperty ("param", "").toString();

                if (cc >= 0 && paramID.isNotEmpty())
                    mappings[cc] = { channel, cc, paramID };
            }
        }
    }

    /** Get all current mappings (for UI display). */
    std::map<int, MidiMapping> getMappings() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        return mappings;
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    mutable std::mutex mutex;
    std::map<int, MidiMapping> mappings; // CC number -> mapping
    bool learning = false;
    juce::String learnTargetParam;
};
