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

inline int slopeToSections (int dbPerOct)
{
    if (dbPerOct == 0) return 8;    // Brickwall: max sections
    if (dbPerOct <= 6) return 1;    // First-order
    return dbPerOct / 12;           // 12->1, 24->2, 36->3, etc.
}
