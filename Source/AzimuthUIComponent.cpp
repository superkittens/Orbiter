#include <JuceHeader.h>
#include "AzimuthUIComponent.h"

//==============================================================================
AzimuthUIComponent::AzimuthUIComponent()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    setSize(300, 300);
    addAndMakeVisible(sourceComponent);
    
    sourceComponent.addComponentListener(this);
}

AzimuthUIComponent::~AzimuthUIComponent()
{
}

void AzimuthUIComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background
    
    g.setColour(juce::Colours::white);
    
    //  Draw Concentric Circles
    juce::Rectangle<float> circleArea(getWidth(), getHeight());
    circleArea.setCentre(getWidth() / 2, getHeight() / 2);
    g.drawEllipse(circleArea, 5);
    
//    circleArea = juce::Rectangle<float>(2 * getWidth() / 3, 2 * getHeight() / 3);
//    circleArea.setCentre(getWidth() / 2, getHeight() / 2);
//    g.drawEllipse(circleArea, 2);
//
//    circleArea = juce::Rectangle<float>(getWidth() / 3, getHeight() / 3);
//    circleArea.setCentre(getWidth() / 2, getHeight() / 2);
//    g.drawEllipse(circleArea, 2);
    
    circleArea = juce::Rectangle<float>(30, 30);
    circleArea.setCentre(getWidth() / 2, getHeight() / 2);
    g.fillEllipse(circleArea);
    
    //  Draw Axes
    g.drawLine(getWidth() / 2, 0, getWidth() / 2, getHeight());
    g.drawLine(0, getHeight() / 2, getWidth(), getHeight() / 2);
}

void AzimuthUIComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    sourceComponent.setCentrePosition(getWidth() / 2, getHeight() / 3);
}


/*
 *  Calculate the angle and readius of the source relative to the listener position
 */
void AzimuthUIComponent::calculateSourceAngleAndRadius()
{
    auto sourcePos = sourceComponent.getBounds().getCentre().toFloat();
    
    auto sourceXPosRelative = sourcePos.getX() - (getWidth() / 2);
    auto sourceYPosRelative = sourcePos.getY() - (getHeight() / 2);
    
    auto radius = sqrt((sourceXPosRelative * sourceXPosRelative) + (sourceYPosRelative * sourceYPosRelative));
    auto angle = (atan2(sourceXPosRelative, -sourceYPosRelative) * 180 / juce::MathConstants<float>::pi);
    
    auto radiusNormalised = juce::jmap<float>(radius, 0, getWidth() / 2, 0, 1);
    auto angleNormalised = juce::jmap<float>(angle, -179, 180, 0, 1);
    
    normalisedSourceAngleAndRadius.first = angleNormalised;
    normalisedSourceAngleAndRadius.second = radiusNormalised;
}


void AzimuthUIComponent::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized)
{
    calculateSourceAngleAndRadius();
}


void AzimuthUIComponent::updateSourcePosition(float normalisedAngle, float normalisedRadius)
{
    auto angle = juce::jmap<float>(normalisedAngle, 0, 1, -179, 180);
    auto radius = juce::jmap<float>(normalisedRadius, 0, 1, 0, getWidth() / 2);
    
    angle = angle * juce::MathConstants<float>::pi / 180.f;
    
    //  Transform the vector to get it in terms of the window frame of reference
    int xPos = (getWidth() / 2) - (radius * cos(angle));
    int yPos = (getHeight() / 2) + (radius * sin(angle));
    
    sourceComponent.setCentrePosition(yPos, xPos);
    repaint();
}


std::pair<float, float> AzimuthUIComponent::getNormalisedAngleAndRadius()
{
    return normalisedSourceAngleAndRadius;
}
