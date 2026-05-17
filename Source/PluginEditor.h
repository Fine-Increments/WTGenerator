/*
  ==============================================================================

    PluginEditor.h
    WTGenerator's editor. v1 expression-mode UI: a "Load Expression..." button,
    a status line, the dynamic parameter panel (one slider per declared
    parameter, in a scrolling Viewport), and the playback-trigger / output-gain
    controls. No generator-mode selector yet - only Expression mode is
    functional in v1 (WTGENERATOR.md section 11), so v2 adds that control with
    the Built-in generators.

    Responsive scaling (kBase* constants, scale(), sx()/sf(), WTLookAndFeel)
    per PRINCIPLES.md section 2; UI is product name plus controls only,
    per section 3.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DynamicParameterPanel.h"

//==============================================================================
// LookAndFeel that scales JUCE-owned fonts (TextButton, ComboBox, Label, and
// the Label inside a Slider's text box) by a runtime factor so the whole UI
// grows uniformly with the window. Carries a coherent dark colour scheme.
class WTLookAndFeel  : public juce::LookAndFeel_V4
{
public:
    WTLookAndFeel()
    {
        setColourScheme (juce::LookAndFeel_V4::getDarkColourScheme());
    }

    void setUiScale (float newScale) noexcept { uiScale = newScale; }

    juce::Font getTextButtonFont (juce::TextButton&, int /*buttonHeight*/) override
    {
        return juce::Font (juce::FontOptions (13.0f * uiScale));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.0f * uiScale));
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font (juce::FontOptions (13.0f * uiScale));
    }

private:
    float uiScale = 1.0f;
};

//==============================================================================
class WTGeneratorAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         private juce::ChangeListener
{
public:
    // The layout in paint()/resized() is authored at this size; every pixel
    // and font value runs through sx()/sf().
    static constexpr int kBaseWidth  = 540;
    static constexpr int kBaseHeight = 400;
    static constexpr int kMinWidth   = 432;
    static constexpr int kMinHeight  = 320;
    static constexpr int kMaxWidth   = 2160;
    static constexpr int kMaxHeight  = 1600;

    explicit WTGeneratorAudioProcessorEditor (WTGeneratorAudioProcessor&);
    ~WTGeneratorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refreshFromProcessor();
    void chooseExpressionFile();

    float scale() const noexcept
    {
        return juce::jmin ((float) getWidth()  / (float) kBaseWidth,
                           (float) getHeight() / (float) kBaseHeight);
    }

    int   sx (int   v) const noexcept { return juce::roundToInt ((float) v * scale()); }
    float sf (float v) const noexcept { return v * scale(); }

    WTGeneratorAudioProcessor& audioProcessor;
    WTLookAndFeel lookAndFeel;

    // Header.
    juce::TextButton loadButton { "Load Expression..." };

    // Currently loaded .xml name (or "Default expression"), and the most
    // recent parse / compile diagnostic.
    juce::Label fileLabel;
    juce::Label statusLabel;

    // Dynamic parameter sliders, scrolled when they exceed the panel area.
    juce::Viewport        paramViewport;
    DynamicParameterPanel paramPanel;

    // Playback controls.
    juce::Label    triggerLabel { {}, "Playback Trigger" };
    juce::ComboBox triggerBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;

    juce::Label  gainLabel { {}, "Output Gain" };
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    // Held for the duration of an async file-open dialog.
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WTGeneratorAudioProcessorEditor)
};
