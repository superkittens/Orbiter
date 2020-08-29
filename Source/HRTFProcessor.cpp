#include "HRTFProcessor.h"

HRTFProcessor::HRTFProcessor()
{
    olaWriteIndex = 0;
    hrirChanged = false;
    hrirLoaded = false;
}

HRTFProcessor::HRTFProcessor(const double *hrir, size_t hrirSize, float samplingFreq, size_t audioBufferSize)
{
    olaWriteIndex = 0;
    hrirChanged = false;
    hrirLoaded = false;
    
    if (!init(hrir, hrirSize, fs, audioBufferSize))
        hrirLoaded = false;
}


bool HRTFProcessor::init(const double *hrir, size_t hrirSize, float samplingFreq, size_t audioBufferSize)
{
    if (hrirLoaded)
        return false;
    
    if (hrir == nullptr)
        return false;
    
    if (hrirSize <= 0 || samplingFreq <= 0.0 || audioBufferSize <= 1)
        return false;
    
    //  audioBufferSize must be a power of 2
    //  Check this constraint here
    auto audioBufferSizeCopy = audioBufferSize;
    size_t bitSum = 0;
    for (auto i = 0; i < sizeof(audioBufferSize) * 8; ++i)
    {
        bitSum += audioBufferSizeCopy & 0x01;
        audioBufferSizeCopy = audioBufferSizeCopy >> 1;
    }
    
    if (bitSum > 1)
        return false;
    
    
    //  When performing overlap and add with windowing, we need to have 2x audioBufferSize in order to get audioBufferSize outputs
    fs = samplingFreq;
    audioBlockSize = (audioBufferSize * 2) + 1;
    hopSize = audioBufferSize;
    
    //  Calculate buffer sizes and initialize them
    auto bufferPower = calculateNextPowerOfTwo(hrirSize + audioBlockSize);
    zeroPaddedBufferSize = pow(2, bufferPower);
    
    fftEngine.reset(new juce::dsp::FFT(bufferPower));
    if (fftEngine.get() == nullptr)
        return false;
    
    inputBuffer = std::vector<float>(3 * audioBufferSize+ 1);
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0);
    
    outputBuffer = std::vector<float>(zeroPaddedBufferSize);
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0);
    
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
    
    reverbBuffer = std::vector<float>(zeroPaddedBufferSize);
    std::fill(reverbBuffer.begin(), reverbBuffer.end(), 0.0);
    
    outputSampleStart = 0;
    outputSampleEnd = 0;
    numSamplesAdded = 0;
    inputBlockStart = 0;
    inputSampleAddIndex = 0;
    numOutputSamplesAvailable = 0;
    
    reverbBufferStartIndex = 0;
    reverbBufferAddIndex = 0;
    
    reverb.setSampleRate(samplingFreq);
    juce::Reverb::Parameters reverbParam;
    
    reverbParam.roomSize = 1.0f;
    reverbParam.damping = 0.3f;
    reverbParam.dryLevel = 0.2f;
    reverbParam.wetLevel = 0.7f;
    
    reverb.setParameters(reverbParam);
    
    //  Create input window
    //  To satisfy the COLA constraint for a Hamming window, the last value should be 0
    window = std::vector<float>(audioBlockSize);
    juce::dsp::WindowingFunction<float>::fillWindowingTables(window.data(), audioBlockSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hamming);
    window[audioBlockSize - 1] = 0.0;
    
    
    //  Transform HRIR into HRTF
    if (!setupHRTF(hrir, hrirSize))
        return false;
    
    for (auto i = 0; i < audioBlockSize; ++i)
    {
        fadeOutEnvelope.push_back(pow(juce::dsp::FastMathApproximations::cos((i * juce::MathConstants<float>::pi) / (2 * audioBlockSize)), 2));
        fadeInEnvelope.push_back(pow(juce::dsp::FastMathApproximations::sin((i * juce::MathConstants<float>::pi) / (2 * audioBlockSize)), 2));
    }
    
    hrirLoaded = true;
    
    return true;
}


