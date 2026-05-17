/*
  ==============================================================================

    PluginEditor.cpp
    WTGenerator v0 skeleton editor. See PluginEditor.h for milestone context.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "Colors.h"

//==============================================================================
WTGeneratorAudioProcessorEditor::WTGeneratorAudioProcessorEditor (WTGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    getConstrainer()->setFixedAspectRatio ((double) kBaseWidth / (double) kBaseHeight);

    setSize (kBaseWidth, kBaseHeight);
}

WTGeneratorAudioProcessorEditor::~WTGeneratorAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void WTGeneratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    lookAndFeel.setUiScale (scale());

    g.fillAll (WTColors::background);

    // v0: the window is the product name and nothing else (PRINCIPLES.md
    // section 3 - UI is product name and controls only; no controls yet).
    g.setColour (WTColors::text);
    g.setFont (juce::FontOptions (sf (28.0f)));
    g.drawText ("WTGenerator", getLocalBounds(), juce::Justification::centred, false);
}

void WTGeneratorAudioProcessorEditor::resized()
{
    lookAndFeel.setUiScale (scale());

    // No child components to lay out yet. The generator / playback / sweep
    // panels (WTGENERATOR.md section 10.2) get positioned here as they land.
}
