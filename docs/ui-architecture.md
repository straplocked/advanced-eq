# UI Architecture

This document describes the complete user interface of Advanced EQ, covering the component hierarchy, interaction model, coordinate system, painting pipeline, timer-driven updates, color scheme, and custom look and feel.

## Component Hierarchy

```
AdvancedEQEditor (1000x600 default, resizable 750-1920 x 450-1080)
|
|-- AdvancedEqLookAndFeel (applied globally via setLookAndFeel)
|
|-- HeaderToolbar (top, 36px height)
|   |-- Title label "Advanced EQ"
|   |-- undoButton "Undo" / redoButton "Redo"
|   |-- abToggleButton "A" / copyABButton "A>B"
|   |-- prevButton "<" (preset navigation)
|   |-- presetNameButton (displays current preset name, opens menu)
|   |-- nextButton ">" (preset navigation)
|   |-- saveButton "Save" (save current state as user preset)
|   |-- fullscreenButton "FS"
|   |-- outputGainSlider (rotary, SliderAttachment -> output_gain)
|   +-- outputWidthSlider (rotary, SliderAttachment -> output_width)
|
|-- EqCurveDisplay (center, fills remaining space between header and bottom strip)
|   |-- Background + grid (frequency lines + gain lines)
|   |-- SpectrumAnalyzer pre-EQ (gray fill)
|   |-- SpectrumAnalyzer post-EQ (blue fill)
|   |-- Per-band curves (colored fill + stroke, one per active band)
|   |-- Preview curve (shown when hovering empty space)
|   |-- Composite curve (yellow, sum of all active bands)
|   |-- Band handles (colored dots, draggable)
|   |-- Frequency axis labels (bottom edge)
|   |-- Gain axis labels (left edge)
|   |-- Range buttons (top-right: +/-3, +/-6, +/-12, +/-30 dB)
|   +-- Tooltip (shows frequency + gain + note name during hover/drag)
|
+-- BottomControlStrip (bottom, 120px height, two rows)
    |-- Top row: band controls (shown when a band is selected)
    |   |-- bandLabel (colored dot + "Band N")
    |   |-- typeSelector, slopeSelector, stereoSelector
    |   |-- freqSlider, gainSlider, qSlider (rotary knobs + value labels)
    |   +-- dynEnableButton, dynThresh/Range/Attack/Release knobs
    |
    +-- Bottom row: global toolbar (always visible, full width)
        |-- processModeSelector (Zero Latency / Natural Phase / Linear Phase)
        |-- lpQualitySelector (Low / Medium / High)
        |-- characterSelector (Clean / Smooth / Vintage / Aggressive)
        |-- phaseInvertButton, analyzerButton, freezeButton
        |-- matchButton, sketchButton, browseButton
        +-- midiPCButton, helpButton
```

**Source files**:
- `source/PluginEditor.h` / `.cpp`
- `source/ui/components/EqCurveDisplay.h` / `.cpp`
- `source/ui/components/HeaderToolbar.h` / `.cpp`
- `source/ui/components/BottomControlStrip.h` / `.cpp`
- `source/ui/LookAndFeel/AdvancedEqLookAndFeel.h`
- `source/ui/LookAndFeel/ColourPalette.h`

## Layout

The editor uses a simple three-row layout managed in `AdvancedEQEditor::resized()`:

```cpp
auto bounds = getLocalBounds();
headerToolbar.setBounds (bounds.removeFromTop (36));       // 36px header
controlStrip.setBounds (bounds.removeFromBottom (90));     // 90px bottom strip
curveDisplay.setBounds (bounds);                           // Remaining space for EQ curves
```

### Window Sizing

| Property | Value |
|----------|-------|
| Default size | 1000 x 600 px |
| Minimum size | 750 x 450 px |
| Maximum size | 1920 x 1080 px |
| Resizable | Yes (both corners) |

## Interaction Model (EqCurveDisplay)

All mouse interactions are handled by the `EqCurveDisplay` component:

