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
    HRTFProcessor(const double *hrir, size_t hrirSize, float samplingFreq, size_t audioBufferSize);
    
    bool            init(const double *hrir, size_t hrirSize, float samplingFreq, size_t audioBufferSize);
    bool            swapHRIR(const double *hrir, size_t hrirSize);
    const float*    calculateOutput(const std::vector<float> &x);
    //bool            getFadedOutput(std::vector<float> &dest, bool fadeInOut = true);
    bool            copyOLABuffer(std::vector<float> &dest, size_t numSamplesToCopy);
    bool            isHRIRLoaded() { return hrirLoaded; }
    
    
    
protected:
    
    bool            setupHRTF(const double *hrir, size_t hrirSize);
    bool            overlapAndAdd();
    bool            crossfadeWithNewHRTF(const std::vector<float> &x);
    unsigned int    calculateNextPowerOfTwo(float x);
    
    double          fs;
    
    std::vector<float>                              shadowOLABuffer;
    std::vector<std::complex<float>>                activeHRTF;
    std::vector<std::complex<float>>                auxHRTFBuffer;
    std::vector<std::complex<float>>                xBuffer;
    std::vector<float>                              olaBuffer;
    std::vector<std::complex<float>>                auxBuffer;
    size_t                                          audioBlockSize;

    bool                                            hrirChanged;
    size_t                                          olaWriteIndex;
    size_t                                          zeroPaddedBufferSize;
    std::vector<float>                              fadeInEnvelope;
    std::vector<float>                              fadeOutEnvelope;
    
    std::unique_ptr<juce::dsp::FFT>                 fftEngine;
    
    juce::SpinLock                                  hrirChangingLock;
    juce::SpinLock                                  shadowOLACopyingLock;
    
    bool                                            hrirLoaded;
    
    static constexpr size_t                         NUM_BUFFERS = 2;
};


#ifdef JUCE_UNIT_TESTS
class HRTFProcessorTest : public juce::UnitTest
{
public:
    HRTFProcessorTest() : UnitTest("HRTFProcessorUnitTest", "HRTFProcessor") {};
    
    void runTest() override;
    
private:
    bool createTestSignal(float fs, float f0, std::vector<float> &dest);
};

static HRTFProcessorTest hrtfProcessorUnitTest;

#endif
