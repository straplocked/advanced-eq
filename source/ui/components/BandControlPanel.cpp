#include "BandControlPanel.h"

BandControlPanel::BandControlPanel (AdvancedEQProcessor& p)
    : processor (p)
{
    // Type selector
    typeSelector.addItemList (ParamIDs::getFilterTypeNames(), 1);
    typeSelector.setJustificationType (juce::Justification::centred);
    typeSelector.onChange = [this] { updateSlopeEnabled(); };
    addAndMakeVisible (typeSelector);

    // Slope selector
    slopeSelector.addItemList (ParamIDs::getSlopeNames(), 1);
    slopeSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (slopeSelector);

    // Stereo placement selector
    stereoSelector.addItemList (ParamIDs::getStereoPlacementNames(), 1);
    stereoSelector.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (stereoSelector);

    setVisible (false);
    startTimerHz (10);
}

BandControlPanel::~BandControlPanel()
{
    typeAttachment.reset();
    slopeAttachment.reset();
    stereoAttachment.reset();
}

void BandControlPanel::setSelectedBand (int bandIndex)
{
    if (selectedBand == bandIndex)
        return;

    selectedBand = bandIndex;

    if (selectedBand < 0)
    {
        setVisible (false);
        typeAttachment.reset();
        slopeAttachment.reset();
        stereoAttachment.reset();
        return;
    }

    updateAttachments();
    updateSlopeEnabled();
    setVisible (true);
}

void BandControlPanel::updateAttachments()
{
    auto& apvts = processor.getAPVTS();

    // Must reset old attachments before creating new ones
    typeAttachment.reset();
    slopeAttachment.reset();
    stereoAttachment.reset();

    if (selectedBand < 0)
        return;

    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kType), typeSelector);

    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kSlope), slopeSelector);

    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kStereo), stereoSelector);
}

void BandControlPanel::updateSlopeEnabled()
{
    if (selectedBand < 0)
        return;

    auto* typeParam = processor.getAPVTS().getRawParameterValue (
        ParamIDs::bandParam (selectedBand, ParamIDs::kType));
    auto type = static_cast<FilterType> ((int) typeParam->load());

    bool slopeApplies = (type == FilterType::LowPass
                      || type == FilterType::HighPass
                      || type == FilterType::LowShelf
                      || type == FilterType::HighShelf);

    slopeSelector.setEnabled (slopeApplies);
    slopeSelector.setAlpha (slopeApplies ? 1.0f : 0.4f);
}

void BandControlPanel::timerCallback()
{
    if (selectedBand >= 0)
        updateSlopeEnabled();
}

void BandControlPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Panel background with rounded corners
    g.setColour (ColourPalette::controlBar().withAlpha (0.95f));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Border
    g.setColour (ColourPalette::gridLineStrong());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

void BandControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced (6, 4);

    auto row1 = bounds.removeFromTop (22);
    bounds.removeFromTop (3); // spacing
    auto row2 = bounds.removeFromTop (22);
    bounds.removeFromTop (3); // spacing
    auto row3 = bounds.removeFromTop (22);

    typeSelector.setBounds (row1);
    slopeSelector.setBounds (row2);
    stereoSelector.setBounds (row3);
}

void BandControlPanel::positionNear (float x, float y, juce::Rectangle<int> parentBounds)
{
    int panelW = 160;
    int panelH = 88;

    // Position above the handle by default, below if near top
    int px = (int) (x - panelW / 2.0f);
    int py = (int) (y - panelH - 16);

    if (py < parentBounds.getY())
        py = (int) (y + 16);

    // Clamp to parent bounds
    px = juce::jlimit (parentBounds.getX(), parentBounds.getRight() - panelW, px);
    py = juce::jlimit (parentBounds.getY(), parentBounds.getBottom() - panelH, py);

    setBounds (px, py, panelW, panelH);
}
