/*
  ==============================================================================

    PluginContainer.h
    Created: 3 Feb 2018 11:32:26am
    Author:  Agneya Kerure

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class PluginContainer
{
public:
    PluginContainer();
    ~PluginContainer();
    void setPluginInstance (AudioProcessor& pluginInstance);
    void setProcessor (AudioProcessor& processor);
    void setParameter (int parameterIndex, float newValue);
    String getParameterName (int parameterIndex);
private:
    AudioProcessor* pluginInstance;
    AudioProcessor* newProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginContainer)
};
