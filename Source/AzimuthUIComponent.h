#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class AzimuthUIComponent  : public juce::Component, public juce::ComponentListener
{
public:
    
    class SourceUIComponent : public juce::Component
    {
    public:
        SourceUIComponent()
        {
            setSize(30, 30);
            sourceColour = juce::Colours::orange;
        }
        
        void paint(juce::Graphics &g) override
        {
            juce::Rectangle<float> sourceArea(30, 30);
            g.setColour (sourceColour);
            g.fillEllipse(sourceArea);
        }
        
        void resized() override
        {
            constrainer.setMinimumOnscreenAmounts (getHeight(), getWidth(),
                                                   getHeight(), getWidth());
        }
        
        void mouseDown(const juce::MouseEvent &e) override
        {
            dragger.startDraggingComponent(this, e);
        }
        
        void mouseDrag(const juce::MouseEvent &e) override
        {
            dragger.dragComponent(this, e, &constrainer);
        }
        
        void mouseEnter(const juce::MouseEvent &e) override
        {
            sourceColour = juce::Colours::lightgreen;
            repaint();
        }
        
        void mouseExit(const juce::MouseEvent &e) override
        {
            sourceColour = juce::Colours::orange;
            repaint();
        }
        
    private:
        juce::ComponentBoundsConstrainer constrainer;
        juce::ComponentDragger dragger;
        
        juce::Colour sourceColour;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceUIComponent);
    };
    
    
    AzimuthUIComponent();
    ~AzimuthUIComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    std::pair<float, float> getNormalisedAngleAndRadius();
    void updateSourcePosition(float normalisedAngle, float normalisedRadius);
    
    
    SourceUIComponent sourceComponent;

private:
    
    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;
    void calculateSourceAngleAndRadius();
    
    
    std::pair<float, float> normalisedSourceAngleAndRadius;
    float circleScaleValue;
    float circleLineThickness;
    float maxRadius;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AzimuthUIComponent)
};
