# Filter Engine Deep Dive

This document covers the mathematical foundations and implementation details of the Advanced EQ filter engine, from the individual SVF processing tick through Butterworth cascading and frequency response evaluation.

## Architecture Overview

The filter engine is structured in three layers:

```
SVFFilter          -- Single 2nd-order (or 1st-order) TPT state variable filter section
    ^
    |
CascadedFilter     -- Chains up to 8 SVFFilter sections for higher-order slopes
    ^
    |
EqBand             -- Owns two CascadedFilter instances (L/R), adds smoothing + saturation
```

**Source files**:
- `source/dsp/SVFFilter.h` / `.cpp`
- `source/dsp/CascadedFilter.h` / `.cpp`
- `source/dsp/EqBand.h` / `.cpp`
- `source/dsp/FilterTypes.h`

## SVF Theory: Cytomic TPT

The filter is based on Andy Simper's Topology-Preserving Transform (TPT) papers from Cytomic. The TPT approach maps an analog filter prototype to the discrete-time domain while preserving the analog circuit's internal topology. This yields several key advantages over the traditional bilinear transform (as used in Robert Bristow-Johnson's Audio EQ Cookbook):

1. **Modulation stability**: Coefficient changes never produce transient artifacts (no zipper noise, no instability), because the integrator states remain consistent with the new coefficients.

2. **Unified structure**: All filter types (Bell, Shelf, HP, LP, Notch, BP) share the same processing tick. Only the mix coefficients change.

3. **Numerical robustness**: Double-precision state variables and the TPT structure maintain accuracy across the full audio frequency range.

### Core Coefficient: g

The fundamental mapping coefficient is:

```
g = tan(pi * frequency / sampleRate)
```

This performs the bilinear transform's frequency warping, mapping the analog cutoff frequency to its discrete-time equivalent. At low frequencies, `g` approaches `pi * frequency / sampleRate` (i.e., minimal warping). At high frequencies approaching Nyquist, `g` approaches infinity, naturally limiting the filter's response.

### Damping Coefficient: k

For second-order sections:

```
k = 1 / Q
```

where `Q` is the quality factor. For Bell filters, `k` is modified to incorporate gain:

```
k = 1 / (Q * A)     where A = 10^(gainDB / 40)
```

This ensures the Bell filter has a constant bandwidth (in terms of the -3dB points) regardless of gain.

## Second-Order SVF Processing Equations

**File**: `source/dsp/SVFFilter.cpp`, lines 202-224

The second-order TPT SVF tick processes one sample through two cascaded integrators:

```
State variables: ic1eq, ic2eq (integrator outputs from previous sample)
Precomputed:     a1 = 1 / (1 + g * (g + k))
                 a2 = g * a1
                 a3 = g * a2

Processing tick:
    v3 = v0 - ic2eq                     // Input minus lowpass feedback
    v1 = a1 * ic1eq + a2 * v3           // Bandpass output
    v2 = ic2eq + a2 * ic1eq + a3 * v3   // Lowpass output
    ic1eq = 2 * v1 - ic1eq              // Update integrator 1 state
    ic2eq = 2 * v2 - ic2eq              // Update integrator 2 state
    output = m0 * v0 + m1 * v1 + m2 * v2  // Mix: input + bandpass + lowpass
```

The internal signals correspond to the analog prototype's nodes:
- `v0` = input signal
- `v1` = bandpass output (output of first integrator)
- `v2` = lowpass output (output of second integrator)
- `v0 - k*v1 - v2` = highpass output (implicit)

The output is a weighted sum of these three signals. The mix coefficients `m0`, `m1`, `m2` determine the filter shape.

## First-Order SVF Processing Equations

**File**: `source/dsp/SVFFilter.cpp`, lines 206-213

For first-order (one-pole) operation, only a single integrator is used:

```
State variable: ic1eq (single integrator state)

Processing tick:
    v = (v0 - ic1eq) * g / (1 + g)     // Voltage across integrator
    lp = v + ic1eq                       // Lowpass output
    ic1eq = lp + v                       // Update integrator state
    output = m0 * v0 + m2 * lp          // Mix: input + lowpass
```

The highpass output is implicitly `hp = v0 - lp`. First-order mode produces 6 dB/oct slopes.

## Mix Coefficient Table

The following table shows how the mix coefficients are set for each filter type. These are computed in `SVFFilter::updateCoefficients()`.

### Second-Order Mode

| Filter Type | m0 | m1 | m2 | Special g/k Modifications |
|-------------|----|----|----|----|
| **Bell** | 1 | k * (A^2 - 1) | 0 | k = 1/(Q*A); recompute a1, a2, a3 |
| **Low Shelf** | 1 | k * (A - 1) | A^2 - 1 | k = 1/Q (standard) |
| **High Shelf** | A^2 | k * (1-A) * A | 1 - A^2 | k = 1/Q (standard) |
| **High Pass** | 1 | -k | -1 | (standard g, k) |
| **Low Pass** | 0 | 0 | 1 | (standard g, k) |
| **Notch** | 1 | -k | 0 | (standard g, k) |
| **Band Pass** | 0 | 1 | 0 | (standard g, k) |
| **Tilt Shelf** | 1 | k * (A - 1) | A^2 - 1 | g = tan(pi*f/sr) / sqrt(A); k = 1/Q |
| **Flat Tilt** | 1 | 0 | A^2 - 1 | Switches to first-order; g = tan(pi*f/sr) |

Where `A = 10^(gainDB / 40.0)` = square root of the linear gain.

### First-Order Mode

| Filter Type | m0 | m2 | Special g Modifications |
|-------------|----|----|-----|
| **Low Pass** | 0 | 1 | (standard g) |
| **High Pass** | 1 | -1 | (standard g) |
| **Low Shelf** | 1 | A^2 - 1 | g *= sqrt(A) |
| **High Shelf** | A^2 | 1 - A^2 | g /= sqrt(A) |
| **Other types** | 1 | 0 | Pass-through |

Note: `m1` is unused in first-order mode.

## Butterworth Cascading

**File**: `source/dsp/CascadedFilter.h` / `.cpp`

For high-pass and low-pass filter types, multiple SVF sections are cascaded to achieve steeper slopes. The `CascadedFilter` class chains up to 8 `SVFFilter` instances.

### Butterworth Q Assignment

For an Nth-order Butterworth filter, the Q value for each second-order section is:

```
Q_k = 1 / (2 * cos(pi * (2k + 1) / (2N)))    for k = 0, 1, ..., numSecondOrder - 1
```

where `N` is the total filter order and `k` is the zero-based section index. This pole-placement formula yields a maximally-flat magnitude response in the passband.

**Implementation** (CascadedFilter.cpp, lines 86-93):

```cpp
for (int i = 0; i < numSecondOrder; ++i)
{
    double angle = kPi * (2.0 * i + 1.0) / (2.0 * order);
    double butterworthQ = 1.0 / (2.0 * std::cos (angle));
    sections[i].setFirstOrder (false);
    sections[i].setParameters (type, freq, butterworthQ, 0.0);
}
```

For odd-order filters, the last section is switched to first-order mode with a default Q of 0.707 (the Q parameter is ignored in first-order mode but set for consistency):

```cpp
if (slopeInfo.lastIsFirstOrder)
{
    sections[lastIdx].setFirstOrder (true);
    sections[lastIdx].setParameters (type, freq, 0.707, 0.0);
}
```

### Slope to Section Mapping

The `getSlopeInfo()` function in `FilterTypes.h` computes the section count:

```
Order = dB_per_oct / 6
isOdd = (Order % 2) != 0
numSections = ceil(Order / 2)     // i.e., (Order + 1) / 2
lastIsFirstOrder = isOdd
```

| Slope | Order (N) | Sections | Last First-Order? | Butterworth Q Values |
|-------|-----------|----------|-------------------|---------------------|
| 6 dB/oct | 1 | 1 | Yes | (first-order, Q N/A) |
| 12 dB/oct | 2 | 1 | No | Q0 = 0.7071 |
| 18 dB/oct | 3 | 2 | Yes | Q0 = 1.0; last = first-order |
| 24 dB/oct | 4 | 2 | No | Q0 = 0.5412, Q1 = 1.3066 |
| 36 dB/oct | 6 | 3 | No | Q0 = 0.5176, Q1 = 0.7071, Q2 = 1.9319 |
| 48 dB/oct | 8 | 4 | No | Q0 = 0.5098, Q1 = 0.6013, Q2 = 0.9000, Q3 = 2.5629 |
| 72 dB/oct | 12 | 6 | No | (6 sections with Butterworth pole placement) |
| 96 dB/oct | 16 | 8 | No | (8 sections with Butterworth pole placement) |
| Brickwall | - | 8 | No | Same as 96 dB/oct (maximum available attenuation) |

### Non-HP/LP Types

For filter types other than HighPass and LowPass, cascading is handled differently:

- **Bell, Notch, BandPass, TiltShelf, FlatTilt**: Always use a single section. The slope parameter is ignored. The user's Q value is passed directly.

- **LowShelf, HighShelf**: Use the slope-derived section count, but each section receives the user's Q value (not Butterworth Q). The gain is distributed evenly:

```cpp
double perSectionGain = gainDB / numActiveSections;
```

This distributes the total shelf gain across cascaded sections, producing a steeper shelf transition. The first-order flag is set on the last section if the order is odd.

## Frequency Response Evaluation

**File**: `source/dsp/SVFFilter.cpp`, lines 233-278

The `getResponse(freq)` method evaluates the filter's complex frequency response at an arbitrary frequency. This is called from the UI thread for real-time curve rendering.

### Second-Order Transfer Function

The evaluation uses the z-domain transfer function derived from the SVF structure:

```
omega = 2 * pi * freq / sampleRate
z^-1 = e^(-j * omega)
z^-2 = (z^-1)^2

D = 1 + g * (g + k)

Common denominator:
    H_den(z) = 1 + a1_bq * z^-1 + a2_bq * z^-2
    where: a1_bq = 2 * (g^2 - 1) / D
           a2_bq = (1 - g*k + g^2) / D

Lowpass:   H_lp(z) = (g^2/D) * (1 + 2*z^-1 + z^-2) / H_den(z)
Bandpass:  H_bp(z) = (g/D) * (1 - z^-2) / H_den(z)
Highpass:  H_hp(z) = (1/D) * (1 - 2*z^-1 + z^-2) / H_den(z)

Output response:
    H(z) = m0 * (H_hp + k * H_bp + H_lp) + m1 * H_bp + m2 * H_lp
```

The `m0 * (H_hp + k * H_bp + H_lp)` term equals `m0` when the filter is a pure allpass (hp + k*bp + lp = 1 for the SVF topology). The remaining `m1` and `m2` terms add the desired filter shaping.

### First-Order Transfer Function

For first-order mode:

```
G = g / (1 + g)
c = (g - 1) / (g + 1)

H_lp(z) = G * (1 + z^-1) / (1 + c * z^-1)

H(z) = m0 + m2 * H_lp(z)
```

### Cascaded Response

The composite response of a `CascadedFilter` is the product of all active section responses:

```cpp
std::complex<double> composite (1.0, 0.0);
for (int i = 0; i < numActiveSections; ++i)
    composite *= sections[i].getResponse (freq);
```

This is evaluated at 512 logarithmically-spaced frequency points for the UI curve rendering.

### Magnitude in dB

```cpp
double getMagnitudeAtFrequency (double freq) const
{
    auto response = getResponse (freq);
    return 20.0 * std::log10 (std::max (std::abs (response), 1e-10));
}
```

## Analog Mode: Proportional Q

When analog mode is enabled, Bell filters exhibit proportional Q behavior -- the bandwidth narrows as gain increases:

```cpp
if (analogMode && params.type == FilterType::Bell)
    q *= (1.0f + 0.4f * std::abs (gain) / 30.0f);
```

At maximum gain (+/-30 dB), the Q multiplier is 1.4x. This models the behavior of analog parametric EQs where the resonance increases with gain, yielding a more "musical" frequency response.

## Analog Mode: Per-Band Saturation

Each band applies a soft-clipping waveshaper when analog mode is enabled:

```cpp
float saturate (float x)
{
    constexpr float drive = 1.2f;
    constexpr float asymmetry = 0.05f;
    constexpr float normFactor = 1.0f / 0.8336546f;  // 1/tanh(1.2)
    float shaped = x + asymmetry * x * x;
    return std::tanh (shaped * drive) * normFactor;
}
```

**Transfer function analysis:**
- For small signals (|x| << 1): `saturate(x) ~ x` (unity gain, transparent)
- For moderate signals: the `x^2` term introduces 2nd-order harmonics; `tanh` introduces odd-order harmonics
- For large signals: `tanh` compresses toward +/-1, then normFactor scales back to approximately unity
- The asymmetry of the `x^2` term creates an even/odd harmonic blend characteristic of tube and transformer circuits

## Filter Type Descriptions

### Bell (Parametric Peak/Dip)

A symmetric peak or dip centered at the specified frequency. The bandwidth is controlled by Q (higher Q = narrower bandwidth). Gain specifies the boost or cut in dB. Uses a single second-order section regardless of slope setting.

### Low Shelf

Boosts or cuts all frequencies below the corner frequency. The transition slope is determined by the selected dB/oct setting. For multi-section shelves, gain is distributed equally across sections.

### High Shelf

Boosts or cuts all frequencies above the corner frequency. Same cascading behavior as Low Shelf.

### High Pass (Low Cut)

Attenuates frequencies below the cutoff with the selected Butterworth slope. Supports all slopes from 6 dB/oct to Brickwall (96 dB/oct). Q values are set automatically by the Butterworth formula.

### Low Pass (High Cut)

Attenuates frequencies above the cutoff. Same Butterworth cascading as High Pass.

### Notch

A deep null at the center frequency. The width of the null is controlled by Q. Always uses a single section.

### Band Pass

Passes a narrow band of frequencies around the center. The bandwidth is controlled by Q. Always uses a single section.

### Tilt Shelf

A combined low-shelf cut / high-shelf boost (or vice versa) that pivots around the center frequency. Produces a gradual tilt across the spectrum. Uses a modified `g` value (`g /= sqrt(A)`) for a different transition characteristic compared to standard shelves.

### Flat Tilt

A gentle first-order broadband tilt across the entire spectrum. The response crosses 0 dB at the specified frequency. Forces first-order mode internally regardless of the slope parameter. Produces a constant 6 dB/oct slope.

## Numerical Precision

- All SVF state variables (`ic1eq`, `ic2eq`) and coefficients are stored as `double` (64-bit)
- Input/output samples are `float` (32-bit), cast to `double` during processing
- The frequency parameter is clamped to `[10 Hz, 0.49 * sampleRate]` to prevent aliasing at Nyquist
- Q is clamped to a minimum of 0.1 to prevent division by zero (since `k = 1/Q`)
- `ScopedNoDenormals` is set at the `processBlock()` level to prevent denormalized numbers in the feedback paths
