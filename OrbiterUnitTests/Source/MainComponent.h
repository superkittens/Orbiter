#pragma once

#include <JuceHeader.h>
#include "HRTFProcessor.h"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    
    class TestRunnerThread : public juce::Thread, private juce::Timer
    {
    public:
        TestRunnerThread (MainComponent &utd, const juce::String &ctg) : juce::Thread("Unit Tests"), owner(utd), category(ctg) {};
        
        void run() override
        {
            CustomTestRunner runner(*this);
            runner.runTestsInCategory(category);
            
            startTimer(50);
        }
        
        void logMessage(const juce::String &message)
        {
            juce::WeakReference<MainComponent> safeOwner(&owner);
            
            juce::MessageManager::callAsync([=]
                                            {
                                                if (auto *o = safeOwner.get())
                                                    o->logMessage (message);
                                            });
        }
        
        void timerCallback() override
        {
            if (!isThreadRunning())
                owner.testFinished();
        }
        
    private:
        
        class CustomTestRunner : public juce::UnitTestRunner
        {
        public:
            CustomTestRunner (TestRunnerThread &trt) : owner(trt) {}
            
            void logMessage(const juce::String &message) override
            {
                owner.logMessage(message);
            }
            
            bool shouldAbortTests() override
            {
                return owner.threadShouldExit();
            }
            
        private:
            TestRunnerThread &owner;
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomTestRunner)
        };
        
        MainComponent &owner;
        const juce::String category;
    };
    
    //==============================================================================
    // Your private member variables go here...
    void logMessage(const juce::String &message);
    void testFinished();
    void startTest(const juce::String &category);
    void stopTest();
    void start();
    
    
    juce::TextButton startTestButton {"Run Unit Tests"};
    juce::TextEditor testResultsBox;
    juce::ComboBox categoriesBox;
    
    std::unique_ptr<TestRunnerThread> currentTestThread;
    
//    const std::string sofaFilePath = "/Users/superkittens/projects/sound_prototypes/hrtf/hrtfs/BRIRs_from_a_room/A/002.sofa";
//    BasicSOFA::BasicSOFA sofa;


    JUCE_DECLARE_WEAK_REFERENCEABLE(MainComponent)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
