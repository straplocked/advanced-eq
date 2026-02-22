/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "CurveDisplayRenderer.h"
#include "EqCurveDisplay.h"
#include "../LookAndFeel/ColourPalette.h"

using namespace juce::gl;

// GLSL shaders — no version directive for maximum compatibility (GL 2.0+)
static const char* vertexShaderSource =
    "attribute vec2 position;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4 (position.x * 2.0 - 1.0,\n"
    "                        1.0 - position.y * 2.0,\n"
    "                        0.0, 1.0);\n"
    "}\n";

static const char* fragmentShaderSource =
    "uniform vec4 colour;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = colour;\n"
    "}\n";

// ---- Construction / Destruction ----

CurveDisplayRenderer::CurveDisplayRenderer (EqCurveDisplay& owner)
    : display (owner)
{
}

CurveDisplayRenderer::~CurveDisplayRenderer() = default;

// ---- OpenGLRenderer callbacks ----

void CurveDisplayRenderer::newOpenGLContextCreated()
{
    shadersReady = createShaders();

    if (shadersReady)
        glGenBuffers (1, &vbo);
}

void CurveDisplayRenderer::openGLContextClosing()
{
    colourUniform.reset();
    shader.reset();
    positionAttribute = -1;

    if (vbo != 0)
    {
        glDeleteBuffers (1, &vbo);
        vbo = 0;
    }

    shadersReady = false;
}

bool CurveDisplayRenderer::createShaders()
{
    auto* context = juce::OpenGLContext::getCurrentContext();
    if (context == nullptr)
        return false;

    auto newShader = std::make_unique<juce::OpenGLShaderProgram> (*context);

    if (! newShader->addVertexShader (vertexShaderSource)
        || ! newShader->addFragmentShader (fragmentShaderSource)
        || ! newShader->link())
    {
        return false;
    }

    shader = std::move (newShader);
    colourUniform = std::make_unique<juce::OpenGLShaderProgram::Uniform> (*shader, "colour");
    positionAttribute = glGetAttribLocation (shader->getProgramID(), "position");

    return positionAttribute >= 0;
}

// ---- Thread-safe data updates ----

void CurveDisplayRenderer::updateGridData (const GridData& data)
{
    const juce::ScopedLock sl (dataLock);
    currentGrid = data;
}

void CurveDisplayRenderer::updateCurveData (const CurveData& data)
{
    const juce::ScopedLock sl (dataLock);
    currentCurves = data;
}

void CurveDisplayRenderer::updateSpectrumData (const SpectrumData& data)
{
    const juce::ScopedLock sl (dataLock);
    currentSpectrum = data;
}

// ---- Main render loop ----

void CurveDisplayRenderer::renderOpenGL()
{
    if (! shadersReady)
        return;

    // Compute viewport for EqCurveDisplay within the top-level component
    auto* topLevel = display.getTopLevelComponent();
    if (topLevel == nullptr)
        return;

    auto relBounds = topLevel->getLocalArea (&display, display.getLocalBounds());
    if (relBounds.isEmpty())
        return;

    // Get scale factor for HiDPI
    float scale = 1.0f;
    if (auto* peer = topLevel->getPeer())
        scale = (float) peer->getPlatformScaleFactor();

    int topLevelHeight = topLevel->getHeight();

    // Set viewport (GL Y is bottom-up)
    GLint vpX = (GLint) (relBounds.getX() * scale);
    GLint vpY = (GLint) ((topLevelHeight - relBounds.getBottom()) * scale);
    GLsizei vpW = (GLsizei) (relBounds.getWidth() * scale);
    GLsizei vpH = (GLsizei) (relBounds.getHeight() * scale);

    glViewport (vpX, vpY, vpW, vpH);

    // Restrict drawing to our region
    glEnable (GL_SCISSOR_TEST);
    glScissor (vpX, vpY, vpW, vpH);

    // Blending for alpha
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Anti-aliased lines
    glEnable (GL_LINE_SMOOTH);
    glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

    shader->use();

    // Take a snapshot of the data under the lock
    GridData grid;
    CurveData curves;
    SpectrumData spectrum;
    {
        const juce::ScopedLock sl (dataLock);
        grid = currentGrid;
        curves = currentCurves;
        spectrum = currentSpectrum;
    }

    drawBackground();
    drawGridGL (grid);
    drawSpectrumGL (spectrum);
    drawCurvesGL (curves);
    drawCompositeCurveGL (curves);

    glDisable (GL_LINE_SMOOTH);
    glDisable (GL_SCISSOR_TEST);
    glDisable (GL_BLEND);
}

