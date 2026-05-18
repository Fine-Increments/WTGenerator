/*
  ==============================================================================

    PresetLibrary.h
    The curated built-in-generator preset library (WTGENERATOR.md section 8.5).

    A preset is a named, fully-specified parameter setup for a common test
    signal - "1 kHz Sine, -6 dBFS", "Log Sine Sweep 20 Hz-20 kHz, 10 s". One
    click sets up the test. Each preset writes EVERY parameter it depends on
    (generator mode, generator selection, repeat mode, output gain and the
    generator's own controls), so the resulting signal never depends on
    whatever preset or hand-tweaked state preceded it.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
namespace BuiltInPresets
{
    // One parameter write. `value` is the real (denormalised) parameter value -
    // Hz, dB, a Choice index, an Int count - in the parameter's own units;
    // apply() normalises it through the parameter's range.
    struct Setting
    {
        juce::String paramID;
        float        value;
    };

    struct Preset
    {
        juce::String         category;   // combo section heading
        juce::String         name;
        std::vector<Setting> settings;
    };

    // The full curated library, in display order. Grouped by `category`; the
    // UI emits a section heading each time the category changes.
    const std::vector<Preset>& library();

    // Writes preset `index`'s settings into the APVTS. Message thread only -
    // setValueNotifyingHost is not real-time safe. Out-of-range indices and
    // unknown parameter IDs are ignored.
    void apply (juce::AudioProcessorValueTreeState& apvts, int index);
}
