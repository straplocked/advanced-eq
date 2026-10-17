/*
 * Advanced EQ - Open-source parametric equalizer plugin
 * Copyright (C) 2024-2026 Advanced EQ contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "PluginEditor.h"
#include "utils/Constants.h"
#include "utils/MathUtils.h"
#include "parameters/ParameterIDs.h"
#include "ui/LookAndFeel/ColourPalette.h"

#include <cstdlib>

namespace
{
    // How long to wait for the GL renderer to become usable before giving up
    // and painting in software instead.
    constexpr int kGLProbeIntervalMs = 250;
    constexpr int kGLProbeMaxAttempts = 8;   // ~2 seconds

    /** Returns true if the user has explicitly asked us not to use OpenGL.
     *
     *  Set ADVANCED_EQ_DISABLE_OPENGL to any value other than "0" to force the
     *  software rendering path. This exists for machines whose GL drivers are
     *  broken or absent, and for headless/automated captures where no usable
     *  GL surface is available.
     */
    bool openGLDisabledByEnvironment()
    {
        if (const char* raw = std::getenv ("ADVANCED_EQ_DISABLE_OPENGL"))
            return juce::String (raw).trim() != "0";

        return false;
    }
}

AdvancedEQEditor::AdvancedEQEditor (AdvancedEQProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      headerToolbar (p),
      curveDisplay (p),
      controlStrip (p),
      levelMeter (p),
      matchPanel (p),
      presetBrowser (p.getPresetManager()),
      instanceList (p.getInstanceRegistry()),
      midiMappingsPanel (p.getMidiLearnManager())
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (headerToolbar);
    addAndMakeVisible (curveDisplay);
    addAndMakeVisible (controlStrip);
    addAndMakeVisible (levelMeter);

    // Wire band selection callback to bottom control strip
    curveDisplay.onBandSelected = [this] (int band)
    {
        controlStrip.setSelectedBand (band);
    };

    // Wire freeze toggle (bottom control strip)
    controlStrip.onFreezeToggled = [this] (bool frozen)
    {
        curveDisplay.setAnalyzerFrozen (frozen);
    };

    // Wire match button (bottom control strip)
    controlStrip.onMatchClicked = [this]
    {
        matchPanel.setVisible (! matchPanel.isVisible());
    };

    // Sketch mode toggle (bottom control strip)
    controlStrip.onSketchToggled = [this] (bool active)
    {
        curveDisplay.setSketchMode (active);
    };

    // Preset name click - toggle browser panel
    headerToolbar.onPresetNameClicked = [this]
    {
        presetBrowser.setVisible (! presetBrowser.isVisible());
    };

    // Fullscreen toggle
    headerToolbar.onFullscreenClicked = [this]
    {
        toggleFullScreen();
    };

    // Match panel (initially hidden, overlaid on curve display)
    addChildComponent (matchPanel);

    // Preset browser panel (initially hidden)
    addChildComponent (presetBrowser);

    // Instance list panel (initially hidden)
    addChildComponent (instanceList);

    // MIDI mappings panel (initially hidden)
    addChildComponent (midiMappingsPanel);

    // Wire MIDI Map button in control strip
    controlStrip.onMidiMapClicked = [this]
    {
        midiMappingsPanel.setVisible (! midiMappingsPanel.isVisible());
    };

    // Help overlay (topmost child, transparent to mouse clicks — shows on hover)
    addAndMakeVisible (helpOverlay);

    // Register keyboard listener for undo/redo
    addKeyListener (this);

    setSize (Constants::kDefaultEditorWidth, Constants::kDefaultEditorHeight);
    setResizable (true, true);
    setResizeLimits (825, 495, 2112, 1188);

    // Enable GPU-accelerated rendering for all child components, unless the
    // user has opted out. ADVANCED_EQ_DISABLE_OPENGL=1 forces the software
    // path, which is useful on machines with broken GL drivers and for
    // headless captures.
    if (openGLDisabledByEnvironment())
    {
        curveDisplay.setUseOpenGL (false);
    }
    else
    {
        openGLContext.setRenderer (curveDisplay.getGLRenderer());
        openGLContext.setComponentPaintingEnabled (true);
        openGLContext.setContinuousRepainting (false);
        openGLContext.setMultisamplingEnabled (true);
        openGLContext.attachTo (*this);
        openGLActive = true;

        // Attaching does not guarantee the renderer will come up, so check back
        // shortly and fall back to software if it has not.
        startTimer (kGLProbeIntervalMs);
    }
}

void AdvancedEQEditor::timerCallback()
{
    auto* renderer = curveDisplay.getGLRenderer();

    if (renderer != nullptr && renderer->isReady())
    {
        stopTimer();   // GL is up; nothing to do
        return;
    }

    if (++glProbeAttempts >= kGLProbeMaxAttempts)
    {
        stopTimer();
        fallBackToSoftwareRendering();
    }
}

