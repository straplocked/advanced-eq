#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../PluginProcessor.h"
#include "../../utils/Constants.h"
#include "../../utils/MathUtils.h"
#include "../../dsp/FilterTypes.h"
#include "../../dsp/SVFFilter.h"
#include "../../analyzer/SpectrumAnalyzer.h"
#include "../LookAndFeel/ColourPalette.h"

class EqCurveDisplay : public juce::Component,
                       public juce::Timer
{
public:
    explicit EqCurveDisplay (AdvancedEQProcessor& processor);
    ~EqCurveDisplay() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Mouse interaction
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e,
                         const juce::MouseWheelDetails& wheel) override;
    void mouseExit (const juce::MouseEvent& e) override;

    int getSelectedBand() const { return selectedBand; }
    void setAnalyzerFrozen (bool frozen) { analyzerFrozen = frozen; }
    bool isAnalyzerFrozen() const { return analyzerFrozen; }

    // Callback when band selection changes
    std::function<void (int)> onBandSelected;

private:
    AdvancedEQProcessor& processor;

    // Display range
    enum DisplayRange { dB3 = 0, dB6, dB12, dB30 };
    DisplayRange currentRange = dB30;
    float displayMinDB = -30.0f;
    float displayMaxDB = 30.0f;
    void setDisplayRange (DisplayRange range);
    float getGridStep() const;
    void drawRangeButtons (juce::Graphics& g);
    juce::Rectangle<float> getRangeButtonRect (int index) const;

    // Interaction state
    int selectedBand = -1;
    int hoveredBand = -1;
    bool isDragging = false;

    // Hover preview
    bool showPreview = false;
    float previewMouseX = 0.0f;
    float previewMouseY = 0.0f;
    SVFFilter previewFilter;
    void drawPreviewCurve (juce::Graphics& g);

    // Pre-computed frequency points for curve rendering
    std::array<double, Constants::kCurveResolution> frequencyPoints {};
    std::array<double, Constants::kCurveResolution> compositeMagnitudes {};
    std::array<std::array<double, Constants::kCurveResolution>, Constants::kMaxBands> bandMagnitudes {};

    // Coordinate mapping
    float frequencyToX (float freq) const;
    float xToFrequency (float x) const;
    float gainToY (float gainDB) const;
    float yToGain (float y) const;

    // Drawing helpers
    void drawGrid (juce::Graphics& g);
    void drawFrequencyLabels (juce::Graphics& g);
    void drawGainLabels (juce::Graphics& g);
    void drawBandCurves (juce::Graphics& g);
    void drawCompositeCurve (juce::Graphics& g);
    void drawBandHandles (juce::Graphics& g);
    void drawSpectrum (juce::Graphics& g, const SpectrumAnalyzer& analyzer, juce::Colour colour);
    void drawTooltip (juce::Graphics& g);

    // Interaction helpers
    int findBandAtPosition (juce::Point<float> pos) const;
    int findFirstInactiveBand() const;
    void activateBandAt (float freq, float gain);
    void showBandContextMenu (int band);
    void showEmptySpaceContextMenu (float freq, float gain);

    void updateCurves();

    // Spectrum analyzers (pre and post EQ)
    SpectrumAnalyzer spectrumAnalyzerPre;
    SpectrumAnalyzer spectrumAnalyzerPost;
    bool analyzerFrozen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqCurveDisplay)
};
