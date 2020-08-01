/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrbiterAudioProcessor::OrbiterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                  .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
#endif
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                  ),
Thread("HRTF Parameter Watcher"),
valueTreeState(*this, nullptr, "PARAMETERS", createParameters())
#endif
{
    //    bool success = sofa.readSOFAFile(defaultSOFAFilePath.toStdString());
    //    if (success)
    //        sofaFileLoaded = true;
    
    sofaFileLoaded = false;
    newSofaFileWaiting = false;
    newSofaFilePath = "";
    currentSOFA = nullptr;
    
    prevTheta = -1;
    prevPhi = -1;
    prevRadius = -1;
    hrtfParamChangeLoop = true;
    
    audioBlockSize = 0;
    
    startThread();
}

OrbiterAudioProcessor::~OrbiterAudioProcessor()
{
    hrtfParamChangeLoop = false;
    stopThread(4000);
}

//==============================================================================
const juce::String OrbiterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OrbiterAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool OrbiterAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool OrbiterAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double OrbiterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OrbiterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int OrbiterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrbiterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OrbiterAudioProcessor::getProgramName (int index)
{
    return {};
}

void OrbiterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OrbiterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    audioBlockSize = samplesPerBlock;
    
    //    if (sofaFileLoaded)
    //    {
    //        leftHRTFProcessor.init(sofa.getHRIR(0, 0, 0, 0), 15000, (float)sampleRate, (size_t)samplesPerBlock / 2);
    //        rightHRTFProcessor.init(sofa.getHRIR(1, 0, 0, 0), 15000, (float)sampleRate, (size_t)samplesPerBlock / 2);
    //    }
}

void OrbiterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OrbiterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    
    return true;
#endif
}
#endif

void OrbiterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    if (sofaFileLoaded)
    {
        ReferenceCountedSOFA::Ptr retainedSofa(currentSOFA);
        
        for (int channel = 0; channel < 1; ++channel)
        {
            auto *channelData = buffer.getWritePointer (channel);
            
            std::vector<float> data(buffer.getNumSamples());
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                data[i] = channelData[i];
            
            
            auto *left = retainedSofa->leftHRTFProcessor.calculateOutput(data);
            auto *right = retainedSofa->rightHRTFProcessor.calculateOutput(data);
            
            if (left != nullptr || right != nullptr)
            {
                auto *outLeft = buffer.getWritePointer(0);
                auto *outRight = buffer.getWritePointer(1);
                
                for (auto i = 0; i < buffer.getNumSamples(); ++i)
                {
                    outLeft[i] = left[i];
                    outRight[i] = right[i];
                }
            }
        }
    }
}

//==============================================================================
bool OrbiterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OrbiterAudioProcessor::createEditor()
{
    return new OrbiterAudioProcessorEditor (*this);
}

//==============================================================================
void OrbiterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void OrbiterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}


juce::AudioProcessorValueTreeState::ParameterLayout OrbiterAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(HRTF_THETA_ID, "Theta", 0, 1, 0));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(HRTF_PHI_ID, "Phi", 0, 1, 0));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(HRTF_RADIUS_ID, "Radius", 0, 1, 0));
    //parameters.push_back(std::make_unique<juce::AudioParameterBool>("ORBIT", "Enable Orbit", false));
    return {parameters.begin(), parameters.end()};
}


void OrbiterAudioProcessor::run()
{
    while (hrtfParamChangeLoop)
    {
        checkForNewSofaToLoad();
        checkForGUIParameterChanges();
        juce::Thread::wait(10);
    }
}


