/*
  ==============================================================================

    FeatureExtraction.h
    Created: 14 Apr 2018 8:09:45pm
    Author:  Somesh Ganesh

  ==============================================================================
*/

#pragma once

#include <stdio.h>
#include <string>

using namespace std;

class CFeatureExtraction
{
public:
    
    CFeatureExtraction(){}
    ~CFeatureExtraction(){}
    
    void setSampleRate(int iParamValue);
    void setFrameSize(int iParamValue);
    void setHopSize(int iParamValue);
    
    int getSampleRate();
    int getFrameSize();
    int getHopSize();
    
    void initEssentia();
    void shutDownEssentia();
    
    void doFeatureExtract(string sInputFilePath, string sOutputFilePath);
    
private:
    int m_iSampleRate;
    int m_iFrameSize;
    int m_iHopSize;
    
};
