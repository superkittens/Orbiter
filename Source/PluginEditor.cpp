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
    
    hrtfPhiSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    hrtfPhiSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
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
    g.drawFittedText("Theta", bounds.removeFromLeft(100).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
    bounds = getLocalBounds();
    g.drawFittedText("Phi", bounds.withTrimmedLeft(100).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
    
    bounds = getLocalBounds();
    g.drawFittedText("Radius",bounds.withTrimmedLeft(200).withTrimmedTop(10).withSize(100, 10), juce::Justification::Flags::centred, 1);
}

void OrbiterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int componentSize = 100;

    hrtfThetaSlider.setBounds(bounds.withTrimmedTop(20).withSize(componentSize, componentSize));
    hrtfPhiSlider.setBounds(getLocalBounds().withTrimmedTop(20).withTrimmedLeft(100).withSize(componentSize, componentSize));
    hrtfRadiusSlider.setBounds(getLocalBounds().withTrimmedTop(20).withTrimmedLeft(200).withSize(componentSize, componentSize));

    sofaFileButton.setBounds(getLocalBounds().withTrimmedTop(50).removeFromRight(85).withSize(70, 20));

    auto radParam = audioProcessor.valueTreeState.getParameter(HRTF_RADIUS_ID);
    radParam->setValue(0.5);
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
    auto paramAngle = audioProcessor.valueTreeState.getParameter(HRTF_THETA_ID)->getValue();
    auto paramRadius = audioProcessor.valueTreeState.getParameter(HRTF_RADIUS_ID)->getValue();
    
    if (paramAngle != prevParamAngle || paramRadius != prevParamRadius)
    {
        azimuthComp.updateSourcePosition(paramAngle, paramRadius);
        
        prevAzimuthRadius = paramRadius;
        prevAzimuthAngle = paramAngle;
        prevParamRadius = paramRadius;
        prevParamAngle = paramAngle;
    }
    else if (sourceAngleAndRadius.first != prevAzimuthAngle || sourceAngleAndRadius.second != prevAzimuthRadius)
    {
        audioProcessor.valueTreeState.getParameter(HRTF_THETA_ID)->setValue(sourceAngleAndRadius.first);
        audioProcessor.valueTreeState.getParameter(HRTF_RADIUS_ID)->setValue(sourceAngleAndRadius.second);
        
        prevAzimuthRadius = sourceAngleAndRadius.second;
        prevAzimuthAngle = sourceAngleAndRadius.first;
        prevParamRadius = sourceAngleAndRadius.second;
        prevParamAngle = sourceAngleAndRadius.first;
    }
    else{}
    
}

