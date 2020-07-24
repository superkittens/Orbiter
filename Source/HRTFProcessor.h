/*
  ==============================================================================

    HRTFProcessor.h
    Created: 16 Jul 2020 3:59:00pm
    Author:  Allen Lee

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <complex>


class HRTFProcessor
{
#ifdef JUCE_UNIT_TESTS
    friend class HRTFProcessorTest;
#endif
    
public:
    
    HRTFProcessor();
    HRTFProcessor(const std::vector<float> &hrir, float samplingFreq, size_t audioBufferSize);
    
    bool            init(const std::vector<float> &hrir, float samplingFreq, size_t audioBufferSize);
    bool            swapHRIR(const std::vector<float> &hrir);
    const float*    calculateOutput(const std::vector<float> &x);
    //bool            getFadedOutput(std::vector<float> &dest, bool fadeInOut = true);
    bool            copyOLABuffer(std::vector<float> &dest, size_t numSamplesToCopy);
    bool            isHRIRLoaded() { return hrirLoaded; }
    
protected:
    
    bool            setupHRTF(const std::vector<float> &hrir);
    bool            overlapAndAdd();
    bool            crossfadeWithNewHRTF(const std::vector<float> &x);
    unsigned int    calculateNextPowerOfTwo(float x);
    
    double          fs;
    
    std::vector<float>                              olaBuffer;
    std::vector<float>                              shadowOLABuffer;
    std::vector<std::complex<float>>                activeHRTF;
    std::vector<std::complex<float>>                auxHRTFBuffer;
    std::vector<std::complex<float>>                xBuffer;
    std::vector<std::complex<float>>                auxBuffer;
    size_t                                          audioBlockSize;
    size_t                                          zeroPaddedBufferSize;
    bool                                            hrirChanged;
    size_t                                          olaWriteIndex;
    
    std::vector<float>                              fadeInEnvelope;
    std::vector<float>                              fadeOutEnvelope;
    
    std::unique_ptr<juce::dsp::FFT>                 fftEngine;
    
    juce::SpinLock                                  hrirChangingLock;
    juce::SpinLock                                  shadowOLACopyingLock;
    
    bool                                            hrirLoaded;
    
    static constexpr size_t                         NUM_BUFFERS = 2;
};



class HRTFProcessorTest : public juce::UnitTest
{
public:
    HRTFProcessorTest() : UnitTest("HRTFProcessorUnitTest", "HRTFProcessor") {};
    
    void runTest() override;
};

static HRTFProcessorTest hrtfProcessorUnitTest;
