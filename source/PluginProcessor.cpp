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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "parameters/ParameterLayout.h"
#include "parameters/ParameterIDs.h"
#include "utils/MathUtils.h"

AdvancedEQProcessor::AdvancedEQProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "AdvancedEQState", ParameterLayout::createLayout()),
      presetManager (apvts),
      midiLearnManager (apvts)
{
    // Initialize band parameter attachments
    for (int i = 0; i < Constants::kMaxBands; ++i)
        bandAttachments[(size_t) i].init (i, apvts);

    // Cache global parameter pointers
    rawOutputGain  = apvts.getRawParameterValue (ParamIDs::kOutputGain);
    rawAnalyzerOn  = apvts.getRawParameterValue (ParamIDs::kAnalyzerOn);
    rawAnalyzerPre = apvts.getRawParameterValue (ParamIDs::kAnalyzerPre);
    rawAnalogMode  = apvts.getRawParameterValue (ParamIDs::kAnalogMode);
    rawOversample  = apvts.getRawParameterValue (ParamIDs::kOversample);
    rawAutoGain    = apvts.getRawParameterValue (ParamIDs::kAutoGain);
    rawPhaseInvert = apvts.getRawParameterValue (ParamIDs::kPhaseInvert);
    rawAnalyzerRes = apvts.getRawParameterValue (ParamIDs::kAnalyzerRes);
    rawProcessMode = apvts.getRawParameterValue (ParamIDs::kProcessMode);
}

AdvancedEQProcessor::~AdvancedEQProcessor() = default;

const juce::String AdvancedEQProcessor::getName() const
{
    return JucePlugin_Name;
}

void AdvancedEQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    processingChain.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    fftProcessorPre.prepare (sampleRate, samplesPerBlock);
    fftProcessorPost.prepare (sampleRate, samplesPerBlock);

    // RMS window: ~300ms
    rmsWindowSamples = (int) (sampleRate * 0.3);
    rmsAccumL = 0.0f;
    rmsAccumR = 0.0f;
    rmsSampleCount = 0;

    // Peak decay: ~20 dB/s at the given sample rate
    peakDecayCoeff = std::exp (-1.0f / (float) (sampleRate * 0.3f));
    peakHoldL = 0.0f;
    peakHoldR = 0.0f;
}

void AdvancedEQProcessor::releaseResources()
{
    processingChain.reset();
    fftProcessorPre.reset();
    fftProcessorPost.reset();
}

