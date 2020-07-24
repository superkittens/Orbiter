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
    olaWriteIndex = 0;
    hrirChanged = false;
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
    
    if (hrir.size() <= 0 || fs <= 0.0 || audioBufferSize <= 1)
        return false;
    
    auto audioBufferSizeCopy = audioBufferSize;
    size_t bitSum = 0;
    for (auto i = 0; i < sizeof(audioBufferSize) * 8; ++i)
    {
        bitSum += audioBufferSizeCopy & 0x01;
        audioBufferSizeCopy = audioBufferSizeCopy >> 1;
    }
    
    if (bitSum > 1)
        return false;
    
    
    fs = samplingFreq;
    audioBlockSize = audioBufferSize;
    
    //  Calculate buffer sizes and initialize them
    auto bufferPower = calculateNextPowerOfTwo(hrir.size() + audioBlockSize);
    zeroPaddedBufferSize = pow(2, bufferPower);
    
    fftEngine.reset(new juce::dsp::FFT(bufferPower));
    if (fftEngine.get() == nullptr)
        return false;
    
    xBuffer = std::vector<std::complex<float>>(zeroPaddedBufferSize);
    std::fill(xBuffer.begin(), xBuffer.end(), std::complex<float>(0.0, 0.0));
    
    auxBuffer = std::vector<std::complex<float>>(zeroPaddedBufferSize);
    
    olaBuffer = std::vector<float>(zeroPaddedBufferSize);
    std::fill(olaBuffer.begin(), olaBuffer.end(), 0.0);
    
    shadowOLABuffer = std::vector<float>(zeroPaddedBufferSize);
    std::fill(shadowOLABuffer.begin(), shadowOLABuffer.end(), 0.0);
    
    activeHRTF = std::vector<std::complex<float>>(zeroPaddedBufferSize);
    std::fill(xBuffer.begin(), xBuffer.end(), std::complex<float>(0.0, 0.0));
    
    auxHRTFBuffer = std::vector<std::complex<float>>(zeroPaddedBufferSize);
    std::fill(xBuffer.begin(), xBuffer.end(), std::complex<float>(0.0, 0.0));
    
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


bool HRTFProcessor::swapHRIR(const std::vector<float> &hrir)
{
    if (!hrirLoaded || hrir.size() <= 0)
        return false;
    
    bool success = setupHRTF(hrir);
    if (!success)
        return false;
    
    hrirChanged = true;
    
    return true;
}


/*
 *  HRTFProcessor::calculateOutput()
 *
 *  DO NOT CALL THIS FUNCTION ON MULTIPLE THREADS
 *  Apply the HRTF to an input sample of audio data
 *  If the HRTF is changed, the output will be a crossfaded mix of audio data
 *  with both HRTFs applied
 */
const float* HRTFProcessor::calculateOutput(const std::vector<float> &x)
{
    if (!hrirLoaded || x.size() == 0)
        return nullptr;
    
    olaWriteIndex += audioBlockSize;
    if (olaWriteIndex >= zeroPaddedBufferSize)
        olaWriteIndex = 0;
    
    std::fill(xBuffer.begin(), xBuffer.end(), std::complex<float>(0.0, 0.0));
    for (auto i = 0; i < x.size(); ++i)
        xBuffer.at(i) = std::complex<float>(x.at(i), 0.0);
    
    fftEngine->perform(xBuffer.data(), xBuffer.data(), false);
    
    
    for (auto i = 0; i < zeroPaddedBufferSize; ++i)
        xBuffer.at(i) = xBuffer.at(i) * activeHRTF.at(i);
    
    fftEngine->perform(xBuffer.data(), xBuffer.data(), true);
    
    if (hrirChanged)
    {
        juce::SpinLock::ScopedTryLockType hrirChangingScopedLock(hrirChangingLock);
        if (hrirChangingScopedLock.isLocked())
        {
            hrirChanged = false;
            crossfadeWithNewHRTF(x);
        }
    }
    
    if(!overlapAndAdd())
        return nullptr;
    
    return olaBuffer.data() + olaWriteIndex;
}


bool HRTFProcessor::setupHRTF(const std::vector<float> &hrir)
{
    if (hrir.size() == 0 || hrir.size() > zeroPaddedBufferSize)
        return false;
    
    if (fftEngine.get() == nullptr)
        return false;
    
    juce::SpinLock::ScopedLockType scopedLock(hrirChangingLock);
    
    std::fill(auxHRTFBuffer.begin(), auxHRTFBuffer.end(), std::complex<float>(0.0, 0.0));
    for (auto i = 0; i < hrir.size(); ++i)
        auxHRTFBuffer.at(i) = std::complex<float>(hrir.at(i), 0.0);
    
    fftEngine->perform(auxHRTFBuffer.data(), auxHRTFBuffer.data(), false);
    
    hrirChanged = true;
    
    return true;
}