// ---- Drawing helpers ----

void CurveDisplayRenderer::setColour (juce::Colour c)
{
    if (colourUniform != nullptr)
        colourUniform->set (c.getFloatRed(), c.getFloatGreen(),
                            c.getFloatBlue(), c.getFloatAlpha());
}

void CurveDisplayRenderer::uploadAndDraw (const float* vertices, int numFloats,
                                           GLenum mode, float lineWidth)
{
    if (numFloats <= 0 || vertices == nullptr)
        return;

    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glBufferData (GL_ARRAY_BUFFER, (GLsizeiptr) (numFloats * sizeof (float)),
                  vertices, GL_STREAM_DRAW);

    glEnableVertexAttribArray ((GLuint) positionAttribute);
    glVertexAttribPointer ((GLuint) positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    if (mode == GL_LINES || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP)
        glLineWidth (lineWidth);

    glDrawArrays (mode, 0, numFloats / 2);

    glDisableVertexAttribArray ((GLuint) positionAttribute);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
}

void CurveDisplayRenderer::drawFilledCurve (const float* curveVertices, int numPoints,
                                             float refY, juce::Colour fillColour,
                                             juce::Colour strokeColour, float strokeWidth)
{
    if (numPoints < 2)
        return;

    // Build triangle strip: for each curve point (x, y), emit (x, y) and (x, refY)
    std::vector<float> fillVerts ((size_t) numPoints * 4);
    for (int i = 0; i < numPoints; ++i)
    {
        float x = curveVertices[i * 2];
        float y = curveVertices[i * 2 + 1];
        fillVerts[(size_t) (i * 4 + 0)] = x;
        fillVerts[(size_t) (i * 4 + 1)] = y;
        fillVerts[(size_t) (i * 4 + 2)] = x;
        fillVerts[(size_t) (i * 4 + 3)] = refY;
    }

    // Draw fill
    setColour (fillColour);
    uploadAndDraw (fillVerts.data(), (int) fillVerts.size(), GL_TRIANGLE_STRIP);

    // Draw stroke
    setColour (strokeColour);
    uploadAndDraw (curveVertices, numPoints * 2, GL_LINE_STRIP, strokeWidth);
}

void CurveDisplayRenderer::drawBackground()
{
    auto bg = ColourPalette::curveArea();

    // Full-viewport quad
    float vertices[] = {
        0.0f, 0.0f,   1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f
    };

    setColour (bg);
    uploadAndDraw (vertices, 12, GL_TRIANGLES);
}

void CurveDisplayRenderer::drawGridGL (const GridData& grid)
{
    // Sub-octave vertical lines (faint)
    if (! grid.subFreqLineXPositions.empty())
    {
        std::vector<float> verts;
        verts.reserve (grid.subFreqLineXPositions.size() * 4);
        for (float x : grid.subFreqLineXPositions)
        {
            verts.push_back (x);
            verts.push_back (0.0f);
            verts.push_back (x);
            verts.push_back (1.0f);
        }
        setColour (ColourPalette::gridLine().withMultipliedAlpha (0.4f));
        uploadAndDraw (verts.data(), (int) verts.size(), GL_LINES);
    }

    // Octave vertical lines (normal)
    if (! grid.octaveFreqLineXPositions.empty())
    {
        std::vector<float> verts;
        verts.reserve (grid.octaveFreqLineXPositions.size() * 4);
        for (float x : grid.octaveFreqLineXPositions)
        {
            verts.push_back (x);
            verts.push_back (0.0f);
            verts.push_back (x);
            verts.push_back (1.0f);
        }
        setColour (ColourPalette::gridLineStrong());
        uploadAndDraw (verts.data(), (int) verts.size(), GL_LINES);
    }

    // Decade vertical lines (accent — brighter, wider)
    if (! grid.decadeFreqLineXPositions.empty())
    {
        std::vector<float> verts;
        verts.reserve (grid.decadeFreqLineXPositions.size() * 4);
        for (float x : grid.decadeFreqLineXPositions)
        {
            verts.push_back (x);
            verts.push_back (0.0f);
            verts.push_back (x);
            verts.push_back (1.0f);
        }
        setColour (ColourPalette::compositeCurve().withAlpha (0.12f));
        uploadAndDraw (verts.data(), (int) verts.size(), GL_LINES, 1.5f);
    }

    // Horizontal gain lines (normal)
    if (! grid.gainLineYPositions.empty())
    {
        std::vector<float> verts;
        verts.reserve (grid.gainLineYPositions.size() * 4);
        for (float y : grid.gainLineYPositions)
        {
            verts.push_back (0.0f);
            verts.push_back (y);
            verts.push_back (1.0f);
            verts.push_back (y);
        }
        setColour (ColourPalette::gridLine());
        uploadAndDraw (verts.data(), (int) verts.size(), GL_LINES);
    }

    // 0 dB line (accent)
    {
        float verts[] = {
            0.0f, grid.zeroDbY,
            1.0f, grid.zeroDbY
        };
        setColour (ColourPalette::compositeCurve().withAlpha (0.12f));
        uploadAndDraw (verts, 4, GL_LINES, 1.5f);
    }
}

void CurveDisplayRenderer::drawSpectrumGL (const SpectrumData& spectrum)
{
    auto drawSpectrumFill = [this] (const std::vector<float>& vertices, juce::Colour colour)
    {
        int numPoints = (int) vertices.size() / 2;
        if (numPoints < 2)
            return;

        // Build triangle strip: curve point → bottom edge
        std::vector<float> fillVerts ((size_t) numPoints * 4);
        for (int i = 0; i < numPoints; ++i)
        {
            fillVerts[(size_t) (i * 4 + 0)] = vertices[(size_t) (i * 2)];
            fillVerts[(size_t) (i * 4 + 1)] = vertices[(size_t) (i * 2 + 1)];
            fillVerts[(size_t) (i * 4 + 2)] = vertices[(size_t) (i * 2)];
            fillVerts[(size_t) (i * 4 + 3)] = 1.0f;  // bottom edge
        }

        // Fill
        setColour (colour);
        uploadAndDraw (fillVerts.data(), (int) fillVerts.size(), GL_TRIANGLE_STRIP);

        // Top edge stroke
        setColour (colour.withMultipliedAlpha (1.5f));
        uploadAndDraw (vertices.data(), (int) vertices.size(), GL_LINE_STRIP);
    };

    if (spectrum.hasPreData && ! spectrum.preVertices.empty())
        drawSpectrumFill (spectrum.preVertices, spectrum.preColour);

    if (spectrum.hasPostData && ! spectrum.postVertices.empty())
        drawSpectrumFill (spectrum.postVertices, spectrum.postColour);
}

void CurveDisplayRenderer::drawCurvesGL (const CurveData& curves)
{
    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        if (! curves.bandActive[(size_t) b])
            continue;

        auto colour = curves.bandColours[(size_t) b];
        bool isSelected = (b == curves.selectedBand);

        float fillAlpha = isSelected ? 0.5f : 0.2f;
        float strokeAlpha = isSelected ? 1.0f : 0.6f;
        float strokeWidth = isSelected ? 2.0f : 1.0f;

        drawFilledCurve (curves.bandVertices[(size_t) b].data(),
                         Constants::kCurveResolution,
                         curves.zeroDbY,
                         colour.withAlpha (fillAlpha),
                         colour.withAlpha (strokeAlpha),
                         strokeWidth);
    }
}

void CurveDisplayRenderer::drawCompositeCurveGL (const CurveData& curves)
{
    // Per-channel composites take priority over the single composite
    if (curves.hasPerChannelLR || curves.hasPerChannelMS)
    {
        auto drawChannelCurve = [this, &curves] (const float* vertices, juce::Colour colour)
        {
            drawFilledCurve (vertices, Constants::kCurveResolution,
                             curves.zeroDbY,
                             colour.withAlpha (0.08f),
                             colour,
                             2.0f);
        };

        if (curves.hasPerChannelLR)
        {
            drawChannelCurve (curves.leftVertices.data(), ColourPalette::leftColour());
            drawChannelCurve (curves.rightVertices.data(), ColourPalette::rightColour());
        }

        if (curves.hasPerChannelMS)
        {
            drawChannelCurve (curves.midVertices.data(), ColourPalette::midColour());
            drawChannelCurve (curves.sideVertices.data(), ColourPalette::sideColour());
        }
        return;
    }

    // Single composite curve
    drawFilledCurve (curves.compositeVertices.data(),
                     Constants::kCurveResolution,
                     curves.zeroDbY,
                     ColourPalette::compositeFill(),
                     ColourPalette::compositeCurve(),
                     2.0f);
}