bool HRTFProcessor::swapHRIR(const double *hrir, size_t hrirSize)
{
    if (!hrirLoaded || hrirSize <= 0)
        return false;
    
    bool success = setupHRTF(hrir, hrirSize);
    if (!success)
        return false;
    
    hrirChanged = true;
    
    return true;
}


/*
 *  DO NOT CALL THIS FUNCTION ON MULTIPLE THREADS
 *  Add input samples into the input buffer
 *  If enough samples have been collected then process those samples
 *  To get the processed output, call getOutput()
 */
bool HRTFProcessor::addSamples(float *samples, size_t numSamples)
{
    if (numSamples > inputBuffer.size() - numSamplesAdded)
        return false;
    
    for (auto i = 0; i < numSamples; ++i)
    {
        //  Add samples into the input buffer and reverb buffer
        inputBuffer[inputSampleAddIndex] = samples[i];
        reverbBuffer[reverbBufferAddIndex] = samples[i];
        
        inputSampleAddIndex = (inputSampleAddIndex + 1) % inputBuffer.size();
        reverbBufferAddIndex = (reverbBufferAddIndex + 1) % reverbBuffer.size();
        
        numSamplesAdded++;
    }
    
    
    //  Execute when we have added enough samples for processing
    if (numSamplesAdded >= audioBlockSize)
    {
        numSamplesAdded -= hopSize;
        std::vector<float> x(audioBlockSize);
        auto blockStart = inputBlockStart;
        
        for (auto i = 0; i < audioBlockSize; ++i)
        {
            x[i] = inputBuffer[blockStart] * window[i];
            blockStart = (blockStart + 1) % inputBuffer.size();
        }
        
        inputBlockStart = (inputBlockStart + hopSize) % inputBuffer.size();
        calculateOutput(x);
    }
    
    return true;
    
}


std::vector<float> HRTFProcessor::getOutput(size_t numSamples)
{
    std::vector<float> out(numSamples);
    if (numSamples > numOutputSamplesAvailable)
        return std::vector<float>(0);
    
    //  Get reverberated input signal
    reverb.processMono(reverbBuffer.data() + reverbBufferStartIndex, (int)numSamples);
    
    for (auto i = 0; i < numSamples; ++i)
    {
        out[i] = outputBuffer[outputSampleStart] + (0.5 * reverbBuffer[reverbBufferStartIndex]);
        outputSampleStart = (outputSampleStart + 1) % outputBuffer.size();
        reverbBufferStartIndex = (reverbBufferStartIndex + 1) % reverbBuffer.size();
    }
    
    numOutputSamplesAvailable -= numSamples;
    
    return out;
}


/*
 *  Clear everything in the input, output and OLA buffers
 *  Any indices related to these buffers are also reset
 */
void HRTFProcessor::flushBuffers()
{
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0);
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0);
    std::fill(olaBuffer.begin(), olaBuffer.end(), 0.0);
    
    inputBlockStart = 0;
    inputSampleAddIndex = 0;
    numSamplesAdded = 0;
    outputSamplesStart = 0;
    outputSamplesEnd = 0;
    numOutputSamplesAvailable = 0;
}


/*
 *  DO NOT CALL THIS FUNCTION ON MULTIPLE THREADS
 *  Apply the HRTF to an input sample of audio data
 *  If the HRTF is changed, the output will be a crossfaded mix of audio data
 *  with both HRTFs applied
 */
const float* HRTFProcessor::calculateOutput(const std::vector<float> &x)
{
    if (!hrirLoaded || x.size() == 0)
        return nullptr;
    
    if (x.size() != audioBlockSize)
        return nullptr;
    
    std::fill(olaBuffer.begin() + olaWriteIndex, olaBuffer.begin() + olaWriteIndex + hopSize, 0.0);
    
    olaWriteIndex = (olaWriteIndex + hopSize) % olaBuffer.size();
    
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
            
            std::copy(auxHRTFBuffer.begin(), auxHRTFBuffer.end(), activeHRTF.begin());
            
            crossFaded = true;
        }
    }else
        crossFaded = false;
    
    if(!overlapAndAdd())
        return nullptr;
    
    
    //  Copy outputtable audio data to the output buffer
    std::copy(olaBuffer.begin() + olaWriteIndex, olaBuffer.begin() + olaWriteIndex + hopSize, outputBuffer.begin() + outputSampleEnd);
    outputSampleEnd = (outputSampleEnd + hopSize) % outputBuffer.size();
    
    numOutputSamplesAvailable += hopSize;
    
    
    return olaBuffer.data() + olaWriteIndex;
}



