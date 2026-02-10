#include "EqCurveDisplay.h"
#include "../../parameters/ParameterIDs.h"
#include "../../utils/FrequencyUtils.h"

EqCurveDisplay::EqCurveDisplay (AdvancedEQProcessor& p)
    : processor (p)
{
    // Pre-compute logarithmically spaced frequency points
    for (int i = 0; i < Constants::kCurveResolution; ++i)
    {
        float normalized = (float) i / (float) (Constants::kCurveResolution - 1);
        frequencyPoints[(size_t) i] = (double) MathUtils::normalizedToFrequency (normalized);
    }

    startTimerHz (Constants::kUITimerHz);
}

void EqCurveDisplay::timerCallback()
{
    updateCurves();
    repaint();
}

void EqCurveDisplay::updateCurves()
{
    auto& chain = processor.getProcessingChain();

    // Get composite magnitude
    chain.computeCompositeMagnitude (frequencyPoints.data(), compositeMagnitudes.data(),
                                     Constants::kCurveResolution);

    // Get per-band magnitudes
    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        chain.computeBandMagnitude (b, frequencyPoints.data(),
                                    bandMagnitudes[(size_t) b].data(),
                                    Constants::kCurveResolution);
    }
}

// --- Coordinate mapping ---

float EqCurveDisplay::frequencyToX (float freq) const
{
    return MathUtils::frequencyToX (freq, (float) getWidth());
}

float EqCurveDisplay::xToFrequency (float x) const
{
    return MathUtils::xToFrequency (x, (float) getWidth());
}

float EqCurveDisplay::gainToY (float gainDB) const
{
    return MathUtils::gainToY (gainDB, (float) getHeight(), displayMinDB, displayMaxDB);
}

float EqCurveDisplay::yToGain (float y) const
{
    return MathUtils::yToGain (y, (float) getHeight(), displayMinDB, displayMaxDB);
}

// --- Drawing ---

void EqCurveDisplay::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (ColourPalette::curveArea());

    drawGrid (g);
    drawBandCurves (g);
    drawCompositeCurve (g);
    drawBandHandles (g);
    drawFrequencyLabels (g);
    drawGainLabels (g);
    drawTooltip (g);
}

void EqCurveDisplay::drawGrid (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Frequency grid lines
    static const float freqLines[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float freq : freqLines)
    {
        float x = frequencyToX (freq);
        bool isMajor = (freq == 100 || freq == 1000 || freq == 10000);
        g.setColour (isMajor ? ColourPalette::gridLineStrong() : ColourPalette::gridLine());
        g.drawVerticalLine ((int) x, bounds.getY(), bounds.getBottom());
    }

    // Gain grid lines
    for (float db = displayMinDB; db <= displayMaxDB; db += 6.0f)
    {
        float y = gainToY (db);
        bool isZero = (std::abs (db) < 0.1f);
        g.setColour (isZero ? ColourPalette::gridLineStrong() : ColourPalette::gridLine());
        g.drawHorizontalLine ((int) y, bounds.getX(), bounds.getRight());
    }
}

void EqCurveDisplay::drawFrequencyLabels (juce::Graphics& g)
{
    g.setColour (ColourPalette::axisText());
    g.setFont (11.0f);

    static const float labelFreqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    static const char* labels[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    for (int i = 0; i < 10; ++i)
    {
        float x = frequencyToX (labelFreqs[i]);
        g.drawText (labels[i],
                    (int) (x - 20), getHeight() - 16, 40, 14,
                    juce::Justification::centred, false);
    }
}

void EqCurveDisplay::drawGainLabels (juce::Graphics& g)
{
    g.setColour (ColourPalette::axisText());
    g.setFont (10.0f);

    for (float db = displayMinDB + 6.0f; db < displayMaxDB; db += 6.0f)
    {
        float y = gainToY (db);
        juce::String label = (db > 0 ? "+" : "") + juce::String ((int) db);
        g.drawText (label, 2, (int) (y - 7), 30, 14,
                    juce::Justification::centredLeft, false);
    }
}

void EqCurveDisplay::drawBandCurves (juce::Graphics& g)
{
    float w = (float) getWidth();
    float h = (float) getHeight();

    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        auto* band = processor.getProcessingChain().getBand (b);
        if (band == nullptr || ! band->isActive())
            continue;

        juce::Path curvePath;
        bool started = false;

        for (int i = 0; i < Constants::kCurveResolution; ++i)
        {
            float x = (float) i / (float) (Constants::kCurveResolution - 1) * w;
            float y = gainToY ((float) bandMagnitudes[(size_t) b][(size_t) i]);

            if (! started)
            {
                curvePath.startNewSubPath (x, y);
                started = true;
            }
            else
            {
                curvePath.lineTo (x, y);
            }
        }

        auto colour = ColourPalette::bandColour (b);
        g.setColour (colour.withAlpha (b == selectedBand ? 0.5f : 0.2f));

        // Fill under/over the curve relative to 0 dB line
        juce::Path fillPath (curvePath);
        float zeroY = gainToY (0.0f);
        fillPath.lineTo (w, zeroY);
        fillPath.lineTo (0, zeroY);
        fillPath.closeSubPath();
        g.fillPath (fillPath);

        // Draw the curve line
        g.setColour (colour.withAlpha (b == selectedBand ? 1.0f : 0.6f));
        g.strokePath (curvePath, juce::PathStrokeType (b == selectedBand ? 2.0f : 1.0f));
    }
}

