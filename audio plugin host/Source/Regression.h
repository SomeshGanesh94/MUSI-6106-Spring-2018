/*
  ==============================================================================

    Regression.h
    Created: 30 Apr 2018 1:31:36pm
    Author:  Somesh Ganesh

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include <iostream>
#include "../dlib-19.10/dlib/svm.h"
#include <map>

class Regression
{
public:
    
    Regression();
    ~Regression();
    
    void trainModel(std::string sInputTrainingData, std::string sInputTrainingLabels);
    std::vector<float> predictOutput(std::string sFeatureFile);
    void init(std::string sInputTrainingData, std::string sInputTrainingLabels);
    void reset();
    
private:
    const int m_kiFeatureSize = 209;
    
    typedef dlib::matrix<double, 209, 1> sampleType;
    typedef dlib::matrix<double> labelType;
    typedef dlib::radial_basis_kernel<sampleType> kernelType;
    dlib::svr_trainer<kernelType> SVRtrainer;
    std::vector<dlib::decision_function<kernelType>> vDecisionFunctions;
    
    int m_iNumLabels;
    sampleType m_DummySample;
    double m_DummyLabel;
    
    std::vector<sampleType> m_Samples;
    std::map<int, std::vector<double>> m_Targets;
    
    std::vector<float> extractDataFromYAML(std::string sInputTrainingData);
    void getTrainingData(std::string sInputFileFolder);
    void getLabels(std::string sInputFileFolder);
    
    DirectoryIterator* m_DTrainingIter;
    DirectoryIterator* m_DLabelIter;
};
