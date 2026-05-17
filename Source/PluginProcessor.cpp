/*
  ==============================================================================

    PluginProcessor.cpp
    WTGenerator v0 skeleton. See PluginProcessor.h for the milestone context.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WTGeneratorAudioProcessor::WTGeneratorAudioProcessor()
    : AudioProcessor (BusesProperties()
        // WTGenerator is a source, not an effect: one stereo output, no audio
        // input buses (WTGENERATOR.md section 3.1). Mono signals fill both
        // channels; future stereo wavetables get true L/R separation.
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

WTGeneratorAudioProcessor::~WTGeneratorAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
WTGeneratorAudioProcessor::createParameterLayout()
{
    // No parameters at v0. v1 appends the expression engine's declared
    // parameters here (WTGENERATOR.md section 4.3); v2 the built-in
    // generator selection. The empty layout still gives a valid APVTS so
    // getStateInformation / setStateInformation round-trip cleanly.
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    return layout;
}

//==============================================================================
void WTGeneratorAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate.store ((float) sampleRate);
}

void WTGeneratorAudioProcessor::releaseResources()
{
}

bool WTGeneratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // One stereo output, no audio inputs. A source plugin has nothing to
    // negotiate on the input side.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void WTGeneratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // v0: emit silence. Signal generation (expression mode, built-in
    // generators) lands in v1 / v2 - see WTGENERATOR.md sections 4 and 11.
    // Clearing here means the skeleton outputs a clean zero rather than
    // whatever the host left in the buffer.
    buffer.clear();
}

//==============================================================================
juce::AudioProcessorEditor* WTGeneratorAudioProcessor::createEditor()
{
    return new WTGeneratorAudioProcessorEditor (*this);
}

bool WTGeneratorAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
const juce::String WTGeneratorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WTGeneratorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WTGeneratorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WTGeneratorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WTGeneratorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int WTGeneratorAudioProcessor::getNumPrograms()
{
    return 1;   // Some hosts cope poorly with 0 programs; report at least 1.
}

int WTGeneratorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WTGeneratorAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String WTGeneratorAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void WTGeneratorAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void WTGeneratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);
    apvts.state.writeToStream (stream);
}

void WTGeneratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (tree.isValid() && tree.hasType (apvts.state.getType()))
        apvts.replaceState (tree);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WTGeneratorAudioProcessor();
}
