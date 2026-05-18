/*
  ==============================================================================

    PluginProcessor.cpp
    WTGenerator v0 skeleton. See PluginProcessor.h for the milestone context.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Expression/ExpressionDefinition.h"   // kMaxExpressionParameters
#include "Expression/ExpressionXmlParser.h"

namespace
{
    // Child tree / property under apvts.state that persists the loaded .xml
    // path across session save / load (WTGENERATOR.md section 9.3).
    const juce::Identifier kExpressionStateTag { "expressionState" };
    const juce::Identifier kFilePathProperty   { "filePath" };

    // A frequency parameter range with a log-style skew, so the lower
    // octaves of the audio band keep usable knob / automation resolution.
    juce::NormalisableRange<float> freqRange (float minHz, float maxHz)
    {
        return { minHz, maxHz, 0.0f, 0.30f };
    }
}

//==============================================================================
WTGeneratorAudioProcessor::WTGeneratorAudioProcessor()
    : AudioProcessor (BusesProperties()
        // WTGenerator is a source, not an effect: one stereo output, no audio
        // input buses (WTGENERATOR.md section 3.1). Mono signals fill both
        // channels; future stereo wavetables get true L/R separation.
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "Parameters", createParameterLayout())
    , signalGenerator (apvts)
{
    // Cache the APVTS raw-value pointers once - the audio thread then reads
    // parameters without a string lookup.
    for (int i = 0; i < kMaxExpressionParameters; ++i)
        poolParamPtrs[(size_t) i] = apvts.getRawParameterValue (poolParamID (i));

    outputGainPtr       = apvts.getRawParameterValue ("outputGain");
    playbackTriggerPtr  = apvts.getRawParameterValue ("playbackTrigger");
    generatorModePtr    = apvts.getRawParameterValue ("generatorMode");
    builtInGeneratorPtr = apvts.getRawParameterValue ("builtInGenerator");

    // Seed the pool slots from the baked-in default expression so a fresh
    // insert is audible immediately rather than running amp = 0.
    applyPoolDefaultsFromDefinition();
}

void WTGeneratorAudioProcessor::applyPoolDefaultsFromDefinition()
{
    const auto& def = signalGenerator.getDefinition();
    const int   numParams = juce::jmin ((int) def.parameters.size(),
                                        kMaxExpressionParameters);

    for (int i = 0; i < numParams; ++i)
    {
        const auto&  p    = def.parameters[(size_t) i];
        const double span = p.maxValue - p.minValue;
        const float  norm = (span > 0.0)
            ? (float) juce::jlimit (0.0, 1.0, (p.defaultValue - p.minValue) / span)
            : 0.0f;

        if (auto* param = apvts.getParameter (poolParamID (i)))
            param->setValueNotifyingHost (norm);
    }
}

bool WTGeneratorAudioProcessor::loadExpressionFile (const juce::File& file)
{
    return loadExpressionFileInternal (file, true);
}

bool WTGeneratorAudioProcessor::loadExpressionFileInternal (const juce::File& file,
                                                            bool applyPoolDefaults)
{
    const auto result = ExpressionXmlParser::parseFile (file);
    if (! result.succeeded)
    {
        statusMessage = result.error;
        sendChangeMessage();             // editor refreshes its status display
        return false;
    }

    // Parsed cleanly - try to compile. loadDefinition is the RT-safe swap,
    // so this is safe even if the host calls us mid-playback.
    if (! signalGenerator.loadDefinition (result.definition))
    {
        statusMessage = signalGenerator.getLastError();
        sendChangeMessage();
        return false;
    }

    loadedFile    = file;
    statusMessage = {};

    // Persist the path under apvts.state so the session restores this
    // definition (WTGENERATOR.md section 9.3).
    apvts.state.getOrCreateChildWithName (kExpressionStateTag, nullptr)
               .setProperty (kFilePathProperty, file.getFullPathName(), nullptr);

    if (applyPoolDefaults)
        applyPoolDefaultsFromDefinition();

    sendChangeMessage();                 // editor rebuilds its parameter UI
    return true;
}

juce::String WTGeneratorAudioProcessor::getSignalSourceLabel() const
{
    const int mode = (generatorModePtr != nullptr) ? (int) generatorModePtr->load() : 0;

    // Built-in mode (WTGENERATOR.md section 10.4). Wavetable / Render are v3.
    if (mode == 1)
    {
        juce::String name;
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (
                               apvts.getParameter ("builtInGenerator")))
            name = choice->getCurrentChoiceName();

        return "Built-in generator - " + name;
    }

    // Expression mode.
    if (loadedFile == juce::File())
        return "Built-in expression - Sine";

    return "Expression file - " + loadedFile.getFileName();
}

WTGeneratorAudioProcessor::~WTGeneratorAudioProcessor()
{
}

juce::String WTGeneratorAudioProcessor::poolParamID (int slot)
{
    return "exprParam" + juce::String (slot + 1).paddedLeft ('0', 2);
}

juce::String WTGeneratorAudioProcessor::poolParamName (int slot)
{
    return "Param " + juce::String (slot + 1).paddedLeft ('0', 2);
}

juce::AudioProcessorValueTreeState::ParameterLayout
WTGeneratorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Parameter ORDER is host-visible: hosts list parameters in the order
    // they are added here. The fixed controls - the handful a user touches
    // every session - are added FIRST so they head the host's parameter and
    // automation menus. The 32-slot expression pool is added LAST: it is
    // mostly unused placeholders, and no one should scroll past it to reach
    // a common control. Any regular parameter added in a later version
    // (builtInGenerator in v2, the sweep controls in v4, ...) goes in the
    // fixed block below - never after the pool loop.

    // ---- Fixed controls ----------------------------------------------------

    // Generator mode. WTGENERATOR.md section 4 - the top-level signal-path
    // dispatch. All four values are declared now even though only Expression
    // is functional in v1: Built-in lands in v2, Wavetable / Render in v3.
    // Declaring the full enum up front spares those milestones a
    // parameter-list change and the forced host re-insert it triggers (the
    // new-instance memory).
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "generatorMode", 1 },
        "Generator Mode",
        juce::StringArray { "Expression", "Built-in", "Wavetable", "Render" },
        0));

    // Built-in generator selection. WTGENERATOR.md section 4.4. All 14
    // values declared now; v2 implements the generators that read them.
    // Meaningful only while Generator Mode is Built-in.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "builtInGenerator", 1 },
        "Built-in Generator",
        juce::StringArray { "Sine", "Sine Sweep", "Two-Tone", "Multisine",
                            "Chirp", "Impulse", "Step", "Tone Burst",
                            "White Noise", "Pink Noise", "Brown Noise",
                            "MLS", "DC", "Silence" },
        0));

    // Playback trigger. WTGENERATOR.md section 5.2. All three values are
    // declared now even though MIDI is non-functional until v5 - adding a
    // Choice value later would change the parameter and invalidate host
    // state. Until v5, MIDI is treated as Transport.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "playbackTrigger", 1 },
        "Playback Trigger",
        juce::StringArray { "Transport", "MIDI", "Always" },
        0));

    // Repeat mode. WTGENERATOR.md section 5.3 - how the signal repeats once
    // playback is active. One-Shot fires once per playback start (impulse,
    // step); Periodic re-fires every 1 / periodicRate seconds.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oneShot", 1 },
        "Repeat Mode",
        juce::StringArray { "Loop", "One-Shot", "Periodic" },
        0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "periodicRate", 1 },
        "Periodic Rate (Hz)",
        juce::NormalisableRange<float> (0.1f, 100.0f, 0.0f, 0.30f),
        1.0f));

    // Output gain. WTGENERATOR.md section 5.4. Linear-in-dB range; the
    // minimum doubles as -inf (SignalGenerator maps kOutputGainMinDb to a
    // linear gain of 0).
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (kOutputGainMinDb, kOutputGainMaxDb),
        0.0f));

    // ---- Built-in generator parameters -------------------------------------
    // WTGENERATOR.md section 4.4. Dedicated, named parameters per generator
    // (section 6.1 - "parameters exposed as themselves"). Declared together
    // here, grouped by generator; v2 wires the generators that read them.
    // dB level parameters reuse the Output Gain range (kOutputGainMinDb as
    // -inf). Sweep / Chirp / Multisine take their level from Output Gain and
    // so declare none of their own.
    const juce::NormalisableRange<float> dbRange { kOutputGainMinDb, kOutputGainMaxDb };

    // Sine.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sineFreq", 1 }, "Sine Frequency (Hz)",
        freqRange (20.0f, 20000.0f), 1000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sineLevel", 1 }, "Sine Level (dB)", dbRange, -6.0f));

    // Sine sweep.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sweepStartHz", 1 }, "Sweep Start (Hz)",
        freqRange (20.0f, 20000.0f), 20.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sweepEndHz", 1 }, "Sweep End (Hz)",
        freqRange (20.0f, 20000.0f), 20000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sweepDuration", 1 }, "Sweep Duration (s)",
        juce::NormalisableRange<float> (0.1f, 60.0f), 10.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sweepCurve", 1 }, "Sweep Curve",
        juce::StringArray { "Linear", "Logarithmic" }, 1));

    // Two-tone.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "twoToneF1", 1 }, "Two-Tone f1 (Hz)",
        freqRange (20.0f, 20000.0f), 1000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "twoToneF2", 1 }, "Two-Tone f2 (Hz)",
        freqRange (20.0f, 20000.0f), 1100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "twoToneLevel1", 1 }, "Two-Tone Level 1 (dB)", dbRange, -12.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "twoToneLevel2", 1 }, "Two-Tone Level 2 (dB)", dbRange, -12.0f));

    // Multisine.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "multisineFundamental", 1 }, "Multisine Fundamental (Hz)",
        freqRange (10.0f, 2000.0f), 100.0f));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "multisineMaxHarmonic", 1 }, "Multisine Max Harmonic",
        1, 256, 32));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "multisinePhase", 1 }, "Multisine Phase",
        juce::StringArray { "Zero", "Random", "Schroeder" }, 2));

    // Chirp (Farina).
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chirpStartHz", 1 }, "Chirp Start (Hz)",
        freqRange (20.0f, 20000.0f), 20.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chirpEndHz", 1 }, "Chirp End (Hz)",
        freqRange (20.0f, 20000.0f), 20000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chirpDuration", 1 }, "Chirp Duration (s)",
        juce::NormalisableRange<float> (0.1f, 60.0f), 5.0f));

    // Impulse.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "impulsePolarity", 1 }, "Impulse Polarity",
        juce::StringArray { "Positive", "Negative" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "impulseLevel", 1 }, "Impulse Level (dB)", dbRange, 0.0f));

    // Step.
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "stepRiseTime", 1 }, "Step Rise Time (samples)",
        0, 1024, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stepLevel", 1 }, "Step Level (dB)", dbRange, -6.0f));

    // Tone burst.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstFreq", 1 }, "Burst Frequency (Hz)",
        freqRange (20.0f, 20000.0f), 1000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstLevel", 1 }, "Burst Level (dB)", dbRange, -6.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstAttack", 1 }, "Burst Attack (ms)",
        juce::NormalisableRange<float> (0.0f, 1000.0f), 5.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstDecay", 1 }, "Burst Decay (ms)",
        juce::NormalisableRange<float> (0.0f, 1000.0f), 50.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstSustain", 1 }, "Burst Sustain (dB)",
        juce::NormalisableRange<float> (kOutputGainMinDb, 0.0f), -6.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstRelease", 1 }, "Burst Release (ms)",
        juce::NormalisableRange<float> (0.0f, 1000.0f), 50.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "burstGate", 1 }, "Burst Gate (ms)",
        juce::NormalisableRange<float> (1.0f, 10000.0f), 200.0f));

    // White / pink / brown noise.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "whiteNoiseLevel", 1 }, "White Noise Level (dB)", dbRange, -12.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pinkNoiseLevel", 1 }, "Pink Noise Level (dB)", dbRange, -12.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "brownNoiseLevel", 1 }, "Brown Noise Level (dB)", dbRange, -12.0f));

    // MLS.
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "mlsOrder", 1 }, "MLS Order", 2, 20, 16));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mlsLevel", 1 }, "MLS Level (dB)", dbRange, -12.0f));

    // DC offset.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dcLevel", 1 }, "DC Offset",
        juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));

    // Silence has no parameters.

    // ---- Expression parameter pool (always last) ---------------------------
    // 32 generic 0..1 float slots. Declared once and never changed: the host
    // caches the parameter list per instance, so growing the pool later
    // would invalidate saved sessions (the new-instance memory). An
    // expression parameter maps onto a slot at load time; the engine reads
    // the slot 0..1 and denormalises it through the parameter's [min,max].
    // The slot the host automates as "Param 03" the plugin window labels
    // "03 - mod_index".
    for (int slot = 0; slot < kMaxExpressionParameters; ++slot)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { poolParamID (slot), 1 },
            poolParamName (slot),
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.0f));

    return layout;
}

//==============================================================================
void WTGeneratorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store ((float) sampleRate);
    signalGenerator.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (signalGenerator.getLatencySamples());
    freeRunSamples = 0;
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

    const int    numSamples = buffer.getNumSamples();
    const double sr         = (double) currentSampleRate.load();

    // ---- Playback gate and time base ---------------------------------------
    // playbackTrigger: 0 Transport, 1 MIDI, 2 Always. MIDI is treated as
    // Transport until v5 (WTGENERATOR.md section 5.2).
    constexpr int kAlways = 2;
    const int trigger = (int) playbackTriggerPtr->load();

    // Default true: a host with no playhead, and the Standalone build,
    // free-run rather than fall silent (WTGENERATOR.md section 5.1).
    bool   hostPlaying  = true;
    bool   haveHostTime = false;
    double hostTime     = 0.0;

    if (auto* playHead = getPlayHead())
    {
        if (const auto pos = playHead->getPosition())
        {
            hostPlaying = pos->getIsPlaying();

            if (const auto secs = pos->getTimeInSeconds())
            {
                hostTime     = *secs;
                haveHostTime = true;
            }
        }
    }

    const bool playing = (trigger == kAlways) ? true : hostPlaying;

    // Always mode free-runs its time base so output stays smooth while the
    // transport is stopped. Transport / MIDI ride host time when the host
    // supplies it (sample-accurate, resets on rewind), and fall back to the
    // free-run counter otherwise.
    const bool useFreeRun = (trigger == kAlways) || ! haveHostTime;

    double startTime = 0.0;
    if (playing)
    {
        if (useFreeRun)
        {
            startTime       = (double) freeRunSamples / sr;
            freeRunSamples += numSamples;
        }
        else
        {
            startTime = hostTime;
        }
    }
    else
    {
        freeRunSamples = 0;   // a fresh start begins at t = 0
    }

    // ---- Parameter feed (per block) ----------------------------------------
    // Snapshot the pool slots. DAW automation lands on the APVTS atomics at
    // block boundaries, so a per-block refresh is the honest granularity.
    // SignalGenerator::process feeds these into the active engine itself, so
    // the values always reach the same engine that evaluate() runs.
    float poolValues[kMaxExpressionParameters];
    for (int i = 0; i < kMaxExpressionParameters; ++i)
        poolValues[(size_t) i] = poolParamPtrs[(size_t) i]->load();

    // ---- Render ------------------------------------------------------------
    // SignalGenerator dispatches on generatorMode. Expression mode is live;
    // the Built-in generators fill in across v2; Wavetable / Render in v3.
    const int generatorMode    = (int) generatorModePtr->load();
    const int builtInGenerator = (int) builtInGeneratorPtr->load();

    auto* mono = buffer.getWritePointer (0);
    signalGenerator.process (mono, numSamples, playing, startTime,
                             generatorMode, builtInGenerator,
                             poolValues, kMaxExpressionParameters);

    // ---- Output gain -------------------------------------------------------
    const float gainDb = outputGainPtr->load();
    const float gain   = (gainDb <= kOutputGainMinDb)
                            ? 0.0f
                            : juce::Decibels::decibelsToGain (gainDb);
    juce::FloatVectorOperations::multiply (mono, gain, numSamples);

    // ---- Mono -> all output channels ---------------------------------------
    // The signal is mono; it fills both channels identically (WTGENERATOR.md
    // section 7.5).
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::copy (buffer.getWritePointer (ch), mono, numSamples);
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
    if (! tree.isValid() || ! tree.hasType (apvts.state.getType()))
        return;

    apvts.replaceState (tree);

    // Restore the loaded expression file if the session had one. The pool
    // values were just restored by replaceState, so pass applyPoolDefaults =
    // false: the user's saved parameter values win over the definition's
    // defaults. A missing / invalid file leaves the default expression
    // active and statusMessage explaining why (WTGENERATOR.md section 9.3).
    const auto child = apvts.state.getChildWithName (kExpressionStateTag);
    if (child.isValid())
    {
        const auto path = child.getProperty (kFilePathProperty).toString();
        if (path.isNotEmpty())
            loadExpressionFileInternal (juce::File (path), false);
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WTGeneratorAudioProcessor();
}
