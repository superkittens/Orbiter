/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <BasicSOFA.hpp>
#include "HRTFProcessor.h"

#define HRTF_THETA_ID "HRTF_THETA"
#define HRTF_PHI_ID "HRTF_PHI"
#define HRTF_RADIUS_ID "HRTF_RADIU"

//==============================================================================
/**
*/
class OrbiterAudioProcessor  : public juce::AudioProcessor, public juce::Thread
{
public:
    //==============================================================================
    OrbiterAudioProcessor();
    ~OrbiterAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    void run() override;
    
    BasicSOFA::BasicSOFA sofa;
    bool sofaFileLoaded;
    const juce::String defaultSOFAFilePath = "/Users/superkittens/projects/sound_prototypes/hrtf/hrtfs/BRIRs_from_a_room/A/002.sofa";
    
    HRTFProcessor leftHRTFProcessor;
    HRTFProcessor rightHRTFProcessor;
    
    juce::AudioProcessorValueTreeState valueTreeState;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

private:
    //==============================================================================
    
    float prevTheta;
    bool hrtfParamChangeLoop;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbiterAudioProcessor)
};
