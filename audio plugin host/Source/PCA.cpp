/*
  ==============================================================================

    PCA.cpp
    Created: 20 Apr 2018 4:28:35am
    Author:  Somesh Ganesh

  ==============================================================================
*/

#include "PCA.h"
#include "essentia/essentia.h"
#include "essentia/algorithmfactory.h"
#include "essentia/essentiamath.h"
#include "essentia/pool.h"
#include <map>

using namespace essentia;
using namespace essentia::standard;

void dimensionRed::doPCA(string sInputFilePath, string sOutputFilePath)
{
    Pool pFeaturePool;
    Pool pPCAPool;
    map<string,Real> mSingleFeatures;
    map<string,vector<Real>> mVecFeatures;
    string sStats[] = {"mean", "var", "min", "max", "median", "skew", "kurt", "dmean", "dvar", "dmean2", "dvar2"};
    string sSingleFeatures[] = {"Flux", "Rms", "RollOff", "SpecCent", "Zcr"};
    string sVecFeatures[] = {"HarmonicAmpFreq", "HarmonicAmpMag", "MfccCoeff", "SpecContrastCoeff", "SpecPeakAmp"};
    
    Pool pInputPCAPool;
    
    Algorithm* Ainput = AlgorithmFactory::create("YamlInput", "filename", sInputFilePath, "format", "json");
    
    Ainput->output("pool").set(pFeaturePool);
    Ainput->compute();
    
    mSingleFeatures = pFeaturePool.getSingleRealPool();
    
    for (int iFile = 2; iFile <= 3; iFile++)
    {
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
        
        mVecFeatures = pFeaturePool.getRealPool();
        
        for (int iFeature = 0; iFeature < 5; iFeature++)
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
        }
        pInputPCAPool.add("features", vrSingleFileFeatures);
        
    }
    
    Algorithm* APCA = AlgorithmFactory::create("PCA", "namespaceIn", "features", "namespaceOut", "pcaFeatures");
    
    APCA->input("poolIn").set(pInputPCAPool);
    APCA->output("poolOut").set(pPCAPool);
    //    APCA->compute();
    
    //    std::cout << mFeatures["Flux.dmean"] << std::endl;
    
    Algorithm* Aoutput = AlgorithmFactory::create("YamlOutput", "filename", sOutputFilePath);
    Aoutput->input("pool").set(pInputPCAPool);
    Aoutput->compute();
}


