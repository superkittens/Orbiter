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
#define HRTF_RADIUS_ID "HRTF_RADIUS"
#define HRTF_INPUT_GAIN_ID "HRTF_INPUT_GAIN"
#define HRTF_OUTPUT_GAIN_ID "HRTF_OUTPUT_GAIN"
#define HRTF_REVERB_ROOM_SIZE_ID "HRTF_REVERB_ROOM_SIZE"
#define HRTF_REVERB_DAMPING_ID "HRTF_REVERB_DAMPING"
#define HRTF_REVERB_WET_LEVEL_ID "HRTF_REVERB_WET_LEVEL"
#define HRTF_REVERB_DRY_LEVEL_ID "HRTF_REVERB_DRY_LEVEL"
#define HRTF_REVERB_WIDTH_ID "HRTF_REVERB_WIDTH"


//  Define parameter IDs here
enum
{
    HRTF_THETA = 0,
    HRTF_PHI,
    HRTF_RADIUS,
    HRTF_INPUT_GAIN,
    HRTF_OUTPUT_GAIN,
    HRTF_REVERB_ROOM_SIZE,
    HRTF_REVERB_DAMPING,
    HRTF_REVERB_WET_LEVEL,
    HRTF_REVERB_DRY_LEVEL,
    HRTF_REVERB_WIDTH
};


//==============================================================================
/**
*/
class OrbiterAudioProcessor  : public juce::AudioProcessor, public juce::Thread, public juce::AudioProcessorValueTreeState::Listener
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
    
    bool newSofaFileWaiting;
    bool sofaFileLoaded;
    juce::String newSofaFilePath;
    
    juce::AudioProcessorValueTreeState valueTreeState;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    

private:
    //==============================================================================
    
    class ReferenceCountedSOFA : public juce::ReferenceCountedObject
    {
    public:
        typedef juce::ReferenceCountedObjectPtr<ReferenceCountedSOFA> Ptr;
        
        ReferenceCountedSOFA(){}
        BasicSOFA::BasicSOFA *getSOFA() { return &sofa; }
        
        BasicSOFA::BasicSOFA sofa;
        HRTFProcessor leftHRTFProcessor;
        HRTFProcessor rightHRTFProcessor;
        
        size_t hrirSize;
        
    private:
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceCountedSOFA)
    };
    
    
    //==============================================================================
    
    void checkSofaInstancesToFree();
    void checkForNewSofaToLoad();
    void checkForGUIParameterChanges();
    void checkForHRTFReverbParamChanges();
    
    float mapAndQuantize(float value, float inputMin, float inputMax, float outputMin, float outputMax, float outputDelta);
    
    void parameterChanged(const juce::String &parameterID, float newValue) override;
    
    float prevTheta;
    float prevPhi;
    float prevRadius;
    bool hrtfParamChangeLoop;
    
    int audioBlockSize;
    
    float prevInputGain;
    float prevOutputGain;
    
    static constexpr size_t MAX_HRIR_LENGTH = 15000;
    
    juce::Reverb::Parameters reverbParams;
    std::atomic<bool> reverbParamsChanged;

    ReferenceCountedSOFA::Ptr currentSOFA;
    juce::ReferenceCountedArray<ReferenceCountedSOFA> sofaInstances;
    
    //  Debug variables
    size_t blockNum = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbiterAudioProcessor)
};
