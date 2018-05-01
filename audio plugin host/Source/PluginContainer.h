/*
 ==============================================================================
 
 PluginContainer.h
 Created: 9 Feb 2018 11:32:26am
 Author:  Agneya Kerure
 
 This class acts as a container for the instantiated plugin providing interfaceto other classes.
 
 
 ==============================================================================
 */

#pragma once
#include <iostream>
#include <fstream>

#include "../JuceLibraryCode/JuceHeader.h"

class PluginContainer: public Component
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
    void generateParameterTextFiles(int depth, std::vector<float> & numbers, std::vector<float> & maxes);
    
    //connect midi input to midi source
    //connect audio output to destination
    void setConnections();
    
    //run through parameters
    //print them in a file
    void generateAudioFiles(int iNumParams, double dStepSize, double* pdParamValArray);
    void setNumberOfParameters(int numParams);
    void genFiles(int numParams, int maxLimit);
    
    void init(AudioProcessor& plugin);
    void reset();
    int m_kiNumpParams;
    
private:
    const int m_kNumSteps = 5;
    double* m_pdParamValArray;
    AudioProcessor* pluginInstance;
    AudioProcessor* newProcessor;
    std::ofstream file;
    int fileNum;
    Time t;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginContainer)
};
