/*
  ==============================================================================

    PluginEditor.h
    WTGenerator's editor. v0 skeleton: a fully responsive window that paints
    the product name and nothing else - there are no controls yet.

    The responsive-scaling machinery (kBase* constants, scale(), sx()/sf(),
    WTLookAndFeel) is in place from day one because retrofitting it later is
    painful (PRINCIPLES.md section 2). Every later panel - generator panel,
    playback panel, sweep panel, output preview (WTGENERATOR.md section 10.2)
    - is then an add against an already-responsive shell.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// LookAndFeel that scales JUCE-owned fonts (TextButton, ComboBox, etc.) by a
// runtime-adjustable factor so the whole UI grows uniformly when the window is
// resized. Components whose fonts we set directly have their fonts updated by
// their owning panels' setUiScale(). No controls use it yet at v0; it is here
// so the first control added in v1 inherits scaled fonts for free.
class WTLookAndFeel  : public juce::LookAndFeel_V4
{
public:
    void setUiScale (float newScale) noexcept { uiScale = newScale; }

    juce::Font getTextButtonFont (juce::TextButton&, int /*buttonHeight*/) override
    {
        return juce::Font (juce::FontOptions (13.0f * uiScale));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.0f * uiScale));
    }

private:
    float uiScale = 1.0f;
};

//==============================================================================
class WTGeneratorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    // The layout in paint()/resized() is authored at this size. Every pixel
    // and font value runs through sx()/sf() so the same layout description
    // renders identically at any window size between the min/max bounds.
    static constexpr int kBaseWidth  = 520;
    static constexpr int kBaseHeight = 360;
    static constexpr int kMinWidth   = 416;
    static constexpr int kMinHeight  = 288;
    static constexpr int kMaxWidth   = 2080;
    static constexpr int kMaxHeight  = 1440;

    explicit WTGeneratorAudioProcessorEditor (WTGeneratorAudioProcessor&);
    ~WTGeneratorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Uniform scale - no aspect-ratio distortion of content.
    float scale() const noexcept
    {
        return juce::jmin ((float) getWidth()  / (float) kBaseWidth,
                           (float) getHeight() / (float) kBaseHeight);
    }

    int   sx (int   v) const noexcept { return juce::roundToInt ((float) v * scale()); }
    float sf (float v) const noexcept { return v * scale(); }

    WTGeneratorAudioProcessor& audioProcessor;
    WTLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WTGeneratorAudioProcessorEditor)
};
