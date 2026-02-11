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

    spectrumAnalyzerPre.prepare (Constants::kCurveResolution);
    spectrumAnalyzerPost.prepare (Constants::kCurveResolution);
    previewFilter.prepare (44100.0);

    setComponentID ("curve.display");

    startTimerHz (Constants::kUITimerHz);
}

void EqCurveDisplay::setDisplayRange (DisplayRange range)
{
    currentRange = range;
    switch (range)
    {
        case dB3:  displayMinDB = -3.0f;  displayMaxDB = 3.0f;  break;
        case dB6:  displayMinDB = -6.0f;  displayMaxDB = 6.0f;  break;
        case dB12: displayMinDB = -12.0f; displayMaxDB = 12.0f; break;
        case dB30: displayMinDB = -30.0f; displayMaxDB = 30.0f; break;
    }
    repaint();
}

float EqCurveDisplay::getGridStep() const
{
    switch (currentRange)
    {
        case dB3:  return 1.0f;
        case dB6:  return 2.0f;
        case dB12: return 3.0f;
        case dB30: return 6.0f;
    }
    return 6.0f;
}

juce::Rectangle<float> EqCurveDisplay::getRangeButtonRect (int index) const
{
    float btnW = 22.0f;
    float btnH = 16.0f;
    float spacing = 2.0f;
    float startX = (float) getWidth() - 4 * (btnW + spacing) - 4.0f;
    float startY = 6.0f;
    return { startX + index * (btnW + spacing), startY, btnW, btnH };
}

void EqCurveDisplay::drawRangeButtons (juce::Graphics& g)
{
    static const char* labels[] = { "3", "6", "12", "30" };
    static const DisplayRange ranges[] = { dB3, dB6, dB12, dB30 };

    g.setFont (10.0f);
    for (int i = 0; i < 4; ++i)
    {
        auto rect = getRangeButtonRect (i);
        bool isActive = (currentRange == ranges[i]);

        // Gradient fill — active = accent, inactive = inset
        if (isActive)
        {
            juce::ColourGradient grad (ColourPalette::compositeCurve().withAlpha (0.35f), 0, rect.getY(),
                                       ColourPalette::compositeCurve().withAlpha (0.15f), 0, rect.getBottom(), false);
            g.setGradientFill (grad);
        }
        else
        {
            juce::ColourGradient grad (ColourPalette::knobBackground().darker (0.2f), 0, rect.getY(),
                                       ColourPalette::knobBackground().brighter (0.1f), 0, rect.getBottom(), false);
            g.setGradientFill (grad);
        }
        g.fillRoundedRectangle (rect, 3.0f);

        // Bevel edges
        g.setColour (isActive ? ColourPalette::bevelLight().withMultipliedAlpha (1.5f)
                              : ColourPalette::bevelDark());
        g.drawLine (rect.getX() + 3.0f, rect.getY(), rect.getRight() - 3.0f, rect.getY(), 1.0f);

        // Outline
        g.setColour (ColourPalette::gridLine().withAlpha (0.5f));
        g.drawRoundedRectangle (rect, 3.0f, 0.5f);

        g.setColour (isActive ? ColourPalette::textBright() : ColourPalette::axisText());
        g.drawText (labels[i], rect, juce::Justification::centred, false);
    }
}

void EqCurveDisplay::timerCallback()
{
    updateCurves();

    // Update spectrum analyzers (skip when frozen to keep last captured state)
    auto* analyzerOnParam = processor.getAPVTS().getRawParameterValue (ParamIDs::kAnalyzerOn);
    if (analyzerOnParam != nullptr && analyzerOnParam->load() >= 0.5f && ! analyzerFrozen)
    {
        float w = (float) getWidth();
        float h = (float) getHeight();
        if (w > 0 && h > 0)
        {
            spectrumAnalyzerPre.update (processor.getFFTProcessorPre(), w, h);
            spectrumAnalyzerPost.update (processor.getFFTProcessorPost(), w, h);
        }
    }

    repaint();
}

