/*
 ==============================================================================
 
 PluginContainer.h
 Created: 9 Feb 2018 11:32:26am
 Author:  Agneya Kerure
 
 ==============================================================================
 */

#pragma once
#include <iostream>
#include <fstream>

#include "../JuceLibraryCode/JuceHeader.h"

class PluginContainer
{
public:
    PluginContainer();
    ~PluginContainer();
    
    //Setters
    void setPluginInstance(AudioProcessor& pluginInstance);
    void setProcessor(AudioProcessor& processor);
    void setParameter(int parameterIndex, float newValue);
    
    //Getters
    int getNumberOfParameters();
    String getParameterName(int parameterIndex);
    
    //run through parameters
    //print them in a file
    void generateParameterTextFiles(int numParams, double step, const String &prefix);
    
    //connect midi input to midi source
    //connect audio output to destination
    void setConnections();
    
private:
    AudioProcessor* pluginInstance;
    AudioProcessor* newProcessor;
    std::ofstream file;
    int fileNum;
    Time t;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginContainer)
};

