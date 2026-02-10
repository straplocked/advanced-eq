#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "parameters/ParameterLayout.h"
#include "parameters/ParameterIDs.h"

AdvancedEQProcessor::AdvancedEQProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AdvancedEQState", ParameterLayout::createLayout()),
      presetManager (apvts)
{
    // Initialize band parameter attachments
    for (int i = 0; i < Constants::kMaxBands; ++i)
        bandAttachments[(size_t) i].init (i, apvts);

    // Cache global parameter pointers
    rawOutputGain  = apvts.getRawParameterValue (ParamIDs::kOutputGain);
    rawAnalyzerOn  = apvts.getRawParameterValue (ParamIDs::kAnalyzerOn);
    rawAnalyzerPre = apvts.getRawParameterValue (ParamIDs::kAnalyzerPre);
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
}

void AdvancedEQProcessor::releaseResources()
{
    processingChain.reset();
    fftProcessorPre.reset();
    fftProcessorPost.reset();
}

bool AdvancedEQProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AdvancedEQProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Read parameters and update DSP (lock-free via atomic reads)
    for (int i = 0; i < Constants::kMaxBands; ++i)
    {
        auto params = bandAttachments[(size_t) i].getCurrentParameters();
        processingChain.updateBand (i, params);
    }

    processingChain.setOutputGain (rawOutputGain->load());

    // Capture pre-EQ spectrum
    bool analyzerOn = rawAnalyzerOn->load() >= 0.5f;
    if (analyzerOn)
        fftProcessorPre.pushSamples (buffer, buffer.getNumSamples());

    // Process audio through EQ
    processingChain.process (buffer);

    // Capture post-EQ spectrum
    if (analyzerOn)
        fftProcessorPost.pushSamples (buffer, buffer.getNumSamples());
}

juce::AudioProcessorEditor* AdvancedEQProcessor::createEditor()
{
    return new AdvancedEQEditor (*this);
}

void AdvancedEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AdvancedEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdvancedEQProcessor();
}