void EqCurveDisplay::drawCompositeCurve (juce::Graphics& g)
{
    float w = (float) getWidth();

    juce::Path curvePath;
    bool started = false;

    for (int i = 0; i < Constants::kCurveResolution; ++i)
    {
        float x = (float) i / (float) (Constants::kCurveResolution - 1) * w;
        float y = gainToY ((float) compositeMagnitudes[(size_t) i]);

        if (! started)
        {
            curvePath.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            curvePath.lineTo (x, y);
        }
    }

    // Draw white composite curve
    g.setColour (ColourPalette::compositeCurve());
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));
}

void EqCurveDisplay::drawBandHandles (juce::Graphics& g)
{
    auto& apvts = processor.getAPVTS();

    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        auto* activeParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kActive));
        if (activeParam->load() < 0.5f)
            continue;

        auto* freqParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kFrequency));
        auto* gainParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kGain));

        float freq = freqParam->load();
        float gain = gainParam->load();

        float x = frequencyToX (freq);
        float y = gainToY (gain);

        auto colour = ColourPalette::bandColour (b);
        float radius = (b == selectedBand) ? 8.0f : 6.0f;

        // Outer ring
        g.setColour (colour);
        g.drawEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        // Fill
        g.setColour (colour.withAlpha (b == hoveredBand ? 0.8f : 0.5f));
        g.fillEllipse (x - radius + 2, y - radius + 2,
                       radius * 2.0f - 4, radius * 2.0f - 4);

        // Band number label
        g.setColour (ColourPalette::textBright());
        g.setFont (9.0f);
        g.drawText (juce::String (b + 1),
                    (int) (x - 6), (int) (y - 5), 12, 10,
                    juce::Justification::centred, false);
    }
}

void EqCurveDisplay::drawTooltip (juce::Graphics& g)
{
    if (hoveredBand < 0 && selectedBand < 0)
        return;

    int band = (selectedBand >= 0) ? selectedBand : hoveredBand;
    auto& apvts = processor.getAPVTS();

    auto* freqParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kFrequency));
    auto* gainParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kGain));
    auto* qParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kQ));

    float freq = freqParam->load();
    float gain = gainParam->load();
    float q = qParam->load();

    juce::String text = FrequencyUtils::formatFrequency (freq) + "  "
                      + juce::String (gain, 1) + " dB  Q " + juce::String (q, 2);

    float x = frequencyToX (freq);
    float y = gainToY (gain) - 22.0f;

    auto textWidth = g.getCurrentFont().getStringWidthFloat (text) + 12.0f;
    auto tooltipBounds = juce::Rectangle<float> (x - textWidth / 2, y - 10, textWidth, 18.0f);

    // Clamp to display bounds
    if (tooltipBounds.getX() < 0)
        tooltipBounds.setX (0);
    if (tooltipBounds.getRight() > (float) getWidth())
        tooltipBounds.setX ((float) getWidth() - textWidth);

    g.setColour (ColourPalette::controlBar().withAlpha (0.9f));
    g.fillRoundedRectangle (tooltipBounds, 4.0f);

    g.setColour (ColourPalette::textBright());
    g.setFont (11.0f);
    g.drawText (text, tooltipBounds, juce::Justification::centred, false);
}

void EqCurveDisplay::resized()
{
}

// --- Mouse Interaction ---

