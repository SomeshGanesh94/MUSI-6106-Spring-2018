/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "FilterGraph.h"


//==============================================================================
/**
    A panel that displays and edits a FilterGraph.
*/
class GraphEditorPanel   : public Component,
                           public ChangeListener
{
public:
    GraphEditorPanel (FilterGraph& graph);
    ~GraphEditorPanel();

    void createNewPlugin (const PluginDescription&, Point<int> position);

    void paint (Graphics&) override;
    void mouseDown (const MouseEvent&) override;
    void resized() override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void updateComponents();

    //==============================================================================
    void beginConnectorDrag (PluginContainerProcessor::NodeAndChannel source,
                             PluginContainerProcessor::NodeAndChannel dest,
                             const MouseEvent&);
    void dragConnector (const MouseEvent&);
    void endDraggingConnector (const MouseEvent&);

    //==============================================================================
    FilterGraph& graph;

private:
    struct FilterComponent;
    struct ConnectorComponent;
    struct PinComponent;

    OwnedArray<FilterComponent> nodes;
    OwnedArray<ConnectorComponent> connectors;
    ScopedPointer<ConnectorComponent> draggingConnector;

    FilterComponent* getComponentForFilter (PluginContainerProcessor::NodeID) const;
    ConnectorComponent* getComponentForConnection (const PluginContainerProcessor::Connection&) const;
    PinComponent* findPinAt (Point<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphEditorPanel)
};


//==============================================================================
/**
    A panel that embeds a GraphEditorPanel with a midi keyboard at the bottom.

    It also manages the graph itself, and plays it.
*/
class GraphDocumentComponent  : public Component, public Button::Listener
{
public:
    GraphDocumentComponent (AudioPluginFormatManager& formatManager,
                            AudioDeviceManager& deviceManager);
    ~GraphDocumentComponent();

    //==============================================================================
    void createNewPlugin (const PluginDescription&, Point<int> position);
    void setDoublePrecision (bool doublePrecision);
    bool closeAnyOpenPluginWindows();

    //==============================================================================
    ScopedPointer<FilterGraph> graph;

    void resized() override;
    void unfocusKeyboardComponent();
    void releaseGraph();

    ScopedPointer<GraphEditorPanel> graphPanel;
    ScopedPointer<MidiKeyboardComponent> keyboardComp;
    static MidiKeyboardState keyState;
    
    TextButton m_bAudioOn;
    TextButton m_bAudioOff;
    
    void buttonClicked(Button* button) override;
    
private:
    //==============================================================================
    AudioDeviceManager& deviceManager;
    AudioProcessorPlayer graphPlayer;
    
    struct TooltipBar;
    ScopedPointer<TooltipBar> statusBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphDocumentComponent)
};
