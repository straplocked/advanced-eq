/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp/ProcessingChain.h"
#include "analyzer/FFTProcessor.h"
#include "parameters/BandParameterAttachment.h"
#include "presets/PresetManager.h"
#include "midi/MidiLearnManager.h"
#include "ipc/InstanceRegistry.h"
#include "utils/Constants.h"

class AdvancedEQProcessor : public juce::AudioProcessor
{
public:
    AdvancedEQProcessor();
    ~AdvancedEQProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    ProcessingChain& getProcessingChain() { return processingChain; }
    FFTProcessor& getFFTProcessorPre()  { return fftProcessorPre; }
    FFTProcessor& getFFTProcessorPost() { return fftProcessorPost; }
    PresetManager& getPresetManager()   { return presetManager; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }
    InstanceRegistry& getInstanceRegistry() { return instanceRegistry; }

    // A/B comparison
    void toggleAB();
    void copyAtoB();
    void copyBtoA();
    bool isSlotA() const { return currentSlotIsA; }

    // Level metering (Phase 4D)
    float getPeakLevelL() const { return peakLevelL.load (std::memory_order_relaxed); }
    float getPeakLevelR() const { return peakLevelR.load (std::memory_order_relaxed); }
    float getRmsLevelL() const { return rmsLevelL.load (std::memory_order_relaxed); }
    float getRmsLevelR() const { return rmsLevelR.load (std::memory_order_relaxed); }

private:
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;

    // A/B comparison
    juce::ValueTree stateSlotA;
    juce::ValueTree stateSlotB;
    bool currentSlotIsA = true;
    MidiLearnManager midiLearnManager;
    InstanceRegistry instanceRegistry;
    ProcessingChain processingChain;
    std::array<BandParameterAttachment, Constants::kMaxBands> bandAttachments;

    // Spectrum analyzer FFT engines (pre and post EQ)
    FFTProcessor fftProcessorPre;
    FFTProcessor fftProcessorPost;

    // Cached raw parameter pointers for global params
    std::atomic<float>* rawOutputGain  = nullptr;
    std::atomic<float>* rawAnalyzerOn  = nullptr;
    std::atomic<float>* rawAnalyzerPre = nullptr;
    std::atomic<float>* rawAnalogMode  = nullptr;
    std::atomic<float>* rawOversample  = nullptr;
    std::atomic<float>* rawAutoGain    = nullptr;
    std::atomic<float>* rawPhaseInvert = nullptr;
    std::atomic<float>* rawAnalyzerRes = nullptr;
    std::atomic<float>* rawProcessMode = nullptr;

    // Output level metering
    std::atomic<float> peakLevelL { -100.0f };
    std::atomic<float> peakLevelR { -100.0f };
    std::atomic<float> rmsLevelL  { -100.0f };
    std::atomic<float> rmsLevelR  { -100.0f };

    // RMS window accumulators
    float rmsAccumL = 0.0f;
    float rmsAccumR = 0.0f;
    int rmsSampleCount = 0;
    int rmsWindowSamples = 0; // computed in prepare

    // Peak decay
    float peakHoldL = 0.0f;
    float peakHoldR = 0.0f;
    float peakDecayCoeff = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQProcessor)
};
