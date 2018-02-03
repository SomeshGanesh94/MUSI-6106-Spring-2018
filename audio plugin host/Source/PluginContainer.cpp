/*
  ==============================================================================

    PluginContainer.cpp
    Created: 3 Feb 2018 11:32:26am
    Author:  Agneya Kerure

  ==============================================================================
*/

#include "PluginContainer.h"

PluginContainer::PluginContainer()
{
}

PluginContainer::~PluginContainer()
{
    delete pluginInstance;
    delete newProcessor;
}

void PluginContainer::setPluginInstance(AudioProcessor& plugin)
{
    pluginInstance = &plugin;
}

void PluginContainer::setProcessor(AudioProcessor& processor)
{
    newProcessor = &processor;
}

void PluginContainer::setParameter(int index, float value)
{
    pluginInstance -> setParameter(index, value);
}

String PluginContainer::getParameterName(int index)
{
    return (pluginInstance -> getParameterName(index));
}
