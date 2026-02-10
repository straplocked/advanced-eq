# Phase 6: Spectrum Analyzer

## Overview

Phase 6 adds a real-time spectrum analyzer that displays the frequency content of the audio signal behind the EQ curves. It shows either the pre-EQ or post-EQ spectrum, providing essential visual feedback when mixing.

## Data Flow

```
Audio Thread                        UI Thread (30Hz)
---                                 ---
processBlock()                      timerCallback()
 |-- PRE-EQ: fftPre.pushSamples()    |-- spectrumPre.update()
 |    `-- ring buffer -> FFT ->       |    `-- TripleBuffer.swapRead()
 |        TripleBuffer.finishWrite()  |    `-- smooth + resample -> Path
 |                                    |-- spectrumPost.update()
 |-- processingChain.process()        `-- repaint()
 |
 `-- POST-EQ: fftPost.pushSamples()  paint():
      `-- ring buffer -> FFT ->         grid -> spectrum -> bandCurves
          TripleBuffer.finishWrite()      -> composite -> handles
```

## Architecture

### TripleBuffer (lock-free)

A header-only template (`Source/analyzer/TripleBuffer.h`) providing lock-free single-producer/single-consumer data transfer between audio and UI threads:

- 3 vectors + 3 atomic indices (write/mid/read) + atomic dirty flag
- Writer (audio thread): `getWritePointer()` / `finishWrite()` - swaps write and mid indices
- Reader (UI thread): `swapRead()` / `getReadPointer()` - swaps read and mid indices
- Reader always gets the most recently completed write; frames may be skipped (fine for visualization)
- Zero runtime allocation - all buffers sized in `resize()`

### FFTProcessor (audio thread)

`Source/analyzer/FFTProcessor.h/.cpp` - Accumulates audio samples and performs windowed FFT:

- Ring buffer of 4096 floats, hop size = 2048 (50% overlap, ~21 FFTs/sec at 44.1kHz)
- `pushSamples()`: averages L+R to mono, fills ring buffer, triggers FFT when full
- `performFFT()`: Hann window -> `juce::dsp::FFT::performFrequencyOnlyForwardTransform()` -> normalize (divides by fftSize, x2 for single-sided) -> convert to dB -> write to TripleBuffer
- Zero allocations in audio path

### SpectrumAnalyzer (UI thread)

`Source/analyzer/SpectrumAnalyzer.h/.cpp` - Reads FFT data and builds display paths:

- `prepare()`: pre-computes log-spaced display frequencies
- `update()`: swapRead from TripleBuffer -> linear interpolation of FFT bins at log frequencies -> asymmetric exponential smoothing (fast attack, slow decay) -> builds `juce::Path`
- Y mapping: 0 dBFS aligns with the EQ curve's 0 dB gain line (center), same pixels-per-dB scale (60 dB over full height)
- Outputs both stroke path (top edge) and fill path (closed to bottom)

## Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `analyzer_on` | Bool | on/off | on | Enable/disable analyzer |
| `analyzer_pre` | Bool | on/off | off | Show pre-EQ (off = post-EQ) |
| `analyzer_speed` | Float | 0.0-1.0 | 0.7 | Smoothing coefficient |
| `analyzer_res` | Choice | Low/Normal/High/Max | High | Reserved for future FFT size switching |

## Display

- Spectrum is painted between grid lines and EQ band curves (correct Z-order)
- Pre-EQ spectrum uses `ColourPalette::spectrumPre()` (muted gray, 25% opacity)
- Post-EQ spectrum uses `ColourPalette::spectrumPost()` (blue, 38% opacity)
- Top edge has a slightly brighter stroke for definition
- Spectrum decays smoothly to silence when audio stops (asymmetric smoothing)

## Files Changed

### New Files
- `Source/analyzer/TripleBuffer.h` - Lock-free triple buffer template
- `Source/analyzer/FFTProcessor.h` - FFT processor header
- `Source/analyzer/FFTProcessor.cpp` - FFT processor implementation
- `Source/analyzer/SpectrumAnalyzer.h` - Spectrum analyzer header
- `Source/analyzer/SpectrumAnalyzer.cpp` - Spectrum analyzer implementation

### Modified Files
- `Source/parameters/ParameterLayout.cpp` - Added `kAnalyzerSpd` and `kAnalyzerRes` parameters
- `Source/PluginProcessor.h` - Added FFTProcessor members and getters
- `Source/PluginProcessor.cpp` - Wired FFT capture in prepareToPlay/releaseResources/processBlock
- `Source/ui/components/EqCurveDisplay.h` - Added SpectrumAnalyzer members and drawSpectrum declaration
- `Source/ui/components/EqCurveDisplay.cpp` - Integrated spectrum display with correct paint order
