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

void PluginContainer::generateParameterTextFiles(int numParams, double stepSize, const String &prefix)
{
    if(numParams != 0)
    {
        if(!file.is_open())
        {
            fileNum = 1;
            file.open("/Users/agneyakerure/Desktop/test/parameterFile" + std::to_string(fileNum) + ".txt");

        }
        for( double x = 0; x <= 1; x = x + stepSize)
        {
            file << prefix << x << std::endl;
            std::ifstream ifs("/Users/agneyakerure/Desktop/test/parameterFile" + std::to_string(fileNum) + ".txt");
            std::string content( (std::istreambuf_iterator<char>(ifs) ),
                                (std::istreambuf_iterator<char>()    ) );
            generateParameterTextFiles(numParams - 1, stepSize, content);
        }
    }
    else
    {
        std::cout << prefix << std::endl;
        file.close();
        fileNum++;
        file.open("/Users/agneyakerure/Desktop/test/parameterFile" + std::to_string(fileNum) + ".txt");
    }
}

