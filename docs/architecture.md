# System Architecture Overview

## Project Summary

Advanced EQ is an open-source parametric equalizer plugin modeled after FabFilter Pro-Q 3. It provides a professional-grade 24-band EQ with real-time spectrum analysis, per-band stereo routing (including Mid/Side), an analog emulation mode, and a factory/user preset system.

- **License**: GPL-3.0-or-later
- **Framework**: JUCE 7.0.12
- **Language**: C++20
- **Build system**: CMake 3.25+
- **Plugin formats**: VST3, AU (macOS), CLAP, Standalone
- **Version**: 0.1.0

## High-Level Architecture

The plugin follows the standard JUCE audio plugin pattern: a processor (`AdvancedEQProcessor`) that handles all audio-thread work and state persistence, and an editor (`AdvancedEQEditor`) that provides the GUI on the message thread. Communication between the two is entirely lock-free.

```
                      +--------------------------+
                      |    Host (DAW / CLAP)     |
                      +------+--------+----------+
                             |        |
                   processBlock()   createEditor()
                             |        |
                      +------v--------v----------+
                      |   AdvancedEQProcessor     |
                      |   (source/PluginProcessor)|
                      +--+---+---+---+---+-------+
                         |   |   |   |   |
          +--------------+   |   |   |   +----------------+
          |                  |   |   |                    |
   +------v------+   +------v---v---v------+   +--v------v------+
   | APVTS       |   | ProcessingChain     |   | FFTProcessor   |
   | (298 params)|   | (24 x EqBand)       |   | (pre & post)   |
   +------+------+   +---+---+---+---------+   +------+---------+
          |               |   |   |                    |
   +------v------+   +---v-+ | +-v-----------+  +-----v--------+
   | BandParam   |   |EqBand| | |MidSideProc.|  |TripleBuffer  |
   | Attachment   |   +--+--+ | +-------------+  |(lock-free)   |
   | (x24)       |      |    |                   +-----+--------+
   +--------------+   +--v----v------+                  |
                      |CascadedFilter|           +------v--------+
                      | (L and R)    |           |SpectrumAnalyzer|
                      +------+-------+           |(UI thread)     |
                             |                   +----------------+
                      +------v-------+
                      |  SVFFilter   |
                      |  (x8 max)    |
                      +--------------+
```

## Module Dependency Graph

```
AdvancedEQProcessor
|-- ProcessingChain
|   |-- EqBand (x24)
|   |   |-- CascadedFilter (filterL, filterR)
|   |   |   +-- SVFFilter (x8 max per cascade)
|   |   +-- juce::SmoothedValue (freq, gain, Q)
|   +-- MidSideProcessor (encode/decode)
|-- FFTProcessor (pre-EQ instance)
|   |-- juce::dsp::FFT (order 12, size 4096)
|   |-- juce::dsp::WindowingFunction (Hann)
|   +-- TripleBuffer<float> (magnitude output)
|-- FFTProcessor (post-EQ instance)
|   +-- (same structure as above)
|-- BandParameterAttachment (x24)
|   +-- ParameterIDs (band_XX_suffix convention)
|-- PresetManager
|   +-- FactoryPresets (57 hardcoded presets)
+-- APVTS (ParameterLayout: 288 band + 10 global = 298 params)

AdvancedEQEditor
|-- AdvancedEqLookAndFeel
|   +-- ColourPalette
|-- HeaderToolbar
|   |-- PresetManager::Listener
|   +-- APVTS Attachments (analog, analyzer, process mode, output gain)
|-- EqCurveDisplay
|   |-- SpectrumAnalyzer (pre-EQ)
|   |-- SpectrumAnalyzer (post-EQ)
|   +-- SVFFilter (preview cursor)
+-- BottomControlStrip
    +-- APVTS Attachments (type, slope, stereo, freq, gain, Q)
```

## Source File Layout

