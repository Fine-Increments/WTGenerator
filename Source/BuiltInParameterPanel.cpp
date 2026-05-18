/*
  ==============================================================================

    BuiltInParameterPanel.cpp
    See BuiltInParameterPanel.h.

  ==============================================================================
*/

#include "BuiltInParameterPanel.h"
#include "Colors.h"
#include <vector>

//==============================================================================
namespace
{
    // Parameter IDs per built-in generator, in builtInGenerator Choice order
    // (WTGENERATOR.md section 4.4). The UI's view of which parameters belong
    // to which generator; the generator classes themselves cache the same
    // IDs independently.
    const std::vector<juce::String>& generatorParamIDs (int generatorIndex)
    {
        static const std::vector<std::vector<juce::String>> table
        {
            { "sineFreq", "sineLevel" },                                          // Sine
            { "sweepStartHz", "sweepEndHz", "sweepDuration", "sweepCurve" },       // Sine Sweep
            { "twoToneF1", "twoToneF2", "twoToneLevel1", "twoToneLevel2" },        // Two-Tone
            { "multisineFundamental", "multisineMaxHarmonic", "multisinePhase" },  // Multisine
            { "chirpStartHz", "chirpEndHz", "chirpDuration" },                     // Chirp
            { "impulsePolarity", "impulseLevel" },                                 // Impulse
            { "stepRiseTime", "stepLevel" },                                       // Step
            { "burstFreq", "burstLevel", "burstAttack", "burstDecay",
              "burstSustain", "burstRelease", "burstGate" },                       // Tone Burst
            { "whiteNoiseLevel" },                                                 // White Noise
            { "pinkNoiseLevel" },                                                  // Pink Noise
            { "brownNoiseLevel" },                                                 // Brown Noise
            { "mlsOrder", "mlsLevel" },                                            // MLS
            { "dcLevel" },                                                         // DC
            {}                                                                     // Silence
        };

        static const std::vector<juce::String> none;
        return (generatorIndex >= 0 && generatorIndex < (int) table.size())
                   ? table[(size_t) generatorIndex]
                   : none;
    }
}

//==============================================================================
BuiltInParameterPanel::BuiltInParameterPanel (juce::AudioProcessorValueTreeState& s)
    : apvts (s)
{
}

void BuiltInParameterPanel::setGenerator (int generatorIndex)
{
    // Clearing the OwnedArray destroys each Row; within a row the attachment
    // (declared last) is destroyed before the control it references.
    rows.clear();

    for (const auto& id : generatorParamIDs (generatorIndex))
    {
        auto* param = apvts.getParameter (id);
        if (param == nullptr)
            continue;

        auto* row = rows.add (new Row());
        row->label.setText (param->getName (128), juce::dontSendNotification);
        addAndMakeVisible (row->label);

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            // Choice -> combo box. Item IDs 1..N, as the attachment requires.
            row->isChoice = true;
            for (int i = 0; i < choice->choices.size(); ++i)
                row->combo.addItem (choice->choices[i], i + 1);

            addAndMakeVisible (row->combo);
            row->comboAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, row->combo);
        }
        else
        {
            // Float / Int -> slider. The attachment configures the slider's
            // range (and integer stepping for Int) from the parameter.
            row->slider.setSliderStyle (juce::Slider::LinearHorizontal);
            addAndMakeVisible (row->slider);
            row->sliderAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, row->slider);
        }
    }

    resized();
}

void BuiltInParameterPanel::setUiScale (float newScale)
{
    uiScale = newScale;
    resized();
}

int BuiltInParameterPanel::getRequiredHeight() const
{
    return rows.size() * juce::roundToInt ((float) kRowHeight * uiScale);
}

void BuiltInParameterPanel::paint (juce::Graphics& g)
{
    g.fillAll (WTColors::background);
}

void BuiltInParameterPanel::resized()
{
    const int rowH    = juce::roundToInt ((float) kRowHeight * uiScale);
    const int labelW  = juce::roundToInt ((float) getWidth() * 0.45f);
    const int boxW    = juce::roundToInt (64.0f * uiScale);
    const int boxH    = juce::roundToInt (18.0f * uiScale);
    const int comboPad = juce::roundToInt (3.0f * uiScale);

    int y = 0;
    for (auto* row : rows)
    {
        row->label.setBounds (0, y, labelW, rowH);
        const juce::Rectangle<int> controlArea (labelW, y, getWidth() - labelW, rowH);

        if (row->isChoice)
        {
            row->combo.setBounds (controlArea.reduced (0, comboPad));
        }
        else
        {
            row->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, boxW, boxH);
            row->slider.setBounds (controlArea);
        }

        y += rowH;
    }
}
