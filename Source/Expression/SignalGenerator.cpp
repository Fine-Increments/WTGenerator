/*
  ==============================================================================

    SignalGenerator.cpp
    See SignalGenerator.h for the threading contract.

  ==============================================================================
*/

#include "SignalGenerator.h"
#include "DefaultExpression.h"

//==============================================================================
SignalGenerator::SignalGenerator()
{
    // The plugin emits this until the user loads an .xml of their own. It is
    // compiled into an engine by the first prepare() call.
    definition = getDefaultExpression();
}

void SignalGenerator::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    oversampling.initProcessing ((size_t) juce::jmax (1, maxBlockSize));
    oversampling.reset();

    // Recompile the active definition at the new rate. prepareToPlay is not
    // concurrent with processBlock, so the atomic swap here is uncontended.
    installEngine (definition);
}

bool SignalGenerator::loadDefinition (const ExpressionDefinition& newDefinition)
{
    if (! installEngine (newDefinition))
        return false;

    definition = newDefinition;   // adopt only on a successful compile
    return true;
}

bool SignalGenerator::installEngine (const ExpressionDefinition& def)
{
    // Compile into a fresh engine, fully, before the audio thread can see it.
    auto next = std::make_unique<ExpressionEngine>();

    if (! next->compile (def, sampleRate))
    {
        lastError = juce::String (next->getLastError());
        return false;   // leave the running engine in place
    }

    lastError.clear();

    // Retire the engine from one load ago (untouched by the audio thread
    // since the previous load) and publish the new one. The release store
    // pairs with the audio thread's acquire load in process().
    previous = std::move (current);
    current  = std::move (next);
    activeEngine.store (current.get(), std::memory_order_release);
    return true;
}

int SignalGenerator::getLatencySamples() const noexcept
{
    return juce::roundToInt (oversampling.getLatencyInSamples());
}

//==============================================================================
void SignalGenerator::process (float* out, int numSamples, bool playing,
                               double startTime,
                               const float* poolValues, int numPoolValues) noexcept
{
    auto* engine = activeEngine.load (std::memory_order_acquire);

    // No engine published yet -> hard silence, and skip the oversampler so
    // its filter state stays clean for the first valid definition.
    if (engine == nullptr || ! engine->isCompiled())
    {
        juce::FloatVectorOperations::clear (out, numSamples);
        lastEngine = engine;
        wasPlaying = false;
        return;
    }

    // A changed engine pointer (a definition load) or a playback restart
    // breaks the parameter history: prime paramPrev to the host values
    // below rather than ramping the parameters from a stale baseline.
    const bool primeParams = (engine != lastEngine) || ! wasPlaying;
    lastEngine = engine;

    // Generate at the oversampled rate, then decimate. processSamplesUp
    // returns an AudioBlock over the oversampler's internal buffer; its
    // upsampled contents are irrelevant here - we overwrite them with the
    // expression evaluated at the oversampled rate - and processSamplesDown
    // reads that same buffer back down, applying the anti-imaging FIR.
    // Running it every block (silence included) keeps the FIR state
    // continuous, so transport start / stop has no filter transient.
    float* channelPtrs[1] = { out };
    juce::dsp::AudioBlock<float> ioBlock (channelPtrs, 1, (size_t) numSamples);

    auto      osBlock      = oversampling.processSamplesUp (ioBlock);
    float*    os           = osBlock.getChannelPointer (0);
    const int osNumSamples = (int) osBlock.getNumSamples();

    if (playing && osNumSamples > 0)
    {
        const int numParams = juce::jmin (engine->getNumParameters(), numPoolValues);

        // Prime the smoother baseline on the first block of a new engine or
        // a fresh playback start - that block holds steady at the host
        // values instead of ramping.
        if (primeParams)
            for (int i = 0; i < numParams; ++i)
                paramPrev[(size_t) i] = poolValues[i];

        const double osRate     = sampleRate * (double) oversampling.getOversamplingFactor();
        const double invOsRate  = 1.0 / osRate;
        const double invOsCount = 1.0 / (double) osNumSamples;

        // Linear-interpolate each declared parameter from its previous-block
        // value toward the host's current value across the block, so
        // automation reads as a smooth ramp rather than a once-per-block
        // staircase. The parameters reach the host value at the block's last
        // sample. invOsRate doubles as the per-sample time step the engine
        // integrates phasors against.
        for (int j = 0; j < osNumSamples; ++j)
        {
            const double frac = (double) (j + 1) * invOsCount;

            for (int i = 0; i < numParams; ++i)
            {
                const double prev = (double) paramPrev[(size_t) i];
                const double curr = (double) poolValues[i];
                engine->setParameterNormalised (i, prev + (curr - prev) * frac);
            }

            os[j] = (float) engine->evaluate (startTime + (double) j * invOsRate, invOsRate);
        }

        for (int i = 0; i < numParams; ++i)
            paramPrev[(size_t) i] = poolValues[i];
    }
    else
    {
        juce::FloatVectorOperations::clear (os, osNumSamples);
    }

    wasPlaying = playing;
    oversampling.processSamplesDown (ioBlock);
}
