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
    bool success = sofa.readSOFAFile(defaultSOFAFilePath.toStdString());
    if (success)
        sofaFileLoaded = true;
    
    prevTheta = 0;
    hrtfParamChangeLoop = true;
    
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
    if (sofaFileLoaded)
    {
        leftHRTFProcessor.init(sofa.getHRIR(0, 90, 0, 0), 15000, (float)sampleRate, (size_t)samplesPerBlock / 2);
        rightHRTFProcessor.init(sofa.getHRIR(1, 90, 0, 0), 15000, (float)sampleRate, (size_t)samplesPerBlock / 2);
    }
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
    
    for (int channel = 0; channel < 1; ++channel)
    {
        auto *channelData = buffer.getWritePointer (channel);

        std::vector<float> data(buffer.getNumSamples());
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = channelData[i];
        }
        
        
        auto *left = leftHRTFProcessor.calculateOutput(data);
        auto *right = rightHRTFProcessor.calculateOutput(data);
        
        if (left != nullptr || right != nullptr)
        {
            auto *outLeft = buffer.getWritePointer(0);
            auto *outRight = buffer.getWritePointer(1);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                outLeft[i] = left[i];
                outRight[i] = right[i];
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
    
    parameters.push_back(std::make_unique<juce::AudioParameterInt>(HRTF_THETA_ID, "Theta", 0, 355, 0));
    //parameters.push_back(std::make_unique<juce::AudioParameterBool>("ORBIT", "Enable Orbit", false));
    return {parameters.begin(), parameters.end()};
}


void OrbiterAudioProcessor::run()
{
    while (hrtfParamChangeLoop)
    {
        auto *theta = valueTreeState.getRawParameterValue(HRTF_THETA_ID);

        if (*theta != prevTheta)
        {
            auto *hrirLeft = sofa.getHRIR(0, *theta, 0, 0);
            auto *hrirRight = sofa.getHRIR(1, *theta, 0, 0);
            
            if ((hrirLeft != nullptr) && (hrirRight != nullptr))
            {
                leftHRTFProcessor.swapHRIR(hrirLeft, 15000);
                rightHRTFProcessor.swapHRIR(hrirRight, 15000);
            }
            prevTheta = *theta;
        }

        juce::Thread::wait(10);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbiterAudioProcessor();
}