### Mouse Actions

| Action | Target | Result |
|--------|--------|--------|
| Left-click | Empty space | Create a new band at the clicked frequency/gain position. Finds the first inactive band slot, activates it, and sets its frequency and gain. |
| Left-click | Band handle | Select the band. Notifies `BottomControlStrip` via the `onBandSelected` callback. |
| Left-drag | Band handle | Adjust frequency (X axis, logarithmic) and gain (Y axis, linear dB). Both parameters are updated in real-time via `setValueNotifyingHost()`. |
| Scroll wheel | Over a band | Adjust Q factor. Scrolling up narrows the filter (increases Q), scrolling down widens it (decreases Q). |
| Double-click | Band handle | Deactivate the band (sets `active` parameter to false). |
| Right-click | Band handle | Opens a context menu with options: bypass/enable, delete, set type submenu, set slope submenu, set stereo placement submenu, reset to defaults. |
| Right-click | Empty space | Opens a context menu with "Add Band Here" option. |
| Mouse move | Empty space | Shows a preview curve -- a temporary Bell filter at the mouse position (frequency from X, gain from Y). |
| Mouse move | Band handle | Shows a hover glow effect on the handle. |
| Mouse exit | Component | Clears hover state and hides the preview curve. |

### Band Selection Flow

```
User clicks band handle in EqCurveDisplay
    |
    v
EqCurveDisplay.selectedBand = bandIndex
    |
    v
EqCurveDisplay.onBandSelected(bandIndex) callback fires
    |
    v
BottomControlStrip.setSelectedBand(bandIndex)
    |
    v
BottomControlStrip destroys old APVTS attachments
    |
    v
BottomControlStrip creates new attachments for band_XX_type, band_XX_slope, etc.
    |
    v
Sliders and combo boxes automatically display the new band's parameter values
```

### Band Creation Flow

```
User left-clicks empty space in EqCurveDisplay
    |
    v
findFirstInactiveBand() searches for first band with active == false
    |
    v
activateBandAt(freq, gain):
    - Set band active = true
    - Set band type = Bell (default)
    - Set band frequency = freq from mouse X (log-mapped)
    - Set band gain = gain from mouse Y (linear dB)
    |
    v
Select the new band (triggers onBandSelected callback)
```

### Band Handle Hit Detection

`findBandAtPosition()` iterates through all active bands and checks if the mouse position is within a threshold distance of the band's handle position. The handle position is computed from the band's frequency (X) and gain (Y) using the current coordinate mapping.

## Coordinate System

### X Axis: Logarithmic Frequency

The X axis maps frequency logarithmically from 10 Hz (left edge) to 30,000 Hz (right edge):

```cpp
// source/utils/MathUtils.h

float frequencyToNormalized (float freq)
{
    return log(freq / kMinFrequency) / log(kMaxFrequency / kMinFrequency);
    // kMinFrequency = 10 Hz, kMaxFrequency = 30000 Hz
}

float normalizedToFrequency (float normalized)
{
    return kMinFrequency * pow(kMaxFrequency / kMinFrequency, normalized);
}

float frequencyToX (float freq, float width)
{
    return frequencyToNormalized(freq) * width;
}

float xToFrequency (float x, float width)
{
    return normalizedToFrequency(x / width);
}
```

This logarithmic mapping ensures that octaves are equally spaced on screen: 100 Hz to 200 Hz occupies the same screen width as 1 kHz to 2 kHz.

### Y Axis: Linear Gain (dB)

The Y axis maps gain linearly in dB. The range is switchable between four options:

| Range | Min dB | Max dB | Grid Step |
|-------|--------|--------|-----------|
| +/-3 dB | -3 | +3 | 1 dB |
| +/-6 dB | -6 | +6 | 2 dB |
| +/-12 dB | -12 | +12 | 3 dB |
| +/-30 dB | -30 | +30 | 6 dB |

