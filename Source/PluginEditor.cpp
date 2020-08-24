/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrbiterAudioProcessorEditor::OrbiterAudioProcessorEditor (OrbiterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (550, 300);
    
    hrtfThetaSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    hrtfThetaSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
    hrtfThetaSlider.setRange(0, 1);
    addChildComponent(hrtfThetaSlider);
    
    hrtfRadiusSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    hrtfRadiusSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
    hrtfRadiusSlider.setRange(0, 1);
    addChildComponent(hrtfRadiusSlider);
    
    hrtfPhiSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    hrtfPhiSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    hrtfPhiSlider.setRange(0, 1);
    addAndMakeVisible(hrtfPhiSlider);
    
    inputGainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    inputGainSlider.setRange(0, 1);
    addAndMakeVisible(inputGainSlider);
    
    outputGainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    outputGainSlider.setRange(0, 1);
    addAndMakeVisible(outputGainSlider);
    
    sofaFileButton.setButtonText("Open SOFA");
    sofaFileButton.onClick = [this]{ openSofaButtonClicked(); };
    addAndMakeVisible(sofaFileButton);
    
    hrtfThetaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_THETA_ID, hrtfThetaSlider);
    
    hrtfPhiAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_PHI_ID, hrtfPhiSlider);
    
    hrtfRadiusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_RADIUS_ID, hrtfRadiusSlider);
    
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_INPUT_GAIN_ID, inputGainSlider);
    
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_OUTPUT_GAIN_ID, outputGainSlider);
    
    addAndMakeVisible(azimuthComp);
    
    prevAzimuthAngle = -1;
    prevAzimuthRadius = -1;
    prevParamAngle = -1;
    prevParamRadius = -1;
    
    startTimerHz(30);
}

OrbiterAudioProcessorEditor::~OrbiterAudioProcessorEditor()
{
}

//==============================================================================
void OrbiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    
    
    auto bounds = getLocalBounds();
    g.drawFittedText("Elevation", bounds.withTrimmedLeft(315).withTrimmedTop(65).withSize(100, 10), juce::Justification::Flags::centred, 1);
    g.drawFittedText("Input Gain", bounds.withTrimmedLeft(418).withTrimmedTop(65).withSize(100, 10), juce::Justification::Flags::centred, 1);
    g.drawFittedText("Output Gain", bounds.withTrimmedLeft(418).withTrimmedTop(165).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
//    bounds = getLocalBounds();
//    g.drawFittedText("Radius",bounds.withTrimmedLeft(200).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
//    g.drawFittedText("Theta", bounds.removeFromLeft(100).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
}

void OrbiterAudioProcessorEditor::resized()
{
    //hrtfThetaSlider.setBounds(bounds.withTrimmedTop(20).withTrimmedLeft(350).withSize(componentSize, componentSize));
    //hrtfRadiusSlider.setBounds(getLocalBounds().withTrimmedTop(150).withTrimmedLeft(350).withSize(componentSize, componentSize));
    hrtfPhiSlider.setBounds(getLocalBounds().withTrimmedTop(75).withTrimmedLeft(330).withSize(70, 170));
    inputGainSlider.setBounds(getLocalBounds().withTrimmedTop(70).withTrimmedLeft(430).withSize(75, 75));
    outputGainSlider.setBounds(getLocalBounds().withTrimmedTop(175).withTrimmedLeft(430).withSize(75, 75));
    sofaFileButton.setBounds(getLocalBounds().withTrimmedTop(20).withTrimmedLeft(385).withSize(80, 20));
}


void OrbiterAudioProcessorEditor::openSofaButtonClicked()
{
    juce::FileChooser fileChooser("Select SOFA File", {}, "*.sofa");
    
    if (fileChooser.browseForFileToOpen())
    {
        auto file = fileChooser.getResult();
        auto path = file.getFullPathName();
        
        notifyNewSOFA(path);
    }
}


void OrbiterAudioProcessorEditor::notifyNewSOFA(juce::String filePath)
{
    audioProcessor.newSofaFilePath.swapWith(filePath);
    audioProcessor.newSofaFileWaiting = true;
}


void OrbiterAudioProcessorEditor::timerCallback()
{
    auto sourceAngleAndRadius = azimuthComp.getNormalisedAngleAndRadius();
    auto paramAngle = audioProcessor.valueTreeState.getRawParameterValue(HRTF_THETA_ID);
    auto paramRadius = audioProcessor.valueTreeState.getRawParameterValue(HRTF_RADIUS_ID);
    
    auto paramAngleValue = floorValue(*paramAngle, 0.0001);
    auto paramRadiusValue = floorValue(*paramRadius, 0.0001);
    auto uiAngleValue = floorValue(sourceAngleAndRadius.first, 0.0001);
    auto uiRadiusValue = floorValue(sourceAngleAndRadius.second, 0.0001);
    
    if (paramAngleValue != prevParamAngle || paramRadiusValue != prevParamRadius)
    {
        azimuthComp.updateSourcePosition(paramAngleValue, paramRadiusValue);
        prevAzimuthRadius = paramRadiusValue;
        prevAzimuthAngle = paramAngleValue;
        prevParamRadius = paramRadiusValue;
        prevParamAngle = paramAngleValue;
    }
    else if (uiAngleValue != prevAzimuthAngle || uiRadiusValue != prevAzimuthRadius)
    {

        audioProcessor.valueTreeState.getParameter(HRTF_THETA_ID)->setValueNotifyingHost(uiAngleValue);
        audioProcessor.valueTreeState.getParameter(HRTF_RADIUS_ID)->setValueNotifyingHost(uiRadiusValue);

        prevAzimuthRadius = uiRadiusValue;
        prevAzimuthAngle = uiAngleValue;
        prevParamRadius = uiRadiusValue;
        prevParamAngle = uiAngleValue;
    }
    else{}

}


float OrbiterAudioProcessorEditor::floorValue(float value, float epsilon)
{
    int valueTruncated = value / epsilon;
    return (float)valueTruncated * epsilon;
}

