/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_core/juce_core.h>
#include "../utils/Constants.h"
#include <array>
#include <atomic>
#include <cstring>

/**
 * Inter-instance collision detection registry.
 *
 * Each plugin instance publishes its active band data (frequency, gain, Q)
 * to a shared memory region. Other instances read this data to detect
 * frequency overlap and highlight collision regions on the curve display.
 *
 * Uses a temp file with InterprocessLock for safe concurrent access.
 */
class InstanceRegistry : public juce::Timer
{
public:
    static constexpr int kMaxInstances = 8;
    static constexpr int kMaxBandsPerInstance = Constants::kMaxBands;
    static constexpr int kTimeoutMs = 3000; // Consider instance dead after 3s

    struct BandInfo
    {
        float frequency = 0.0f;
        float gain = 0.0f;
        float q = 0.0f;
        bool active = false;
    };

    struct InstanceSlot
    {
        int64_t lastHeartbeat = 0; // ms since epoch
        int instanceId = 0;
        BandInfo bands[kMaxBandsPerInstance];
    };

    struct CollisionInfo
    {
        float frequency = 0.0f;
        float bandwidth = 0.0f; // Width in octaves of the collision zone
        int otherInstanceId = 0;
    };

    InstanceRegistry()
    {
        instanceId = (int) (juce::Time::currentTimeMillis() & 0x7FFFFFFF);

        // Use temp directory for shared data file
        sharedFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getChildFile ("advanced-eq-instances.dat");

        startTimerHz (2); // Update every 500ms
    }

    ~InstanceRegistry() override
    {
        stopTimer();
        unregister();
    }

    /** Publish current band data to the shared registry. */
    void publishBands (const BandInfo* bands, int numBands)
    {
        std::memcpy (localBands.data(), bands,
                     sizeof (BandInfo) * (size_t) juce::jmin (numBands, kMaxBandsPerInstance));
        localBandCount = juce::jmin (numBands, kMaxBandsPerInstance);
    }

    /** Get detected collision zones with other instances. */
    const std::vector<CollisionInfo>& getCollisions() const { return collisions; }

    /** Check if collision detection found any overlaps. */
    bool hasCollisions() const { return ! collisions.empty(); }

    void timerCallback() override
    {
        writeSlot();
        readOtherSlots();
        detectCollisions();
    }

private:
    void writeSlot()
    {
        juce::InterProcessLock lock ("AdvancedEQ_InstanceRegistry");

        if (lock.enter (50))
        {
            auto stream = sharedFile.createOutputStream();
            if (stream == nullptr)
            {
                lock.exit();
                return;
            }

            // Read existing data
            std::array<InstanceSlot, kMaxInstances> slots {};
            readSlotsFromFile (slots);

            // Find our slot or an empty/expired one
            int mySlotIdx = -1;
            int64_t now = juce::Time::currentTimeMillis();

            for (int i = 0; i < kMaxInstances; ++i)
            {
                if (slots[i].instanceId == instanceId)
                {
                    mySlotIdx = i;
                    break;
                }
            }

            if (mySlotIdx < 0)
            {
                // Find expired or empty slot
                for (int i = 0; i < kMaxInstances; ++i)
                {
                    if (slots[i].instanceId == 0 ||
                        (now - slots[i].lastHeartbeat) > kTimeoutMs)
                    {
                        mySlotIdx = i;
                        break;
                    }
                }
            }

            if (mySlotIdx >= 0)
            {
                slots[mySlotIdx].instanceId = instanceId;
                slots[mySlotIdx].lastHeartbeat = now;
                std::memcpy (slots[mySlotIdx].bands, localBands.data(),
                             sizeof (BandInfo) * kMaxBandsPerInstance);

                // Write all slots back
                stream->setPosition (0);
                stream->write (slots.data(), sizeof (slots));
                stream->flush();
            }

            lock.exit();
        }
    }

    void readOtherSlots()
    {
        juce::InterProcessLock lock ("AdvancedEQ_InstanceRegistry");

        if (lock.enter (50))
        {
            readSlotsFromFile (otherSlots);
            lock.exit();
        }
    }

    void readSlotsFromFile (std::array<InstanceSlot, kMaxInstances>& slots)
    {
        if (! sharedFile.existsAsFile())
        {
            std::memset (slots.data(), 0, sizeof (slots));
            return;
        }

        auto stream = sharedFile.createInputStream();
        if (stream == nullptr || stream->getTotalLength() < (int64_t) sizeof (slots))
        {
            std::memset (slots.data(), 0, sizeof (slots));
            return;
        }

        stream->read (slots.data(), sizeof (slots));
    }

    void unregister()
    {
        juce::InterProcessLock lock ("AdvancedEQ_InstanceRegistry");

        if (lock.enter (100))
        {
            std::array<InstanceSlot, kMaxInstances> slots {};
            readSlotsFromFile (slots);

            for (int i = 0; i < kMaxInstances; ++i)
            {
                if (slots[i].instanceId == instanceId)
                {
                    std::memset (&slots[i], 0, sizeof (InstanceSlot));
                    break;
                }
            }

            auto stream = sharedFile.createOutputStream();
            if (stream)
            {
                stream->setPosition (0);
                stream->write (slots.data(), sizeof (slots));
                stream->flush();
            }

            lock.exit();
        }
    }

    void detectCollisions()
    {
        collisions.clear();

        int64_t now = juce::Time::currentTimeMillis();

        for (int s = 0; s < kMaxInstances; ++s)
        {
            auto& slot = otherSlots[s];
            if (slot.instanceId == 0 || slot.instanceId == instanceId)
                continue;
            if ((now - slot.lastHeartbeat) > kTimeoutMs)
                continue; // Expired instance

            // Compare our bands with this instance's bands
            for (int myBand = 0; myBand < localBandCount; ++myBand)
            {
                if (! localBands[myBand].active)
                    continue;

                for (int otherBand = 0; otherBand < kMaxBandsPerInstance; ++otherBand)
                {
                    if (! slot.bands[otherBand].active)
                        continue;

                    float myFreq = localBands[myBand].frequency;
                    float otherFreq = slot.bands[otherBand].frequency;

                    // Check if bands overlap (within ~1 octave considering Q)
                    float myBW = myFreq / std::max (localBands[myBand].q, 0.1f);
                    float otherBW = otherFreq / std::max (slot.bands[otherBand].q, 0.1f);

                    float myLow = myFreq - myBW * 0.5f;
                    float myHigh = myFreq + myBW * 0.5f;
                    float otherLow = otherFreq - otherBW * 0.5f;
                    float otherHigh = otherFreq + otherBW * 0.5f;

                    // Check for overlap
                    if (myLow < otherHigh && myHigh > otherLow)
                    {
                        float overlapCenter = (std::max (myLow, otherLow) +
                                               std::min (myHigh, otherHigh)) * 0.5f;
                        float overlapWidth = std::min (myHigh, otherHigh) -
                                             std::max (myLow, otherLow);

                        float bwOctaves = 0.0f;
                        if (overlapCenter > 0.0f)
                            bwOctaves = std::log2 ((overlapCenter + overlapWidth * 0.5f) /
                                                    std::max (overlapCenter - overlapWidth * 0.5f, 1.0f));

                        collisions.push_back ({ overlapCenter, bwOctaves, slot.instanceId });
                    }
                }
            }
        }
    }

    int instanceId = 0;
    juce::File sharedFile;

    std::array<BandInfo, kMaxBandsPerInstance> localBands {};
    int localBandCount = 0;

    std::array<InstanceSlot, kMaxInstances> otherSlots {};
    std::vector<CollisionInfo> collisions;
};