void EqCurveDisplay::updateCurves()
{
    auto& apvts = processor.getAPVTS();
    double sr = processor.getSampleRate();
    if (sr <= 0.0)
        sr = 44100.0;

    // Zero all accumulators
    compositeMagnitudes.fill (0.0);
    std::array<double, Constants::kCurveResolution> stereoAccum {};
    leftCompositeMagnitudes.fill (0.0);
    rightCompositeMagnitudes.fill (0.0);
    midCompositeMagnitudes.fill (0.0);
    sideCompositeMagnitudes.fill (0.0);

    bool hasLeft = false, hasRight = false, hasMid = false, hasSide = false;

    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        auto* activeParam = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kActive));
        if (activeParam->load() < 0.5f)
        {
            // Zero inactive band magnitudes
            bandMagnitudes[(size_t) b].fill (0.0);
            continue;
        }

        // Read all parameters directly from APVTS for lag-free UI response
        auto* freqParam  = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kFrequency));
        auto* gainParam  = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kGain));
        auto* qParam     = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kQ));
        auto* typeParam  = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kType));
        auto* slopeParam = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kSlope));
        auto* stereoParam = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kStereo));

        auto filterType = static_cast<FilterType> ((int) typeParam->load());
        double freq  = (double) freqParam->load();
        double gain  = (double) gainParam->load();
        double q     = (double) qParam->load();
        int slopeDB  = slopeIndexToDBPerOct ((int) slopeParam->load());
        auto placement = static_cast<StereoPlacement> ((int) stereoParam->load());

        // Configure UI-side filter from current parameters (no DSP-layer lag)
        auto& uiFilter = uiFilters[(size_t) b];
        uiFilter.prepare (sr);
        uiFilter.setParameters (filterType, freq, q, gain, slopeDB);

        // Compute magnitude response
        auto& mags = bandMagnitudes[(size_t) b];
        for (int i = 0; i < Constants::kCurveResolution; ++i)
            mags[(size_t) i] = uiFilter.getMagnitudeAtFrequency (frequencyPoints[(size_t) i]);

        // Accumulate into all-bands composite
        for (int i = 0; i < Constants::kCurveResolution; ++i)
            compositeMagnitudes[(size_t) i] += mags[(size_t) i];

        // Partition into per-channel buckets
        switch (placement)
        {
            case StereoPlacement::Stereo:
                for (int i = 0; i < Constants::kCurveResolution; ++i)
                    stereoAccum[(size_t) i] += mags[(size_t) i];
                break;
            case StereoPlacement::Left:
                hasLeft = true;
                for (int i = 0; i < Constants::kCurveResolution; ++i)
                    leftCompositeMagnitudes[(size_t) i] += mags[(size_t) i];
                break;
            case StereoPlacement::Right:
                hasRight = true;
                for (int i = 0; i < Constants::kCurveResolution; ++i)
                    rightCompositeMagnitudes[(size_t) i] += mags[(size_t) i];
                break;
            case StereoPlacement::Mid:
                hasMid = true;
                for (int i = 0; i < Constants::kCurveResolution; ++i)
                    midCompositeMagnitudes[(size_t) i] += mags[(size_t) i];
                break;
            case StereoPlacement::Side:
                hasSide = true;
                for (int i = 0; i < Constants::kCurveResolution; ++i)
                    sideCompositeMagnitudes[(size_t) i] += mags[(size_t) i];
                break;
        }
    }

    // Add stereo base to each active channel composite
    showPerChannelLR = hasLeft || hasRight;
    showPerChannelMS = hasMid || hasSide;

    if (showPerChannelLR)
    {
        for (int i = 0; i < Constants::kCurveResolution; ++i)
        {
            leftCompositeMagnitudes[(size_t) i] += stereoAccum[(size_t) i];
            rightCompositeMagnitudes[(size_t) i] += stereoAccum[(size_t) i];
        }
    }

    if (showPerChannelMS)
    {
        for (int i = 0; i < Constants::kCurveResolution; ++i)
        {
            midCompositeMagnitudes[(size_t) i] += stereoAccum[(size_t) i];
            sideCompositeMagnitudes[(size_t) i] += stereoAccum[(size_t) i];
        }
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

    // Recessed screen inner shadows
    {
        float w = (float) getWidth();
        float h = (float) getHeight();
        auto shadow = ColourPalette::screenShadow();
        auto transparent = shadow.withAlpha (0.0f);

        // Top inner shadow
        juce::ColourGradient topShadow (shadow, 0, 0, transparent, 0, 8.0f, false);
        g.setGradientFill (topShadow);
        g.fillRect (0.0f, 0.0f, w, 8.0f);

        // Left inner shadow
        juce::ColourGradient leftShadow (shadow, 0, 0, transparent, 6.0f, 0, false);
        g.setGradientFill (leftShadow);
        g.fillRect (0.0f, 0.0f, 6.0f, h);

        // Bottom catch-light
        g.setColour (ColourPalette::screenHighlight());
        g.drawLine (0.0f, h - 0.5f, w, h - 0.5f, 1.0f);

        // Right catch-light
        g.drawLine (w - 0.5f, 0.0f, w - 0.5f, h, 1.0f);
    }

    drawGrid (g);

    // Spectrum analyzer (drawn behind EQ curves)
    auto* analyzerOnParam = processor.getAPVTS().getRawParameterValue (ParamIDs::kAnalyzerOn);
    if (analyzerOnParam != nullptr && analyzerOnParam->load() >= 0.5f)
    {
        // Always draw both pre and post spectra for visual comparison
        drawSpectrum (g, spectrumAnalyzerPre, ColourPalette::spectrumPre());
        drawSpectrum (g, spectrumAnalyzerPost, ColourPalette::spectrumPost());
    }

    drawBandCurves (g);
    if (showPreview && ! sketchMode)
        drawPreviewCurve (g);
    if (sketchMode && ! sketchPoints.empty())
        drawSketchPath (g);
    drawCompositeCurve (g);
    drawBandHandles (g);
    drawFrequencyLabels (g);
    drawGainLabels (g);
    drawRangeButtons (g);
    drawTooltip (g);
}

