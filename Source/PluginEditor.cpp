/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrbiterAudioProcessorEditor::OrbiterAudioProcessorEditor (OrbiterAudioProcessor& p)
    :   AudioProcessorEditor (&p),
        inputGainSlider(gainSliderSize, "Input Gain"),
        outputGainSlider(gainSliderSize, "Output Gain"),
        reverbRoomSizeSlider(reverbSliderSize, "Room Size"),
        reverbDampingSlider(reverbSliderSize, "Damping"),
        reverbWetLevelSlider(reverbSliderSize, "Wet"),
        reverbDryLevelSlider(reverbSliderSize, "Dry"),
        reverbWidthSlider(reverbSliderSize, "Width"),
        audioProcessor (p)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (900, 350);
    
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
    
    addAndMakeVisible(inputGainSlider);
    addAndMakeVisible(outputGainSlider);
    
    addAndMakeVisible(reverbRoomSizeSlider);
    addAndMakeVisible(reverbDampingSlider);
    addAndMakeVisible(reverbWetLevelSlider);
    addAndMakeVisible(reverbDryLevelSlider);
    addAndMakeVisible(reverbWidthSlider);
    
    
    sofaFileButton.setButtonText("Open SOFA");
    sofaFileButton.onClick = [this]{ openSofaButtonClicked(); };
    addAndMakeVisible(sofaFileButton);
    
    hrtfThetaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_THETA_ID, hrtfThetaSlider);
    
    hrtfPhiAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_PHI_ID, hrtfPhiSlider);
    
    hrtfRadiusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_RADIUS_ID, hrtfRadiusSlider);
    
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_INPUT_GAIN_ID, inputGainSlider.slider);
    
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_OUTPUT_GAIN_ID, outputGainSlider.slider);
    
    reverbRoomSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_ROOM_SIZE_ID, reverbRoomSizeSlider.slider);
    
    reverbDampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_DAMPING_ID, reverbDampingSlider.slider);
    
    reverbWetLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_WET_LEVEL_ID, reverbWetLevelSlider.slider);
    
    reverbDryLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_DRY_LEVEL_ID, reverbDryLevelSlider.slider);
    
    reverbWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTreeState, HRTF_REVERB_WIDTH_ID, reverbWidthSlider.slider);
    
    
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
    g.drawFittedText("Elevation", bounds.withTrimmedLeft(azimuthComp.getWidth() + azimuthXOffset).withTrimmedTop(paramCategoryTextOffset).withSize(gainSliderSize, sliderTextHeight), juce::Justification::Flags::centred, 1);
    
    g.drawFittedText("Gain Params", bounds.withTrimmedLeft(gainSliderXOffset - (gainSliderSize / 2)).withTrimmedTop(paramCategoryTextOffset).withSize(gainSliderSize, sliderTextHeight), juce::Justification::Flags::centred, 1);
    
    g.drawFittedText("Reverb Params", bounds.withTrimmedLeft(reverbSliderXOffset - 10).withTrimmedTop(paramCategoryTextOffset).withSize(100, sliderTextHeight), juce::Justification::Flags::centred, 1);
    
    
    //  Draw gain parameters enclosing box
    g.setColour(juce::Colours::grey);
    
    juce::Rectangle<float> gainEnclosingBox(gainSliderXOffset - (gainSliderEnclosingBoxWidth / 2), gainSliderEnclosingBoxYOffset, gainSliderEnclosingBoxWidth, gainSliderEnclosingBoxHeight);
    g.fillRoundedRectangle(gainEnclosingBox, 10);
    
    //  Draw reverb parameters enclosing box
    float enclosureBuffer = (reverbSliderEnclosingBoxWidth - (reverbSliderSeparation + reverbSliderSize)) / 2;
    
    juce::Rectangle<float> reverbEnclosingBox(reverbSliderXOffset - (reverbSliderSize / 2) - enclosureBuffer, reverbSliderEnclosingBoxYOffset, reverbSliderEnclosingBoxWidth, reverbSliderEnclosingBoxHeight);
    g.fillRoundedRectangle(reverbEnclosingBox, 10);
    
    
    //  Draw SOFA load status text
    juce::String sofaStatus;
    if (audioProcessor.sofaFileLoaded)
    {
        g.setColour(juce::Colours::green);
        sofaStatus = "SOFA Loaded";
    }
    else
    {
        g.setColour(juce::Colours::red);
        sofaStatus = "SOFA Not Loaded";
    }

    g.drawFittedText(sofaStatus, getLocalBounds().withTrimmedTop(sofaStatusYOffset).withTrimmedLeft(sofaStatusXOffset).withSize(sofaStatusWidth, sofaStatusHeight), juce::Justification::Flags::centred, 1);
    
    
    
}

void OrbiterAudioProcessorEditor::resized()
{
    //  Theta and Radius sliders for debugging purposes
    hrtfThetaSlider.setBounds(getLocalBounds().withTrimmedTop(20).withTrimmedLeft(800).withSize(100, 100));
    hrtfRadiusSlider.setBounds(getLocalBounds().withTrimmedTop(150).withTrimmedLeft(800).withSize(100, 100));
    
    azimuthComp.setCentrePosition((azimuthComp.getWidth() / 2) + azimuthXOffset, getHeight() / 2);
    
    inputGainSlider.setCentrePosition(gainSliderXOffset, gainSliderYOffset);
    outputGainSlider.setCentrePosition(gainSliderXOffset, gainSliderYOffset + gainSliderEnclosingBoxHeight - gainSliderSize - 10);
    
    hrtfPhiSlider.setBounds(getLocalBounds().withTrimmedTop(elevationSliderYOffset).withTrimmedLeft(elevationSliderXOffset).withSize(50, 200));

    sofaFileButton.setBounds(getLocalBounds().withTrimmedTop(sofaButtonYOffset).withTrimmedLeft(sofaButtonXOffset).withSize(sofaButtonWidth, sofaButtonHeight));
    
    reverbRoomSizeSlider.setCentrePosition(reverbSliderXOffset, reverbSliderYOffset);
    reverbDampingSlider.setCentrePosition(reverbSliderXOffset + reverbSliderSeparation, reverbSliderYOffset);
    reverbWetLevelSlider.setCentrePosition(reverbSliderXOffset, reverbSliderYOffset + reverbSliderSeparation);
    reverbDryLevelSlider.setCentrePosition(reverbSliderXOffset + reverbSliderSeparation, reverbSliderYOffset + reverbSliderSeparation);
    reverbWidthSlider.setCentrePosition(reverbSliderXOffset + (reverbSliderSeparation / 2), reverbSliderYOffset + (2 * reverbSliderSeparation));
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
    if (audioProcessor.sofaFileLoaded)
        repaint();
    
    auto sourceAngleAndRadius = azimuthComp.getNormalisedAngleAndRadius();
    auto paramAngle = audioProcessor.valueTreeState.getRawParameterValue(HRTF_THETA_ID);
    auto paramRadius = audioProcessor.valueTreeState.getRawParameterValue(HRTF_RADIUS_ID);
    
    auto paramAngleValue = floorValue(*paramAngle, 0.00001);
    auto paramRadiusValue = floorValue(*paramRadius, 0.00001);
    auto uiAngleValue = floorValue(sourceAngleAndRadius.first, 0.00001);
    auto uiRadiusValue = floorValue(sourceAngleAndRadius.second, 0.00001);
    
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

