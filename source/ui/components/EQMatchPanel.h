/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../PluginProcessor.h"
#include "../../dsp/EQMatcher.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * EQ Match panel overlay.
 *
 * Workflow: "Capture Reference" → "Capture Current" → "Match" → "Apply"
 * Captures averaged spectra from the FFT processor and uses EQMatcher
 * to fit Bell bands to the difference curve.
 */
class EQMatchPanel : public juce::Component,
                     public juce::Timer
{
public:
    explicit EQMatchPanel (AdvancedEQProcessor& p)
        : processor (p)
    {
        captureRefButton.setButtonText ("1. Capture Reference");
        captureRefButton.setColour (juce::TextButton::buttonColourId, ColourPalette::knobBackground());
        captureRefButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
        captureRefButton.onClick = [this] { onCaptureRef(); };
        addAndMakeVisible (captureRefButton);

        captureCurButton.setButtonText ("2. Capture Current");
        captureCurButton.setColour (juce::TextButton::buttonColourId, ColourPalette::knobBackground());
        captureCurButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
        captureCurButton.onClick = [this] { onCaptureCur(); };
        addAndMakeVisible (captureCurButton);

        matchButton.setButtonText ("3. Match");
        matchButton.setColour (juce::TextButton::buttonColourId, ColourPalette::knobBackground());
        matchButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
        matchButton.setEnabled (false);
        matchButton.onClick = [this] { onMatch(); };
        addAndMakeVisible (matchButton);

        applyButton.setButtonText ("4. Apply");
        applyButton.setColour (juce::TextButton::buttonColourId, ColourPalette::compositeCurve().withAlpha (0.3f));
        applyButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textBright());
        applyButton.setEnabled (false);
        applyButton.onClick = [this] { onApply(); };
        addAndMakeVisible (applyButton);

        closeButton.setButtonText ("X");
        closeButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        closeButton.setColour (juce::TextButton::textColourOffId, ColourPalette::textDim());
        closeButton.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeButton);

        statusLabel.setColour (juce::Label::textColourId, ColourPalette::axisText());
        statusLabel.setFont (12.0f);
        statusLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (statusLabel);

        // Component IDs for help system
        captureRefButton.setComponentID ("match.captureRef");
        captureCurButton.setComponentID ("match.captureCur");
        matchButton.setComponentID ("match.compute");
        applyButton.setComponentID ("match.apply");
        closeButton.setComponentID ("match.close");
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Panel background with rounded corners
        g.setColour (ColourPalette::panelBottom().withAlpha (0.95f));
        g.fillRoundedRectangle (bounds, 6.0f);

        g.setColour (ColourPalette::gridLine());
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

        // Title
        g.setFont (14.0f);
        g.setColour (ColourPalette::textBright());
        g.drawText ("EQ Match", bounds.removeFromTop (24).reduced (8, 0),
                     juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (24); // Title space

        auto row1 = bounds.removeFromTop (28);
        captureRefButton.setBounds (row1.removeFromLeft (row1.getWidth() / 2 - 2));
        row1.removeFromLeft (4);
        captureCurButton.setBounds (row1);

        bounds.removeFromTop (4);

        auto row2 = bounds.removeFromTop (28);
        matchButton.setBounds (row2.removeFromLeft (row2.getWidth() / 2 - 2));
        row2.removeFromLeft (4);
        applyButton.setBounds (row2);

        bounds.removeFromTop (4);
        statusLabel.setBounds (bounds.removeFromTop (18));

        // Close button in top-right corner
        closeButton.setBounds (getWidth() - 24, 2, 22, 22);
    }

    void timerCallback() override
    {
        // Check if capture is complete
        if (capturingRef)
        {
            captureTimer += 1;
            if (captureTimer >= captureFrames)
            {
                processor.getFFTProcessorPost().stopCapture();
                referenceSpectrum = processor.getFFTProcessorPost().getAverageSpectrum();
                hasReference = true;
                capturingRef = false;
                stopTimer();
                statusLabel.setText ("Reference captured. Now capture current signal.",
                                     juce::dontSendNotification);
                updateButtonStates();
            }
            else
            {
                statusLabel.setText ("Capturing reference... " +
                                     juce::String ((int) (100.0f * captureTimer / captureFrames)) + "%",
                                     juce::dontSendNotification);
            }
        }
        else if (capturingCur)
        {
            captureTimer += 1;
            if (captureTimer >= captureFrames)
            {
                processor.getFFTProcessorPost().stopCapture();
                currentSpectrum = processor.getFFTProcessorPost().getAverageSpectrum();
                hasCurrent = true;
                capturingCur = false;
                stopTimer();
                statusLabel.setText ("Current captured. Press Match to compute.",
                                     juce::dontSendNotification);
                updateButtonStates();
            }
            else
            {
                statusLabel.setText ("Capturing current... " +
                                     juce::String ((int) (100.0f * captureTimer / captureFrames)) + "%",
                                     juce::dontSendNotification);
            }
        }
    }

    /** Get the computed match bands for UI display. */
    const std::vector<EQMatcher::MatchBand>& getMatchBands() const { return matchBands; }
    bool hasMatchResult() const { return matchComputed; }

    /** Get the diff curve for overlay display. */
    const std::vector<float>& getDiffCurve() const { return diffCurve; }
    bool hasDiffCurve() const { return hasReference && hasCurrent; }

