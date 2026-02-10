# Phase 7: Mid/Side + Left/Right Processing

## Overview

Phase 7 adds per-band stereo placement routing. Each of the 24 bands can independently target Stereo (both channels), Left only, Right only, Mid, or Side. This enables surgical frequency adjustments to specific channels or M/S components — essential for mixing and mastering workflows.

## Stereo Placements

| Placement | Description | Processing |
|-----------|-------------|------------|
| Stereo | Both L and R channels (default) | `filterL` processes L, `filterR` processes R |
| Left | Left channel only | `filterL` processes L, R passes through |
| Right | Right channel only | `filterL` processes R, L passes through |
| Mid | Center/mono content | M/S encode → `filterL` processes Mid → decode |
| Side | Stereo/difference content | M/S encode → `filterL` processes Side → decode |

## DSP Architecture

### Processing Order

Since EQ bands are LTI (linear time-invariant) filters, cascade order doesn't affect the result. Bands are grouped by placement for efficient M/S encode/decode:

```
Input L/R
  1. Process all Stereo bands (both L+R)
  2. Process all Left bands (channel 0 only)
  3. Process all Right bands (channel 1 only)
  4. If any Mid or Side bands exist:
     a. M/S encode in-place (ch0=Mid, ch1=Side)
     b. Process all Mid bands (channel 0)
     c. Process all Side bands (channel 1)
     d. M/S decode in-place (ch0=L, ch1=R)
  5. Apply output gain
Output L/R
```

### M/S Math

```
Encode: Mid = 0.5 * (L + R),  Side = 0.5 * (L - R)
Decode: L = Mid + Side,        R = Mid - Side
```

M/S encode/decode is performed in-place on the buffer's two channels — no temporary buffers are allocated.

### Single-Channel Processing

`EqBand::processSingleChannel(float*, int)` uses `filterL` only for Left, Right, Mid, and Side placements. When a band's stereo placement changes, both filters are reset to prevent clicks from stale integrator state.

## UI Changes

### BandControlPanel

The floating control panel now has 3 rows (previously 2):
- **Row 1**: Filter type selector (9 types)
- **Row 2**: Slope selector (9 options, disabled for types where slope doesn't apply)
- **Row 3**: Stereo placement selector (Stereo, Left, Right, Mid, Side)

Panel height increased from 60px to 88px.

### Band Handle Tinting

Band handles in the EQ curve display show stereo placement via outer ring color:
- **Stereo**: Default band color (no tinting)
- **Left**: White ring (`ColourPalette::leftColour()`)
- **Right**: Red ring (`ColourPalette::rightColour()`)
- **Mid**: Green ring (`ColourPalette::midColour()`)
- **Side**: Blue ring (`ColourPalette::sideColour()`)

### Tooltip

The tooltip now shows the stereo placement name when it's not the default "Stereo" (e.g., "Bell  1.0 kHz  +3.0 dB  Q 1.00  Left").

## Files Changed

### New Files
- `Source/dsp/MidSideProcessor.h` — Header-only M/S encode/decode utility

### Modified Files
- `Source/dsp/EqBand.h` — Added `processSingleChannel()` declaration
- `Source/dsp/EqBand.cpp` — Implemented `processSingleChannel()`, added placement-change filter reset
- `Source/dsp/ProcessingChain.cpp` — Grouped routing by stereo placement with M/S encode/decode
- `Source/ui/components/BandControlPanel.h` — Added `stereoSelector` ComboBox and attachment
- `Source/ui/components/BandControlPanel.cpp` — Wired stereo selector, 3-row layout, larger panel
- `Source/ui/components/EqCurveDisplay.cpp` — Handle ring tinting and tooltip placement label
