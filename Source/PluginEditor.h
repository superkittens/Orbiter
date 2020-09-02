/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AzimuthUIComponent.h"

//==============================================================================
/**
*/
class OrbiterAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    
    OrbiterAudioProcessorEditor (OrbiterAudioProcessor&);
    ~OrbiterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void openSofaButtonClicked();
    void notifyNewSOFA(juce::String filePath);
    
    
    AzimuthUIComponent azimuthComp;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    void timerCallback() override;
    float floorValue(float value, float epsilon);
    
    juce::Slider hrtfThetaSlider;
    juce::Slider hrtfPhiSlider;
    juce::Slider hrtfRadiusSlider;
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbWetLevelSlider;
    juce::Slider reverbDryLevelSlider;
    juce::Slider reverbWidthSlider;
    
    juce::TextButton sofaFileButton;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hrtfThetaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hrtfPhiAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hrtfRadiusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbRoomSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWetLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDryLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
    
    float prevAzimuthAngle;
    float prevAzimuthRadius;
    float prevParamAngle;
    float prevParamRadius;

    
    OrbiterAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbiterAudioProcessorEditor)
};
