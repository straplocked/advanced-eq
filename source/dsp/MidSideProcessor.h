#pragma once

// In-place Mid/Side encoding and decoding for stereo audio buffers.
// Encode: L → Mid, R → Side
// Decode: Mid → L, Side → R
namespace MidSideProcessor
{
    inline void encode (float* L, float* R, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float mid  = 0.5f * (L[i] + R[i]);
            float side = 0.5f * (L[i] - R[i]);
            L[i] = mid;
            R[i] = side;
        }
    }

    inline void decode (float* L, float* R, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float l = L[i] + R[i];  // mid + side
            float r = L[i] - R[i];  // mid - side
            L[i] = l;
            R[i] = r;
        }
    }
}