private:
    void onCaptureRef()
    {
        processor.getFFTProcessorPost().startCapture();
        capturingRef = true;
        capturingCur = false;
        captureTimer = 0;
        startTimerHz (10); // Check every 100ms
        statusLabel.setText ("Capturing reference...", juce::dontSendNotification);
    }

    void onCaptureCur()
    {
        processor.getFFTProcessorPost().startCapture();
        capturingCur = true;
        capturingRef = false;
        captureTimer = 0;
        startTimerHz (10);
        statusLabel.setText ("Capturing current...", juce::dontSendNotification);
    }

    void onMatch()
    {
        if (! hasReference || ! hasCurrent)
            return;

        int numBins = processor.getFFTProcessorPost().getNumBins();
        double sr = processor.getFFTProcessorPost().getSampleRate();
        int fftSize = processor.getFFTProcessorPost().getCurrentFFTSize();

        matchBands = EQMatcher::computeMatch (
            referenceSpectrum.data(), currentSpectrum.data(),
            numBins, sr, fftSize, 8);

        diffCurve = EQMatcher::computeDiffCurve (
            referenceSpectrum.data(), currentSpectrum.data(), numBins);

        matchComputed = true;
        statusLabel.setText ("Match computed: " + juce::String ((int) matchBands.size()) +
                             " bands. Press Apply to set EQ.",
                             juce::dontSendNotification);
        updateButtonStates();
    }

    void onApply()
    {
        if (! matchComputed)
            return;

        auto& apvts = processor.getAPVTS();
        auto& chain = processor.getProcessingChain();

        // Find inactive bands and apply match results
        int applied = 0;
        for (size_t i = 0; i < matchBands.size() && applied < (int) matchBands.size(); ++i)
        {
            // Find next inactive band
            int bandIdx = -1;
            for (int b = 0; b < Constants::kMaxBands; ++b)
            {
                auto* band = chain.getBand (b);
                if (band != nullptr && ! band->isActive())
                {
                    bandIdx = b;
                    break;
                }
            }

            if (bandIdx < 0)
                break;

            // Set parameters via APVTS
            auto setParam = [&apvts] (const juce::String& id, float value)
            {
                if (auto* param = apvts.getParameter (id))
                    param->setValueNotifyingHost (param->convertTo0to1 (value));
            };

            juce::String prefix = "band" + juce::String (bandIdx) + "_";
            setParam (prefix + "enabled", 1.0f);
            setParam (prefix + "type", 0.0f); // Bell
            setParam (prefix + "freq", matchBands[i].frequency);
            setParam (prefix + "gain", matchBands[i].gain);
            setParam (prefix + "q", matchBands[i].q);

            ++applied;
        }

        statusLabel.setText ("Applied " + juce::String (applied) + " match bands.",
                             juce::dontSendNotification);
    }

    void updateButtonStates()
    {
        matchButton.setEnabled (hasReference && hasCurrent);
        applyButton.setEnabled (matchComputed);
    }

    AdvancedEQProcessor& processor;

    juce::TextButton captureRefButton;
    juce::TextButton captureCurButton;
    juce::TextButton matchButton;
    juce::TextButton applyButton;
    juce::TextButton closeButton;
    juce::Label statusLabel;

    bool capturingRef = false;
    bool capturingCur = false;
    int captureTimer = 0;
    static constexpr int captureFrames = 30; // ~3 seconds at 10 Hz timer

    std::vector<float> referenceSpectrum;
    std::vector<float> currentSpectrum;
    std::vector<float> diffCurve;
    std::vector<EQMatcher::MatchBand> matchBands;

    bool hasReference = false;
    bool hasCurrent = false;
    bool matchComputed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQMatchPanel)
};
