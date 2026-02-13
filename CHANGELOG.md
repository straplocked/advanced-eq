# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- Open source release under GPL-3.0-or-later

## [0.1.0] - 2026-02-12

### Added
- 24-band parametric EQ with interactive curve display
- 9 filter types: Bell, Low Shelf, High Shelf, Low Pass, High Pass, Notch, Band Pass, Tilt Shelf, Flat Tilt
- 10 slope options: 6, 12, 18, 24, 36, 48, 72, 96 dB/oct, and Brickwall
- Cytomic TPT state variable filters with Butterworth pole placement
- 3 processing modes: Zero Latency (IIR), Natural Phase (mixed-phase FIR), Linear Phase (symmetric FIR)
- Oversampling (1x, 2x, 4x) for reduced aliasing
- Auto gain compensation
- Per-band dynamic EQ with envelope follower and bidirectional gain modulation
- Real-time spectrum analyzer with pre/post EQ views, freeze mode, and configurable resolution
- Spectrum tilt compensation for pink noise reference display
- EQ Match: capture reference and current spectra, auto-fit bands to match the difference
- Spectrum Grab: click near analyzer peaks to snap new bands to spectral content
- Per-band stereo placement: Stereo, Left, Right, Mid, Side
- Surround support: mono, stereo, 5.1, 7.1, and 7.1.4 (Atmos) bus layouts
- Phase inversion toggle
- Undo/redo with Cmd+Z / Cmd+Shift+Z keyboard shortcuts
- A/B comparison with state snapshot toggle and copy
- Band solo monitor mode
- MIDI Learn with CC-to-parameter mapping
- Inter-instance collision detection
- 57 factory presets across Instruments, Bus, and Mastering categories
- User preset save/load with categories and JSON import/export
- Piano overlay with note markers on frequency axis
- Output level meter with peak and RMS display
- Window size presets: Mini, Small, Medium, Large, XL
- Analog mode with proportional Q, soft saturation, channel crosstalk, and noise floor
- Hover help tooltips for all controls
- 298 automatable parameters (24 bands x 12 + 10 global)
- VST3, AU, CLAP, and Standalone formats (AAX optional with SDK)

[Unreleased]: https://github.com/straplocked/advanced-eq/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/straplocked/advanced-eq/releases/tag/v0.1.0
