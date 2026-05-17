/*
  ==============================================================================

    PluginProcessor.h
    WTGenerator - Fine Increments' purpose-built test-signal generator plugin.
    Companion to WTAnalyzer; the source side of the precision DSP test bench.

    This is the v0 skeleton (WTGENERATOR.md section 11): one stereo output bus,
    optional MIDI input, an empty processBlock that emits silence, and an APVTS
    carrying no parameters yet. The APVTS exists from day one so the state-
    persistence path (WTGENERATOR.md section 9) and the parameter idiom are in
    place before v1 adds the expression engine - adding a parameter is then an
    append to createParameterLayout(), not a structural change.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class WTGeneratorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WTGeneratorAudioProcessor();
    ~WTGeneratorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public parameter tree. The editor binds controls to it via attachments;
    // the host serializes it automatically through getStateInformation. Empty
    // at v0 - the first parameters land with expression mode in v1.
    juce::AudioProcessorValueTreeState apvts;

    // Sample rate captured during prepareToPlay. Generators compute their
    // frequencies against this so output is rate-correct at 44.1/48/96/192 kHz
    // (WTGENERATOR.md section 7.3). Atomic so the editor can read it too.
    std::atomic<float> currentSampleRate { 48000.0f };

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WTGeneratorAudioProcessor)
};
