#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    addAndMakeVisible(startTestButton);
    startTestButton.onClick = [this]{ start (); };
    
    addAndMakeVisible(testResultsBox);
    testResultsBox.setMultiLine(true);
    testResultsBox.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    
    addAndMakeVisible(categoriesBox);
    categoriesBox.addItem("All Tests", 1);
    
    auto categories = juce::UnitTest::getAllCategories();
    categories.sort(true);
    
    categoriesBox.addItemList(categories, 2);
    categoriesBox.setSelectedId(1);
    
//    bool success = sofa.readSOFAFile(sofaFilePath);
//    if (!success)
//        DBG("Failed to read SOFA file");

    
    setSize (500, 500);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(6);
    auto topSlice = bounds.removeFromTop(25);
    
    startTestButton.setBounds(topSlice.removeFromLeft(200));
    topSlice.removeFromLeft(10);
    
    categoriesBox.setBounds(topSlice.removeFromLeft(250));
    
    bounds.removeFromTop(5);
    testResultsBox.setBounds(bounds);
}

void MainComponent::start()
{
    startTest(categoriesBox.getText());
}

void MainComponent::startTest(const juce::String &category)
{
    testResultsBox.clear();
    startTestButton.setEnabled(false);
    
    currentTestThread.reset(new TestRunnerThread (*this, category));
    currentTestThread->startThread();
}

void MainComponent::stopTest()
{
    if (currentTestThread.get() != nullptr)
    {
        currentTestThread->stopThread(15000);
        currentTestThread.reset();
    }
}

void MainComponent::logMessage(const juce::String &message)
{
    testResultsBox.moveCaretToEnd();
    testResultsBox.insertTextAtCaret(message + juce::newLine);
    testResultsBox.moveCaretToEnd();
}

void MainComponent::testFinished()
{
    stopTest();
    startTestButton.setEnabled(true);
    logMessage("*** Tests Finished ***");
}
