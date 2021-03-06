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
#include "PluginContainerProcessor.h"

PluginContainer::PluginContainer()
{
}

PluginContainer::~PluginContainer()
{
    delete pluginInstance;
    delete newProcessor;
}

void PluginContainer::init(AudioProcessor& plugin)
{
    pluginInstance = &plugin;
    m_kiNumpParams = pluginInstance->getNumParameters();
    m_pdParamValArray = new double[(unsigned long)m_kiNumpParams];
}

void PluginContainer::reset()
{
    delete [] m_pdParamValArray;
    m_pdParamValArray = 0;
}

//Setters
void PluginContainer::setProcessor(AudioProcessor& processor)
{
    newProcessor = &processor;
}

void PluginContainer::setParameter(int index, float value)
{
    pluginInstance -> setParameterNotifyingHost(index, value);
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

void PluginContainer::generateParameterTextFiles(int depth, std::vector<float> & numbers, std::vector<float> & maxes)
{
    String filePath = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getFullPathName() + "/../../../../../generatedDatasetFiles";
    String sTestFilePath = filePath.toStdString() + String("/parameterFiles/");
    String sAudioFilePath = filePath.toStdString() + String("/audioFiles/");
    String sFeatureFilePath = filePath.toStdString() + String("/featureFiles/");
    String sInputAudioFeatureFilePath = filePath.toStdString() + String("/inputAudioFeatures/");
    
    File tempParamFile(sTestFilePath);
    File tempAudioFile(sAudioFilePath);
    File tempFeatureFile(sFeatureFilePath);
    File tempInputAudioFeatureFile(sInputAudioFeatureFilePath);
    
    if (!tempParamFile.isDirectory())
    {
        tempParamFile.createDirectory();
    }
    if (!tempAudioFile.isDirectory())
    {
        tempAudioFile.createDirectory();
    }
    if (!tempFeatureFile.isDirectory())
    {
        tempFeatureFile.createDirectory();
    }
    if (!tempInputAudioFeatureFile.isDirectory())
    {
        tempInputAudioFeatureFile.createDirectory();
    }
    
    static int num = 0;
    if (depth>0)
    {
        for(double i = 0; i <= 1; i = i + 0.25)
        {
            numbers[depth - 1] = i;
            generateParameterTextFiles(depth-1, numbers,maxes) ;
        }
    }
    else
    {
        num = num + 1;
        file.open(sTestFilePath.toStdString() + std::to_string(num) + ".txt");
        for(int i = 0; i < numbers.size(); i++)
        {
            file << numbers[i] << std::endl;
            this->setParameter(i, numbers[i]);
        }
        PluginContainerProcessor::generateAudioFile(true, sAudioFilePath.toStdString() + std::to_string(num) + ".wav");
        while(PluginContainerProcessor::m_bRecording) {}
        PluginContainerProcessor::writeAudioFile();
        file.close();
    }
}

//set connection with input and output
void PluginContainer::genFiles(int numParams, int maxLimit)
{
    std::vector<float> numbers;
    numbers.resize( numParams );
    std::vector<float> maxes;
    for(int i = 0; i < numParams; i++)
    {
        maxes.push_back(maxLimit);
    }
    this->generateParameterTextFiles( numbers.size(), numbers, maxes );
}
