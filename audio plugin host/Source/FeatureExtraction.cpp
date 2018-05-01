/*
  ==============================================================================

    FeatureExtraction.cpp
    Created: 14 Apr 2018 8:09:45pm
    Author:  Somesh Ganesh

  ==============================================================================
*/

#include "FeatureExtraction.h"
#include "essentia/essentia.h"
#include "essentia/algorithmfactory.h"
#include "essentia/essentiamath.h"
#include "essentia/pool.h"

using namespace essentia;
using namespace essentia::standard;

void CFeatureExtraction::setSampleRate(int iParamValue)
{
    m_iSampleRate = iParamValue;
}

void CFeatureExtraction::setFrameSize(int iParamValue)
{
    m_iFrameSize = iParamValue;
}

void CFeatureExtraction::setHopSize(int iParamValue)
{
    m_iHopSize = iParamValue;
}

int CFeatureExtraction::getSampleRate()
{
    return m_iSampleRate;
}

int CFeatureExtraction::getFrameSize()
{
    return m_iFrameSize;
}

int CFeatureExtraction::getHopSize()
{
    return m_iHopSize;
}

void CFeatureExtraction::initEssentia()
{
    essentia::init();
    m_iFrameSize = 2048;
    m_iHopSize = 1024;
}

void CFeatureExtraction::shutDownEssentia()
{
    essentia::shutdown();
}

