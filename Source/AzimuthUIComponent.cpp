#include <JuceHeader.h>
#include "AzimuthUIComponent.h"

//==============================================================================
AzimuthUIComponent::AzimuthUIComponent()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    setSize(300, 300);
    addAndMakeVisible(sourceComponent);
    
    circleScaleValue = 0.75;
    circleLineThickness = 5;
    
    maxRadius = (getWidth() / 2) * 0.75;
    
    sourceComponent.addComponentListener(this);
}

AzimuthUIComponent::~AzimuthUIComponent()
{
}

void AzimuthUIComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background
    
    //  Draw Axes
    float lineThickness = 2;
    
    g.setColour(juce::Colours::white);
    g.drawLine(getWidth() / 2, 0, getWidth() / 2, getHeight(), lineThickness);
    g.drawLine(0, getHeight() / 2, getWidth(), getHeight() / 2, lineThickness);
    
    g.setColour(juce::Colours::white);
    
    //  Draw Angle Indicators
    float angleTextWidth = 50;
    float angleTextHeight = 10;
    
    g.drawFittedText("0", getWidth() / 2, 0, angleTextWidth, angleTextHeight, juce::Justification::Flags::left, 1);
    g.drawFittedText("90", 0, getHeight() / 2, angleTextWidth, angleTextHeight, juce::Justification::Flags::left, 1);
    g.drawFittedText("180", getWidth() / 2, getHeight() - angleTextHeight, angleTextWidth, angleTextHeight, juce::Justification::Flags::left, 1);
    g.drawFittedText("-90", getWidth() - (angleTextWidth / 2), getHeight() / 2, angleTextWidth, angleTextHeight, juce::Justification::Flags::left, 1);
    
    
    //  Draw "sound field"
    juce::Rectangle<float> circleArea(getWidth() * circleScaleValue, getHeight() * circleScaleValue);
    circleArea.setCentre(getWidth() / 2, getHeight() / 2);
    
    g.setColour(juce::Colours::grey);
    g.fillEllipse(circleArea);
    
    g.setColour(juce::Colours::white);
    g.drawEllipse(circleArea, circleLineThickness);
    
    circleArea = juce::Rectangle<float>(30, 30);
    circleArea.setCentre(getWidth() / 2, getHeight() / 2);
    g.fillEllipse(circleArea);
}

void AzimuthUIComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    sourceComponent.setCentrePosition(getWidth() / 2, 100);
}


/*
 *  Calculate the angle and readius of the source relative to the listener position
 */
std::pair<float, float> AzimuthUIComponent::calculateSourceAngleAndRadius()
{
    auto sourcePos = sourceComponent.getBounds().getCentre().toFloat();

    auto sourceXPosRelative = sourcePos.getX() - (getWidth() / 2);
    auto sourceYPosRelative = sourcePos.getY() - (getHeight() / 2);

    auto radius = sqrt((sourceXPosRelative * sourceXPosRelative) + (sourceYPosRelative * sourceYPosRelative));
    auto angle = (atan2(-sourceXPosRelative, -sourceYPosRelative) * 180 / juce::MathConstants<float>::pi);

    auto radiusNormalised = juce::jmap<float>(radius, 0, getWidth() / 2, 0, 1);
    auto angleNormalised = juce::jmap<float>(angle, -179, 180, 0, 1);

    normalisedSourceAngleAndRadius.first = angleNormalised;
    normalisedSourceAngleAndRadius.second = radiusNormalised;

    return std::pair<float, float>(angle, radius);
}


void AzimuthUIComponent::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized)
{
    std::pair<float, float> angleAndRadius = calculateSourceAngleAndRadius();
    
    if (angleAndRadius.second > maxRadius)
        angleAndRadius.second = maxRadius;
    
    auto radiusNormalised = juce::jmap<float>(angleAndRadius.second, 0, getWidth() / 2, 0, 1);
    auto angleNormalised = juce::jmap<float>(angleAndRadius.first, -179, 180, 0, 1);

    normalisedSourceAngleAndRadius.first = angleNormalised;
    normalisedSourceAngleAndRadius.second = radiusNormalised;
}


void AzimuthUIComponent::updateSourcePosition(float normalisedAngle, float normalisedRadius)
{
    auto angle = juce::jmap<float>(normalisedAngle, 0, 1, -179, 180);
    auto radius = juce::jmap<float>(normalisedRadius, 0, 1, 0, getWidth() / 2);
    
    angle = angle * juce::MathConstants<float>::pi / 180.f;
    
    //  Transform the vector to get it in terms of the window frame of reference
    int xPos = (getWidth() / 2) - (radius * sin(angle));
    int yPos = (getHeight() / 2) - (radius * cos(angle));
    
    sourceComponent.setCentrePosition(yPos, xPos);
    repaint();
}


std::pair<float, float> AzimuthUIComponent::getNormalisedAngleAndRadius()
{
    return normalisedSourceAngleAndRadius;
}
