/*
  ==============================================================================

    DynamicParameterPanel.h
    Renders one labelled slider per declared expression parameter. Each slider
    is bound to its pool-slot APVTS parameter (Param 01..32): the slider track
    is the slot's 0..1 range, but the text box shows the parameter's real
    denormalised value. The row label is "NN - name" - the 1-based slot
    number the host shows, plus the expression's own parameter name - so the
    user can match a plugin-window control to a host automation lane.

    Rebuilt whenever the active definition changes. Sits inside a Viewport in
    the editor, so any parameter count up to the 32-slot maximum scrolls.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Expression/ExpressionDefinition.h"

//==============================================================================
class DynamicParameterPanel  : public juce::Component
{
public:
    explicit DynamicParameterPanel (WTGeneratorAudioProcessor& processor);

    // Message thread. Rebuilds the rows for `definition` - one per declared
    // parameter.
    void setDefinition (const ExpressionDefinition& definition);

    void setUiScale (float newScale);

    // Total height the rows need at the current scale; the editor sizes the
    // panel inside its Viewport from this.
    int getRequiredHeight() const;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct ParamRow
    {
        juce::Label  label;
        juce::Slider slider;
        // Declared last so it is destroyed first - the attachment must
        // detach before the slider it references is torn down.
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    static constexpr int kRowHeight = 30;   // base height, pre-scale

    WTGeneratorAudioProcessor& processor;
    juce::OwnedArray<ParamRow> rows;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicParameterPanel)
};
