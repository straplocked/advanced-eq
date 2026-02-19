# CLAUDE.md — Advanced EQ

## Project Overview

Advanced EQ is a professional-grade open-source parametric equalizer audio plugin built with **C++20** and **JUCE 7.0.12**. It provides 24 bands, 9 filter types, 10 slopes, dynamic EQ, real-time spectrum analysis, EQ matching, Mid/Side processing, and analog emulation. Ships as VST3, AU, CLAP, AAX, and Standalone.

**License**: GPL-3.0-or-later
**Version**: 0.1.0

## Build Commands

```bash
# Configure (from repo root)
cmake -B build

# Build
cmake --build build

# Build with tests
cmake -B build -DADVANCED_EQ_BUILD_TESTS=ON
cmake --build build

# Linux dependencies (install first)
sudo apt-get install libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libgl1-mesa-dev libxcomposite-dev
```

Build artifacts land in `build/AdvancedEQ_artefacts/`. On macOS, AU/VST3 are auto-copied to system plugin directories.

No linter or formatter commands. Code quality enforced by compiler warnings (`-Wall -Wextra -Wpedantic` / `/W4`).

## Architecture

### Source Layout

```
source/
├── PluginProcessor.h/cpp    # Audio entry point, APVTS, state, A/B compare
├── PluginEditor.h/cpp       # UI editor, component aggregation, window sizing
├── dsp/                     # DSP engine
│   ├── SVFFilter            # Cytomic TPT state variable filter (all shapes)
│   ├── CascadedFilter       # Multi-section cascading for slopes (up to 8x)
│   ├── EqBand               # Single band: stereo pair + smoothing + dynamic
│   ├── ProcessingChain      # Four-pass stereo routing orchestrator
│   ├── BiquadCoefficients   # Audio EQ Cookbook coefficients
│   ├── DynamicProcessor     # Per-band dynamics (header-only)
│   ├── EnvelopeFollower     # Attack/release envelope (header-only)
│   ├── EQMatcher            # Spectral matching algorithm (header-only)
│   ├── LinearPhaseEngine    # Symmetric FIR convolution
│   ├── NaturalPhaseEngine   # Mixed-phase FIR processing
│   ├── MidSideProcessor     # M/S encode/decode (header-only)
│   ├── SpectralDynamicsProcessor
│   └── FilterTypes          # Enums: FilterType, StereoPlacement, ProcessingMode, SlopeInfo
├── analyzer/                # Spectrum analysis
│   ├── FFTProcessor         # Ring buffer, FFT, magnitude computation
│   ├── SpectrumAnalyzer     # Log-frequency resampling, peak detection
│   └── TripleBuffer         # Lock-free SPSC template
├── parameters/              # Parameter system (298 total: 24 bands × 12 + 10 global)
│   ├── ParameterIDs         # ID constants, string arrays for UI
│   ├── ParameterLayout      # APVTS layout factory
│   └── BandParameterAttachment  # Atomic pointer bridge (lock-free reads)
├── presets/                 # Preset management
│   ├── PresetManager        # Load/save/navigate with listener pattern
│   ├── FactoryPresets       # 57 hardcoded presets
│   ├── PresetData           # Serialization structures
│   └── PresetMetadata       # Category/metadata types
├── midi/MidiLearnManager    # CC-to-parameter MIDI learn
├── ipc/InstanceRegistry     # Cross-instance band collision detection
├── ui/                      # UI components
│   ├── EqCurveDisplay       # Interactive curve + spectrum overlay + spectrum grab
│   ├── HeaderToolbar        # Presets, mode, analog, analyzer, gain
│   ├── BottomControlStrip   # Band parameter editing controls
│   ├── BandControlPanel, EQMatchPanel, LevelMeter, PianoOverlay
│   ├── PresetBrowserPanel, InstanceListPanel, HelpOverlay
│   └── LookAndFeel/         # AdvancedEqLookAndFeel, ColourPalette, Fonts
└── utils/
    ├── Constants.h          # 23 compile-time constants
    ├── FrequencyUtils       # Hz ↔ pixel conversions
    └── MathUtils            # dB conversions, interpolation
```

