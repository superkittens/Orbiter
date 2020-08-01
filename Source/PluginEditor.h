/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class OrbiterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OrbiterAudioProcessorEditor (OrbiterAudioProcessor&);
    ~OrbiterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void openSofaButtonClicked();
    void notifyNewSOFA(juce::String filePath);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    
    juce::Slider hrtfThetaSlider;
    juce::Slider hrtfPhiSlider;
    juce::Slider hrtfRadiusSlider;
    
    juce::TextButton sofaFileButton;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hrtfThetaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hrtfPhiAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hrtfRadiusAttachment;
    
    
    OrbiterAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbiterAudioProcessorEditor)
};
