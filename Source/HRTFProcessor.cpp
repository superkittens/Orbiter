/*
  ==============================================================================

    HRTFProcessor.cpp
    Created: 16 Jul 2020 3:59:00pm
    Author:  Allen Lee

  ==============================================================================
*/

#include "HRTFProcessor.h"

HRTFProcessor::HRTFProcessor()
{
    hrirChanged = false;
    olaCopyIndex = NUM_BUFFERS - 1;
    olaWriteIndex = 0;
    olaReadIndex = 0;
    hrtfWriteIndex = 0;
    hrtfReadIndex = NUM_BUFFERS - 1;
    
    hrirLoaded = false;
}

HRTFProcessor::HRTFProcessor(const std::vector<float> &hrir, float samplingFreq, size_t audioBufferSize)
{
    if (!init(hrir, fs, audioBufferSize))
        hrirLoaded = false;
}


bool HRTFProcessor::init(const std::vector<float> &hrir, float samplingFreq, size_t audioBufferSize)
{
    if (hrirLoaded)
        return false;
    
    if (hrir.size() <= 0 || fs <= 0.0 || audioBufferSize <= 0)
        return false;
    
    fs = samplingFreq;
    audioBlockSize = audioBufferSize;
    
    //  Calculate buffer sizes and initialize them
    auto bufferPower = calculateNextPowerOfTwo(hrir.size() + audioBlockSize);
    zeroPaddedBufferSize = pow(2, bufferPower);
    
    fftEngine.reset(new juce::dsp::FFT(bufferPower));
    if (fftEngine.get() == nullptr)
        return false;
    
    xBuffer.reserve(zeroPaddedBufferSize);
    olaBuffer = std::vector<std::vector<float>>(NUM_BUFFERS);
    hrtfs = std::vector<std::vector<std::complex<float>>>(NUM_BUFFERS);
    
    for (auto i = 0; i < NUM_BUFFERS; ++i)
    {
        olaBuffer.at(i) = std::vector<float>(zeroPaddedBufferSize);
        std::fill(olaBuffer.at(i).begin(), olaBuffer.at(i).end(), 0.0);
        
        hrtfs.at(i) = std::vector<std::complex<float>>(zeroPaddedBufferSize);
        std::fill(hrtfs.at(i).begin(), hrtfs.at(i).end(), std::complex<float>(0.0, 0.0));
    }
    
    //  Transform HRIR into HRTF
    if (!setupHRTF(hrir))
        return false;
    
    for (auto i = 0; i < audioBlockSize; ++i)
    {
        fadeOutEnvelope.push_back(pow(juce::dsp::FastMathApproximations::sin((i * juce::MathConstants<float>::pi) / (2 * audioBlockSize)), 2));
        fadeInEnvelope.push_back(pow(juce::dsp::FastMathApproximations::cos((i * juce::MathConstants<float>::pi) / (2 * audioBlockSize)), 2));
    }
    
    hrirLoaded = true;
    
    return true;
}


bool HRTFProcessor::setupHRTF(const std::vector<float> &hrir)
{
    if (hrir.size() == 0 || hrir.size() > zeroPaddedBufferSize)
        return false;
    
    if (fftEngine.get() == nullptr)
        return false;
    
    juce::SpinLock::ScopedLockType scopedLock(hrirChangingLock);
    
    std::fill(hrtfs.at(hrtfWriteIndex).begin(), hrtfs.at(hrtfWriteIndex).end(), std::complex<float>(0.0, 0.0));
    for (auto i = 0; i < hrir.size(); ++i)
        hrtfs.at(hrtfWriteIndex).at(i) = std::complex<float>(hrir.at(i), 0.0);
    
    fftEngine->perform(hrtfs.at(hrtfWriteIndex).data(), hrtfs.at(hrtfWriteIndex).data(), false);
    
    hrirChanged = true;
    
    return true;
}


unsigned int HRTFProcessor::calculateNextPowerOfTwo(float x)
{
    return static_cast<unsigned int>(log2(x)) + 1;
}



void HRTFProcessorTest::runTest()
{
    beginTest("HRTFProcessor Initialization");
    {
        HRTFProcessor processor;
        
        float samplingFreq = 44100.0;
        size_t audioBufferSize = 256;
        
        //  Input an impulse to the HRTFProcessor
        //  HRTF shoudl be a constant (1)
        std::vector<float> hrir(512);
        std::fill(hrir.begin(), hrir.end(), 0.0);
        hrir[255] = 1.0;
        
        processor.init(hrir, samplingFreq, audioBufferSize);
        
        expectEquals<int>(processor.isHRIRLoaded(), 1);
        expectEquals<size_t>(processor.zeroPaddedBufferSize, 1024);
        expectEquals<float>(processor.fs, samplingFreq);
        expectEquals<size_t>(processor.audioBlockSize, audioBufferSize);
        
        for (auto i = 0; i < processor.zeroPaddedBufferSize; ++i)
            expectWithinAbsoluteError<float>(std::abs(processor.hrtfs[processor.hrtfWriteIndex][i]), 1.0, 0.01);
    }
    
    
    
}
