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

/**
 * @file PluginProcessor.h
 * @brief Main audio processor -- entry point for all audio processing.
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp/ProcessingChain.h"
#include "analyzer/FFTProcessor.h"
#include "parameters/BandParameterAttachment.h"
#include "presets/PresetManager.h"
#include "utils/Constants.h"

/**
 * @class AdvancedEQProcessor
 * @brief Core audio processor for the Advanced EQ plugin.
 *
 * Extends juce::AudioProcessor and owns the AudioProcessorValueTreeState (APVTS),
 * ProcessingChain, PresetManager, pre/post FFT processors, and per-band
 * BandParameterAttachments (lock-free atomic reads).
 *
 * processBlock() reads parameters via lock-free atomics, feeds the pre-EQ FFT,
 * runs the ProcessingChain for filtering, and then feeds the post-EQ FFT.
 * State serialization is handled by getStateInformation() / setStateInformation().
 *
 * @see ProcessingChain, FFTProcessor, PresetManager
 */
class AdvancedEQProcessor : public juce::AudioProcessor
{
public:
    /** @brief Constructs the processor, creating the APVTS, processing chain, and preset manager. */
    AdvancedEQProcessor();
    ~AdvancedEQProcessor() override;

    /** @brief Initializes DSP resources for the given sample rate and block size.
     *  @param sampleRate      The host-provided sample rate in Hz.
     *  @param samplesPerBlock The maximum expected block size.
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    /** @brief Releases DSP resources when playback stops. */
    void releaseResources() override;

    /** @brief Reports whether a given bus layout is supported.
     *  @param layouts The bus layout to check.
     *  @return True if the layout is supported.
     */
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    /** @brief Processes a block of audio: reads parameters, runs pre-FFT, applies EQ, runs post-FFT.
     *  @param buffer    The audio buffer to process in-place.
     *  @param midiMessages MIDI buffer (unused -- this plugin does not process MIDI).
     */
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** @brief Creates and returns the plugin editor (AdvancedEQEditor).
     *  @return A new AdvancedEQEditor instance.
     */
    juce::AudioProcessorEditor* createEditor() override;

    /** @brief Returns true; this plugin has a GUI editor. */
    bool hasEditor() const override { return true; }

    /** @brief Returns the plugin name.
     *  @return The plugin name string.
     */
    const juce::String getName() const override;

    /** @brief Returns false; this plugin does not accept MIDI input. */
    bool acceptsMidi() const override { return false; }
    /** @brief Returns false; this plugin does not produce MIDI output. */
    bool producesMidi() const override { return false; }
    /** @brief Returns false; this plugin is not a MIDI effect. */
    bool isMidiEffect() const override { return false; }
    /** @brief Returns 0.0; this plugin produces no audio tail.
     *  @return Tail length in seconds (always 0.0).
     */
    double getTailLengthSeconds() const override { return 0.0; }

    /** @brief Returns 1; only a single program is supported. */
    int getNumPrograms() override { return 1; }
    /** @brief Returns 0; the current program index. */
    int getCurrentProgram() override { return 0; }
    /** @brief No-op; program switching is not supported. */
    void setCurrentProgram (int) override {}
    /** @brief Returns an empty string; no program name.
     *  @return An empty juce::String.
     */
    const juce::String getProgramName (int) override { return {}; }
    /** @brief No-op; program name changes are not supported. */
    void changeProgramName (int, const juce::String&) override {}

    /** @brief Serializes the complete plugin state (APVTS + preset info) to a memory block.
     *  @param destData The memory block to write the state into.
     */
    void getStateInformation (juce::MemoryBlock& destData) override;

    /** @brief Restores the plugin state from previously serialized data.
     *  @param data       Pointer to the serialized state data.
     *  @param sizeInBytes Size of the serialized data in bytes.
     */
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** @brief Returns a reference to the AudioProcessorValueTreeState.
     *  @return The APVTS reference for parameter access.
     */
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    /** @brief Returns a reference to the DSP processing chain.
     *  @return The ProcessingChain reference.
     */
    ProcessingChain& getProcessingChain() { return processingChain; }

    /** @brief Returns a reference to the pre-EQ FFT processor for spectrum analysis.
     *  @return The pre-EQ FFTProcessor reference.
     */
    FFTProcessor& getFFTProcessorPre()  { return fftProcessorPre; }

    /** @brief Returns a reference to the post-EQ FFT processor for spectrum analysis.
     *  @return The post-EQ FFTProcessor reference.
     */
    FFTProcessor& getFFTProcessorPost() { return fftProcessorPost; }

    /** @brief Returns a reference to the preset manager.
     *  @return The PresetManager reference.
     */
    PresetManager& getPresetManager()   { return presetManager; }

private:
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;
    ProcessingChain processingChain;
    std::array<BandParameterAttachment, Constants::kMaxBands> bandAttachments;

    // Spectrum analyzer FFT engines (pre and post EQ)
    FFTProcessor fftProcessorPre;
    FFTProcessor fftProcessorPost;

    // Cached raw parameter pointers for global params
    std::atomic<float>* rawOutputGain = nullptr;
    std::atomic<float>* rawAnalyzerOn = nullptr;
    std::atomic<float>* rawAnalyzerPre = nullptr;
    std::atomic<float>* rawAnalogMode = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQProcessor)
};