void AdvancedEQEditor::fallBackToSoftwareRendering()
{
    if (! openGLActive)
        return;

    openGLContext.detach();
    openGLActive = false;
    curveDisplay.setUseOpenGL (false);
    curveDisplay.repaint();
}

AdvancedEQEditor::~AdvancedEQEditor()
{
    stopTimer();

    if (openGLActive)
        openGLContext.detach();
    removeKeyListener (this);
    setLookAndFeel (nullptr);
}

void AdvancedEQEditor::paint (juce::Graphics& g)
{
    g.fillAll (ColourPalette::background());
}

void AdvancedEQEditor::resized()
{
    auto bounds = getLocalBounds();

    headerToolbar.setBounds (bounds.removeFromTop (36));
    controlStrip.setBounds (bounds.removeFromBottom (120));

    // Level meter on the right edge
    levelMeter.setBounds (bounds.removeFromRight (20));

    curveDisplay.setBounds (bounds);

    // Match panel overlay (centered on curve display)
    auto curveBounds = curveDisplay.getBounds();
    int panelW = 300;
    int panelH = 140;
    matchPanel.setBounds (curveBounds.getCentreX() - panelW / 2,
                          curveBounds.getCentreY() - panelH / 2,
                          panelW, panelH);

    // Preset browser panel (right overlay on curve display)
    auto curveBoundsForBrowser = curveDisplay.getBounds();
    presetBrowser.setBounds (curveBoundsForBrowser.getRight() - 350,
                              curveBoundsForBrowser.getY(),
                              350, curveBoundsForBrowser.getHeight());

    // Instance list panel (left overlay on curve display)
    instanceList.setBounds (curveBoundsForBrowser.getX(),
                             curveBoundsForBrowser.getY(),
                             350, curveBoundsForBrowser.getHeight());

    // MIDI mappings panel (left overlay on curve display)
    midiMappingsPanel.setBounds (curveBoundsForBrowser.getX(),
                                  curveBoundsForBrowser.getY(),
                                  300, curveBoundsForBrowser.getHeight());

    // Help overlay covers entire editor, always on top
    helpOverlay.setBounds (getLocalBounds());
    helpOverlay.toFront (false);
}

bool AdvancedEQEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    auto commandKey = juce::ModifierKeys::commandModifier;

    // Cmd+Z = Undo
    if (key == juce::KeyPress ('z', commandKey, 0))
    {
        processorRef.getUndoManager().undo();
        return true;
    }

    // Cmd+Shift+Z = Redo
    if (key == juce::KeyPress ('z', commandKey | juce::ModifierKeys::shiftModifier, 0))
    {
        processorRef.getUndoManager().redo();
        return true;
    }

    // Cmd+C = Copy selected band
    if (key == juce::KeyPress ('c', commandKey, 0))
    {
        int band = curveDisplay.getSelectedBand();
        if (band >= 0)
            curveDisplay.copyBandToClipboard (band);
        return true;
    }

    // Cmd+V = Paste to selected band (or first inactive)
    if (key == juce::KeyPress ('v', commandKey, 0))
    {
        int band = curveDisplay.getSelectedBand();
        if (band < 0)
        {
            // Find first inactive band and activate it
            auto& apvts = processorRef.getAPVTS();
            for (int b = 0; b < Constants::kMaxBands; ++b)
            {
                auto* active = apvts.getRawParameterValue (ParamIDs::bandParam (b, ParamIDs::kActive));
                if (active->load() < 0.5f)
                {
                    if (auto* p = apvts.getParameter (ParamIDs::bandParam (b, ParamIDs::kActive)))
                        p->setValueNotifyingHost (1.0f);
                    band = b;
                    break;
                }
            }
        }
        if (band >= 0)
            curveDisplay.pasteBandFromClipboard (band);
        return true;
    }

    // F11 = Toggle fullscreen
    if (key == juce::KeyPress (juce::KeyPress::F11Key))
    {
        toggleFullScreen();
        return true;
    }

    // Escape = Exit fullscreen
    if (key == juce::KeyPress (juce::KeyPress::escapeKey) && isFullScreen)
    {
        toggleFullScreen();
        return true;
    }

    return false;
}

void AdvancedEQEditor::toggleFullScreen()
{
    if (isFullScreen)
    {
        setBounds (previousBounds);
        isFullScreen = false;
    }
    else
    {
        previousBounds = getBounds();
        auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds());
        if (display != nullptr)
            setBounds (display->userArea);
        isFullScreen = true;
    }
}

void AdvancedEQEditor::setWindowSizePreset (WindowSizePreset preset)
{
    switch (preset)
    {
        case Mini:   setSize (825, 495); break;
        case Small:  setSize (935, 561); break;
        case Medium: setSize (1100, 660); break;
        case Large:  setSize (1320, 792); break;
        case XL:     setSize (1650, 990); break;
    }
}
