/*
  ==============================================================================

    AudioFileInput.h
    Created: 1 May 2018 4:32:10pm
    Author:  Agneya Kerure

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class AudioFileInput
{
public:
    AudioFileInput();
    ~AudioFileInput();
    
    void selectAudioFile();
    void playAudioFile();
    void stopAudioFile();
    std::string getAddress();
    void getState();
    
private:
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    AudioFormatManager m_formatManager;
    std::unique_ptr<AudioFormatReaderSource> m_readerSource;
    AudioTransportSource m_transportSource;
    TransportState m_state;
    String m_audioFileAddress;
    bool m_isFileInitialized;
    
};
