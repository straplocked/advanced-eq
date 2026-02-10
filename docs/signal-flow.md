# DSP Signal Flow

This document traces the complete audio processing path from the moment a buffer enters `processBlock()` to the moment it exits, including the FFT analysis side-chains and all analog-mode effects.

## processBlock() Entry Point

**File**: `source/PluginProcessor.cpp`, line 68

```cpp
void AdvancedEQProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
```

### Step 1: Flush Denormals

```cpp
juce::ScopedNoDenormals noDenormals;
```

Sets the CPU's denormal-flush-to-zero (FTZ) and denormals-are-zero (DAZ) flags for the duration of the block. This prevents the IIR filter feedback paths from generating denormalized floating-point numbers, which cause severe performance penalties on x86 CPUs.

### Step 2: Clear Unused Output Channels

```cpp
for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear (i, 0, buffer.getNumSamples());
```

If the host provides more output channels than input channels, the extra channels are zeroed to prevent garbage output.

### Step 3: Read Parameters (Lock-Free)

```cpp
for (int i = 0; i < Constants::kMaxBands; ++i)
{
    auto params = bandAttachments[i].getCurrentParameters();
    processingChain.updateBand (i, params);
}
processingChain.setOutputGain (rawOutputGain->load());
processingChain.setAnalogMode (rawAnalogMode->load() >= 0.5f);
```

All 24 bands read their 12 parameters from cached `std::atomic<float>*` pointers via `BandParameterAttachment::getCurrentParameters()`. This produces a `BandParameters` struct per band. Global parameters (output gain, analog mode) are read from similarly cached raw pointers. No locks, no allocations.

Each `EqBand::updateParameters()` call:
- Sets the `SmoothedValue` target for frequency, gain, and Q (20ms ramp)
- Immediately applies type, slope, and stereo placement changes
- In analog mode for Bell filters: applies proportional Q scaling (`q *= 1.0 + 0.4 * |gain| / 30.0`)
- Forwards parameters to both `CascadedFilter` instances (filterL and filterR)

### Step 4: Pre-EQ FFT Analysis

```cpp
bool analyzerOn = rawAnalyzerOn->load() >= 0.5f;
if (analyzerOn)
    fftProcessorPre.pushSamples (buffer, buffer.getNumSamples());
```

When the analyzer is enabled, the current buffer is pushed into the pre-EQ `FFTProcessor`:

1. **Channel averaging**: All channels are averaged to mono: `sample = (L + R) / numChannels`
2. **Ring buffer accumulation**: Mono samples are written sequentially into a 4096-sample ring buffer
3. **FFT trigger**: When the ring buffer is full (4096 samples accumulated):
   - Copy ring buffer into FFT work buffer
   - Apply Hann window via `juce::dsp::WindowingFunction`
   - Execute forward FFT via `juce::dsp::FFT` (order 12, 4096 points)
   - `performFrequencyOnlyForwardTransform()` returns magnitude values
   - Normalize: divide by FFT size, multiply by 2 for single-sided spectrum (except DC and Nyquist bins)
   - Convert to dB: `20 * log10(max(magnitude, 1e-10))`
   - Clamp to analyzer floor: `max(dB, -100.0)`
   - Write 2049 magnitude bins to `TripleBuffer` write buffer
   - Call `finishWrite()` (release fence) to publish data
4. **50% overlap**: After FFT, the second half of the ring buffer is shifted to the first half, and the write position resets to `hopSize` (2048). This provides 50% overlap between consecutive FFT frames.

### Step 5: EQ Processing (ProcessingChain::process)

```cpp
processingChain.process (buffer);
```

This is the core DSP operation. The processing chain implements a **four-pass stereo routing model**:

#### Pass 1: Stereo Bands

```
for each band where stereoPlacement == Stereo:
    band.process(buffer, numSamples)  // processes both L and R
```

Bands configured for stereo operation process both channels identically through independent `CascadedFilter` instances (filterL for the left channel, filterR for the right channel).

#### Pass 2: Left-Only Bands

```
for each band where stereoPlacement == Left:
    band.processSingleChannel(dataL, numSamples)  // filterL only
```

Bands routed to the left channel only process channel 0 through `filterL`. The right channel passes through unchanged.

#### Pass 3: Right-Only Bands

```
for each band where stereoPlacement == Right:
    band.processSingleChannel(dataR, numSamples)  // filterL only
```

Same as Pass 2 but for channel 1. Note that `processSingleChannel()` always uses `filterL` -- the "L" and "R" naming refers to the EqBand's dual filter pair, not the audio channel.

#### Pass 4: Mid/Side Bands

This pass only executes if at least one active band has Mid or Side placement:

