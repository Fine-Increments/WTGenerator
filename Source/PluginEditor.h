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
#include "BuiltInParameterPanel.h"

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
                                         private juce::ChangeListener,
                                         private juce::Timer
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
    void timerCallback() override;
    void refreshFromProcessor();
    void chooseExpressionFile();

    // Shows the controls for the active generator mode and hides the rest.
    void updateModeVisibility();

    // Applies the preset chosen in presetBox, then clears the selection so
    // the same preset can be picked again.
    void applySelectedPreset();

    // Repeat Mode applies to the swept and triggered generators; Periodic
    // Rate only to the triggered ones. Both are hidden for generators - and
    // for Expression mode - that ignore them. updatePlaybackControlVisibility
    // pushes that to the controls; the playback strip in resized() reflows
    // around whichever remain.
    bool repeatModeApplies() const;
    bool periodicRateApplies() const;
    void updatePlaybackControlVisibility();

    float scale() const noexcept
    {
        return juce::jmin ((float) getWidth()  / (float) kBaseWidth,
                           (float) getHeight() / (float) kBaseHeight);
    }

    int   sx (int   v) const noexcept { return juce::roundToInt ((float) v * scale()); }
    float sf (float v) const noexcept { return v * scale(); }

    WTGeneratorAudioProcessor& audioProcessor;
    WTLookAndFeel lookAndFeel;

    // Generator-mode selector (header, always visible), the Built-in
    // generator selector and the preset picker (both shown only in Built-in
    // mode). presetBox is a momentary action, not a bound parameter - it
    // writes a curated parameter set and snaps back to its placeholder.
    juce::ComboBox generatorModeBox;
    juce::ComboBox builtInGeneratorBox;
    juce::ComboBox presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> generatorModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> builtInGeneratorAttachment;

    // generatorMode / builtInGenerator are polled (the timer) so the UI
    // follows host-driven changes too, not just clicks on the selectors.
    std::atomic<float>* generatorModeParam    = nullptr;
    std::atomic<float>* builtInGeneratorParam = nullptr;
    std::atomic<float>* oneShotParam          = nullptr;
    int lastGeneratorMode    = -1;
    int lastBuiltInGenerator = -1;
    int lastRepeatMode       = -1;

    // Header.
    juce::TextButton loadButton { "Load Expression..." };

    // Currently loaded .xml name (or "Default expression"), and the most
    // recent parse / compile diagnostic.
    juce::Label fileLabel;
    juce::Label statusLabel;

    // Expression-mode parameter sliders, and Built-in-mode parameter
    // controls - one viewport per mode, same bounds, one visible at a time.
    juce::Viewport        paramViewport;
    DynamicParameterPanel paramPanel;
    juce::Viewport        builtInViewport;
    BuiltInParameterPanel builtInPanel;

    // Playback controls. Playback Trigger and Output Gain always apply.
    // Repeat Mode shows only for the swept / triggered generators, Periodic
    // Rate only for the triggered ones (see repeatModeApplies /
    // periodicRateApplies); Periodic Rate is additionally greyed out unless
    // Repeat Mode is Periodic, the only mode it affects.
    juce::Label    triggerLabel { {}, "Playback Trigger" };
    juce::ComboBox triggerBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;

    juce::Label    repeatLabel { {}, "Repeat Mode" };
    juce::ComboBox repeatBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> repeatAttachment;

    juce::Label  periodicRateLabel { {}, "Periodic Rate" };
    juce::Slider periodicRateSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> periodicRateAttachment;

    juce::Label  gainLabel { {}, "Output Gain" };
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    // Held for the duration of an async file-open dialog.
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WTGeneratorAudioProcessorEditor)
};