```
source/
|-- PluginProcessor.h / .cpp        Main audio processor
|-- PluginEditor.h / .cpp           Main editor window
|-- dsp/
|   |-- FilterTypes.h               Enums: FilterType, StereoPlacement, ProcessingMode, SlopeInfo
|   |-- SVFFilter.h / .cpp          Cytomic TPT state variable filter (1st and 2nd order)
|   |-- CascadedFilter.h / .cpp     Cascaded SVF sections for higher-order slopes
|   |-- EqBand.h / .cpp             Single EQ band with stereo filter pair + smoothing
|   |-- ProcessingChain.h / .cpp    Top-level DSP: 4-pass routing, analog mode, output gain
|   |-- MidSideProcessor.h          Inline M/S encode/decode
|   +-- BiquadCoefficients.h / .cpp Audio EQ Cookbook biquad (legacy/utility)
|-- analyzer/
|   |-- FFTProcessor.h / .cpp       Ring buffer + Hann window + 4096-pt FFT
|   |-- SpectrumAnalyzer.h / .cpp   Log-frequency resampling + path builder
|   +-- TripleBuffer.h              Lock-free SPSC triple buffer template
|-- parameters/
|   |-- ParameterIDs.h              ID string constants + name arrays
|   |-- ParameterLayout.h / .cpp    APVTS layout factory (298 parameters)
|   +-- BandParameterAttachment.h   Atomic param caching for audio thread
|-- presets/
|   |-- PresetData.h                BandPreset and Preset structs
|   |-- FactoryPresets.h / .cpp     57 built-in presets
|   +-- PresetManager.h / .cpp      Load, save, navigate, notify
|-- ui/
|   |-- components/
|   |   |-- EqCurveDisplay.h / .cpp Interactive curve editor + spectrum overlay
|   |   |-- HeaderToolbar.h / .cpp  Top bar: presets, mode, analog, analyzer, gain
|   |   |-- BottomControlStrip.h / .cpp  Band parameter editing knobs + selectors
|   |   +-- BandControlPanel.h / .cpp    (Extended band controls)
|   +-- LookAndFeel/
|       |-- ColourPalette.h         All UI color definitions
|       +-- AdvancedEqLookAndFeel.h Custom LookAndFeel_V4 subclass
+-- utils/
    |-- Constants.h                 All magic numbers (band count, FFT size, ranges, etc.)
    |-- MathUtils.h                 dB/gain conversion, frequency/pixel mapping
    +-- FrequencyUtils.h            MIDI note <-> frequency, note names, formatting
```

## Threading Model

### Audio Thread

The audio thread executes `AdvancedEQProcessor::processBlock()` once per buffer. The entire audio path is lock-free and allocation-free:

1. **Parameter reads**: `BandParameterAttachment::getCurrentParameters()` loads 12 `std::atomic<float>` values per band via relaxed atomic loads. Global parameters (`output_gain`, `analog_mode`, `analyzer_on`, `analyzer_pre`) are read from cached `std::atomic<float>*` pointers.

2. **DSP processing**: `ProcessingChain::process()` runs the four-pass stereo routing model. Each `EqBand` uses `juce::SmoothedValue` for zipper-free parameter transitions. When smoothing is active, coefficient updates happen per-sample; when stable, filters process in blocks for efficiency.

3. **FFT data production**: `FFTProcessor::pushSamples()` accumulates mono-averaged samples in a ring buffer. When 4096 samples are collected, a Hann-windowed FFT is performed and magnitudes (in dB) are written to a `TripleBuffer`. The `finishWrite()` call uses `memory_order_release` to publish the data.

### Message Thread (UI)

The message thread runs JUCE's event loop and handles all painting and user interaction:

1. **Spectrum updates**: `EqCurveDisplay::timerCallback()` runs at 30 Hz. It calls `SpectrumAnalyzer::update()` which executes `TripleBuffer::swapRead()` with `memory_order_acquire` to obtain the latest FFT magnitudes. The magnitudes are resampled from linear FFT bins to logarithmic display coordinates.

2. **Curve computation**: The timer callback also calls `ProcessingChain::computeCompositeMagnitude()` and `computeBandMagnitude()` to evaluate filter frequency responses at 512 points for the UI curve rendering.

3. **Parameter writes**: User interactions (dragging band handles, turning knobs) write to the APVTS via `setValueNotifyingHost()`, which atomically updates the parameter values that the audio thread reads on the next `processBlock()`.

### Shared State Summary

| Data | Writer | Reader | Mechanism |
|------|--------|--------|-----------|
| Band parameters | Message thread (APVTS) | Audio thread (BandParameterAttachment) | `std::atomic<float>` |
| Global parameters | Message thread (APVTS) | Audio thread (cached raw pointers) | `std::atomic<float>` |
| FFT magnitudes | Audio thread (FFTProcessor) | Message thread (SpectrumAnalyzer) | `TripleBuffer` (lock-free) |
| Filter coefficients | Audio thread (ProcessingChain) | Message thread (getResponse) | Read-only from UI; coefficients are stable between processBlock calls |

**No mutexes, spinlocks, or heap allocations exist on the audio path.**

## State Persistence

### DAW Session State

The plugin's full parameter state is serialized to and from binary via JUCE's `AudioProcessorValueTreeState`:

