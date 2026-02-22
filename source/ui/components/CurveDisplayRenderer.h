/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_opengl/juce_opengl.h>
#include <vector>
#include <array>
#include "../../utils/Constants.h"

class EqCurveDisplay;

/**
 * @class CurveDisplayRenderer
 * @brief OpenGL renderer for the EQ curve display's heavy draw calls.
 *
 * Renders the background grid, spectrum analyzer fills/strokes, individual band
 * curves, and the composite curve using GL vertex arrays and simple shaders.
 * Text labels, tooltips, and band handles remain on JUCE's software overlay.
 *
 * Data is transferred from the message thread via thread-safe update methods.
 * The renderer consumes the latest snapshot each frame under a CriticalSection.
 */
class CurveDisplayRenderer : public juce::OpenGLRenderer
{
public:
    explicit CurveDisplayRenderer (EqCurveDisplay& owner);
    ~CurveDisplayRenderer() override;

    // juce::OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    /** Returns true if GL shaders compiled and the renderer is usable. */
    bool isReady() const { return shadersReady; }

    // ---- Data structures for thread-safe transfer ----

    struct GridData
    {
        std::vector<float> subFreqLineXPositions;     // normalized 0..1
        std::vector<float> octaveFreqLineXPositions;  // normalized 0..1
        std::vector<float> decadeFreqLineXPositions;  // normalized 0..1 (accent)
        std::vector<float> gainLineYPositions;        // normalized 0..1
        float zeroDbY = 0.5f;                         // normalized 0..1
    };

    struct CurveData
    {
        // Each curve: kCurveResolution (x,y) pairs in normalized coords
        std::array<float, Constants::kCurveResolution * 2> compositeVertices {};
        std::array<std::array<float, Constants::kCurveResolution * 2>, Constants::kMaxBands> bandVertices {};
        std::array<bool, Constants::kMaxBands> bandActive {};
        std::array<juce::Colour, Constants::kMaxBands> bandColours {};
        int selectedBand = -1;
        float zeroDbY = 0.5f;

        // Per-channel composite curves
        bool hasPerChannelLR = false;
        bool hasPerChannelMS = false;
        std::array<float, Constants::kCurveResolution * 2> leftVertices {};
        std::array<float, Constants::kCurveResolution * 2> rightVertices {};
        std::array<float, Constants::kCurveResolution * 2> midVertices {};
        std::array<float, Constants::kCurveResolution * 2> sideVertices {};
    };

    struct SpectrumData
    {
        std::vector<float> preVertices;   // (x,y) pairs in normalized coords
        std::vector<float> postVertices;
        bool hasPreData = false;
        bool hasPostData = false;
        juce::Colour preColour;
        juce::Colour postColour;
    };

    // Thread-safe data update (called from message thread)
    void updateGridData (const GridData& data);
    void updateCurveData (const CurveData& data);
    void updateSpectrumData (const SpectrumData& data);

private:
    EqCurveDisplay& display;

    // Shader program
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> colourUniform;
    GLint positionAttribute = -1;

    // Single VBO for all drawing (re-uploaded each draw call)
    GLuint vbo = 0;

    bool shadersReady = false;

    // Cached data (protected by lock)
    juce::CriticalSection dataLock;
    GridData currentGrid;
    CurveData currentCurves;
    SpectrumData currentSpectrum;

    // GL drawing helpers (accept snapshot data to avoid re-locking)
    void drawBackground();
    void drawGridGL (const GridData& grid);
    void drawSpectrumGL (const SpectrumData& spectrum);
    void drawCurvesGL (const CurveData& curves);
    void drawCompositeCurveGL (const CurveData& curves);

    bool createShaders();

    // Helper to set uniform colour from juce::Colour
    void setColour (juce::Colour c);

    // Helper to upload vertices to VBO and draw
    void uploadAndDraw (const float* vertices, int numFloats, GLenum mode,
                        float lineWidth = 1.0f);

    // Draws a filled curve between the polyline and a reference Y line, plus stroke
    void drawFilledCurve (const float* curveVertices, int numPoints,
                          float refY, juce::Colour fillColour,
                          juce::Colour strokeColour, float strokeWidth);
};
