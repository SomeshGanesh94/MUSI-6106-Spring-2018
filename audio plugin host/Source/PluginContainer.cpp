/*
 ==============================================================================
 
 PluginContainer.cpp
 Created: 9 Feb 2018 11:32:26am
 Author:  Agneya Kerure
 
 ==============================================================================
 */
#include <iostream>
#include <fstream>

#include "PluginContainer.h"

PluginContainer::PluginContainer()
{
    
}

PluginContainer::~PluginContainer()
{
    delete pluginInstance;
    delete newProcessor;
}
//Setters
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

//Getters
String PluginContainer::getParameterName(int index)
{
    return (pluginInstance -> getParameterName(index));
}

int PluginContainer::getNumberOfParameters()
{
    return (pluginInstance->getNumParameters());
}

void PluginContainer::generateParameterTextFiles(int iNumParams, double dStepSize, const String &prefix)
{
    std::string sTestFilePath = "/Users/someshganesh/Documents/GitHub/SynthIO/audio plugin host/Source/test/parameterFile";
    
    if(iNumParams != 0)
    {
        if(!file.is_open())
        {
            fileNum = 1;
            file.open(sTestFilePath + std::to_string(fileNum) + ".txt");

        }
        for( double dParamValue = 0; dParamValue <= 1; dParamValue = dParamValue + dStepSize)
        {
            file << prefix << dParamValue << std::endl;
            std::ifstream ifs(sTestFilePath + std::to_string(fileNum) + ".txt");
            std::string content( (std::istreambuf_iterator<char>(ifs) ),
                                (std::istreambuf_iterator<char>()    ) );
            generateParameterTextFiles(iNumParams - 1, dStepSize, content);
        }
    }
    else
    {
//        std::cout << prefix << std::endl;
        
        //open txt file
        //set parameter of synth
        //connect synth input to midi input
        //connect synth output to midi output
        //simulate keypress
        //wait for 1000ms
        //write buffer to text file
        //keyRelease
        
        file.close();
        fileNum++;
        file.open(sTestFilePath + std::to_string(fileNum) + ".txt");
    }
}

void setConnections()
{
    
}
