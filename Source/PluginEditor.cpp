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

    // Generator-mode selector. Item IDs 1..N match the Choice value order.
    generatorModeBox.addItem ("Expression", 1);
    generatorModeBox.addItem ("Built-in",   2);
    generatorModeBox.addItem ("Wavetable",  3);
    generatorModeBox.addItem ("Render",     4);
    addAndMakeVisible (generatorModeBox);
    generatorModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "generatorMode", generatorModeBox);

    // Built-in generator selector - the 14 generators in builtInGenerator
    // Choice order.
    const char* const generatorNames[] =
        { "Sine", "Sine Sweep", "Two-Tone", "Multisine", "Chirp", "Impulse",
          "Step", "Tone Burst", "White Noise", "Pink Noise", "Brown Noise",
          "MLS", "DC", "Silence" };
    for (int i = 0; i < (int) juce::numElementsInArray (generatorNames); ++i)
        builtInGeneratorBox.addItem (generatorNames[i], i + 1);
    addAndMakeVisible (builtInGeneratorBox);
    builtInGeneratorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "builtInGenerator", builtInGeneratorBox);

    generatorModeParam = audioProcessor.apvts.getRawParameterValue ("generatorMode");

    audioProcessor.addChangeListener (this);

    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    getConstrainer()->setFixedAspectRatio ((double) kBaseWidth / (double) kBaseHeight);
    setSize (kBaseWidth, kBaseHeight);

    refreshFromProcessor();
    updateModeVisibility();
    startTimerHz (15);
}

WTGeneratorAudioProcessorEditor::~WTGeneratorAudioProcessorEditor()
{
    stopTimer();
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

void WTGeneratorAudioProcessorEditor::timerCallback()
{
    // Follow generatorMode changes from any source - the selector, host
    // automation, a preset load - and swap the mode-specific UI.
    const int mode = (generatorModeParam != nullptr)
                        ? (int) generatorModeParam->load() : 0;
    if (mode != lastGeneratorMode)
        updateModeVisibility();
}

void WTGeneratorAudioProcessorEditor::updateModeVisibility()
{
    const int mode = (generatorModeParam != nullptr)
                        ? (int) generatorModeParam->load() : 0;
    lastGeneratorMode = mode;

    const bool expression = (mode == 0);
    const bool builtIn    = (mode == 1);

    // Expression-mode controls.
    loadButton.setVisible    (expression);
    fileLabel.setVisible     (expression);
    statusLabel.setVisible   (expression);
    paramViewport.setVisible (expression);

    // Built-in-mode controls. The per-generator parameter panel arrives in
    // the next sub-step; Built-in mode currently shows just the selector.
    builtInGeneratorBox.setVisible (builtIn);
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

    // Header: product name (painted) + Generator Mode selector.
    auto header = area.removeFromTop (sx (28));
    generatorModeBox.setBounds (header.removeFromRight (sx (150)));

    area.removeFromTop (sx (8));

    // Mode-specific action row: Load Expression... (Expression mode) or the
    // Built-in Generator selector (Built-in mode) - one slot, one visible.
    auto actionRect = area.removeFromTop (sx (26)).removeFromLeft (sx (180));
    loadButton.setBounds          (actionRect);
    builtInGeneratorBox.setBounds (actionRect);

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
