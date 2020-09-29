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
    HRTFProcessor(const double *hrir, size_t hrirSize, float samplingFreq, size_t audioBufferSize, size_t numDelaySamples);
    
    bool                init(const double *hrir, size_t hrirSize, float samplingFreq, size_t audioBufferSize, size_t numDelaySamples);
    bool                swapHRIR(const double *hrir, size_t hrirSize, size_t numDelaySamples);
    bool                addSamples(float *samples, size_t numSamples);
    std::vector<float>  getOutput(size_t numSamples);
    void                flushBuffers();
    bool                copyOLABuffer(std::vector<float> &dest, size_t numSamplesToCopy);
    bool                isHRIRLoaded() { return hrirLoaded; }
    void                setReverbParameters(juce::Reverb::Parameters params);
    
    bool                crossFaded;
    
    
protected:
    
    bool                        setupHRTF(const double *hrir, size_t hrirSize, size_t numDelaySamples);
    const float*                calculateOutput(const std::vector<float> &x);
    bool                        overlapAndAdd();
    bool                        crossfadeWithNewHRTF(const std::vector<float> &x);
    unsigned int                calculateNextPowerOfTwo(float x);
    bool                        removeImpulseDelay(std::vector<float> &hrir, size_t numDelaySamples);
    std::pair<float, float>     getMeanAndStd(const std::vector<float> &x) const;
    
    
    double                                          fs;
    
    std::vector<float>                              inputBuffer;
    size_t                                          inputBlockStart;
    size_t                                          inputSampleAddIndex;
    std::vector<float>                              outputBuffer;
    size_t                                          outputSamplesStart;
    size_t                                          outputSamplesEnd;
    size_t                                          numOutputSamplesAvailable;
    size_t                                          numSamplesAdded;
    size_t                                          hopSize;
    
    std::vector<float>                              reverbBuffer;
    size_t                                          reverbBufferStartIndex;
    size_t                                          reverbBufferAddIndex;
    
    std::vector<float>                              window;
    
    std::vector<float>                              shadowOLABuffer;
    std::vector<std::complex<float>>                activeHRTF;
    std::vector<std::complex<float>>                auxHRTFBuffer;
    std::vector<std::complex<float>>                xBuffer;
    std::vector<float>                              olaBuffer;
    size_t                                          outputSampleStart;
    size_t                                          outputSampleEnd;
    std::vector<std::complex<float>>                auxBuffer;
    size_t                                          audioBlockSize;

    bool                                            hrirChanged;
    size_t                                          olaWriteIndex;
    size_t                                          zeroPaddedBufferSize;
    std::vector<float>                              fadeInEnvelope;
    std::vector<float>                              fadeOutEnvelope;
    
    std::unique_ptr<juce::dsp::FFT>                 fftEngine;
    
    juce::Reverb                                    reverb;
    
    juce::SpinLock                                  hrirChangingLock;
    juce::SpinLock                                  shadowOLACopyingLock;
    
    bool                                            hrirLoaded;
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
