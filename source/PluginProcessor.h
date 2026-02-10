#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp/ProcessingChain.h"
#include "parameters/BandParameterAttachment.h"
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
    bool acceptsMidi() const override { return false; }
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

private:
    juce::AudioProcessorValueTreeState apvts;
    ProcessingChain processingChain;
    std::array<BandParameterAttachment, Constants::kMaxBands> bandAttachments;

    // Cached raw parameter pointers for global params
    std::atomic<float>* rawOutputGain = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedEQProcessor)
};
