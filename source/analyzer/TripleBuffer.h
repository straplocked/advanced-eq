#pragma once

#include <array>
#include <atomic>
#include <vector>

// Lock-free triple buffer for audio-thread -> UI-thread data transfer.
// Writer (audio thread) never blocks. Reader (UI thread) always gets the
// most recently completed write. Frames may be skipped, which is fine
// for visualization.
template <typename T>
class TripleBuffer
{
public:
    TripleBuffer() = default;

    // Must be called from the message thread (e.g. prepareToPlay).
    void resize (size_t size)
    {
        for (auto& buf : buffers)
            buf.assign (size, T{});
    }

    size_t getSize() const { return buffers[0].size(); }

    // --- Writer (audio thread) ---

    T* getWritePointer()
    {
        return buffers[(size_t) writeIdx.load (std::memory_order_relaxed)].data();
    }

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

    bool newDataAvailable() const
    {
        return newData.load (std::memory_order_acquire);
    }

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