```
1. MidSideProcessor::encode(dataL, dataR, numSamples)
   // L[i] = 0.5 * (L[i] + R[i])    --> Mid
   // R[i] = 0.5 * (L[i] - R[i])    --> Side

2. for each band where stereoPlacement == Mid:
       band.processSingleChannel(dataL, numSamples)  // process Mid signal

3. for each band where stereoPlacement == Side:
       band.processSingleChannel(dataR, numSamples)  // process Side signal

4. MidSideProcessor::decode(dataL, dataR, numSamples)
   // L_out = Mid + Side
   // R_out = Mid - Side
```

The M/S encode/decode is performed in-place. The encode uses the standard 0.5 scaling factor to maintain unity gain through the round-trip.

### Per-Band Processing Detail (EqBand::process)

Each band's `process()` method handles two distinct modes:

#### Smoothing Mode (Parameter Transition Active)

When any of `smoothedFreq`, `smoothedGain`, or `smoothedQ` are actively ramping:

```
for each sample i:
    freq = smoothedFreq.getNextValue()
    gain = smoothedGain.getNextValue()
    q    = smoothedQ.getNextValue()

    // Analog proportional Q (Bell only)
    if analogMode && type == Bell:
        q *= (1.0 + 0.4 * |gain| / 30.0)

    filterL.setParameters(type, freq, q, gain, slope)
    filterR.setParameters(type, freq, q, gain, slope)

    dataL[i] = filterL.processSample(dataL[i])
    dataR[i] = filterR.processSample(dataR[i])
```

This per-sample coefficient update ensures zipper-free transitions. The `SmoothedValue` ramp time is 20ms (set in `EqBand::prepare()`).

#### Block Mode (Parameters Stable)

When all smoothing values have reached their targets:

```
filterL.process(dataL, numSamples)    // block processing, no per-sample updates
filterR.process(dataR, numSamples)
```

Block processing is significantly more efficient because coefficient computation happens once per block rather than once per sample.

#### Per-Band Analog Saturation

When analog mode is enabled, a soft-clipping waveshaper is applied after filtering:

```cpp
float EqBand::saturate (float x)
{
    constexpr float drive = 1.2f;
    constexpr float asymmetry = 0.05f;
    constexpr float normFactor = 1.0f / 0.8336546f;  // 1/tanh(1.2)
    float shaped = x + asymmetry * x * x;             // Asymmetric pre-shape
    return std::tanh (shaped * drive) * normFactor;    // Soft clip + normalize
}
```

Key characteristics:
- **Drive**: 1.2x input scaling before tanh -- mild saturation
- **Asymmetry**: The `0.05 * x^2` term adds even-order harmonics, modeling the asymmetric transfer function of analog op-amps/transistors
- **Normalization**: Division by `tanh(1.2)` ensures unity gain at low levels
- Applied independently to each channel, after each band's filter processing

### Step 6: Analog Mode Global Effects

After all four passes complete, if analog mode is enabled and the buffer is stereo:

#### Channel Crosstalk

```cpp
constexpr float crosstalk = 0.002f;  // ~-54 dB channel bleed
float l = dataL[i];
float r = dataR[i];
dataL[i] = l + crosstalk * r;
dataR[i] = r + crosstalk * l;
```

This models the inter-channel bleed found in analog mixing consoles and outboard EQ units. At 0.002 (-54 dB), it is subtle but contributes to a slightly less "perfect" stereo image.

#### Pink Noise Floor

```cpp
constexpr float noiseLevel = 1e-5f;  // ~-100 dBFS

float whiteL = noiseRng.nextFloat() * 2.0f - 1.0f;  // [-1, +1]
float whiteR = noiseRng.nextFloat() * 2.0f - 1.0f;
pinkState[0] = 0.99f * pinkState[0] + 0.01f * whiteL;  // One-pole LPF
pinkState[1] = 0.99f * pinkState[1] + 0.01f * whiteR;
dataL[i] += pinkState[0] * noiseLevel;
dataR[i] += pinkState[1] * noiseLevel;
```