void CFeatureExtraction::doFeatureExtract(string sInputFilePath, string sOutputFilePath)
{
    
    
    // creating algorithms
    Pool pPool;
    
    AlgorithmFactory& factory = standard::AlgorithmFactory::instance();
    
    Algorithm* Aaudio = factory.create("MonoLoader", "filename", sInputFilePath);
    m_iSampleRate = 44100;
    
    Algorithm* AframeCutter = factory.create("FrameCutter",
                                             "frameSize", m_iFrameSize,
                                             "hopSize", m_iHopSize);
    
    Algorithm* Awindow = factory.create("Windowing", "type", "blackmanharris62");
    
    Algorithm* Aspectrum = factory.create("Spectrum");
    
    Algorithm* Apitch = factory.create("PitchYin", "frameSize", m_iFrameSize, "sampleRate", m_iSampleRate);
    Algorithm* Amfcc = factory.create("MFCC");
    Algorithm* Arms = factory.create("RMS");
    Algorithm* Azcr = factory.create("ZeroCrossingRate");
    Algorithm* AspecCent = factory.create("SpectralCentroidTime");
    Algorithm* ArollOff = factory.create("RollOff", "sampleRate", m_iSampleRate);
    Algorithm* Aflux = factory.create("Flux");
    Algorithm* AspecPeak = factory.create("SpectralPeaks", "sampleRate", m_iSampleRate, "maxPeaks", 5, "minFrequency", 1);
    Algorithm* AspecContrast = factory.create("SpectralContrast", "frameSize", m_iFrameSize, "sampleRate", m_iSampleRate);
    Algorithm* AharmonicAmp = factory.create("HarmonicPeaks", "maxHarmonics", 1);
    
    
    // connecting the algorithms
    cout << "Connecting the algorithms" << endl;
    
    std::vector<Real> vrAudioBuffer;
    std::vector<Real> vrFrame, vrWindowedFrame;
    std::vector<Real> vrSpectrum;
    std::vector<Real> vrSpecPeakAmp, vrSpecPeakFreq, vrSpecContrastCoeff, vrSpecContrastMag, vrMfccBands, vrMfccCoeff, vrHarmonicAmpFreq, vrHarmonicAmpMag;
    Real rRms, rZcr, rSpecCent, rRollOff, rFlux, rPitch, rPitchConf;
    
    // audio->frame cutter
    Aaudio->output("audio").set(vrAudioBuffer);
    AframeCutter->input("signal").set(vrAudioBuffer);
    
    // frame cutter->windowing->spectrum
    AframeCutter->output("frame").set(vrFrame);
    Awindow->input("frame").set(vrFrame);
    Awindow->output("frame").set(vrWindowedFrame);
    Aspectrum->input("frame").set(vrWindowedFrame);
    Aspectrum->output("spectrum").set(vrSpectrum);
    
    // features
    Apitch->input("signal").set(vrAudioBuffer);
    Apitch->output("pitch").set(rPitch);
    Apitch->output("pitchConfidence").set(rPitchConf);
    
    Arms->input("array").set(vrFrame);
    Arms->output("rms").set(rRms);
    
    Azcr->input("signal").set(vrFrame);
    Azcr->output("zeroCrossingRate").set(rZcr);
    
    AspecCent->input("array").set(vrFrame);
    AspecCent->output("centroid").set(rSpecCent);
    
    ArollOff->input("spectrum").set(vrSpectrum);
    ArollOff->output("rollOff").set(rRollOff);
    
    Aflux->input("spectrum").set(vrSpectrum);
    Aflux->output("flux").set(rFlux);
    
    AspecPeak->input("spectrum").set(vrSpectrum);
    AspecPeak->output("magnitudes").set(vrSpecPeakAmp);
    AspecPeak->output("frequencies").set(vrSpecPeakFreq);
    
    AspecContrast->input("spectrum").set(vrSpectrum);
    AspecContrast->output("spectralContrast").set(vrSpecContrastCoeff);
    AspecContrast->output("spectralValley").set(vrSpecContrastMag);
    
    AharmonicAmp->input("frequencies").set(vrSpecPeakFreq);
    AharmonicAmp->input("magnitudes").set(vrSpecPeakAmp);
    AharmonicAmp->input("pitch").set(rPitch);
    AharmonicAmp->output("harmonicFrequencies").set(vrHarmonicAmpFreq);
    AharmonicAmp->output("harmonicMagnitudes").set(vrHarmonicAmpMag);
    
    Amfcc->input("spectrum").set(vrSpectrum);
    Amfcc->output("bands").set(vrMfccBands);
    Amfcc->output("mfcc").set(vrMfccCoeff);
    
    cout << "Start processing" << endl;
    
    Aaudio->compute();
    Apitch->compute();
    
    while(true)
    {
        AframeCutter->compute();
        
        if(!vrFrame.size())
        {
            break;
        }
        
        if(isSilent(vrFrame))
        {
            continue;
        }
        
        Awindow->compute();
        Aspectrum->compute();
        Arms->compute();
        Azcr->compute();
        AspecCent->compute();
        ArollOff->compute();
        Aflux->compute();
        AspecPeak->compute();
        AspecContrast->compute();
        AharmonicAmp->compute();
        Amfcc->compute();
        
        std::vector<Real> vrFeatureVector;
        vrFeatureVector.insert(vrFeatureVector.end(), rRms);
        //        vrFeatureVector.insert(vrFeatureVector.end(), rZcr);
        //        vrFeatureVector.insert(vrFeatureVector.end(), rSpecCent);
        //        vrFeatureVector.insert(vrFeatureVector.end(), rRollOff);
        //        vrFeatureVector.insert(vrFeatureVector.end(), rFlux);
        //        vrFeatureVector.insert(vrFeatureVector.end(), vrSpecPeakAmp.begin(), vrSpecPeakAmp.end());
        //        vrFeatureVector.insert(vrFeatureVector.end(), vrSpecContrastCoeff.begin(), vrSpecContrastCoeff.end());
        //        vrFeatureVector.insert(vrFeatureVector.end(), vrHarmonicAmpMag.begin(), vrHarmonicAmpMag.end());
        //        vrFeatureVector.insert(vrFeatureVector.end(), vrHarmonicAmpFreq.begin(), vrHarmonicAmpFreq.end());
        //        vrFeatureVector.insert(vrFeatureVector.end(), vrMfccCoeff.begin(), vrMfccCoeff.end());
        
        
        
        pPool.add("Rms", rRms);
        pPool.add("Zcr", rZcr);
        pPool.add("SpecCent", rSpecCent);
        pPool.add("RollOff", rRollOff);
        pPool.add("Flux", rFlux);
        pPool.add("SpecPeakAmp", vrSpecPeakAmp);
        pPool.add("SpecContrastCoeff", vrSpecContrastCoeff);
        pPool.add("HarmonicAmpMag", vrHarmonicAmpMag);
        pPool.add("HarmonicAmpFreq", vrHarmonicAmpFreq);
        pPool.add("MfccCoeff", vrMfccCoeff);
        //        pPool.add("F", vrFeatureVector);
        
        
        cout << "New frame" << endl;
        //        cout << vrMfccCoeff << endl;
    }
    
    cout << "Aggregating results" << endl;
    
    Pool pAggrPool;
    Pool pStatFeaturePool;
    const char* stats[] = {"mean", "var", "min", "max", "median", "skew", "kurt", "dmean", "dvar", "dmean2", "dvar2"};
    
    Algorithm* Aaggr = AlgorithmFactory::create("PoolAggregator", "defaultStats", arrayToVector<string>(stats));
    
    Aaggr->input("input").set(pPool);
    Aaggr->output("output").set(pAggrPool);
    Aaggr->compute();
    
    //    cout << pAggrPool.getRealPool() << endl;
    
    //    Pool pPCAPool;
    
    //    Algorithm* APCA = AlgorithmFactory::create("PCA", "dimensions", 10);
    
    //    APCA->input("poolIn").set(pPool);
    //    APCA->output("poolOut").set(pPCAPool);
    //    APCA->compute();
    
    Algorithm* Aoutput = AlgorithmFactory::create("YamlOutput", "filename", sOutputFilePath);
    
    Aoutput->input("pool").set(pAggrPool);
    Aoutput->compute();
    
    cout << "Done" << endl;
    
    delete Aaudio;
    delete AframeCutter;
    delete Awindow;
    delete Aspectrum;
    delete Apitch;
    delete Amfcc;
    delete Arms;
    delete Azcr;
    delete AspecCent;
    delete ArollOff;
    delete Aflux;
    delete AspecPeak;
    delete AspecContrast;
    delete AharmonicAmp;
    delete Aoutput;
}