//  Convert an HRIR into an HRTF and queue the new HRTF for swapping which is done in calculateOutput()
bool HRTFProcessor::setupHRTF(const double *hrir, size_t hrirSize)
{
    if (hrirSize == 0 || hrirSize > zeroPaddedBufferSize)
        return false;
    
    if (fftEngine.get() == nullptr)
        return false;
    
    
    juce::SpinLock::ScopedLockType scopedLock(hrirChangingLock);
    
    std::fill(auxHRTFBuffer.begin(), auxHRTFBuffer.end(), std::complex<float>(0.0, 0.0));
    
    std::vector<float> hrirVec(hrirSize);
    for (auto i = 0; i < hrirSize; ++i)
        hrirVec[i] = hrir[i];
    
    if (!removeImpulseDelay(hrirVec))
        return false;
    
    for (auto i = 0; i < hrirSize; ++i)
    {
        if (!hrirLoaded)
            activeHRTF.at(i) = std::complex<float>((float)hrirVec[i], 0.0);
        else
            auxHRTFBuffer.at(i) = std::complex<float>((float)hrirVec[i], 0.0);
    }
    
    if (!hrirLoaded)
        fftEngine->perform(activeHRTF.data(), activeHRTF.data(), false);
    
    else
    {
        fftEngine->perform(auxHRTFBuffer.data(), auxHRTFBuffer.data(), false);
        hrirChanged = true;
    }
    
    return true;
}


//  Peel off a copy of the OLA buffer
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
        auxBuffer.at(i) = auxBuffer.at(i) * auxHRTFBuffer.at(i);
    
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
        olaBuffer.at(offset) += xBuffer.at(i).real();
        offset = (offset + 1) % zeroPaddedBufferSize;
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


/*
 *  Some impulse responses will have a delay before the impulse is actually triggered
 *  This isn't much of a problem except when the delays between different impulses are different
 *  which can lead to unpleasant zipper noise between HRIR transitions.
 *  We'll attempt to remove the delay as much as possible to bring all HRIRs to start at a "common" point
 *
 *  The starting point is determined by finding the first point where the impulse has a value >= abs(mean) + std dev
 */
bool HRTFProcessor::removeImpulseDelay(std::vector<float> &hrir)
{
    if (hrir.size() == 0) return false;
    
    auto stats = getMeanAndStd(hrir);
    auto threshold = abs(stats.first) + stats.second;
    size_t impulseStart = 0;
    
    if (threshold < 0) return false;
    
    for (auto i = 0; i < hrir.size(); ++i)
    {
        if (abs(hrir[i]) >= threshold)
        {
            impulseStart = i;
            break;
        }
    }
    
    std::copy(hrir.begin() + impulseStart, hrir.end(), hrir.begin());
    std::fill(hrir.end() - impulseStart, hrir.end(), 0.0);
    
    return true;
}


/*
 *  Simple mean and std deviation calculation function
 *  Mean and std are returned as a std::pair where mean is the first value and std is the second
 */
std::pair<float, float> HRTFProcessor::getMeanAndStd(const std::vector<float> &x) const
{
    std::pair<float, float> stats(0, 0);
    
    if (x.size() != 0)
    {
        //  Calculate mean
        float sum = 0;
        for (auto &i : x)
            sum += i;
        
        stats.first = sum / x.size();
        
        //  Calculate std
        float stdSum = 0;
        for (auto &i : x)
            stdSum += (i - stats.first) * (i - stats.first);
        
        stdSum = sqrt(stdSum / x.size());
        stats.second = stdSum;
    }
    
    return stats;
}



