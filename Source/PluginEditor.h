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
    class OrbiterSliderComponent : public juce::Component
    {
    public:
        
        OrbiterSliderComponent(){};
        
        OrbiterSliderComponent(float width, juce::String sliderName)
        {
            name = sliderName;
            
            setSize(width, width);
            slider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
            slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, width, nameHeight);
            slider.setRange(0, 1);
            
            addAndMakeVisible(slider);
        }
        
        void paint(juce::Graphics &g) override
        {
            g.setFont(nameHeight);
            g.setColour(juce::Colours::white);
            g.drawFittedText(name, getLocalBounds().withTrimmedTop(getHeight() - nameHeight).withSize(getWidth(), nameHeight), juce::Justification::Flags::centred, 1);
        }
        
        void resized() override
        {
            slider.setBounds(getLocalBounds().withTrimmedBottom(nameHeight).withSize(getWidth(), getHeight() - nameHeight));
        }
        
        
        juce::Slider slider;
        juce::String name;
        
        static constexpr float nameHeight = 15;
    };
    
    void timerCallback() override;
    float floorValue(float value, float epsilon);
    
    juce::Slider hrtfThetaSlider;
    juce::Slider hrtfPhiSlider;
    juce::Slider hrtfRadiusSlider;
    OrbiterSliderComponent inputGainSlider;
    OrbiterSliderComponent outputGainSlider;
    OrbiterSliderComponent reverbRoomSizeSlider;
    OrbiterSliderComponent reverbDampingSlider;
    OrbiterSliderComponent reverbWetLevelSlider;
    OrbiterSliderComponent reverbDryLevelSlider;
    OrbiterSliderComponent reverbWidthSlider;
    
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
    
    
    //  UI Element Offsets
    float sliderTextHeight = 10;
    float paramCategoryTextOffset = 35;
    
    float azimuthXOffset = 15;
    
    //  Elevation Slider Characteristics
    float elevationSliderXOffset = 330;
    float elevationSliderYOffset = 75;
    
    //  Gain Slider Characteristics
    static constexpr float gainSliderSize = 80;
    
    float gainSliderXOffset = 470;
    float gainSliderYOffset = 125;
    
    float gainSliderEnclosingBoxWidth = 100;
    float gainSliderEnclosingBoxHeight = 185;
    float gainSliderEnclosingBoxYOffset = 80;
    
    //  Reverb Slider Characteristics
    static constexpr float reverbSliderSize = 75;
    
    float reverbSliderXOffset = 595;
    float reverbSliderYOffset = 120;
    float reverbSliderSeparation = reverbSliderSize + 5;
    
    float reverbSliderEnclosingBoxWidth = 180;
    float reverbSliderEnclosingBoxHeight = 240;
    float reverbSliderEnclosingBoxYOffset = 80;
    
    //  Sofa Button Characteristics
    float sofaButtonXOffset = 765;
    float sofaButtonYOffset = 80;
    float sofaButtonWidth = 100;
    float sofaButtonHeight = 80;
    float sofaStatusXOffset = 765;
    float sofaStatusYOffset = 150;
    float sofaStatusWidth = 100;
    float sofaStatusHeight = 80;

    
    OrbiterAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbiterAudioProcessorEditor)
};
