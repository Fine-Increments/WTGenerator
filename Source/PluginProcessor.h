/*
  ==============================================================================

    PluginProcessor.h
    WTGenerator - Fine Increments' purpose-built test-signal generator plugin.
    Companion to WTAnalyzer; the source side of the precision DSP test bench.

    v1 (WTGENERATOR.md section 11): expression mode. processBlock gates on the
    host transport (with a free-run fallback for the Standalone build, section
    5.1), feeds the SignalGenerator its declared parameters from the APVTS
    pool, and routes the mono result to the stereo output through the output
    gain.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Expression/SignalGenerator.h"

//==============================================================================
class WTGeneratorAudioProcessor  : public juce::AudioProcessor,
                                   public juce::ChangeBroadcaster
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
    // the host serializes it automatically through getStateInformation.
    juce::AudioProcessorValueTreeState apvts;

    // The expression-mode signal source. Owns the ExpressionEngine; the
    // processor feeds it parameters and routes its output. Public so the
    // editor can read the active definition for the dynamic parameter UI.
    SignalGenerator signalGenerator;

    //==============================================================================
    // Expression-mode file loading. The editor's "Load Expression..." button
    // calls loadExpressionFile(); the processor parses the .xml, swaps the
    // definition into the SignalGenerator, resets the pool slots to the new
    // defaults, records the path for session persistence, and broadcasts a
    // change (this is a ChangeBroadcaster) so the editor rebuilds its
    // dynamic parameter UI. All message-thread.

    // Returns false on parse / compile failure; getStatusMessage() then
    // carries the reason.
    bool loadExpressionFile (const juce::File& file);

    // The currently loaded .xml - an invalid File while the baked-in default
    // expression is active.
    juce::File getLoadedFile() const { return loadedFile; }

    // Empty when all is well, otherwise the most recent parse / compile
    // diagnostic, for display in the editor.
    juce::String getStatusMessage() const { return statusMessage; }

    // Sample rate captured during prepareToPlay. Generators compute their
    // frequencies against this so output is rate-correct at 44.1/48/96/192 kHz
    // (WTGENERATOR.md section 7.3). Atomic so the editor can read it too.
    std::atomic<float> currentSampleRate { 48000.0f };

    //==============================================================================
    // Output gain range (WTGENERATOR.md section 5.4). The minimum doubles as
    // -inf: SignalGenerator maps kOutputGainMinDb to a linear gain of 0.
    static constexpr float kOutputGainMinDb = -100.0f;
    static constexpr float kOutputGainMaxDb =    6.0f;

    // Expression parameter pool addressing. A pool slot is a generic 0..1
    // float APVTS parameter that an expression parameter binds onto at load
    // time. There are kMaxExpressionParameters slots, declared once so the
    // host parameter list never changes. The host shows poolParamName(); the
    // editor labels the slot with the expression's real parameter name
    // prefixed by its 1-based slot number. `slot` is 0-based (0..31).
    static juce::String poolParamID   (int slot);   // -> "exprParam01" .. "exprParam32"
    static juce::String poolParamName (int slot);   // -> "Param 01"    .. "Param 32"

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Writes the active definition's parameter defaults into the matching
    // APVTS pool slots, normalised to 0..1. Without this a freshly inserted
    // plugin runs every expression parameter at the pool's generic 0.0 - so
    // the default signal's `amp` would be 0 and the plugin silent. Called
    // at construction for the baked-in default, and on a user-initiated
    // .xml load. A session restore deliberately skips it - the saved pool
    // values are restored verbatim instead.
    void applyPoolDefaultsFromDefinition();

    // Shared implementation of loadExpressionFile. `applyPoolDefaults` is
    // true for a user-initiated load (reset the pool slots to the
    // definition's defaults) and false for a session restore (the pool
    // values were just reloaded from saved state - keep them).
    bool loadExpressionFileInternal (const juce::File& file, bool applyPoolDefaults);

    juce::File   loadedFile;
    juce::String statusMessage;

    // Cached APVTS raw-value pointers, looked up once at construction so the
    // audio thread never does a parameter-name string lookup. The pool
    // pointers feed the expression's declared parameters in slot order.
    std::array<std::atomic<float>*, kMaxExpressionParameters> poolParamPtrs {};
    std::atomic<float>* outputGainPtr      = nullptr;
    std::atomic<float>* playbackTriggerPtr = nullptr;
    std::atomic<float>* generatorModePtr   = nullptr;

    // Free-running sample counter for the playback time base when the host
    // reports no transport time (the Standalone build - WTGENERATOR.md
    // section 5.1) and for Always mode. Audio-thread only.
    juce::int64 freeRunSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WTGeneratorAudioProcessor)
};