### Threading Model

- **Audio thread** (`processBlock`): Zero allocations, no locks. Reads parameters via `std::atomic<float>*` (288 band + 10 global). All DSP uses `juce::SmoothedValue`. FFT data published via `TripleBuffer` with release fence.
- **Message thread** (UI): 30 Hz timer. Reads spectrum data via `TripleBuffer` with acquire fence. Computes 512-point curves. Writes parameters via APVTS.

### Processing Pipeline

Four-pass routing in `ProcessingChain`:
1. Stereo bands → full stereo buffer
2. Left-only bands → left channel
3. Right-only bands → right channel
4. Mid/Side bands → M/S encode, process, decode

### DSP Class Pattern

All DSP classes follow:
```cpp
void prepare( double sampleRate, int samplesPerBlock );
void reset();
void process( juce::AudioBuffer<float>& buffer );
```

## Code Style

- **Indentation**: 4 spaces, no tabs
- **Braces**: Allman style (opening brace on new line for functions/classes)
- **Spacing inside parens**: `if ( condition )`, `foo( bar, baz )`
- **File naming**: PascalCase matching class name (`EqBand.h`, `EqBand.cpp`)
- **Includes**: JUCE via module includes, project headers via relative paths
- **Header-only**: Used for simple processors (`MidSideProcessor.h`, `DynamicProcessor.h`, `EnvelopeFollower.h`, `EQMatcher.h`)

## Key Conventions

- **Parameter attachments**: Always use APVTS attachments (`SliderAttachment`, `ComboBoxAttachment`, `ButtonAttachment`). Never use `Slider::Listener` for parameter-connected controls.
- **Audio thread safety**: No allocations, no mutexes, no I/O on the audio thread. Parameters via `std::atomic<float>*` in `BandParameterAttachment`.
- **Lock-free data transfer**: `TripleBuffer` for audio→UI spectrum data.
- **State persistence**: Binary XML serialization via `getStateInformation`/`setStateInformation`.
- **User presets**: XML files stored per-platform (`~/Library/Application Support/Advanced EQ/User Presets/` on macOS, `~/.config/Advanced EQ/User Presets/` on Linux, `%APPDATA%/Advanced EQ/User Presets/` on Windows).

## How to Add Things

**New filter type**: Add enum in `FilterTypes.h` → coefficients in `SVFFilter::updateCoefficients()` → name in `ParameterIDs.h` string arrays → update `Constants::kNumFilterTypes`

**New per-band parameter**: Add ID in `ParameterIDs.h` → range/default in `Constants.h` → APVTS layout in `ParameterLayout.cpp` (×24 bands) → atomic pointer in `BandParameterAttachment.h` → use in `EqBand`/`ProcessingChain`

**New factory preset**: Add to the appropriate getter in `FactoryPresets.cpp`

## CI/CD

- **build.yml**: Runs on push/PR to `main`. Builds on macOS (macos-14), Windows (VS 2022), Linux (ubuntu-latest). Uploads plugin artifacts.
- **release.yml**: Triggered by `v*` tags. Validates version against CMakeLists.txt, builds platform installers (DMG, NSIS .exe, tar.gz), creates GitHub Release.

## Documentation

Detailed architecture docs live in `docs/`:
- `architecture.md` — System overview, threading, state, constants
- `signal-flow.md` — Complete processBlock() flow
- `filter-engine.md` — SVF implementation, Butterworth pole placement
- `parameter-system.md` — APVTS layout, band/global parameters
- `preset-system.md` — Factory/user preset architecture
- `ui-architecture.md` — Component hierarchy
- `phase-5-filter-slopes.md` — Slope cascade implementation
- `phase-6-spectrum-analyzer.md` — FFT pipeline, TripleBuffer
- `phase-7-mid-side-stereo.md` — M/S encoding/decoding