bool AdvancedEQProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto mainOut = layouts.getMainOutputChannelSet();

    // Supported layouts: mono, stereo, 5.1, 7.1, 7.1.4
    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo()
        && mainOut != juce::AudioChannelSet::create5point1()
        && mainOut != juce::AudioChannelSet::create7point1()
        && mainOut != juce::AudioChannelSet::create7point1point4())
        return false;

    if (mainOut != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AdvancedEQProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Process MIDI CC messages for MIDI Learn
    midiLearnManager.processMidi (midiMessages);

    // Read parameters and update DSP (lock-free via atomic reads)
    for (int i = 0; i < Constants::kMaxBands; ++i)
    {
        auto params = bandAttachments[(size_t) i].getCurrentParameters();
        processingChain.updateBand (i, params);
    }

    // Publish band data to instance registry for collision detection
    {
        std::array<InstanceRegistry::BandInfo, Constants::kMaxBands> bandInfos;
        for (int i = 0; i < Constants::kMaxBands; ++i)
        {
            auto params = bandAttachments[(size_t) i].getCurrentParameters();
            bandInfos[(size_t) i].active = params.active;
            bandInfos[(size_t) i].frequency = params.frequency;
            bandInfos[(size_t) i].gain = params.gain;
            bandInfos[(size_t) i].q = params.q;
        }
        instanceRegistry.publishBands (bandInfos.data(), Constants::kMaxBands);
    }

    // Update processing mode (IIR / Natural Phase / Linear Phase)
    processingChain.setProcessingMode ((int) rawProcessMode->load());

    // Update oversampling factor
    processingChain.setOversamplingFactor ((int) rawOversample->load());

    // Update analyzer resolution
    fftProcessorPre.setResolution ((int) rawAnalyzerRes->load());
    fftProcessorPost.setResolution ((int) rawAnalyzerRes->load());

    // Output gain with optional auto gain compensation
    float outputGainDB = rawOutputGain->load();
    bool autoGainOn = rawAutoGain->load() >= 0.5f;
    if (autoGainOn)
        outputGainDB += processingChain.computeAutoGainCompensation();
    processingChain.setOutputGain (outputGainDB);

    processingChain.setAnalogMode (rawAnalogMode->load() >= 0.5f);

    // Report latency from oversampling
    setLatencySamples (processingChain.getLatencyInSamples());

    // Capture pre-EQ spectrum
    bool analyzerOn = rawAnalyzerOn->load() >= 0.5f;
    if (analyzerOn)
        fftProcessorPre.pushSamples (buffer, buffer.getNumSamples());

    // Process audio through EQ
    processingChain.process (buffer);

    // Phase inversion (after all processing)
    if (rawPhaseInvert->load() >= 0.5f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::negate (
                buffer.getWritePointer (ch),
                buffer.getReadPointer (ch),
                buffer.getNumSamples());
    }

    // Capture post-EQ spectrum
    if (analyzerOn)
        fftProcessorPost.pushSamples (buffer, buffer.getNumSamples());

    // Output level metering
    int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i)
    {
        float sampleL = (buffer.getNumChannels() > 0) ? buffer.getReadPointer (0)[i] : 0.0f;
        float sampleR = (buffer.getNumChannels() > 1) ? buffer.getReadPointer (1)[i] : sampleL;

        // Peak detection with decay
        float absL = std::abs (sampleL);
        float absR = std::abs (sampleR);
        peakHoldL = (absL > peakHoldL) ? absL : peakHoldL * peakDecayCoeff;
        peakHoldR = (absR > peakHoldR) ? absR : peakHoldR * peakDecayCoeff;

        // RMS accumulation
        rmsAccumL += sampleL * sampleL;
        rmsAccumR += sampleR * sampleR;
        ++rmsSampleCount;

        if (rmsSampleCount >= rmsWindowSamples)
        {
            float rmsL = std::sqrt (rmsAccumL / (float) rmsSampleCount);
            float rmsR = std::sqrt (rmsAccumR / (float) rmsSampleCount);
            rmsLevelL.store (MathUtils::gainToDb (rmsL), std::memory_order_relaxed);
            rmsLevelR.store (MathUtils::gainToDb (rmsR), std::memory_order_relaxed);
            rmsAccumL = 0.0f;
            rmsAccumR = 0.0f;
            rmsSampleCount = 0;
        }
    }

    peakLevelL.store (MathUtils::gainToDb (peakHoldL), std::memory_order_relaxed);
    peakLevelR.store (MathUtils::gainToDb (peakHoldR), std::memory_order_relaxed);
}

void AdvancedEQProcessor::toggleAB()
{
    // Save current state into active slot, restore the other
    if (currentSlotIsA)
    {
        stateSlotA = apvts.copyState();
        if (stateSlotB.isValid())
            apvts.replaceState (stateSlotB);
        currentSlotIsA = false;
    }
    else
    {
        stateSlotB = apvts.copyState();
        if (stateSlotA.isValid())
            apvts.replaceState (stateSlotA);
        currentSlotIsA = true;
    }
}

void AdvancedEQProcessor::copyAtoB()
{
    stateSlotA = apvts.copyState();
    stateSlotB = stateSlotA.createCopy();
}

void AdvancedEQProcessor::copyBtoA()
{
    stateSlotB = apvts.copyState();
    stateSlotA = stateSlotB.createCopy();
}

juce::AudioProcessorEditor* AdvancedEQProcessor::createEditor()
{
    return new AdvancedEQEditor (*this);
}

void AdvancedEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Append MIDI mappings to the state tree
    state.appendChild (midiLearnManager.getState(), nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AdvancedEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml (*xml);

        // Restore MIDI mappings
        auto midiTree = newState.getChildWithName ("MidiMappings");
        if (midiTree.isValid())
        {
            midiLearnManager.setState (midiTree);
            newState.removeChild (midiTree, nullptr);
        }

        apvts.replaceState (newState);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdvancedEQProcessor();
}