A one-pole low-pass filter (`y = 0.99*y + 0.01*x`) shapes white noise into an approximately pink spectrum (the filter's -3 dB point is at about 70 Hz at 44.1 kHz). The resulting pink noise is scaled to approximately -100 dBFS, modeling the thermal noise floor of analog circuitry. Independent noise generators run for each channel.

For mono operation, only one channel of noise is generated (no crosstalk is applied).

### Step 7: Output Gain

```cpp
if (outputGain.isSmoothing())
{
    for each channel:
        for each sample:
            data[i] *= outputGain.getNextValue();
}
else
{
    float gainValue = outputGain.getCurrentValue();
    if (abs(gainValue - 1.0) > 1e-6)
        buffer.applyGain (gainValue);
}
```

Output gain is applied with 20ms smoothing (configured in `ProcessingChain::prepare()`). The gain value is converted from dB to linear in `setOutputGain()` via `MathUtils::dbToGain()`. When not smoothing and the gain is effectively unity (within 1e-6), the multiply is skipped entirely.

### Step 8: Post-EQ FFT Analysis

```cpp
if (analyzerOn)
    fftProcessorPost.pushSamples (buffer, buffer.getNumSamples());
```

Identical to the pre-EQ FFT (Step 4), but operates on the post-processing buffer. A separate `FFTProcessor` instance and `TripleBuffer` are used, allowing the UI to display both pre- and post-EQ spectra simultaneously.

## Complete Signal Flow Diagram

```
Input Buffer (from host)
    |
    v
[ScopedNoDenormals: set FTZ+DAZ flags]
    |
    v
[Clear unused output channels]
    |
    v
[Read 24 x BandParameters + globals from atomics]
    |
    v
[FFTProcessor pre: push samples --> ring buffer --> Hann window --> 4096-pt FFT --> dB magnitudes --> TripleBuffer]
    |
    v
+---ProcessingChain::process()-------------------------------------------+
|                                                                         |
|  Pass 1: Stereo bands                                                   |
|  for each active band (placement == Stereo):                           |
|      SmoothedValue --> freq/gain/Q --> CascadedFilter(L,R) --> saturate |
|                                                                         |
|  Pass 2: Left-only bands                                                |
|  for each active band (placement == Left):                             |
|      SmoothedValue --> freq/gain/Q --> CascadedFilter(L) --> saturate   |
|                                                                         |
|  Pass 3: Right-only bands                                               |
|  for each active band (placement == Right):                            |
|      SmoothedValue --> freq/gain/Q --> CascadedFilter(L) --> saturate   |
|                                                                         |
|  Pass 4: Mid/Side bands (only if any M/S bands active)                  |
|      encode(L, R) --> Mid in L, Side in R                              |
|      for each Mid band: CascadedFilter(L) --> saturate on L            |
|      for each Side band: CascadedFilter(L) --> saturate on R           |
|      decode(L, R) --> back to L/R                                      |
|                                                                         |
|  [Analog crosstalk: L += 0.002*R, R += 0.002*L]                       |
|  [Analog noise: pink-filtered white noise at -100 dBFS per channel]    |
|                                                                         |
|  [Output gain: SmoothedValue, 20ms ramp, dB-to-linear]                |
+-------------------------------------------------------------------------+
    |
    v
[FFTProcessor post: push samples --> ring buffer --> Hann window --> 4096-pt FFT --> dB magnitudes --> TripleBuffer]
    |
    v
Output Buffer (to host)
```

## FFT Processing Detail

### Ring Buffer and Overlap

```
Ring buffer (4096 samples):
[--------first half--------][-------second half--------]
         hopSize=2048               hopSize=2048

When ringWritePos reaches 4096:
  1. performFFT() on entire buffer
  2. Copy second half to first half (50% overlap)
  3. Reset ringWritePos = 2048

This means consecutive FFT frames overlap by 50%, yielding one FFT frame
every 2048 samples. At 44.1 kHz, this is approximately 21.5 frames/second.
```

### Magnitude Computation

```
For each bin b (0 to 2048):
    magnitude = fftData[b]
    magnitude /= fftSize                           // Normalize by FFT size
    if (b > 0 && b < numBins - 1):
        magnitude *= 2.0                            // Single-sided correction
    dB = 20 * log10(max(magnitude, 1e-10))          // Convert to dB
    dB = max(dB, -100.0)                            // Clamp to floor
```

The single-sided correction (x2) accounts for the fact that we only look at positive frequencies. DC (bin 0) and Nyquist (bin 2048) are not doubled because they have no corresponding negative-frequency counterpart.

### TripleBuffer Publishing

The `finishWrite()` call atomically swaps the write and middle buffer indices:
- Uses `memory_order_release` to ensure all magnitude writes are visible before the swap
- Sets a `newData` flag with release semantics

On the UI thread, `swapRead()`:
- Checks `newData` with `memory_order_acquire` to synchronize
- Swaps read and middle buffer indices
- Clears the `newData` flag

This guarantees that the UI thread always sees a complete, consistent frame of FFT data.

## Latency

In the current implementation (zero-latency IIR mode), the plugin introduces exactly zero samples of latency. The IIR filters have no inherent latency. The FFT analysis runs as a side-chain and does not affect the audio path.

Future phases may introduce latency for:
- **Linear phase mode**: FIR filtering will add latency equal to half the FIR length
- **Oversampling**: Upsampling/downsampling filters will add a small number of samples
- **Lookahead** for dynamic EQ: Will add latency equal to the attack time window