```cpp
// source/utils/MathUtils.h

float gainToY (float gainDB, float height, float minDB, float maxDB)
{
    return height * (1.0f - (gainDB - minDB) / (maxDB - minDB));
}

float yToGain (float y, float height, float minDB, float maxDB)
{
    return minDB + (1.0f - y / height) * (maxDB - minDB);
}
```

The 0 dB line is always at the vertical center of the display. Positive gains are above, negative gains below.

### Range Buttons

Four range buttons are rendered in the top-right corner of the EqCurveDisplay:

```
[+/-3] [+/-6] [+/-12] [+/-30]
```

Clicking a button switches the Y-axis display range. The currently active range button is highlighted. Band parameters outside the visible range are still applied to the audio but their handles may be off-screen.

## Painting Pipeline (EqCurveDisplay::paint)

The `paint()` method executes the following steps in order (back to front):

### 1. Fill Background

```cpp
g.fillAll (ColourPalette::curveArea());  // Dark purple #1a1a36
```

### 2. Draw Grid

Vertical lines at standard frequencies and horizontal lines at gain increments:

**Frequency grid lines** (vertical):
- Standard positions: 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 Hz
- Color: `gridLine()` (#2a2a4a) for normal, `gridLineStrong()` (#3a3a5a) for major divisions (100, 1k, 10k Hz)

**Gain grid lines** (horizontal):
- Spacing depends on the current display range (1, 2, 3, or 6 dB increments)
- The 0 dB line uses `gridLineStrong()`

### 3. Draw Spectrum Analyzer Paths

Two spectrum paths (pre-EQ and post-EQ) are painted as semi-transparent filled areas:

```cpp
drawSpectrum (g, spectrumAnalyzerPre,  ColourPalette::spectrumPre());   // Gray fill
drawSpectrum (g, spectrumAnalyzerPost, ColourPalette::spectrumPost());  // Blue fill
```

Each `SpectrumAnalyzer` provides both a stroke path (outline) and a fill path (closed to the bottom edge). The fill uses low-alpha colors for transparency.

### 4. Draw Per-Band Curves

For each active band, a magnitude response curve is drawn with:
- **Filled region**: Semi-transparent band color between the curve and the 0 dB line. Positive regions fill above, negative regions fill below.
- **Stroke**: The band's assigned color at full opacity, thin stroke.

Colors are assigned from a 12-color palette cycling by band index (see Color Scheme below).

### 5. Draw Preview Curve

When the mouse hovers over empty space (no band handle nearby), a preview curve is drawn showing what a new Bell filter would look like at the cursor position:

```cpp
previewFilter.setParameters (FilterType::Bell, freq, 1.0, gain);
```

The preview uses a dimmed/dashed appearance to distinguish it from real bands.

### 6. Draw Composite Curve

The sum of all active bands' magnitude responses is drawn as the composite EQ curve:

- **Stroke**: Bright yellow (`compositeCurve()`, #ffd93d), 2px wide
- **Fill**: Very low alpha yellow (`compositeFill()`, #18ffd93d)

This represents the total EQ effect applied to the audio.

### 7. Draw Band Handles

For each active band, a circular handle dot is drawn at the (frequency, gain) position:

- **Normal state**: Solid circle filled with the band's color
- **Selected state**: Larger circle with bright outline
- **Hovered state**: Glow effect (expanded, brighter)

### 8. Draw Frequency Labels

Along the bottom edge of the display area:
- 20 Hz, 50 Hz, 100 Hz, 200 Hz, 500 Hz, 1 kHz, 2 kHz, 5 kHz, 10 kHz, 20 kHz
- Color: `axisText()` (#888899)

### 9. Draw Gain Labels

Along the left edge of the display area:
- Labels at each grid line (e.g., -30, -24, -18, ..., +18, +24, +30 for the +/-30 range)
- Color: `axisText()` (#888899)

### 10. Draw Range Buttons

Four clickable buttons in the top-right corner for switching the Y-axis range.

### 11. Draw Tooltip

When a band is being hovered or dragged, a tooltip displays:
- Frequency (formatted: "1.23 kHz" or "440 Hz")
- Gain ("+3.5 dB" or "-2.0 dB")
- Musical note name ("A4", "C#3")

The tooltip follows the mouse cursor with a small offset.

## Timer-Driven Updates

### EqCurveDisplay Timer (30 Hz)

The curve display runs a timer at `Constants::kUITimerHz` (30 Hz):

```cpp
void EqCurveDisplay::timerCallback()
{
    // Update spectrum analyzers (read FFT data from TripleBuffer)
    if (!analyzerFrozen)
    {
        spectrumAnalyzerPre.update (processor.getFFTProcessorPre(), width, height);
        spectrumAnalyzerPost.update (processor.getFFTProcessorPost(), width, height);
    }

    // Recompute magnitude response curves
    updateCurves();  // Evaluates filter responses at 512 frequency points

    repaint();
}
```

The `updateCurves()` method calls:
- `processingChain.computeCompositeMagnitude()` for the overall yellow curve
- `processingChain.computeBandMagnitude()` for each band's individual curve

These evaluate `CascadedFilter::getResponse()` at 512 logarithmically-spaced frequency points from 10 Hz to 30 kHz.

### BottomControlStrip Timer (10 Hz)

The control strip runs at 10 Hz to update the value labels beneath the rotary knobs:

```cpp
void BottomControlStrip::timerCallback()
{
    updateValueLabels();  // Reads current parameter values and formats text
}
```

This updates the frequency label (e.g., "1.23 kHz"), gain label (e.g., "+3.5 dB"), and Q label (e.g., "1.41").

## Spectrum Analyzer Display

### Data Flow

```
Audio Thread:
    FFTProcessor.pushSamples() --> ring buffer --> Hann window --> 4096-pt FFT
    --> magnitude dB --> TripleBuffer.finishWrite() [release]

UI Thread (30 Hz timer):
    TripleBuffer.swapRead() [acquire] --> linear FFT bins
    --> SpectrumAnalyzer.update():
        --> Resample from linear bins to log-frequency display points
        --> Asymmetric smoothing (fast attack, slow decay)
        --> Build juce::Path for stroke and fill
    --> EqCurveDisplay.paint():
        --> Draw pre-EQ spectrum (gray, spectrumPre #40888899)
        --> Draw post-EQ spectrum (blue, spectrumPost #606bcfff)
```

### Log-Frequency Resampling

FFT bins are linearly spaced in frequency (bin spacing = sampleRate / fftSize). The spectrum display uses logarithmically spaced points. The resampling uses linear interpolation between surrounding FFT bins:

```cpp
float binPos = freq / binWidth;           // Fractional bin index
int binLow = (int) binPos;
int binHigh = binLow + 1;
float frac = binPos - (float) binLow;
float dB = fftMagnitudes[binLow] * (1 - frac) + fftMagnitudes[binHigh] * frac;
```

### Asymmetric Smoothing

The spectrum uses asymmetric smoothing for natural visual behavior:

```cpp
if (dB > current)
    // Fast attack (0.5x smoothing coefficient)
    smoothed = current + (1 - smoothingCoeff * 0.5) * (dB - current);
else
    // Slow decay (full smoothing coefficient)
    smoothed = current + (1 - smoothingCoeff) * (dB - current);
```

With the default smoothing coefficient of 0.7:
- **Attack**: effective coefficient = 0.35, fast response to transients
- **Decay**: effective coefficient = 0.7, smooth falloff after transients

### Spectrum Y-Axis Mapping

The spectrum maps +6 dBFS to the top of the display and -100 dBFS to the bottom (106 dB visible range):

```cpp
float spectrumDBtoY (float dB, float height) const
{
    float normalized = (dB - kAnalyzerMinDB) / (kAnalyzerMaxDB - kAnalyzerMinDB);
    // kAnalyzerMinDB = -100, kAnalyzerMaxDB = +6
    return height * (1.0f - normalized);
}
```

Note that this mapping is independent of the EQ curve's gain display range. The spectrum always shows the full -100 to +6 dB range, while the EQ curve Y-axis can be zoomed to +/-3, +/-6, +/-12, or +/-30 dB.

## Color Scheme (ColourPalette)

All colors are defined in `source/ui/LookAndFeel/ColourPalette.h` as inline functions.

### Background and Grid

| Name | Hex | Usage |
|------|-----|-------|
| `background()` | #121228 | Main window background |
| `curveArea()` | #1a1a36 | EQ curve display area |
| `gridLine()` | #2a2a4a | Standard grid lines |
| `gridLineStrong()` | #3a3a5a | Major grid lines (100 Hz, 1k, 10k, 0 dB) |
| `axisText()` | #888899 | Frequency and gain labels |

### Curve Colors

| Name | Hex | Usage |
|------|-----|-------|
| `compositeCurve()` | #ffd93d | Yellow composite EQ response curve (stroke) |
| `compositeFill()` | #18ffd93d | Semi-transparent composite fill |
| `curveFillAbove()` | #20ffffff | Per-band fill above 0 dB |
| `curveFillBelow()` | #20ffffff | Per-band fill below 0 dB |

### Per-Band Colors (12-Color Cycle)

Bands are colored using `ColourPalette::bandColour(bandIndex)`, which cycles through 12 colors via `bandIndex % 12`:

| Index | Color | Hex |
|-------|-------|-----|
| 0 | Red | #ff6b6b |
| 1 | Orange | #ffa06b |
| 2 | Yellow | #ffd93d |
| 3 | Green | #6bff6b |
| 4 | Teal | #6bffd9 |
| 5 | Light Blue | #6bcfff |
| 6 | Blue | #6b7fff |
| 7 | Purple | #a06bff |
| 8 | Pink | #ff6bd9 |
| 9 | Peach | #ff9e6b |
| 10 | Lime | #c8ff6b |
| 11 | Mint | #6bffb0 |

### Stereo Placement Colors

| Placement | Color | Hex |
|-----------|-------|-----|
| Stereo | Yellow | #ffd93d |
| Left | White | #ffffff |
| Right | Red | #ff6b6b |
| Mid | Green | #6bff6b |
| Side | Blue | #6bcfff |

### Spectrum Colors

| Name | Hex | Usage |
|------|-----|-------|
| `spectrumPre()` | #40888899 | Pre-EQ spectrum (gray, 25% alpha) |
| `spectrumPost()` | #606bcfff | Post-EQ spectrum (blue, 38% alpha) |

### UI Elements

| Name | Hex | Usage |
|------|-----|-------|
| `controlBar()` | #0e0e1e | Bottom control strip background |
| `presetBar()` | #161630 | Preset bar area |
| `knobBackground()` | #2a2a4a | Knob body fill |
| `knobForeground()` | #ccccdd | Knob surface color |
| `textBright()` | #eeeeee | Primary text labels |
| `textDim()` | #777788 | Secondary/disabled text |

### Panel Gradients

| Name | Hex | Usage |
|------|-----|-------|
| `panelTop()` | #1e1e3a | Top of vertical panel gradient |
| `panelBottom()` | #0a0a1a | Bottom of vertical panel gradient |

### Knob Metallic

| Name | Hex | Usage |
|------|-----|-------|
| `knobEdge()` | #3a3a5c | Metallic knob edge highlight |
| `knobShadow()` | #400a0a1a | Knob drop shadow |
| `knobIndicator()` | #ffd93d | Value arc accent (yellow) |

### Bevels and Edges

| Name | Hex | Usage |
|------|-----|-------|
| `bevelLight()` | #18ffffff | Light bevel highlight |
| `bevelDark()` | #30000000 | Dark bevel shadow |
| `screenShadow()` | #50000000 | Recessed screen inner shadow |
| `screenHighlight()` | #08ffffff | Recessed screen inner highlight |

### Special

| Name | Hex | Usage |
|------|-----|-------|
| `analogAccent()` | #cc8844 | Warm accent when analog mode is active |

## Custom Look and Feel (AdvancedEqLookAndFeel)

**File**: `source/ui/LookAndFeel/AdvancedEqLookAndFeel.h`

Extends `juce::LookAndFeel_V4` with custom rendering for three widget types:

### Rotary Sliders (`drawRotarySlider`)

Renders metallic knobs with the following visual elements (bottom to top):

1. **Drop shadow**: Offset 1.5px below center, `knobShadow()` color
2. **Knob body**: Radial gradient from `knobEdge()` (center) to `knobBackground()` (edge)
3. **Rim highlight**: Arc from ~10 o'clock to ~2 o'clock, `bevelLight()` with 2x alpha
4. **Track arc**: Full rotary sweep, `knobBackground().darker(0.3)`, 3px stroke
5. **Value arc**: From start angle to current angle, `knobIndicator()` (yellow), 3px stroke
6. **Pointer dot**: 2.5px radius filled circle at the current angle position, `textBright()`
7. **Notch marks**: Short lines at the start and end angles, `axisText()` at 40% alpha

### Buttons (`drawButtonBackground`)

Renders panel-style buttons:

1. **Background gradient**: Vertical gradient from `panelTop()` to `panelBottom()`. When toggled on, uses the button's `buttonOnColourId` as the gradient base.
2. **Top bevel line**: `bevelLight()` (brighter when toggled on)
3. **Bottom bevel line**: `bevelDark()`
4. **Hover highlight**: Semi-transparent white overlay when mouse is over the button
5. **Press darkening**: Semi-transparent black overlay when button is pressed
6. **Outline**: `gridLine()` color, 0.5px rounded rectangle

### Combo Boxes (`drawComboBox`)

Renders dropdown selectors:

1. **Background gradient**: Same vertical gradient as buttons (`panelTop()` to `panelBottom()`)
2. **Top/bottom bevel lines**: Same as buttons
3. **Outline**: `gridLine()` color, 0.5px
4. **Dropdown arrow**: Triangular arrow on the right side, `axisText()` color (or `textBright()` when pressed)

### Constructor Colour Overrides

The constructor sets default JUCE colour IDs to ColourPalette values:

```cpp
setColour (juce::ResizableWindow::backgroundColourId, ColourPalette::background());
setColour (juce::Slider::rotarySliderFillColourId, ColourPalette::knobForeground());
setColour (juce::ComboBox::backgroundColourId, ColourPalette::knobBackground());
setColour (juce::TextButton::buttonColourId, ColourPalette::knobBackground());
setColour (juce::PopupMenu::backgroundColourId, ColourPalette::controlBar());
// ... etc.
```

This ensures that any JUCE components not explicitly overridden still follow the dark theme.

## Communication Between Components

### EqCurveDisplay -> BottomControlStrip

```cpp
// Set up in AdvancedEQEditor constructor:
curveDisplay.onBandSelected = [this] (int band)
{
    controlStrip.setSelectedBand (band);
};
```

When the user clicks a band handle in the curve display, the `onBandSelected` callback notifies the bottom control strip to switch its APVTS attachments to the newly selected band.

### HeaderToolbar -> EqCurveDisplay

```cpp
// Set up in AdvancedEQEditor constructor:
headerToolbar.onFreezeToggled = [this] (bool frozen)
{
    curveDisplay.setAnalyzerFrozen (frozen);
};
```

When the user toggles the freeze button in the header, the callback tells the curve display to stop (or resume) reading new FFT data from the TripleBuffers.

### PresetManager -> HeaderToolbar

The `HeaderToolbar` implements `PresetManager::Listener`:

```cpp
void HeaderToolbar::presetChanged() override
{
    updatePresetName();  // Update the displayed preset name button text
}
```

When a preset is applied, navigated, saved, or deleted, the PresetManager notifies all listeners, causing the header to update its preset name display.
