/*
  ==============================================================================

    Regression.cpp
    Created: 30 Apr 2018 1:31:36pm
    Author:  Somesh Ganesh

  ==============================================================================
*/
#include <iostream>
#include "Regression.h"
#include "../dlib-19.10/dlib/svm.h"
#include "essentia/essentia.h"
#include "essentia/algorithmfactory.h"
#include "essentia/essentiamath.h"
#include "essentia/pool.h"
#include <map>

using namespace essentia;
using namespace essentia::standard;
using namespace std;
//using namespace dlib;

Regression::Regression()
{
    m_DTrainingIter = 0;
    m_DLabelIter = 0;
}

Regression::~Regression()
{
    delete m_DTrainingIter;
    m_DTrainingIter = nullptr;
    delete m_DLabelIter;
    m_DLabelIter = 0;
}

void Regression::trainModel(std::string sInputTrainingData, std::string sInputTrainingLabels)
{
    essentia::init();
    m_Samples.clear();
    
//    for (int iCount = 0; iCount < 4; iCount++)
//    {
//        m_Targets[iCount].clear();
//    }
    
    m_DTrainingIter = new DirectoryIterator(File (sInputTrainingData), true, "*.txt");
    m_DLabelIter = new DirectoryIterator(File (sInputTrainingLabels), true, "*.txt");
    
    while (m_DTrainingIter->next())
    {
        string sInputTrainingFile = m_DTrainingIter->getFile().getFullPathName().toStdString();
        vector<float> vrFeatures = extractDataFromYAML(sInputTrainingFile, "");
        
        string sInputLabelFile = sInputTrainingLabels + m_DTrainingIter->getFile().getFileNameWithoutExtension().toStdString() + ".txt";
        
        
        if (vrFeatures.size() == 319)
        {
            for (int iFeature = 0; iFeature < 319; iFeature++)
            {
                m_DummySample(iFeature) = vrFeatures[iFeature];
            }
            m_Samples.push_back(m_DummySample);
            
            
            
//            std::cout << File(sInputLabelFile).loadFileAsString().toStdString() << std::endl;
            std::string sLabels = File(sInputLabelFile).loadFileAsString().toStdString();
            
            std::istringstream iss(sLabels);
            std::vector<std::string> vLabels((std::istream_iterator<std::string>(iss)),
                                             std::istream_iterator<std::string>());
            
            for (int iCount = 0; iCount < vLabels.size(); iCount++)
            {
                m_DummyLabel = std::stod(vLabels[iCount]);
                m_Targets[iCount].push_back(m_DummyLabel);
//                m_Targets[iCount].push_back()
            }
        }
        
        vrFeatures.clear();
        
    }
    
    dlib::svr_trainer<kernelType> SVRtrainer;
    SVRtrainer.set_kernel(kernelType(0.1));
    
    SVRtrainer.set_c(100);
    
    SVRtrainer.set_epsilon_insensitivity(0.1);
    
    dlib::decision_function<kernelType> dfs;
    
    std::vector<dlib::decision_function<kernelType>> vDecisionFunctions;
    
    
    for (int iLabel = 0; iLabel < m_Targets.size(); iLabel++)
    {
        vDecisionFunctions.push_back(SVRtrainer.train(m_Samples, m_Targets[iLabel]));
        dlib::randomize_samples(m_Samples, m_Targets[iLabel]);
        std::cout << "MSE and R-Squared: " << dlib::cross_validate_regression_trainer(SVRtrainer, m_Samples, m_Targets[iLabel], 5) << endl;
    }
    
    essentia::shutdown();
}

std::vector<float> Regression::extractDataFromYAML(std::string sInputTrainingData, std::string sInputTrainingLabels)
{
    Pool pFeaturePool;
    map<string,Real> mSingleFeatures;
    map<string,vector<Real>> mVecFeatures;
    string sStats[] = {"mean", "var", "min", "max", "median", "skew", "kurt", "dmean", "dvar", "dmean2", "dvar2"};
    string sSingleFeatures[] = {"Flux", "Rms", "RollOff", "SpecCent", "Zcr"};
    string sVecFeatures[] = {"HarmonicAmpFreq", "HarmonicAmpMag", "MfccCoeff", "SpecContrastCoeff"};
    
    
    Algorithm* Ainput = AlgorithmFactory::create("YamlInput", "filename", sInputTrainingData);
    Ainput->output("pool").set(pFeaturePool);
    Ainput->compute();
    
    mSingleFeatures = pFeaturePool.getSingleRealPool();
    mVecFeatures = pFeaturePool.getRealPool();
    
    vector<float> vrSingleFileFeatures;
    vrSingleFileFeatures.clear();
    for (int iFeature = 0; iFeature < 5; iFeature++)
    {
        for (int iStat = 0; iStat < 11; iStat++)
        {
            string sInputFeature = sSingleFeatures[iFeature] + "." + sStats[iStat];
            //            std::cout << mSingleFeatures[sInputFeature] << std::endl;
            //                pInputPCAPool.add("features", (float)mSingleFeatures[sInputFeature]);
            float val = float(mSingleFeatures[sInputFeature]);
            vrSingleFileFeatures.insert(vrSingleFileFeatures.end(), val);
        }
    }
    
    for (int iFeature = 0; iFeature < 4; iFeature++)
    {
        for (int iStat = 0; iStat < 11; iStat++)
        {
            string sInputFeature = sVecFeatures[iFeature] + "." + sStats[iStat];
            //            std::cout << mVecFeatures[sInputFeature] << std::endl;
            for (int iIdx = 0; iIdx < sizeof(mVecFeatures[sInputFeature]) / sizeof(mVecFeatures[sInputFeature][0]); iIdx++)
            {
                //                    pInputPCAPool.add("features", (float)mVecFeatures[sInputFeature][iIdx]);
                float val = float(mVecFeatures[sInputFeature][iIdx]);
                vrSingleFileFeatures.insert(vrSingleFileFeatures.end(), val);
            }
        }
//        Ainput.add("features", vrSingleFileFeatures);
    }
    
    return vrSingleFileFeatures;
        
//        std::string sInputFilePath = sInputTrainingLabels + m_ent->d_name;
}

void Regression::init(std::string sInputTrainingData, std::string sInputTrainingLabels)
{
    
}

void Regression::reset()
{
    
}
