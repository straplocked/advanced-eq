#pragma once

enum class FilterType
{
    Bell = 0,
    LowShelf,
    HighShelf,
    HighPass,
    LowPass,
    Notch,
    BandPass,
    TiltShelf,
    FlatTilt
};

enum class StereoPlacement
{
    Stereo = 0,
    Left,
    Right,
    Mid,
    Side
};

enum class ProcessingMode
{
    ZeroLatency = 0,
    NaturalPhase,
    LinearPhase
};

// Slope in dB/oct to number of biquad sections
inline int slopeIndexToDBPerOct (int index)
{
    constexpr int slopes[] = { 6, 12, 18, 24, 36, 48, 72, 96, 0 }; // 0 = brickwall
    return (index >= 0 && index < 9) ? slopes[index] : 12;
}

struct SlopeInfo
{
    int numSections;          // Total SVF sections needed
    bool lastIsFirstOrder;    // True if the last section should be first-order (odd order)
};

inline SlopeInfo getSlopeInfo (int dbPerOct)
{
    if (dbPerOct == 0) return { 8, false };    // Brickwall: 8 second-order sections
    int order = dbPerOct / 6;                  // 6->1, 12->2, 18->3, 24->4, etc.
    bool isOdd = (order % 2) != 0;
    int sections = (order + 1) / 2;            // Ceiling division by 2
    return { sections, isOdd };
}
