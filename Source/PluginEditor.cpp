/*
  ==============================================================================

    PluginEditor.cpp
    WTGenerator v1 editor. See PluginEditor.h for the layout intent.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "Colors.h"
#include "BuiltIn/PresetLibrary.h"
#include <vector>

//==============================================================================
WTGeneratorAudioProcessorEditor::WTGeneratorAudioProcessorEditor (WTGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      paramPanel (p), builtInPanel (p.apvts)
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

    builtInViewport.setViewedComponent (&builtInPanel, false);
    builtInViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (builtInViewport);

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

    repeatLabel.setColour (juce::Label::textColourId, WTColors::textDim);
    addAndMakeVisible (repeatLabel);

    // Item IDs 1..3 match the oneShot ("Repeat Mode") choice order.
    repeatBox.addItem ("Loop",     1);
    repeatBox.addItem ("One-Shot", 2);
    repeatBox.addItem ("Periodic", 3);
    addAndMakeVisible (repeatBox);
    repeatAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "oneShot", repeatBox);

    periodicRateLabel.setColour (juce::Label::textColourId, WTColors::textDim);
    addAndMakeVisible (periodicRateLabel);

    periodicRateSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    periodicRateSlider.setTextValueSuffix (" Hz");
    periodicRateSlider.setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (periodicRateSlider);
    periodicRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "periodicRate", periodicRateSlider);

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

    // Preset picker - the curated library (WTGENERATOR.md section 8.5).
    // Item IDs are 1-based preset indices; section headings carry no ID and
    // are not selectable. The placeholder text shows while nothing is chosen.
    {
        const auto& presets = BuiltInPresets::library();
        juce::String currentCategory;
        for (int i = 0; i < (int) presets.size(); ++i)
        {
            if (presets[(size_t) i].category != currentCategory)
            {
                currentCategory = presets[(size_t) i].category;
                presetBox.addSectionHeading (currentCategory);
            }
            presetBox.addItem (presets[(size_t) i].name, i + 1);
        }
    }
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this] { applySelectedPreset(); };
    addAndMakeVisible (presetBox);

    generatorModeParam    = audioProcessor.apvts.getRawParameterValue ("generatorMode");
    builtInGeneratorParam = audioProcessor.apvts.getRawParameterValue ("builtInGenerator");
    oneShotParam          = audioProcessor.apvts.getRawParameterValue ("oneShot");

    // Periodic Rate only bites in Periodic repeat mode - grey it out otherwise.
    if (oneShotParam != nullptr)
    {
        lastRepeatMode = (int) oneShotParam->load();
        periodicRateSlider.setEnabled (lastRepeatMode == 2);   // 2 = Periodic
    }

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
    // Follow generatorMode / builtInGenerator changes from any source - the
    // selectors, host automation, a preset load - and update the UI.
    const int mode = (generatorModeParam != nullptr)
                        ? (int) generatorModeParam->load() : 0;
    if (mode != lastGeneratorMode)
    {
        updateModeVisibility();
        return;
    }

    const int generator = (builtInGeneratorParam != nullptr)
                            ? (int) builtInGeneratorParam->load() : 0;
    if (generator != lastBuiltInGenerator)
    {
        lastBuiltInGenerator = generator;
        builtInPanel.setGenerator (generator);
        updatePlaybackControlVisibility();   // Repeat Mode / Periodic Rate
        resized();
    }

    // Follow Repeat Mode from any source (the selector, a preset, host
    // automation) and enable Periodic Rate only while it is Periodic.
    const int repeatMode = (oneShotParam != nullptr)
                             ? (int) oneShotParam->load() : 0;
    if (repeatMode != lastRepeatMode)
    {
        lastRepeatMode = repeatMode;
        periodicRateSlider.setEnabled (repeatMode == 2);   // 2 = Periodic
    }
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

    // Built-in-mode controls.
    builtInGeneratorBox.setVisible (builtIn);
    presetBox.setVisible           (builtIn);
    builtInViewport.setVisible     (builtIn);

    if (builtIn)
    {
        const int generator = (builtInGeneratorParam != nullptr)
                                ? (int) builtInGeneratorParam->load() : 0;
        lastBuiltInGenerator = generator;
        builtInPanel.setGenerator (generator);
    }

    updatePlaybackControlVisibility();
    resized();
}

bool WTGeneratorAudioProcessorEditor::repeatModeApplies() const
{
    // Built-in mode only - Expression mode ignores Repeat Mode entirely.
    if (generatorModeParam == nullptr || (int) generatorModeParam->load() != 1)
        return false;

    // Sine Sweep (1), Chirp (4), Impulse (5), Step (6) and Tone Burst (7)
    // are the generators that read the Repeat Mode parameter.
    const int g = (builtInGeneratorParam != nullptr) ? (int) builtInGeneratorParam->load() : 0;
    return g == 1 || g == 4 || g == 5 || g == 6 || g == 7;
}

bool WTGeneratorAudioProcessorEditor::periodicRateApplies() const
{
    if (generatorModeParam == nullptr || (int) generatorModeParam->load() != 1)
        return false;

    // Only Impulse (5), Step (6) and Tone Burst (7) read Periodic Rate.
    const int g = (builtInGeneratorParam != nullptr) ? (int) builtInGeneratorParam->load() : 0;
    return g == 5 || g == 6 || g == 7;
}

void WTGeneratorAudioProcessorEditor::updatePlaybackControlVisibility()
{
    const bool repeat   = repeatModeApplies();
    const bool periodic = periodicRateApplies();
    repeatLabel.setVisible        (repeat);
    repeatBox.setVisible          (repeat);
    periodicRateLabel.setVisible  (periodic);
    periodicRateSlider.setVisible (periodic);
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

void WTGeneratorAudioProcessorEditor::applySelectedPreset()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0)
        return;   // a section heading, or the cleared placeholder

    BuiltInPresets::apply (audioProcessor.apvts, id - 1);

    // Snap back to the placeholder so picking the same preset again re-applies
    // it; dontSendNotification keeps this from re-entering onChange.
    presetBox.setSelectedId (0, juce::dontSendNotification);
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
    // Built-in Generator selector plus the preset picker (Built-in mode) -
    // the leftmost slot is shared, one control visible at a time.
    auto actionRow  = area.removeFromTop (sx (26));
    auto actionRect = actionRow.removeFromLeft (sx (180));
    loadButton.setBounds          (actionRect);
    builtInGeneratorBox.setBounds (actionRect);
    actionRow.removeFromLeft (sx (8));
    presetBox.setBounds (actionRow.removeFromLeft (sx (220)));

    area.removeFromTop (sx (8));
    fileLabel.setBounds (area.removeFromTop (sx (20)));

    area.removeFromTop (sx (4));
    statusLabel.setBounds (area.removeFromTop (sx (34)));

    // Playback strip. Trigger and Output Gain always apply; Repeat Mode and
    // Periodic Rate join only for the generators that read them. Whichever
    // apply are laid out two per row, and the strip is sized to suit - so a
    // steady generator gives that height back to the parameter list.
    struct PbControl { juce::Component* label; juce::Component* ctl; };
    std::vector<PbControl> pb;
    pb.push_back ({ &triggerLabel, &triggerBox });
    if (repeatModeApplies())   pb.push_back ({ &repeatLabel, &repeatBox });
    if (periodicRateApplies()) pb.push_back ({ &periodicRateLabel, &periodicRateSlider });
    pb.push_back ({ &gainLabel, &gainSlider });

    const int pbRows = ((int) pb.size() + 1) / 2;
    auto      bottom = area.removeFromBottom (sx (50) * pbRows);

    for (int r = 0; r < pbRows; ++r)
    {
        auto rowRect = bottom.removeFromTop (sx (50));
        juce::Rectangle<int> cells[2];
        cells[0] = rowRect.removeFromLeft (rowRect.getWidth() / 2);
        cells[1] = rowRect;

        for (int c = 0; c < 2; ++c)
        {
            const int idx = r * 2 + c;
            if (idx >= (int) pb.size())
                break;

            auto cell = cells[c].reduced (sx (4), 0);
            pb[(size_t) idx].label->setBounds (cell.removeFromTop (sx (20)));
            pb[(size_t) idx].ctl  ->setBounds (cell.removeFromTop (sx (24)));
        }
    }

    gainSlider.setTextBoxStyle         (juce::Slider::TextBoxRight, false, sx (56), sx (18));
    periodicRateSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, sx (56), sx (18));

    area.removeFromTop    (sx (8));
    area.removeFromBottom (sx (8));

    // Main panel area - one viewport per mode, same bounds, one visible.
    paramViewport.setBounds   (area);
    builtInViewport.setBounds (area);

    paramPanel.setUiScale (scale());
    paramPanel.setSize (juce::jmax (paramViewport.getWidth() - sx (12), sx (10)),
                        juce::jmax (paramViewport.getHeight(),
                                    paramPanel.getRequiredHeight()));

    builtInPanel.setUiScale (scale());
    builtInPanel.setSize (juce::jmax (builtInViewport.getWidth() - sx (12), sx (10)),
                          juce::jmax (builtInViewport.getHeight(),
                                      builtInPanel.getRequiredHeight()));
}