int EqCurveDisplay::findBandAtPosition (juce::Point<float> pos) const
{
    auto& apvts = processor.getAPVTS();
    float closestDist = 20.0f; // Max click distance in pixels
    int closestBand = -1;

    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        auto* activeParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kActive));
        if (activeParam->load() < 0.5f)
            continue;

        auto* freqParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kFrequency));
        auto* gainParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kGain));

        float x = frequencyToX (freqParam->load());
        float y = gainToY (gainParam->load());

        float dist = pos.getDistanceFrom (juce::Point<float> (x, y));
        if (dist < closestDist)
        {
            closestDist = dist;
            closestBand = b;
        }
    }
    return closestBand;
}

int EqCurveDisplay::findFirstInactiveBand() const
{
    auto& apvts = processor.getAPVTS();
    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        auto* activeParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kActive));
        if (activeParam->load() < 0.5f)
            return b;
    }
    return -1; // All bands in use
}

void EqCurveDisplay::activateBandAt (float freq, float gain)
{
    int band = findFirstInactiveBand();
    if (band < 0)
        return;

    auto& apvts = processor.getAPVTS();

    if (auto* param = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kActive)))
        param->setValueNotifyingHost (1.0f);

    if (auto* param = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kFrequency)))
        param->setValueNotifyingHost (param->convertTo0to1 (freq));

    if (auto* param = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kGain)))
        param->setValueNotifyingHost (param->convertTo0to1 (gain));

    // Default to Bell filter
    if (auto* param = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kType)))
        param->setValueNotifyingHost (param->convertTo0to1 (0.0f));

    selectedBand = band;
}

void EqCurveDisplay::mouseDown (const juce::MouseEvent& e)
{
    auto pos = e.position;
    int band = findBandAtPosition (pos);

    if (band >= 0)
    {
        selectedBand = band;
        isDragging = true;
    }
    else
    {
        // Click on empty space: create new band at this position
        float freq = xToFrequency (pos.x);
        float gain = yToGain (pos.y);
        activateBandAt (freq, gain);
        isDragging = true;
    }

    repaint();
}

void EqCurveDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDragging || selectedBand < 0)
        return;

    float freq = juce::jlimit (Constants::kMinFrequency, Constants::kMaxFrequency,
                                xToFrequency (e.position.x));
    float gain = juce::jlimit (Constants::kMinGain, Constants::kMaxGain,
                                yToGain (e.position.y));

    auto& apvts = processor.getAPVTS();

    if (auto* param = apvts.getParameter (ParamIDs::bandParam (selectedBand, ParamIDs::kFrequency)))
        param->setValueNotifyingHost (param->convertTo0to1 (freq));

    if (auto* param = apvts.getParameter (ParamIDs::bandParam (selectedBand, ParamIDs::kGain)))
        param->setValueNotifyingHost (param->convertTo0to1 (gain));
}

void EqCurveDisplay::mouseUp (const juce::MouseEvent&)
{
    isDragging = false;
}

void EqCurveDisplay::mouseMove (const juce::MouseEvent& e)
{
    int band = findBandAtPosition (e.position);
    if (band != hoveredBand)
    {
        hoveredBand = band;
        repaint();
    }
}

void EqCurveDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    int band = findBandAtPosition (e.position);
    if (band >= 0)
    {
        // Deactivate the band
        auto& apvts = processor.getAPVTS();
        if (auto* param = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kActive)))
            param->setValueNotifyingHost (0.0f);

        if (selectedBand == band)
            selectedBand = -1;
        if (hoveredBand == band)
            hoveredBand = -1;

        repaint();
    }
}

void EqCurveDisplay::mouseWheelMove (const juce::MouseEvent& e,
                                      const juce::MouseWheelDetails& wheel)
{
    int band = findBandAtPosition (e.position);
    if (band < 0)
        band = selectedBand;
    if (band < 0)
        return;

    auto& apvts = processor.getAPVTS();
    auto* qParam = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kQ));
    if (qParam == nullptr)
        return;

    // Scroll up = increase Q (narrower), scroll down = decrease Q (wider)
    float currentQ = qParam->convertFrom0to1 (qParam->getValue());
    float delta = wheel.deltaY * currentQ * 0.3f;
    float newQ = juce::jlimit (Constants::kMinQ, Constants::kMaxQ, currentQ + delta);
    qParam->setValueNotifyingHost (qParam->convertTo0to1 (newQ));
}