- `getStateInformation()`: Copies the APVTS ValueTree, converts to XML, and writes binary via `copyXmlToBinary()`.
- `setStateInformation()`: Parses the binary back to XML, validates the tag name matches, and calls `apvts.replaceState()`.

This stores all 298 parameters (24 bands x 12 + 10 global) in the DAW's project file.

### User Presets

User presets are saved as standalone XML files containing the full APVTS state dump. Platform-specific storage locations:

| Platform | Directory |
|----------|-----------|
| macOS | `~/Library/Application Support/Advanced EQ/User Presets/` |
| Windows | `%APPDATA%/Advanced EQ/User Presets/` |
| Linux | `~/.config/Advanced EQ/User Presets/` |

## Configuration Constants

All magic numbers are centralized in `source/utils/Constants.h`:

| Constant | Value | Description |
|----------|-------|-------------|
| `kMaxBands` | 24 | Maximum number of EQ bands |
| `kMinFrequency` | 10.0 Hz | Minimum parameter frequency |
| `kMaxFrequency` | 30000.0 Hz | Maximum parameter frequency |
| `kDefaultFrequency` | 1000.0 Hz | Default band center frequency |
| `kMinGain` / `kMaxGain` | -30 / +30 dB | Gain parameter range |
| `kDefaultGain` | 0.0 dB | Default band gain |
| `kMinQ` / `kMaxQ` | 0.1 / 30.0 | Q factor range |
| `kDefaultQ` | 1.0 | Default Q factor |
| `kFFTOrder` | 12 | FFT order (2^12 = 4096 points) |
| `kFFTSize` | 4096 | FFT frame size |
| `kAnalyzerMinDB` | -100.0 dB | Spectrum display floor |
| `kAnalyzerMaxDB` | +6.0 dB | Spectrum display ceiling |
| `kDefaultAnalyzerSmoothing` | 0.7 | Default spectrum decay coefficient |
| `kUITimerHz` | 30 | UI refresh rate in Hz |
| `kCurveResolution` | 512 | Number of frequency points for curve rendering |
| `kDefaultEditorWidth` | 1000 px | Default editor width |
| `kDefaultEditorHeight` | 600 px | Default editor height |

## Build Configuration

### CMake

The project uses CMake 3.25+ with C++20 required:

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

JUCE is included as a subdirectory. CLAP support is optional via the `clap-juce-extensions` submodule.

### Compiler Flags

- **Non-MSVC**: `-ffast-math` for DSP performance
- **MSVC**: `/fp:fast`
- **macOS**: Build with `-DCMAKE_OSX_ARCHITECTURES=x86_64` on 10.15 SDK (no arm64 support)

### Plugin Metadata

| Field | Value |
|-------|-------|
| Company | AdvancedEQ |
| Bundle ID | com.advancedeq.advancedeq |
| Manufacturer code | AEqP |
| Plugin code | AEq1 |
| VST3 categories | Fx, EQ, Analyzer |
| AU type | kAudioUnitType_Effect |

## Key Design Decisions

1. **JUCE 7.0.12**: Pinned to 7.x because JUCE 8.x requires macOS 11.1 SDK; this project targets 10.15 SDK. Font API uses `g.setFont(24.0f)` rather than JUCE 8's `FontOptions`.

2. **Cytomic SVF over biquad**: The TPT (Topology-Preserving Transform) structure is inherently stable under fast parameter modulation. All filter types share the same processing tick -- only the mix coefficients (m0, m1, m2) differ between Bell, Shelf, HP, LP, Notch, etc.

3. **Triple buffer over ring buffer for FFT data**: A lock-free triple buffer allows the audio thread to overwrite old frames without blocking, and the UI thread always reads the most recent complete frame. Dropped frames are acceptable for visualization.

4. **Four-pass stereo routing**: The ProcessingChain processes bands in four passes (Stereo, Left, Right, Mid/Side) rather than per-sample routing decisions, minimizing branching in the hot path and allowing M/S encode/decode to happen only once per block.

5. **Atomic parameter bridge**: `BandParameterAttachment` caches `std::atomic<float>*` raw parameter pointers at initialization, avoiding any APVTS lookup overhead on the audio thread. Each `processBlock()` reads 288 atomic values (24 bands x 12 params) plus 4 globals.

6. **SmoothedValue for parameter transitions**: Frequency, gain, and Q use `juce::SmoothedValue` with a 20ms ramp time. During smoothing, filters are updated per-sample with new coefficients. Once the target is reached, block processing resumes for efficiency.