void EqCurveDisplay::drawGrid (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Sub-octave frequency lines (very faint, fills in the gaps)
    static const float subFreqLines[] = { 30, 40, 60, 70, 80, 150, 250, 300, 400, 600,
                                          700, 800, 1500, 2500, 3000, 4000, 6000, 7000,
                                          8000, 15000 };
    g.setColour (ColourPalette::gridLine().withMultipliedAlpha (0.4f));
    for (float freq : subFreqLines)
    {
        float x = frequencyToX (freq);
        g.drawLine (x, bounds.getY(), x, bounds.getBottom(), 1.0f);
    }

    // Octave frequency lines (labeled frequencies)
    static const float freqLines[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float freq : freqLines)
    {
        float x = frequencyToX (freq);
        bool isMajor = (freq > 99.0f && freq < 101.0f)
                     || (freq > 999.0f && freq < 1001.0f)
                     || (freq > 9999.0f && freq < 10001.0f);

        if (isMajor)
        {
            // Decade marks: brighter, accent-tinted, slightly wider
            g.setColour (ColourPalette::compositeCurve().withAlpha (0.12f));
            g.drawLine (x, bounds.getY(), x, bounds.getBottom(), 1.5f);
        }
        else
        {
            g.setColour (ColourPalette::gridLineStrong());
            g.drawLine (x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
    }

    // Gain grid lines (anti-aliased, dynamic step)
    float step = getGridStep();
    for (float db = displayMinDB; db <= displayMaxDB; db += step)
    {
        float y = gainToY (db);
        bool isZero = (std::abs (db) < 0.1f);

        if (isZero)
        {
            // 0 dB line: accent-tinted like decade frequency marks
            g.setColour (ColourPalette::compositeCurve().withAlpha (0.12f));
            g.drawLine (bounds.getX(), y, bounds.getRight(), y, 1.5f);
        }
        else
        {
            g.setColour (ColourPalette::gridLine());
            g.drawLine (bounds.getX(), y, bounds.getRight(), y, 1.0f);
        }
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

    float step = getGridStep();
    for (float db = displayMinDB + step; db < displayMaxDB; db += step)
    {
        float y = gainToY (db);
        juce::String label;
        if (step >= 1.0f && std::abs (step - std::floor (step)) < 0.01f)
            label = (db > 0 ? "+" : "") + juce::String ((int) db);
        else
            label = (db > 0 ? "+" : "") + juce::String (db, 1);
        g.drawText (label, getWidth() - 32, (int) (y - 7), 30, 14,
                    juce::Justification::centredRight, false);
    }
}

void EqCurveDisplay::drawBandCurves (juce::Graphics& g)
{
    float w = (float) getWidth();

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
    // When per-channel curves are active, draw those instead of the single yellow composite
    if (showPerChannelLR || showPerChannelMS)
    {
        drawPerChannelCompositeCurves (g);
        return;
    }

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

    // Fill between composite curve and 0 dB line
    float zeroY = gainToY (0.0f);
    juce::Path fillPath (curvePath);
    fillPath.lineTo (w, zeroY);
    fillPath.lineTo (0, zeroY);
    fillPath.closeSubPath();
    g.setColour (ColourPalette::compositeFill());
    g.fillPath (fillPath);

    // Draw composite curve stroke
    g.setColour (ColourPalette::compositeCurve());
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));
}

void EqCurveDisplay::drawPerChannelCompositeCurves (juce::Graphics& g)
{
    float w = (float) getWidth();
    float zeroY = gainToY (0.0f);

    // Helper lambda to draw a single per-channel composite curve
    auto drawChannelCurve = [&] (const std::array<double, Constants::kCurveResolution>& mags,
                                  juce::Colour colour)
    {
        juce::Path curvePath;
        bool started = false;

        for (int i = 0; i < Constants::kCurveResolution; ++i)
        {
            float x = (float) i / (float) (Constants::kCurveResolution - 1) * w;
            float y = gainToY ((float) mags[(size_t) i]);

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

        // Fill between curve and 0 dB line
        juce::Path fillPath (curvePath);
        fillPath.lineTo (w, zeroY);
        fillPath.lineTo (0, zeroY);
        fillPath.closeSubPath();
        g.setColour (colour.withAlpha (0.08f));
        g.fillPath (fillPath);

        // Stroke
        g.setColour (colour);
        g.strokePath (curvePath, juce::PathStrokeType (2.0f));
    };

    if (showPerChannelLR)
    {
        drawChannelCurve (leftCompositeMagnitudes, ColourPalette::leftColour());
        drawChannelCurve (rightCompositeMagnitudes, ColourPalette::rightColour());
    }

    if (showPerChannelMS)
    {
        drawChannelCurve (midCompositeMagnitudes, ColourPalette::midColour());
        drawChannelCurve (sideCompositeMagnitudes, ColourPalette::sideColour());
    }
}

void EqCurveDisplay::drawPreviewCurve (juce::Graphics& g)
{
    float previewFreq = xToFrequency (previewMouseX);
    float w = (float) getWidth();

    // Determine colour for the next band that would be created
    int nextBand = findFirstInactiveBand();
    if (nextBand < 0)
        return;
    auto colour = ColourPalette::bandColour (nextBand);

    // Configure preview filter as a Bell at the hover position
    previewFilter.setParameters (FilterType::Bell, (double) previewFreq, 1.0, 0.0);

    // Draw faint crosshair at mouse position
    g.setColour (colour.withAlpha (0.08f));
    g.drawLine (previewMouseX, 0.0f, previewMouseX, (float) getHeight(), 1.0f);
    g.drawLine (0.0f, previewMouseY, w, previewMouseY, 1.0f);

    // Draw a subtle bell curve preview (showing shape, not actual boost)
    // Use a small gain to show the curve shape
    previewFilter.setParameters (FilterType::Bell, (double) previewFreq, 1.0, 6.0);

    juce::Path curvePath;
    bool started = false;

    for (int i = 0; i < Constants::kCurveResolution; ++i)
    {
        float x = (float) i / (float) (Constants::kCurveResolution - 1) * w;
        double mag = previewFilter.getMagnitudeAtFrequency (frequencyPoints[(size_t) i]);
        float y = gainToY ((float) mag);

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

    g.setColour (colour.withAlpha (0.15f));
    g.strokePath (curvePath, juce::PathStrokeType (1.5f));

    // Subtle fill
    float zeroY = gainToY (0.0f);
    juce::Path fillPath (curvePath);
    fillPath.lineTo (w, zeroY);
    fillPath.lineTo (0, zeroY);
    fillPath.closeSubPath();
    g.setColour (colour.withAlpha (0.06f));
    g.fillPath (fillPath);
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
        bool isSelected = (b == selectedBand);
        bool isHovered = (b == hoveredBand);
        float radius = isSelected ? 5.0f : 4.0f;

        // Stereo placement indicator (tint outer ring for non-Stereo placements)
        auto* stereoParam = apvts.getRawParameterValue (
            ParamIDs::bandParam (b, ParamIDs::kStereo));
        auto placement = static_cast<StereoPlacement> ((int) stereoParam->load());

        juce::Colour ringColour = colour;
        if (placement == StereoPlacement::Left)       ringColour = ColourPalette::leftColour();
        else if (placement == StereoPlacement::Right)  ringColour = ColourPalette::rightColour();
        else if (placement == StereoPlacement::Mid)    ringColour = ColourPalette::midColour();
        else if (placement == StereoPlacement::Side)   ringColour = ColourPalette::sideColour();

        // Hover glow
        if (isHovered || isSelected)
        {
            g.setColour (colour.withAlpha (0.25f));
            float glowRadius = radius + 6.0f;
            g.fillEllipse (x - glowRadius, y - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);
        }

        // Filled dot
        g.setColour (colour.withAlpha (isHovered ? 0.95f : 0.8f));
        g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);

        // Outer ring (stereo indicator)
        if (placement != StereoPlacement::Stereo)
        {
            g.setColour (ringColour);
            g.drawEllipse (x - radius - 1.0f, y - radius - 1.0f,
                           (radius + 1.0f) * 2.0f, (radius + 1.0f) * 2.0f, 1.5f);

            // Channel letter label below the handle dot
            const char* label = nullptr;
            if (placement == StereoPlacement::Left)       label = "L";
            else if (placement == StereoPlacement::Right)  label = "R";
            else if (placement == StereoPlacement::Mid)    label = "M";
            else if (placement == StereoPlacement::Side)   label = "S";

            if (label != nullptr)
            {
                g.setFont (juce::Font (11.0f, juce::Font::bold));
                auto labelRect = juce::Rectangle<float> (x - 7.0f, y + radius + 2.0f, 14.0f, 13.0f);

                // Dark background pill for contrast
                g.setColour (juce::Colour (0xcc1a1a2e));
                g.fillRoundedRectangle (labelRect, 3.0f);

                g.setColour (ringColour);
                g.drawText (label, labelRect.toNearestInt(),
                            juce::Justification::centred, false);
            }
        }
    }
}

void EqCurveDisplay::drawSpectrum (juce::Graphics& g,
                                    const SpectrumAnalyzer& analyzer,
                                    juce::Colour colour)
{
    const auto& fillPath = analyzer.getFillPath();
    const auto& strokePath = analyzer.getPath();

    if (fillPath.isEmpty())
        return;

    // Filled area
    g.setColour (colour);
    g.fillPath (fillPath);

    // Top edge stroke (slightly brighter)
    g.setColour (colour.withMultipliedAlpha (1.5f));
    g.strokePath (strokePath, juce::PathStrokeType (1.0f));
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
    auto* typeParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kType));
    auto* slopeParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kSlope));

    float freq = freqParam->load();
    float gain = gainParam->load();
    float q = qParam->load();
    int typeIndex = (int) typeParam->load();
    int slopeIndex = (int) slopeParam->load();

    auto filterType = static_cast<FilterType> (typeIndex);
    auto typeName = ParamIDs::getFilterTypeNames()[typeIndex];

    // Show slope only for types where it applies
    bool showSlope = (filterType == FilterType::LowPass
                   || filterType == FilterType::HighPass
                   || filterType == FilterType::LowShelf
                   || filterType == FilterType::HighShelf);

    juce::String text = typeName + "  "
                      + FrequencyUtils::formatFrequency (freq) + "  "
                      + juce::String (gain, 1) + " dB  Q " + juce::String (q, 2);

    if (showSlope)
        text += "  " + ParamIDs::getSlopeNames()[slopeIndex];

    // Show stereo placement when not default "Stereo"
    auto* stereoParam = apvts.getRawParameterValue (
        ParamIDs::bandParam (band, ParamIDs::kStereo));
    int stereoIndex = (int) stereoParam->load();
    if (stereoIndex > 0) // 0 = Stereo (default)
        text += "  " + ParamIDs::getStereoPlacementNames()[stereoIndex];

    float x = frequencyToX (freq);
    float y = gainToY (gain) - 22.0f;

    auto textWidth = g.getCurrentFont().getStringWidthFloat (text) + 12.0f;
    auto tooltipBounds = juce::Rectangle<float> (x - textWidth / 2, y - 10, textWidth, 18.0f);

    // Clamp to display bounds
    if (tooltipBounds.getX() < 0)
        tooltipBounds.setX (0);
    if (tooltipBounds.getRight() > (float) getWidth())
        tooltipBounds.setX ((float) getWidth() - textWidth);

    // Frosted glass background
    juce::ColourGradient tooltipGrad (ColourPalette::controlBar().withAlpha (0.92f), 0, tooltipBounds.getY(),
                                      ColourPalette::panelBottom().withAlpha (0.88f), 0, tooltipBounds.getBottom(), false);
    g.setGradientFill (tooltipGrad);
    g.fillRoundedRectangle (tooltipBounds, 4.0f);

    // 3D border edges
    g.setColour (ColourPalette::bevelLight());
    g.drawLine (tooltipBounds.getX() + 4.0f, tooltipBounds.getY(),
                tooltipBounds.getRight() - 4.0f, tooltipBounds.getY(), 0.5f);
    g.drawLine (tooltipBounds.getX(), tooltipBounds.getY() + 4.0f,
                tooltipBounds.getX(), tooltipBounds.getBottom() - 4.0f, 0.5f);
    g.setColour (ColourPalette::bevelDark());
    g.drawLine (tooltipBounds.getX() + 4.0f, tooltipBounds.getBottom(),
                tooltipBounds.getRight() - 4.0f, tooltipBounds.getBottom(), 0.5f);
    g.drawLine (tooltipBounds.getRight(), tooltipBounds.getY() + 4.0f,
                tooltipBounds.getRight(), tooltipBounds.getBottom() - 4.0f, 0.5f);

    // Subtle outline
    g.setColour (ColourPalette::gridLine().withAlpha (0.5f));
    g.drawRoundedRectangle (tooltipBounds, 4.0f, 0.5f);

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

    // Check range button clicks
    if (e.mods.isLeftButtonDown())
    {
        static const DisplayRange ranges[] = { dB3, dB6, dB12, dB30 };
        for (int i = 0; i < 4; ++i)
        {
            if (getRangeButtonRect (i).contains (pos))
            {
                setDisplayRange (ranges[i]);
                return;
            }
        }
    }

    // Sketch mode: capture drawing
    if (sketchMode && e.mods.isLeftButtonDown() && ! e.mods.isPopupMenu())
    {
        isDrawingSketch = true;
        sketchPoints.clear();
        sketchPoints.push_back (pos);
        return;
    }

    int band = findBandAtPosition (pos);

    // Right-click context menu
    if (e.mods.isPopupMenu())
    {
        if (band >= 0)
            showBandContextMenu (band);
        else
            showEmptySpaceContextMenu (xToFrequency (pos.x), yToGain (pos.y));
        return;
    }

    if (band >= 0)
    {
        selectedBand = band;
        isDragging = true;
    }
    else
    {
        // Spectrum grab: check for spectrum peaks near click position
        float clickFreq = xToFrequency (pos.x);
        float peakFreq = spectrumAnalyzerPost.findNearestPeak (clickFreq, 0.3f);

        if (peakFreq > 0.0f)
        {
            // Create a new band at the spectrum peak frequency, gain = 0 dB (ready to drag)
            activateBandAt (peakFreq, 0.0f);
        }
        else
        {
            // Click on empty space: create new band at this position
            float freq = clickFreq;
            float gain = yToGain (pos.y);
            activateBandAt (freq, gain);
        }
        isDragging = true;
    }

    if (onBandSelected)
        onBandSelected (selectedBand);

    repaint();
}

void EqCurveDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (isDrawingSketch)
    {
        sketchPoints.push_back (e.position);
        repaint();
        return;
    }

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
    if (isDrawingSketch)
    {
        isDrawingSketch = false;
        processSketch();
        sketchPoints.clear();
        repaint();
        return;
    }
    isDragging = false;
}

void EqCurveDisplay::mouseMove (const juce::MouseEvent& e)
{
    int band = findBandAtPosition (e.position);

    bool needsRepaint = false;
    if (band != hoveredBand)
    {
        hoveredBand = band;
        needsRepaint = true;
    }

    // Show preview curve when hovering over empty space
    bool shouldShowPreview = (band < 0 && ! isDragging);
    if (shouldShowPreview != showPreview
        || (shouldShowPreview && (std::abs (previewMouseX - e.position.x) > 0.5f
                               || std::abs (previewMouseY - e.position.y) > 0.5f)))
    {
        showPreview = shouldShowPreview;
        previewMouseX = e.position.x;
        previewMouseY = e.position.y;
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void EqCurveDisplay::mouseExit (const juce::MouseEvent&)
{
    if (showPreview || hoveredBand >= 0)
    {
        showPreview = false;
        hoveredBand = -1;
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
        {
            selectedBand = -1;
            if (onBandSelected)
                onBandSelected (selectedBand);
        }
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

void EqCurveDisplay::showBandContextMenu (int band)
{
    juce::PopupMenu menu;
    auto& apvts = processor.getAPVTS();

    // Bypass / Enable
    auto* activeParam = apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kActive));
    bool isActive = (activeParam->load() >= 0.5f);
    menu.addItem (1, isActive ? "Bypass Band" : "Enable Band");
    menu.addItem (2, "Delete Band");
    menu.addSeparator();

    // Filter type submenu
    juce::PopupMenu typeMenu;
    auto typeNames = ParamIDs::getFilterTypeNames();
    for (int i = 0; i < typeNames.size(); ++i)
        typeMenu.addItem (100 + i, typeNames[i]);
    menu.addSubMenu ("Set Type", typeMenu);

    // Slope submenu
    juce::PopupMenu slopeMenu;
    auto slopeNames = ParamIDs::getSlopeNames();
    for (int i = 0; i < slopeNames.size(); ++i)
        slopeMenu.addItem (200 + i, slopeNames[i]);
    menu.addSubMenu ("Set Slope", slopeMenu);

    // Stereo submenu
    juce::PopupMenu stereoMenu;
    auto stereoNames = ParamIDs::getStereoPlacementNames();
    for (int i = 0; i < stereoNames.size(); ++i)
        stereoMenu.addItem (300 + i, stereoNames[i]);
    menu.addSubMenu ("Set Stereo", stereoMenu);

    menu.addSeparator();
    menu.addItem (3, "Reset Band");
    menu.addItem (4, "Split L/R");
    menu.addItem (5, "Copy Band");
    menu.addItem (6, "Paste Band");

    menu.showMenuAsync (juce::PopupMenu::Options(),
        [this, band] (int result)
        {
            if (result == 0)
                return;

            auto& apvts2 = processor.getAPVTS();

            if (result == 1) // Bypass / Enable
            {
                if (auto* param = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kActive)))
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost (current >= 0.5f ? 0.0f : 1.0f);
                }
            }
            else if (result == 2) // Delete
            {
                if (auto* param = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kActive)))
                    param->setValueNotifyingHost (0.0f);

                if (selectedBand == band)
                {
                    selectedBand = -1;
                    if (onBandSelected)
                        onBandSelected (selectedBand);
                }
            }
            else if (result == 3) // Reset
            {
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kFrequency)))
                    p->setValueNotifyingHost (p->convertTo0to1 (Constants::kDefaultFrequency));
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kGain)))
                    p->setValueNotifyingHost (p->convertTo0to1 (Constants::kDefaultGain));
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kQ)))
                    p->setValueNotifyingHost (p->convertTo0to1 (Constants::kDefaultQ));
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kType)))
                    p->setValueNotifyingHost (p->convertTo0to1 (0.0f)); // Bell
            }
            else if (result >= 100 && result < 200) // Filter type
            {
                int typeIndex = result - 100;
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kType)))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) typeIndex));
            }
            else if (result >= 200 && result < 300) // Slope
            {
                int slopeIndex = result - 200;
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kSlope)))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) slopeIndex));
            }
            else if (result >= 300 && result < 400) // Stereo
            {
                int stereoIndex = result - 300;
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kStereo)))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) stereoIndex));
            }
            else if (result == 4) // Split L/R
            {
                // Find two inactive bands
                int bandL = -1, bandR = -1;
                for (int b = 0; b < Constants::kMaxBands; ++b)
                {
                    auto* active = apvts2.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kActive));
                    if (b != band && active->load() < 0.5f)
                    {
                        if (bandL < 0) bandL = b;
                        else if (bandR < 0) { bandR = b; break; }
                    }
                }

                if (bandL < 0 || bandR < 0)
                    return; // Not enough inactive bands

                // Copy all params from source to both new bands
                auto copyBandParam = [&] (int src, int dst)
                {
                    static const char* suffixes[] = {
                        ParamIDs::kType, ParamIDs::kFrequency, ParamIDs::kGain, ParamIDs::kQ,
                        ParamIDs::kSlope, ParamIDs::kDynEnabled, ParamIDs::kDynThresh,
                        ParamIDs::kDynRange, ParamIDs::kDynAttack, ParamIDs::kDynRelease
                    };
                    for (auto* suffix : suffixes)
                    {
                        auto* srcP = apvts2.getParameter (ParamIDs::bandParam (src, suffix));
                        auto* dstP = apvts2.getParameter (ParamIDs::bandParam (dst, suffix));
                        if (srcP && dstP)
                            dstP->setValueNotifyingHost (srcP->getValue());
                    }
                };

                copyBandParam (band, bandL);
                copyBandParam (band, bandR);

                // Set stereo placement
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (bandL, ParamIDs::kStereo)))
                    p->setValueNotifyingHost (p->convertTo0to1 (1.0f)); // Left
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (bandR, ParamIDs::kStereo)))
                    p->setValueNotifyingHost (p->convertTo0to1 (2.0f)); // Right

                // Activate both
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (bandL, ParamIDs::kActive)))
                    p->setValueNotifyingHost (1.0f);
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (bandR, ParamIDs::kActive)))
                    p->setValueNotifyingHost (1.0f);

                // Deactivate original
                if (auto* p = apvts2.getParameter (ParamIDs::bandParam (band, ParamIDs::kActive)))
                    p->setValueNotifyingHost (0.0f);

                selectedBand = bandL;
                if (onBandSelected)
                    onBandSelected (selectedBand);
            }
            else if (result == 5) // Copy Band
            {
                copyBandToClipboard (band);
            }
            else if (result == 6) // Paste Band
            {
                pasteBandFromClipboard (band);
            }

            repaint();
        });
}

