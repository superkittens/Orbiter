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
    setSize (500, 300);
    
    hrtfThetaSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    hrtfThetaSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
    hrtfThetaSlider.setRange(0, 1);
    addChildComponent(hrtfThetaSlider);
    
    hrtfPhiSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    //hrtfPhiSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
    hrtfPhiSlider.setRange(0, 1);
    addAndMakeVisible(hrtfPhiSlider);
    
    hrtfRadiusSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    hrtfRadiusSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
    hrtfRadiusSlider.setRange(0, 1);
    addChildComponent(hrtfRadiusSlider);
    
    sofaFileButton.setButtonText("Open SOFA");
    sofaFileButton.onClick = [this]{ openSofaButtonClicked(); };
    addAndMakeVisible(sofaFileButton);
    
    hrtfThetaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_THETA_ID, hrtfThetaSlider);
    
    hrtfPhiAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_PHI_ID, hrtfPhiSlider);
    
    hrtfRadiusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_RADIUS_ID, hrtfRadiusSlider);
    
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
//    g.drawFittedText("Theta", bounds.removeFromLeft(100).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
    bounds = getLocalBounds();
    g.drawFittedText("Elevation", bounds.withTrimmedLeft(295).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
//    bounds = getLocalBounds();
//    g.drawFittedText("Radius",bounds.withTrimmedLeft(200).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
}

void OrbiterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int componentSize = 100;

    //hrtfThetaSlider.setBounds(bounds.withTrimmedTop(20).withTrimmedLeft(350).withSize(componentSize, componentSize));
    hrtfPhiSlider.setBounds(getLocalBounds().withTrimmedTop(75).withTrimmedLeft(330).withSize(30, 150));
    //hrtfRadiusSlider.setBounds(getLocalBounds().withTrimmedTop(150).withTrimmedLeft(350).withSize(componentSize, componentSize));

    sofaFileButton.setBounds(getLocalBounds().withTrimmedTop(50).removeFromRight(85).withSize(70, 20));
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
        //std::cout << "Slider Angle: " << paramAngleValue << " Slider Rad: " << paramRadiusValue << "\n";
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

