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

    setVisible (false);
    startTimerHz (10);
}

BandControlPanel::~BandControlPanel()
{
    typeAttachment.reset();
    slopeAttachment.reset();
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

    if (selectedBand < 0)
        return;

    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kType), typeSelector);

    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::bandParam (selectedBand, ParamIDs::kSlope), slopeSelector);
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

    auto topRow = bounds.removeFromTop (24);
    bounds.removeFromTop (4); // spacing
    auto bottomRow = bounds.removeFromTop (24);

    typeSelector.setBounds (topRow);
    slopeSelector.setBounds (bottomRow);
}

void BandControlPanel::positionNear (float x, float y, juce::Rectangle<int> parentBounds)
{
    int panelW = 160;
    int panelH = 60;

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
