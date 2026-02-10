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
 * @file TripleBuffer.h
 * @brief Lock-free triple buffer for audio-thread to UI-thread data transfer.
 */

#include <array>
#include <atomic>
#include <vector>

/**
 * @brief Wait-free single-producer single-consumer triple buffer.
 *
 * Provides a lock-free mechanism for passing bulk data (e.g., FFT magnitudes)
 * from the audio thread to the UI thread. Three internal buffers rotate through
 * the roles of write, middle, and read so that neither thread ever blocks.
 *
 * **Memory ordering contract:**
 * - The writer (audio thread) uses relaxed loads/stores internally and a
 *   release on finishWrite() to publish completed data.
 * - The reader (UI thread) uses acquire on swapRead() to observe the
 *   most recently published data.
 *
 * @note Frames may be skipped if the writer produces faster than the reader
 *       consumes. This is acceptable for visualization data where displaying
 *       every frame is not required.
 *
 * @tparam T Element type stored in each buffer (e.g., float for FFT magnitudes).
 */
// Lock-free triple buffer for audio-thread -> UI-thread data transfer.
// Writer (audio thread) never blocks. Reader (UI thread) always gets the
// most recently completed write. Frames may be skipped, which is fine
// for visualization.
template <typename T>
class TripleBuffer
{
public:
    TripleBuffer() = default;

    /**
     * @brief Resizes all three internal buffers to the given size.
     *
     * Must be called from the message thread before the audio thread begins
     * writing (e.g., from prepareToPlay).
     *
     * @param size Number of elements per buffer.
     */
    // Must be called from the message thread (e.g. prepareToPlay).
    void resize (size_t size)
    {
        for (auto& buf : buffers)
            buf.assign (size, T{});
    }

    /**
     * @brief Returns the current element count of each buffer.
     * @return Number of elements per buffer.
     */
    size_t getSize() const { return buffers[0].size(); }

    // --- Writer (audio thread) ---

    /**
     * @brief Returns a raw pointer to the current write buffer.
     *
     * The caller should fill this buffer with new data, then call finishWrite()
     * to publish it. Only the audio thread (writer) should call this method.
     *
     * @return Pointer to the first element of the write buffer.
     */
    T* getWritePointer()
    {
        return buffers[(size_t) writeIdx.load (std::memory_order_relaxed)].data();
    }

    /**
     * @brief Publishes the current write buffer to the reader.
     *
     * Atomically swaps the write and middle buffer indices using release
     * semantics, ensuring all preceding writes are visible to the reader
     * upon its next acquire load.
     */
    // Call after filling the write buffer. Swaps write <-> mid atomically.
    void finishWrite()
    {
        int w = writeIdx.load (std::memory_order_relaxed);
        int m = midIdx.load (std::memory_order_relaxed);
        writeIdx.store (m, std::memory_order_relaxed);
        midIdx.store (w, std::memory_order_release);
        newData.store (true, std::memory_order_release);
    }

    // --- Reader (UI thread) ---

    /**
     * @brief Checks whether the writer has published new data since the last read.
     *
     * Uses acquire semantics to synchronize with the writer's release store.
     *
     * @return True if new data is available for reading.
     */
    bool newDataAvailable() const
    {
        return newData.load (std::memory_order_acquire);
    }

    /**
     * @brief Swaps the read and middle buffers to obtain the latest data.
     *
     * Only the UI thread (reader) should call this method. After a successful
     * swap, getReadPointer() will return the most recently published data.
     *
     * @return True if new data was available and the swap occurred; false otherwise.
     */
    // Swaps read <-> mid to get latest data. Returns true if new data was available.
    bool swapRead()
    {
        if (! newData.load (std::memory_order_acquire))
            return false;

        int r = readIdx.load (std::memory_order_relaxed);
        int m = midIdx.load (std::memory_order_relaxed);
        readIdx.store (m, std::memory_order_relaxed);
        midIdx.store (r, std::memory_order_release);
        newData.store (false, std::memory_order_release);
        return true;
    }

    /**
     * @brief Returns a read-only pointer to the current read buffer.
     *
     * Valid until the next call to swapRead(). Only the UI thread (reader)
     * should call this method.
     *
     * @return Const pointer to the first element of the read buffer.
     */
    const T* getReadPointer() const
    {
        return buffers[(size_t) readIdx.load (std::memory_order_relaxed)].data();
    }

private:
    std::array<std::vector<T>, 3> buffers;
    std::atomic<int> writeIdx { 0 };
    std::atomic<int> midIdx   { 1 };
    std::atomic<int> readIdx  { 2 };
    std::atomic<bool> newData { false };
};