bool HRTFProcessor::copyOLABuffer(std::vector<float> &dest, size_t numSamplesToCopy)
{
    if (!hrirLoaded)
        return false;
    
    if (numSamplesToCopy > zeroPaddedBufferSize)
        return false;
    
    juce::SpinLock::ScopedLockType olaScopeLock(shadowOLACopyingLock);
    
    std::copy(shadowOLABuffer.begin(), shadowOLABuffer.begin() + numSamplesToCopy, dest.begin());
    
    return true;
}


bool HRTFProcessor::crossfadeWithNewHRTF(const std::vector<float> &x)
{
    if (!hrirLoaded)
        return false;
    
    
    //  Calculate the output with the new HRTF applied before we crossfade the old and new outputs together
    std::fill(auxBuffer.begin(), auxBuffer.end(), std::complex<float>(0.0, 0.0));
    for (auto i = 0; i < audioBlockSize; ++i)
        auxBuffer.at(i) = std::complex<float>(x.at(i), 0.0);
    
    fftEngine->perform(auxBuffer.data(), auxBuffer.data(), false);
    
    for (auto i = 0; i < zeroPaddedBufferSize; ++i)
        auxBuffer.at(i) *= auxHRTFBuffer.at(i);
    
    fftEngine->perform(auxBuffer.data(), auxBuffer.data(), true);
    
    
    for (auto i = 0; i < zeroPaddedBufferSize; ++i)
    {
        if (i < audioBlockSize)
        {
            auto fadedSignal = (xBuffer.at(i).real() * fadeOutEnvelope.at(i)) + (auxBuffer.at(i).real() * fadeInEnvelope.at(i));
            xBuffer.at(i) = std::complex<float>(fadedSignal, 0.0);
        }
        else
            xBuffer.at(i) = auxBuffer.at(i);
    }
    
    return true;
}


bool HRTFProcessor::overlapAndAdd()
{
    if (!hrirLoaded)
        return false;
    
    auto offset = olaWriteIndex;
    
    for (auto i = 0; i < zeroPaddedBufferSize; ++i)
    {
        if (offset >= zeroPaddedBufferSize)
            offset = 0;
    
        olaBuffer.at(offset++) += xBuffer.at(i).real();
    }
    
    juce::SpinLock::ScopedTryLockType olaScopeLock(shadowOLACopyingLock);
    if (olaScopeLock.isLocked())
        std::copy(olaBuffer.begin(), olaBuffer.end(), shadowOLABuffer.begin());
        
    return true;
}


unsigned int HRTFProcessor::calculateNextPowerOfTwo(float x)
{
    return static_cast<unsigned int>(log2(x)) + 1;
}



void HRTFProcessorTest::runTest()
{
    HRTFProcessor processor;
    
    //  Input an impulse to the HRTFProcessor
    //  HRTF shoudl be a constant (1)
    std::vector<float> hrir(512);
    std::fill(hrir.begin(), hrir.end(), 0.0);
    hrir[255] = 1.0;
    
    float samplingFreq = 44100.0;
    size_t audioBufferSize = 256;
    
    beginTest("HRTFProcessor Initialization");
    {
        processor.init(hrir, samplingFreq, audioBufferSize);
        
        expectEquals<int>(processor.isHRIRLoaded(), 1);
        expectEquals<size_t>(processor.zeroPaddedBufferSize, 1024);
        expectEquals<float>(processor.fs, samplingFreq);
        expectEquals<size_t>(processor.audioBlockSize, audioBufferSize);
        
        for (auto i = 0; i < processor.zeroPaddedBufferSize; ++i)
            expectWithinAbsoluteError<float>(std::abs(processor.auxHRTFBuffer[i]), 1.0, 0.01);
    }
    
    beginTest("HRTF Application");
    {
        std::vector<float> x(audioBufferSize);
        std::fill(x.begin(), x.end(), 0.0);
        x[audioBufferSize / 2] = 1.0;
        
        const auto* output = processor.calculateOutput(x);
        output = processor.calculateOutput(x);
        
        expect(output != nullptr ?  true : false);
        for (auto i = 0; i < audioBufferSize; ++i)
        {
            //std::cout << i << ": ";
            std::cout << output[i] << std::endl;
        }
    }
    
    
    
}
