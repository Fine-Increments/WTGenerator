/*
  ==============================================================================

    PluginEditor.cpp
    WTGenerator v1 editor. See PluginEditor.h for the layout intent.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "Colors.h"

//==============================================================================
WTGeneratorAudioProcessorEditor::WTGeneratorAudioProcessorEditor (WTGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), paramPanel (p)
{
    setLookAndFeel (&lookAndFeel);

    loadButton.onClick = [this] { chooseExpressionFile(); };
    addAndMakeVisible (loadButton);

    fileLabel.setColour (juce::Label::textColourId, WTColors::textDim);
    fileLabel.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (fileLabel);

    // Parse / compile errors. orangered reads clearly as a fault on the dark
    // background; an empty string (the healthy state) draws nothing.
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::orangered);
    statusLabel.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (statusLabel);

    paramViewport.setViewedComponent (&paramPanel, false);
    paramViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (paramViewport);

    triggerLabel.setColour (juce::Label::textColourId, WTColors::textDim);
    addAndMakeVisible (triggerLabel);

    // Item IDs 1..3 in the same order as the playbackTrigger choices, as the
    // ComboBoxAttachment requires.
    triggerBox.addItem ("Transport", 1);
    triggerBox.addItem ("MIDI",      2);
    triggerBox.addItem ("Always",    3);
    addAndMakeVisible (triggerBox);
    triggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "playbackTrigger", triggerBox);

    gainLabel.setColour (juce::Label::textColourId, WTColors::textDim);
    addAndMakeVisible (gainLabel);

    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextValueSuffix (" dB");
    gainSlider.setNumDecimalPlacesToDisplay (1);
    addAndMakeVisible (gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", gainSlider);

    audioProcessor.addChangeListener (this);

    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    getConstrainer()->setFixedAspectRatio ((double) kBaseWidth / (double) kBaseHeight);
    setSize (kBaseWidth, kBaseHeight);

    refreshFromProcessor();
}

WTGeneratorAudioProcessorEditor::~WTGeneratorAudioProcessorEditor()
{
    audioProcessor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void WTGeneratorAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // The processor broadcasts whenever the active definition or the load
    // status changes (a user load, or a session restore).
    refreshFromProcessor();
}

void WTGeneratorAudioProcessorEditor::refreshFromProcessor()
{
    paramPanel.setDefinition (audioProcessor.signalGenerator.getDefinition());

    fileLabel.setText (audioProcessor.getSignalSourceLabel(), juce::dontSendNotification);

    statusLabel.setText (audioProcessor.getStatusMessage(), juce::dontSendNotification);

    resized();   // the parameter panel's height may have changed
}

void WTGeneratorAudioProcessorEditor::chooseExpressionFile()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load Expression XML", juce::File(), "*.xml");

    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file != juce::File())
                audioProcessor.loadExpressionFile (file);
            // loadExpressionFile broadcasts a change, so the UI (including
            // the status line on failure) refreshes via changeListenerCallback.
        });
}

//==============================================================================
void WTGeneratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    lookAndFeel.setUiScale (scale());

    g.fillAll (WTColors::background);

    g.setColour (WTColors::text);
    g.setFont (juce::FontOptions (sf (20.0f)));
    auto header = getLocalBounds().reduced (sx (12)).removeFromTop (sx (28));
    header.removeFromRight (sx (158));   // keep clear of the load button
    g.drawText ("WTGenerator", header, juce::Justification::centredLeft, false);
}

void WTGeneratorAudioProcessorEditor::resized()
{
    lookAndFeel.setUiScale (scale());

    auto area = getLocalBounds().reduced (sx (12));

    auto header = area.removeFromTop (sx (28));
    loadButton.setBounds (header.removeFromRight (sx (150)));

    area.removeFromTop (sx (8));
    fileLabel.setBounds (area.removeFromTop (sx (20)));

    area.removeFromTop (sx (4));
    statusLabel.setBounds (area.removeFromTop (sx (34)));

    auto bottom = area.removeFromBottom (sx (50));
    {
        auto left = bottom.removeFromLeft (bottom.getWidth() / 2).reduced (sx (4), 0);
        triggerLabel.setBounds (left.removeFromTop (sx (20)));
        triggerBox.setBounds   (left.removeFromTop (sx (24)));

        auto right = bottom.reduced (sx (4), 0);
        gainLabel.setBounds  (right.removeFromTop (sx (20)));
        gainSlider.setBounds (right.removeFromTop (sx (24)));
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, sx (56), sx (18));
    }

    area.removeFromTop    (sx (8));
    area.removeFromBottom (sx (8));

    paramViewport.setBounds (area);

    paramPanel.setUiScale (scale());
    paramPanel.setSize (juce::jmax (paramViewport.getWidth() - sx (12), sx (10)),
                        juce::jmax (paramViewport.getHeight(),
                                    paramPanel.getRequiredHeight()));
}
