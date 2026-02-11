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

AdvancedEQEditor::AdvancedEQEditor (AdvancedEQProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      headerToolbar (p),
      curveDisplay (p),
      controlStrip (p),
      levelMeter (p),
      matchPanel (p)
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

    // Wire freeze toggle
    headerToolbar.onFreezeToggled = [this] (bool frozen)
    {
        curveDisplay.setAnalyzerFrozen (frozen);
    };

    // Wire match button
    headerToolbar.onMatchClicked = [this]
    {
        matchPanel.setVisible (! matchPanel.isVisible());
    };

    // Match panel (initially hidden, overlaid on curve display)
    addChildComponent (matchPanel);

    // Help overlay (topmost child, transparent to mouse clicks)
    addAndMakeVisible (helpOverlay);
    headerToolbar.onHelpToggled = [this] (bool active)
    {
        helpOverlay.setHelpModeActive (active);
    };

    // Register keyboard listener for undo/redo
    addKeyListener (this);

    setSize (Constants::kDefaultEditorWidth, Constants::kDefaultEditorHeight);
    setResizable (true, true);
    setResizeLimits (750, 450, 1920, 1080);
}

AdvancedEQEditor::~AdvancedEQEditor()
{
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
    controlStrip.setBounds (bounds.removeFromBottom (90));

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

    return false;
}

void AdvancedEQEditor::setWindowSizePreset (WindowSizePreset preset)
{
    switch (preset)
    {
        case Mini:   setSize (750, 450); break;
        case Small:  setSize (850, 510); break;
        case Medium: setSize (1000, 600); break;
        case Large:  setSize (1200, 720); break;
        case XL:     setSize (1500, 900); break;
    }
}
