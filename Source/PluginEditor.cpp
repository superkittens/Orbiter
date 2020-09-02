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
    setSize (650, 300);
    
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
    
    reverbRoomSizeSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    reverbRoomSizeSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    reverbRoomSizeSlider.setRange(0, 1);
    addAndMakeVisible(reverbRoomSizeSlider);
    
    reverbDampingSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    reverbDampingSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    reverbDampingSlider.setRange(0, 1);
    addAndMakeVisible(reverbDampingSlider);
    
    reverbWetLevelSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    reverbWetLevelSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    reverbWetLevelSlider.setRange(0, 1);
    addAndMakeVisible(reverbWetLevelSlider);
    
    reverbDryLevelSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    reverbDryLevelSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    reverbDryLevelSlider.setRange(0, 1);
    addAndMakeVisible(reverbDryLevelSlider);
    
    reverbWidthSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    reverbWidthSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 10);
    reverbWidthSlider.setRange(0, 1);
    addAndMakeVisible(reverbWidthSlider);
    
    
    sofaFileButton.setButtonText("Open SOFA");
    sofaFileButton.onClick = [this]{ openSofaButtonClicked(); };
    addAndMakeVisible(sofaFileButton);
    
    hrtfThetaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_THETA_ID, hrtfThetaSlider);
    
    hrtfPhiAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_PHI_ID, hrtfPhiSlider);
    
    hrtfRadiusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_RADIUS_ID, hrtfRadiusSlider);
    
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_INPUT_GAIN_ID, inputGainSlider);
    
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_OUTPUT_GAIN_ID, outputGainSlider);
    
    reverbRoomSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_ROOM_SIZE_ID, reverbRoomSizeSlider);
    
    reverbDampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_DAMPING_ID, reverbDampingSlider);
    
    reverbWetLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_WET_LEVEL_ID, reverbWetLevelSlider);
    
    reverbDryLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_DRY_LEVEL_ID, reverbDryLevelSlider);
    
    reverbWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_WIDTH_ID, reverbWidthSlider);
    
    
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
    g.drawFittedText("Elevation", bounds.withTrimmedLeft(315).withTrimmedTop(55).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
    //  Draw gain parameter text and borders
    //juce::Rectangle<float> gainBorder(410, 55, 100, 150);
    //g.drawRoundedRectangle(gainBorder, 10, 2);
    g.drawFittedText("Input Gain", bounds.withTrimmedLeft(418).withTrimmedTop(55).withSize(100, 10), juce::Justification::Flags::centred, 1);
    g.drawFittedText("Output Gain", bounds.withTrimmedLeft(418).withTrimmedTop(165).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
}

void OrbiterAudioProcessorEditor::resized()
{
    //hrtfThetaSlider.setBounds(bounds.withTrimmedTop(20).withTrimmedLeft(350).withSize(componentSize, componentSize));
    //hrtfRadiusSlider.setBounds(getLocalBounds().withTrimmedTop(150).withTrimmedLeft(350).withSize(componentSize, componentSize));
    hrtfPhiSlider.setBounds(getLocalBounds().withTrimmedTop(75).withTrimmedLeft(330).withSize(70, 170));
    inputGainSlider.setBounds(getLocalBounds().withTrimmedTop(70).withTrimmedLeft(430).withSize(75, 75));
    outputGainSlider.setBounds(getLocalBounds().withTrimmedTop(175).withTrimmedLeft(430).withSize(75, 75));
    sofaFileButton.setBounds(getLocalBounds().withTrimmedTop(15).withTrimmedLeft(385).withSize(80, 20));
    
    reverbRoomSizeSlider.setBounds(getLocalBounds().withTrimmedTop(70).withTrimmedLeft(530).withSize(50, 50));
    reverbDampingSlider.setBounds(getLocalBounds().withTrimmedTop(70).withTrimmedLeft(590).withSize(50, 50));
    reverbWetLevelSlider.setBounds(getLocalBounds().withTrimmedTop(130).withTrimmedLeft(530).withSize(50, 50));
    reverbDryLevelSlider.setBounds(getLocalBounds().withTrimmedTop(130).withTrimmedLeft(590).withSize(50, 50));
    reverbWidthSlider.setBounds(getLocalBounds().withTrimmedTop(190).withTrimmedLeft(555).withSize(50, 50));
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