void OrbiterAudioProcessor::checkForGUIParameterChanges()
{
    ReferenceCountedSOFA::Ptr retainedSofa(currentSOFA);
    
    if (retainedSofa != nullptr)
    {
        auto *theta = valueTreeState.getRawParameterValue(HRTF_THETA_ID);
        auto *phi = valueTreeState.getRawParameterValue(HRTF_PHI_ID);
        auto *radius = valueTreeState.getRawParameterValue(HRTF_RADIUS_ID);
        
        float t = *theta;
        float p = *phi;
        float r = *radius;
        
        auto thetaMapped = juce::jmap<float>(t, 0, 1, (float)retainedSofa->sofa.getMinTheta(), (float)retainedSofa->sofa.getMaxTheta());
        auto phiMapped = juce::jmap<float>(p, 0, 1, (float)retainedSofa->sofa.getMinPhi(), (float)retainedSofa->sofa.getMaxPhi());
        auto radiusMapped = juce::jmap<float>(r, 0, 1, (float)retainedSofa->sofa.getMinRadius(), (float)retainedSofa->sofa.getMaxRadius());

        
        if ((thetaMapped != prevTheta) || (phiMapped != prevPhi) || (radiusMapped != prevRadius))
        {
            auto *hrirLeft = retainedSofa->sofa.getHRIR(0, (int)thetaMapped, (int)phiMapped, (int)radiusMapped);
            auto *hrirRight = retainedSofa->sofa.getHRIR(1, (int)thetaMapped, (int)phiMapped, (int)radiusMapped);
            
            if ((hrirLeft != nullptr) && (hrirRight != nullptr))
            {
                retainedSofa->leftHRTFProcessor.swapHRIR(hrirLeft, currentSOFA->hrirSize);
                retainedSofa->rightHRTFProcessor.swapHRIR(hrirRight, currentSOFA->hrirSize);
            }
            prevTheta = thetaMapped;
            prevPhi = phiMapped;
            prevRadius = radiusMapped;
            
            sofaFileLoaded = true;
        }
        
    }
}


void OrbiterAudioProcessor::checkForNewSofaToLoad()
{
    if (newSofaFileWaiting)
    {
        if (newSofaFilePath.isNotEmpty())
        {
            ReferenceCountedSOFA::Ptr newSofa = new ReferenceCountedSOFA();
            bool success = newSofa->sofa.readSOFAFile(newSofaFilePath.toStdString());
            
            if (success){
                newSofa->hrirSize = juce::jmin((size_t)newSofa->sofa.getN(), MAX_HRIR_LENGTH);
                
                bool leftHRTFSuccess = false;
                bool rightHRTFSuccess = false;
                
                auto thetaMapped = juce::jmap<float>(0, newSofa->sofa.getMinTheta(), newSofa->sofa.getMaxTheta());
                auto phiMapped = juce::jmap<float>(0, newSofa->sofa.getMinPhi(), newSofa->sofa.getMaxPhi());
                auto radiusMapped = juce::jmap<float>(0, newSofa->sofa.getMinRadius(), newSofa->sofa.getMaxRadius());
                
                leftHRTFSuccess = newSofa->leftHRTFProcessor.init(newSofa->sofa.getHRIR(0, (int)thetaMapped, (int)phiMapped, (int)radiusMapped), newSofa->hrirSize, newSofa->sofa.getFs(), audioBlockSize);
                rightHRTFSuccess = newSofa->rightHRTFProcessor.init(newSofa->sofa.getHRIR(1, (int)thetaMapped, (int)phiMapped, (int)radiusMapped), newSofa->hrirSize, newSofa->sofa.getFs(), audioBlockSize);
                
                if (leftHRTFSuccess && rightHRTFSuccess)
                    currentSOFA = newSofa;
                
                sofaInstances.add(newSofa);
            }
        }
        
        newSofaFileWaiting = false;
    }
}


void OrbiterAudioProcessor::checkSofaInstancesToFree()
{
    for (auto i = sofaInstances.size(); i >= 0; --i)
    {
        ReferenceCountedSOFA::Ptr sofaInstance(sofaInstances.getUnchecked(i));
        if (sofaInstance->getReferenceCount() == 2)
            sofaInstances.remove(i);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbiterAudioProcessor();
}