void EqCurveDisplay::showEmptySpaceContextMenu (float freq, float gain)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Add Band Here");
    menu.addItem (7, "Paste Band Here");

    menu.showMenuAsync (juce::PopupMenu::Options(),
        [this, freq, gain] (int result)
        {
            if (result == 1)
            {
                activateBandAt (freq, gain);
                if (onBandSelected)
                    onBandSelected (selectedBand);
                repaint();
            }
            else if (result == 7)
            {
                int newBand = findFirstInactiveBand();
                if (newBand >= 0)
                {
                    auto& apvts2 = processor.getAPVTS();
                    if (auto* p = apvts2.getParameter (ParamIDs::bandParam (newBand, ParamIDs::kActive)))
                        p->setValueNotifyingHost (1.0f);
                    pasteBandFromClipboard (newBand);
                    selectedBand = newBand;
                    if (onBandSelected)
                        onBandSelected (selectedBand);
                    repaint();
                }
            }
        });
}

// --- Band Clipboard ---

void EqCurveDisplay::copyBandToClipboard (int band)
{
    auto& apvts = processor.getAPVTS();

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("type",        (int) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kType))->load());
    obj->setProperty ("frequency",   (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kFrequency))->load());
    obj->setProperty ("gain",        (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kGain))->load());
    obj->setProperty ("q",           (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kQ))->load());
    obj->setProperty ("slope",       (int) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kSlope))->load());
    obj->setProperty ("stereo",      (int) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kStereo))->load());
    obj->setProperty ("dyn_on",      (bool) (apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kDynEnabled))->load() >= 0.5f));
    obj->setProperty ("dyn_thresh",  (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kDynThresh))->load());
    obj->setProperty ("dyn_range",   (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kDynRange))->load());
    obj->setProperty ("dyn_attack",  (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kDynAttack))->load());
    obj->setProperty ("dyn_release", (double) apvts.getRawParameterValue (ParamIDs::bandParam (band, ParamIDs::kDynRelease))->load());

    juce::String json = "AdvancedEQ_Band_v1:" + juce::JSON::toString (juce::var (obj.get()));
    juce::SystemClipboard::copyTextToClipboard (json);
}