#ifdef JUCE_UNIT_TESTS
void HRTFProcessorTest::runTest()
{
    HRTFProcessor processor;
    
    //  Input an impulse to the HRTFProcessor
    //  HRTF should be a constant (1)
    size_t fftSize = 512;
    std::vector<double> hrir(fftSize);
    std::fill(hrir.begin(), hrir.end(), 0.0);
    hrir[(fftSize / 2) - 1] = 1.0;
    
    float samplingFreq = 44100.0;
    size_t audioBufferSize = 256;
    
    beginTest("HRTFProcessor Initialization");
    
    bool success = processor.init(hrir.data(), hrir.size(), samplingFreq, audioBufferSize);
    expect(success);
    
    expectEquals<int>(processor.isHRIRLoaded(), 1);
    expectEquals<size_t>(processor.zeroPaddedBufferSize, 2048);
    expectEquals<float>(processor.fs, samplingFreq);
    expectEquals<size_t>(processor.audioBlockSize, 2 * audioBufferSize + 1);
    
    for (auto i = 0; i < processor.zeroPaddedBufferSize; ++i)
        expectWithinAbsoluteError<float>(std::abs(processor.activeHRTF[i]), 1.0, 0.01);
    
    //===================================================================================================//
    
    
    beginTest("HRTF Application");
    
    std::vector<float> x(audioBufferSize);
    std::fill(x.begin(), x.end(), 1.0);
    
    //  In order to meet the min number of samples needed for processing, call addSamples() 3 times
    processor.addSamples(x.data(), x.size());
    processor.addSamples(x.data(), x.size());
    processor.addSamples(x.data(), x.size());
    
    auto output = processor.getOutput(audioBufferSize);
    
    expectEquals<size_t>(output.size(), audioBufferSize);
    
//    std::cout << "HRTFProcessor Output" << std::endl;
//    std::cout << "------------------" << std::endl;
//
//    for (auto i = 0; i < audioBufferSize; ++i)
//        std::cout << output[i] << std::endl;
    
    //===================================================================================================//
    
    
    beginTest("Changing HRTF");

    size_t testSignalLength = 2048;
    float testFrequency = 500;
    
    //  Create container to hold the results of processed data when HRIR was changed
    std::vector<float> processedData(testSignalLength);
    std::fill(processedData.begin(), processedData.end(), 0.0);
    
    //  First, reset all the internal HRTFProcessor buffers to start fresh
    processor.flushBuffers();
    output = processor.getOutput(audioBufferSize);
    
    expectEquals<size_t>(output.size(), 0);

    std::vector<float> signal(testSignalLength);
    createTestSignal(samplingFreq, testFrequency, signal);

    processor.addSamples(signal.data(), audioBufferSize * 3);
    output = processor.getOutput(audioBufferSize);
    std::copy(output.begin(), output.end(), processedData.begin());
    
    //  Swap HRIR for an impulse response of all ones
    std::fill(hrir.begin(), hrir.end(), 1.0);
    expect(processor.swapHRIR(hrir.data(), hrir.size()));
    
    //  Feed in rest of test signal and get the processed data
    for (auto i = 0; i < (testSignalLength / audioBufferSize) - 3; ++i)
    {
        processor.addSamples(signal.data() + ((i + 3) * audioBufferSize), audioBufferSize);
        output = processor.getOutput(audioBufferSize);
        std::copy(output.begin(), output.end(), processedData.begin() + ((i + 1) * audioBufferSize));
    }
    

    std::cout << "Processed Data from HRTF Change" << std::endl;
    std::cout << "==================" << std::endl;

    for (auto i = 0; i < testSignalLength; ++i)
        std::cout << processedData[i] << std::endl;
    
}


bool HRTFProcessorTest::createTestSignal(float fs, float f0, std::vector<float> &dest)
{
    if (dest.size() == 0)
        return false;
    
    for (auto i = 0; i < dest.size(); ++i)
        dest.at(i) = sin((i * 2 * juce::MathConstants<float>::pi * f0) / fs);
    
    return true;
}

#endif
