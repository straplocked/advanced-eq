/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../PluginProcessor.h"
#include "../../parameters/ParameterIDs.h"
#include "../../dsp/FilterTypes.h"
#include "../LookAndFeel/ColourPalette.h"

class BottomControlStrip : public juce::Component,
                           public juce::Timer
{
public:
    explicit BottomControlStrip (AdvancedEQProcessor& processor);
    ~BottomControlStrip() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setSelectedBand (int bandIndex);
    int getSelectedBand() const { return selectedBand; }

    // Global tool callbacks (moved from HeaderToolbar)
    std::function<void (bool)> onFreezeToggled;
    std::function<void()> onMatchClicked;
    std::function<void (bool)> onHelpToggled;
    std::function<void (bool)> onSketchToggled;

private:
    AdvancedEQProcessor& processor;
    int selectedBand = -1;

    // Band indicator
    juce::Label bandLabel;

    // Selectors
    juce::ComboBox typeSelector;
    juce::ComboBox slopeSelector;
    juce::ComboBox stereoSelector;

    // Rotary knobs
    juce::Slider freqSlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;

    // Value labels under knobs
    juce::Label freqLabel;
    juce::Label gainLabel;
    juce::Label qLabel;

    // Dynamic EQ controls
    juce::TextButton dynEnableButton { "Dyn" };
    juce::Slider dynThreshSlider;
    juce::Slider dynRangeSlider;
    juce::Slider dynAttackSlider;
    juce::Slider dynReleaseSlider;
    juce::Label dynThreshLabel;
    juce::Label dynRangeLabel;
    juce::Label dynAttackLabel;
    juce::Label dynReleaseLabel;

    // Title labels (shown above knobs/selectors when a band is selected)
    juce::Label typeTitleLabel, slopeTitleLabel, stereoTitleLabel;
    juce::Label freqTitleLabel, gainTitleLabel;
    juce::Label dynThreshTitleLabel, dynRangeTitleLabel, dynAttackTitleLabel, dynReleaseTitleLabel;

    // --- Global tools (right zone) ---
    juce::TextButton phaseInvertButton { juce::String::charToString (0x00D8) };
    juce::ComboBox characterSelector;
    juce::TextButton analyzerButton { "Analyzer" };
    juce::TextButton freezeButton { juce::String::charToString (0x2744) };  // ❄
    juce::TextButton matchButton { "Match" };
    juce::TextButton sketchButton { juce::String::charToString (0x270E) }; // ✎
    juce::TextButton midiPCButton { "MIDI PC" };
    juce::TextButton helpButton { "?" };
    juce::ComboBox processModeSelector;
    juce::ComboBox lpQualitySelector;

    // Global tool attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseInvertAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analyzerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lpQualityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midiPCAttachment;

    // Band-specific attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;

    // Dynamic EQ attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dynThreshAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dynRangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dynAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dynReleaseAttachment;

    void updateAttachments();
    void updateSlopeEnabled();
    void updateValueLabels();
    void setControlsVisible (bool visible);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottomControlStrip)
};