void EqCurveDisplay::pasteBandFromClipboard (int band)
{
    auto text = juce::SystemClipboard::getTextFromClipboard();
    if (! text.startsWith ("AdvancedEQ_Band_v1:"))
        return;

    auto json = text.fromFirstOccurrenceOf ("AdvancedEQ_Band_v1:", false, false);
    auto parsed = juce::JSON::parse (json);
    if (! parsed.isObject())
        return;

    auto& apvts = processor.getAPVTS();
    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    auto setParam = [&] (const char* suffix, const juce::String& propName)
    {
        if (obj->hasProperty (propName))
        {
            if (auto* p = apvts.getParameter (ParamIDs::bandParam (band, suffix)))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) (double) obj->getProperty (propName)));
        }
    };

    setParam (ParamIDs::kType,       "type");
    setParam (ParamIDs::kFrequency,  "frequency");
    setParam (ParamIDs::kGain,       "gain");
    setParam (ParamIDs::kQ,          "q");
    setParam (ParamIDs::kSlope,      "slope");
    setParam (ParamIDs::kStereo,     "stereo");
    setParam (ParamIDs::kDynThresh,  "dyn_thresh");
    setParam (ParamIDs::kDynRange,   "dyn_range");
    setParam (ParamIDs::kDynAttack,  "dyn_attack");
    setParam (ParamIDs::kDynRelease, "dyn_release");

    // Handle dynamic enable (bool)
    if (obj->hasProperty ("dyn_on"))
    {
        if (auto* p = apvts.getParameter (ParamIDs::bandParam (band, ParamIDs::kDynEnabled)))
            p->setValueNotifyingHost ((bool) obj->getProperty ("dyn_on") ? 1.0f : 0.0f);
    }
}

