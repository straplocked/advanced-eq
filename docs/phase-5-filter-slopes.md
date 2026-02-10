# Phase 5: All Filter Types + Slopes

## Overview

Phase 5 completes the filter engine with proper support for all 9 slope options (6/12/18/24/36/48/72/96 dB/oct + Brickwall), fixes broken odd-order Butterworth cascades, differentiates FlatTilt from TiltShelf, and adds the BandControlPanel UI for selecting filter type and slope.

## Filter Types

| Type | Description | Slope Applies |
|------|-------------|:---:|
| Bell | Peaking EQ boost/cut around center frequency | No |
| Low Shelf | Shelving boost/cut below frequency | Yes |
| High Shelf | Shelving boost/cut above frequency | Yes |
| High Pass | Removes frequencies below cutoff | Yes |
| Low Pass | Removes frequencies above cutoff | Yes |
| Notch | Narrow band-reject filter | No |
| Band Pass | Passes only frequencies near center | No |
| Tilt Shelf | Second-order tilt around center frequency (steep transition, flat extremes) | No |
| Flat Tilt | First-order tilt across entire spectrum (gradual constant slope) | No |

## Slope Options

| Slope | Order | SVF Sections | Notes |
|-------|:-----:|:------------:|-------|
| 6 dB/oct | 1st | 1 (first-order) | True one-pole TPT filter |
| 12 dB/oct | 2nd | 1 | Standard second-order SVF |
| 18 dB/oct | 3rd | 2 (1 second-order + 1 first-order) | Butterworth Q = 1.0 |
| 24 dB/oct | 4th | 2 | Butterworth cascade |
| 36 dB/oct | 6th | 3 | Butterworth cascade |
| 48 dB/oct | 8th | 4 | Butterworth cascade |
| 72 dB/oct | 12th | 6 | Butterworth cascade |
| 96 dB/oct | 16th | 8 | Butterworth cascade |
| Brickwall | 16th | 8 | Same as 96 dB/oct (IIR limit) |

## DSP Architecture

### SVFFilter First-Order Mode

The SVF now supports a first-order (one-pole) TPT topology alongside the standard second-order mode:

```
First-order tick:
  v1 = (v0 - ic1eq) * g / (1 + g)
  lp = v1 + ic1eq
  ic1eq = lp + v1
  output = m0 * v0 + m2 * lp
```

Mix coefficients for first-order filters:
- **Low Pass**: `m0=0, m2=1` (output = lowpass)
- **High Pass**: `m0=1, m2=-1` (output = input - lowpass)
- **Low Shelf**: `m0=1, m2=(A^2-1)` with `g *= sqrt(A)`
- **High Shelf**: `m0=A^2, m2=(1-A^2)` with `g /= sqrt(A)`

### Butterworth Cascade (Odd Orders)

For odd-order filters (e.g., 18 dB/oct = 3rd order), the cascade uses:
- N-1 second-order sections with Butterworth Q values: `Q_k = 1 / (2 * cos(pi * (2k+1) / (2N)))`
- 1 first-order section for the real pole

### TiltShelf vs FlatTilt

- **TiltShelf**: Second-order SVF with `g /= sqrt(A)`. Creates a shelf-like tilt with a steeper transition at the crossover frequency and flat response at the extremes. Q parameter controls transition width.
- **FlatTilt**: First-order mode with `m0=1, m2=(A^2-1)`. Creates a gradual, approximately constant dB/octave tilt across the spectrum, crossing 0 dB at the specified frequency. Q has no effect.

## UI Changes

### BandControlPanel

A floating panel that appears when a band is selected:
- **Filter type** dropdown (9 types)
- **Slope** dropdown (9 options, disabled for types where slope doesn't apply)
- Positioned near the selected band handle

### Enhanced Tooltip

Hovering or selecting a band shows: `[Type] [Frequency] [Gain] dB Q [Q] [Slope]`

Slope is only shown for Low Pass, High Pass, Low Shelf, and High Shelf.

## Files Changed

### DSP
- `Source/dsp/SVFFilter.h` - Added `setFirstOrder()`, `isFirstOrder` member
- `Source/dsp/SVFFilter.cpp` - First-order processing, FlatTilt differentiation
- `Source/dsp/FilterTypes.h` - `SlopeInfo` struct replacing `slopeToSections()`
- `Source/dsp/CascadedFilter.cpp` - Odd-order Butterworth, first-order section support

### UI
- `Source/ui/components/BandControlPanel.h` - New file
- `Source/ui/components/BandControlPanel.cpp` - New file
- `Source/ui/components/EqCurveDisplay.h` - `onBandSelected` callback
- `Source/ui/components/EqCurveDisplay.cpp` - Enhanced tooltip, selection callbacks
- `Source/PluginEditor.h` - BandControlPanel integration
- `Source/PluginEditor.cpp` - BandControlPanel wiring and positioning
