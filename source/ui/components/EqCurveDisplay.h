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
#pragma once

/**
 * @file EqCurveDisplay.h
 * @brief Interactive parametric EQ curve display with spectrum analyzer overlay.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../PluginProcessor.h"
#include "../../utils/Constants.h"
#include "../../utils/MathUtils.h"
#include "../../dsp/FilterTypes.h"
#include "../../dsp/SVFFilter.h"
#include "../../dsp/CascadedFilter.h"
#include "../../analyzer/SpectrumAnalyzer.h"
#include "../LookAndFeel/ColourPalette.h"
#include "CurveDisplayRenderer.h"

/**
 * @class EqCurveDisplay
 * @brief Main visual component for the parametric EQ curve editor.
 *
 * Renders per-band EQ curves, a composite magnitude response, and a real-time
 * spectrum analyzer overlay (pre- and post-EQ). Handles all mouse interaction
 * for creating, selecting, and editing EQ bands.
 *
 * **Interaction model:**
 * - Click empty space: create a new band at that frequency/gain
 * - Click handle: select an existing band
 * - Drag handle: adjust frequency (X axis) and gain (Y axis)
 * - Scroll on band: adjust Q factor
 * - Double-click handle: deactivate band
 * - Right-click: context menu (bypass, delete, set type/slope/stereo, reset)
 *
 * @see ProcessingChain, BottomControlStrip
 */
class EqCurveDisplay : public juce::Component,
                       public juce::Timer
{
public:
    /** @brief Constructs the curve display, referencing the processor for parameter and DSP access.
     *  @param processor The audio processor that owns the APVTS and processing chain.
     */
    explicit EqCurveDisplay (AdvancedEQProcessor& processor);
    ~EqCurveDisplay() override = default;

    /** @brief Paints the grid, band curves, composite response, spectrum, handles, and tooltip. */
    void paint (juce::Graphics& g) override;

    /** @brief Recalculates coordinate mapping and pre-computes frequency points on resize. */
    void resized() override;

    /** @brief Timer-driven UI refresh: updates curves and spectrum data at the configured rate. */
    void timerCallback() override;

    // Mouse interaction
    /** @brief Handles band selection, creation, and right-click context menus.
     *  @param e The mouse event with position and modifier information.
     */
    void mouseDown (const juce::MouseEvent& e) override;

    /** @brief Drags the selected band handle to adjust frequency and gain.
     *  @param e The mouse event with current drag position.
     */
    void mouseDrag (const juce::MouseEvent& e) override;

    /** @brief Ends a band drag operation.
     *  @param e The mouse event at release position.
     */
    void mouseUp (const juce::MouseEvent& e) override;

    /** @brief Updates hover state and preview curve as the mouse moves.
     *  @param e The mouse event with current cursor position.
     */
    void mouseMove (const juce::MouseEvent& e) override;

    /** @brief Deactivates the band under the cursor on double-click.
     *  @param e The mouse event at the double-click position.
     */
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    /** @brief Adjusts Q factor of the hovered or selected band via mouse wheel.
     *  @param e     The mouse event with cursor position.
     *  @param wheel The scroll delta information.
     */
    void mouseWheelMove (const juce::MouseEvent& e,
                         const juce::MouseWheelDetails& wheel) override;

    /** @brief Clears hover state when the mouse leaves the component.
     *  @param e The mouse event.
     */
    void mouseExit (const juce::MouseEvent& e) override;

    /** @brief Returns the index of the currently selected band, or -1 if none.
     *  @return The selected band index.
     */
    int getSelectedBand() const { return selectedBand; }

    /** @brief Returns the GL renderer for attachment to an OpenGLContext.
     *  @return Pointer to the CurveDisplayRenderer, or nullptr if not available.
     */
    CurveDisplayRenderer* getGLRenderer() { return glRenderer.get(); }

    /** @brief Freezes or unfreezes the spectrum analyzer display.
     *  @param frozen True to freeze the analyzer, false to resume real-time updates.
     */
    void setAnalyzerFrozen (bool frozen) { analyzerFrozen = frozen; }

    /** @brief Returns whether the spectrum analyzer is currently frozen.
     *  @return True if frozen, false otherwise.
     */
    bool isAnalyzerFrozen() const { return analyzerFrozen; }

    /** @brief Callback invoked when the selected band changes.
     *
     * The parameter is the new band index, or -1 if selection is cleared.
     */
    std::function<void (int)> onBandSelected;

    // Public accessors for editor keyboard shortcuts / toolbar
    void copyBandToClipboard (int band);
    void pasteBandFromClipboard (int band);
    void setSketchMode (bool active) { sketchMode = active; }

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

    // UI-side filters for lag-free magnitude computation (bypasses DSP-layer smoothing)
    std::array<CascadedFilter, Constants::kMaxBands> uiFilters;

    // Per-channel composite curves
    bool showPerChannelLR = false;
    bool showPerChannelMS = false;
    std::array<double, Constants::kCurveResolution> leftCompositeMagnitudes {};
    std::array<double, Constants::kCurveResolution> rightCompositeMagnitudes {};
    std::array<double, Constants::kCurveResolution> midCompositeMagnitudes {};
    std::array<double, Constants::kCurveResolution> sideCompositeMagnitudes {};

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
    void drawPerChannelCompositeCurves (juce::Graphics& g);
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

    // EQ Sketch (sketchMode set via public setSketchMode)
    bool sketchMode = false;
    bool isDrawingSketch = false;
    std::vector<juce::Point<float>> sketchPoints;
    void processSketch();
    void drawSketchPath (juce::Graphics& g);

    // Spectrum analyzers (pre and post EQ)
    SpectrumAnalyzer spectrumAnalyzerPre;
    SpectrumAnalyzer spectrumAnalyzerPost;
    bool analyzerFrozen = false;

    // OpenGL rendering
    std::unique_ptr<CurveDisplayRenderer> glRenderer;
    bool useOpenGL = true;

    void prepareGLData();
    void paintSoftware (juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqCurveDisplay)
};