// --- EQ Sketch ---

void EqCurveDisplay::drawSketchPath (juce::Graphics& g)
{
    if (sketchPoints.size() < 2)
        return;

    juce::Path path;
    path.startNewSubPath (sketchPoints[0]);
    for (size_t i = 1; i < sketchPoints.size(); ++i)
        path.lineTo (sketchPoints[i]);

    g.setColour (juce::Colour (0xaaffcc00));
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

void EqCurveDisplay::processSketch()
{
    if (sketchPoints.size() < 3)
        return;

    // Resample sketch points to 512-point log-frequency grid
    constexpr int gridSize = 512;
    std::vector<float> sketchCurve (gridSize, 0.0f);
    float w = (float) getWidth();

    for (int i = 0; i < gridSize; ++i)
    {
        float x = (float) i / (float) (gridSize - 1) * w;

        // Find nearest sketch points and interpolate
        float bestDist = 999999.0f;
        float value = 0.0f;
        for (auto& pt : sketchPoints)
        {
            float dist = std::abs (pt.x - x);
            if (dist < bestDist)
            {
                bestDist = dist;
                value = yToGain (pt.y);
            }
        }
        sketchCurve[(size_t) i] = value;
    }

    // Smooth with 5-point moving average
    std::vector<float> smoothed (gridSize, 0.0f);
    for (int i = 0; i < gridSize; ++i)
    {
        float sum = 0.0f;
        int count = 0;
        for (int j = i - 2; j <= i + 2; ++j)
        {
            if (j >= 0 && j < gridSize)
            {
                sum += sketchCurve[(size_t) j];
                ++count;
            }
        }
        smoothed[(size_t) i] = sum / (float) count;
    }

    // Peak-pick: find up to 8 significant peaks/valleys
    struct SketchBand { float freq; float gain; float q; };
    std::vector<SketchBand> matchBands;

    std::vector<float> residual = smoothed;

    for (int iter = 0; iter < 8; ++iter)
    {
        int peakIdx = 0;
        float peakAbs = 0.0f;
        for (int i = 2; i < gridSize - 2; ++i)
        {
            float absVal = std::abs (residual[(size_t) i]);
            if (absVal > peakAbs)
            {
                peakAbs = absVal;
                peakIdx = i;
            }
        }

        if (peakAbs < 1.0f)
            break;

        float peakX = (float) peakIdx / (float) (gridSize - 1) * w;
        float freq = xToFrequency (peakX);
        float gain = residual[(size_t) peakIdx];

        // Estimate Q from peak width
        float halfGain = peakAbs * 0.5f;
        int leftIdx = peakIdx, rightIdx = peakIdx;
        for (int i = peakIdx - 1; i >= 0; --i)
        {
            if (std::abs (residual[(size_t) i]) < halfGain) { leftIdx = i; break; }
            leftIdx = i;
        }
        for (int i = peakIdx + 1; i < gridSize; ++i)
        {
            if (std::abs (residual[(size_t) i]) < halfGain) { rightIdx = i; break; }
            rightIdx = i;
        }

        float leftFreq = xToFrequency ((float) leftIdx / (float) (gridSize - 1) * w);
        float rightFreq = xToFrequency ((float) rightIdx / (float) (gridSize - 1) * w);
        float bw = rightFreq - leftFreq;
        float q = (bw > 0.0f) ? freq / bw : 1.0f;
        q = juce::jlimit (0.1f, 10.0f, q);
        gain = juce::jlimit (-15.0f, 15.0f, gain);

        matchBands.push_back ({ freq, gain, q });

        // Subtract bell response from residual
        for (int i = 0; i < gridSize; ++i)
        {
            float bx = (float) i / (float) (gridSize - 1) * w;
            float bf = xToFrequency (bx);
            float logRatio = std::log2 (bf / freq);
            float bellMag = gain * std::exp (-0.5f * logRatio * logRatio * q * q * 4.0f);
            residual[(size_t) i] -= bellMag;
        }
    }

    // Activate bands
    auto& apvts = processor.getAPVTS();
    for (auto& mb : matchBands)
    {
        int bandIdx = findFirstInactiveBand();
        if (bandIdx < 0)
            break;

        if (auto* p = apvts.getParameter (ParamIDs::bandParam (bandIdx, ParamIDs::kActive)))
            p->setValueNotifyingHost (1.0f);
        if (auto* p = apvts.getParameter (ParamIDs::bandParam (bandIdx, ParamIDs::kType)))
            p->setValueNotifyingHost (p->convertTo0to1 (0.0f)); // Bell
        if (auto* p = apvts.getParameter (ParamIDs::bandParam (bandIdx, ParamIDs::kFrequency)))
            p->setValueNotifyingHost (p->convertTo0to1 (mb.freq));
        if (auto* p = apvts.getParameter (ParamIDs::bandParam (bandIdx, ParamIDs::kGain)))
            p->setValueNotifyingHost (p->convertTo0to1 (mb.gain));
        if (auto* p = apvts.getParameter (ParamIDs::bandParam (bandIdx, ParamIDs::kQ)))
            p->setValueNotifyingHost (p->convertTo0to1 (mb.q));
    }
}
