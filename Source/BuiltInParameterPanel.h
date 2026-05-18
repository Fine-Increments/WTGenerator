/*
  ==============================================================================

    BuiltInParameterPanel.h
    Shows the parameters of the active built-in generator as controls, rebuilt
    whenever the generator selection changes. Sliders for Float / Int
    parameters, combo boxes for Choice parameters; each bound to its dedicated
    APVTS parameter. Sits in the editor's Built-in-mode Viewport.

    Sibling of DynamicParameterPanel (the expression-mode panel): that one
    drives the generic 0..1 pool, this one drives the built-in generators'
    dedicated named parameters.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class BuiltInParameterPanel  : public juce::Component
{
public:
    explicit BuiltInParameterPanel (juce::AudioProcessorValueTreeState& apvts);

    // Message thread. Rebuilds the controls for the generator at
    // `generatorIndex` (0..13, builtInGenerator Choice order).
    void setGenerator (int generatorIndex);

    void setUiScale (float newScale);

    // Total height the rows need at the current scale; the editor sizes the
    // panel inside its Viewport from this.
    int getRequiredHeight() const;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // One control row. A row uses either the slider or the combo depending
    // on the parameter's type; the attachments are declared last so each is
    // destroyed before the control it references.
    struct Row
    {
        juce::Label    label;
        juce::Slider   slider;
        juce::ComboBox combo;
        bool           isChoice = false;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
    };

    static constexpr int kRowHeight = 30;   // base height, pre-scale

    juce::AudioProcessorValueTreeState& apvts;
    juce::OwnedArray<Row> rows;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInParameterPanel)
};
