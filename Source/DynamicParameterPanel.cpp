/*
  ==============================================================================

    DynamicParameterPanel.cpp
    See DynamicParameterPanel.h.

  ==============================================================================
*/

#include "DynamicParameterPanel.h"
#include "Colors.h"

//==============================================================================
DynamicParameterPanel::DynamicParameterPanel (WTGeneratorAudioProcessor& p)
    : processor (p)
{
}

void DynamicParameterPanel::setDefinition (const ExpressionDefinition& definition)
{
    // Clearing the OwnedArray destroys each ParamRow; within a row the
    // SliderAttachment (declared last) is destroyed before its slider.
    rows.clear();

    for (size_t i = 0; i < definition.parameters.size(); ++i)
    {
        const auto& p   = definition.parameters[i];
        auto*       row = rows.add (new ParamRow());

        const auto slot = juce::String ((int) i + 1).paddedLeft ('0', 2);
        row->label.setText (slot + " - " + p.name, juce::dontSendNotification);
        addAndMakeVisible (row->label);

        row->slider.setSliderStyle (juce::Slider::LinearHorizontal);

        // The slider track is the pool slot's 0..1 value; the text box shows
        // the parameter's real denormalised value, and accepts one typed in.
        const double min      = p.minValue;
        const double span     = p.maxValue - p.minValue;
        const int    decimals = span >= 1000.0 ? 1 : span >= 10.0 ? 2 : 4;

        row->slider.textFromValueFunction = [min, span, decimals] (double norm)
        {
            return juce::String (min + norm * span, decimals);
        };
        row->slider.valueFromTextFunction = [min, span] (const juce::String& text)
        {
            return span > 0.0
                ? juce::jlimit (0.0, 1.0, (text.getDoubleValue() - min) / span)
                : 0.0;
        };

        addAndMakeVisible (row->slider);

        // Bind to the pool slot. The attachment sets the slider's 0..1 range
        // and keeps it synced with host automation; the text functions above
        // only reformat what the box displays.
        row->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts,
            WTGeneratorAudioProcessor::poolParamID ((int) i),
            row->slider);
    }

    resized();
}

void DynamicParameterPanel::setUiScale (float newScale)
{
    uiScale = newScale;
    resized();
}

int DynamicParameterPanel::getRequiredHeight() const
{
    return rows.size() * juce::roundToInt ((float) kRowHeight * uiScale);
}

void DynamicParameterPanel::paint (juce::Graphics& g)
{
    g.fillAll (WTColors::background);
}

void DynamicParameterPanel::resized()
{
    const int rowH   = juce::roundToInt ((float) kRowHeight * uiScale);
    const int labelW = juce::roundToInt ((float) getWidth() * 0.42f);
    const int boxW   = juce::roundToInt (64.0f * uiScale);
    const int boxH   = juce::roundToInt (18.0f * uiScale);

    int y = 0;
    for (auto* row : rows)
    {
        row->label.setBounds (0, y, labelW, rowH);
        row->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, boxW, boxH);
        row->slider.setBounds (labelW, y, getWidth() - labelW, rowH);
        y += rowH;
    }
}
