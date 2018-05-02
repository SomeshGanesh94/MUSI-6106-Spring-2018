/*
  ==============================================================================

    AudioFileInput.cpp
    Created: 1 May 2018 4:32:10pm
    Author:  Agneya Kerure

  ==============================================================================
*/

#include "AudioFileInput.h"

AudioFileInput::AudioFileInput()
{
    m_isFileInitialized = false;
    m_formatManager.registerBasicFormats();
}

AudioFileInput::~AudioFileInput()
{

}

void AudioFileInput::selectAudioFile()
{
    FileChooser chooser ("Select a Wave file to play...",
                         File::nonexistent,
                         "*.wav");
    
    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        auto* reader = m_formatManager.createReaderFor (file);
        m_audioFileAddress = file.getFullPathName();
        m_isFileInitialized = true;
    }
}

void AudioFileInput::playAudioFile()
{
    
}

void AudioFileInput::stopAudioFile()
{
    
}

std::string AudioFileInput::getAddress()
{
    jassert(m_isFileInitialized == true);
    
    if(m_isFileInitialized)
    {
        return m_audioFileAddress.toStdString();
    }
    
    std::cout<<"File not found";
    return NULL;
}

void AudioFileInput::getState()
{
    
}
