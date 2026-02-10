# Advanced EQ

> Open-source parametric equalizer plugin inspired by FabFilter Pro-Q 3

## Features

- **24-band parametric EQ** with interactive curve display
- **9 filter types:** Bell, Low Shelf, High Shelf, Low Pass, High Pass, Notch, Band Pass, Tilt Shelf, Flat Tilt
- **10 slope options:** 6, 12, 18, 24, 36, 48, 72, 96 dB/oct, and Brickwall
- **Real-time spectrum analyzer** with pre/post EQ views and freeze mode
- **Per-band stereo placement:** Stereo, Left, Right, Mid, Side
- **Analog mode** with proportional Q, soft saturation, channel crosstalk, and noise floor
- **57 factory presets** across Instruments, Bus, and Mastering categories, plus user preset save/load
- **Interactive curve UI** -- click to create bands, drag to adjust, scroll to change Q, double-click to remove
- **Studio rack gear visual design** with dark theme

## Plugin Formats

| Format     | macOS | Windows | Linux |
|------------|-------|---------|-------|
| VST3       | Yes   | Yes     | Yes   |
| AU         | Yes   | --      | --    |
| CLAP       | Yes   | Yes     | Yes   |
| Standalone | Yes   | Yes     | Yes   |

## Building

### Requirements

- CMake 3.25+
- C++20-compatible compiler (Clang 13+, GCC 11+, MSVC 2022+)
- Git (for submodules)

### Quick Start

```bash
git clone --recursive https://github.com/straplocked/advanced-eq.git
cd advanced-eq
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

### macOS with 10.15 SDK

If targeting x86_64 only (e.g., macOS 10.15 SDK without arm64 support):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build --config Release -j
```

### CLAP Format

CLAP support requires the `clap-juce-extensions` submodule. If you cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

### Build Options

| Option                      | Default | Description              |
|-----------------------------|---------|--------------------------|
| `ADVANCED_EQ_BUILD_TESTS`   | OFF     | Build unit tests         |
| `CMAKE_OSX_ARCHITECTURES`   | --      | Target architecture(s)   |
| `AAX_SDK_PATH`              | --      | Path to AAX SDK for AAX builds |

## Architecture

```
Audio Input
    |
    v
[Pre-EQ FFT] -------> Spectrum Analyzer (UI)
    |
    v
ProcessingChain
    |--- Pass 1: Stereo bands (both channels)
    |--- Pass 2: Left-only bands
    |--- Pass 3: Right-only bands
    '--- Pass 4: Mid/Side encode -> Mid bands -> Side bands -> decode
    |
    v
[Analog Mode]  saturation / crosstalk / noise floor
    |
    v
[Output Gain]
    |
    v
[Post-EQ FFT] ------> Spectrum Analyzer (UI)
    |
    v
Audio Output
```

**DSP engine:** Cytomic TPT (Topology-Preserving Transform) state variable filters, cascaded for higher-order slopes with Butterworth pole placement.

**Parameter system:** 298 parameters (24 bands x 12 + 10 global) with lock-free atomic reads on the audio thread via `BandParameterAttachment`.

**UI:** Timer-driven at 30 Hz. Lock-free `TripleBuffer` transfers FFT data from the audio thread to the UI thread.

See [docs/architecture.md](docs/architecture.md) for full details.

## Project Structure

```
advanced-eq/
├── Source/
│   ├── PluginProcessor.h/cpp    # Audio processor entry point
│   ├── PluginEditor.h/cpp       # UI editor entry point
│   ├── dsp/                     # DSP engine
│   │   ├── SVFFilter            # Cytomic TPT state variable filter
│   │   ├── CascadedFilter       # Multi-section cascading for slopes
│   │   ├── EqBand               # Single EQ band with smoothing
│   │   ├── ProcessingChain      # Orchestrates all 24 bands
│   │   ├── FilterTypes          # Enums and slope utilities
│   │   ├── BiquadCoefficients   # Biquad coefficient math
│   │   └── MidSideProcessor     # Mid/Side encode/decode
│   ├── analyzer/                # Spectrum analysis
│   │   ├── FFTProcessor         # Windowed FFT on audio thread
│   │   ├── SpectrumAnalyzer     # Log-frequency resampling for UI
│   │   └── TripleBuffer         # Lock-free audio-to-UI transfer
│   ├── parameters/              # Parameter management
│   │   ├── ParameterIDs         # ID naming conventions
│   │   ├── ParameterLayout      # APVTS parameter definitions
│   │   └── BandParameterAttachment  # Lock-free parameter bridge
│   ├── presets/                 # Preset system
│   │   ├── PresetManager        # Load/save/navigate presets
│   │   ├── FactoryPresets       # 57 built-in presets
│   │   └── PresetData           # Preset data structures
│   ├── ui/                      # User interface
│   │   ├── components/
│   │   │   ├── EqCurveDisplay   # Interactive EQ curve + spectrum
│   │   │   ├── HeaderToolbar    # Presets, mode, analyzer controls
│   │   │   ├── BottomControlStrip  # Band parameter knobs
│   │   │   └── BandControlPanel # Filter type/slope selector
│   │   └── LookAndFeel/
│   │       ├── AdvancedEqLookAndFeel  # Custom widget rendering
│   │       └── ColourPalette    # Centralized color scheme
│   └── utils/                   # Utilities
│       ├── Constants            # Global constants
│       ├── FrequencyUtils       # MIDI/frequency conversions
│       └── MathUtils            # dB, coordinate mapping
├── JUCE/                        # JUCE 7.0.12 (submodule)
├── libs/clap-juce-extensions/   # CLAP support (submodule)
├── docs/                        # Architecture documentation
├── tests/                       # Unit tests
├── cmake/                       # Platform build config
└── resources/                   # Fonts, icons, presets
```

## License

This project is licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html) or later.

See [LICENSE](LICENSE) for the full text.
