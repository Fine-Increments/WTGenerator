/*
  ==============================================================================

    SignalGenerator.cpp
    See SignalGenerator.h for the threading contract.

  ==============================================================================
*/

#include "SignalGenerator.h"
#include "DefaultExpression.h"
#include "../BuiltIn/SineGenerator.h"
#include "../BuiltIn/SteadyGenerators.h"
#include "../BuiltIn/SweptGenerators.h"
#include "../BuiltIn/TriggeredGenerators.h"
#include "../BuiltIn/StatefulGenerators.h"

//==============================================================================
SignalGenerator::SignalGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    // Expression mode emits this until the user loads an .xml of their own;
    // it is compiled into an engine by the first prepare() call.
    definition = getDefaultExpression();

    // Built-in generators (WTGENERATOR.md section 4.4), one per builtInGenerator
    // Choice value. Each caches its own APVTS parameter pointers.
    builtInGenerators[ 0] = std::make_unique<SineGenerator>       (apvts);
    builtInGenerators[ 1] = std::make_unique<SineSweepGenerator>  (apvts);
    builtInGenerators[ 2] = std::make_unique<TwoToneGenerator>    (apvts);
    builtInGenerators[ 3] = std::make_unique<MultisineGenerator>  (apvts);
    builtInGenerators[ 4] = std::make_unique<ChirpGenerator>      (apvts);
    builtInGenerators[ 5] = std::make_unique<ImpulseGenerator>    (apvts);
    builtInGenerators[ 6] = std::make_unique<StepGenerator>       (apvts);
    builtInGenerators[ 7] = std::make_unique<ToneBurstGenerator>  (apvts);
    builtInGenerators[ 8] = std::make_unique<WhiteNoiseGenerator> (apvts);
    builtInGenerators[ 9] = std::make_unique<PinkNoiseGenerator>  (apvts);
    builtInGenerators[10] = std::make_unique<BrownNoiseGenerator> (apvts);
    builtInGenerators[11] = std::make_unique<MlsGenerator>        (apvts);
    builtInGenerators[12] = std::make_unique<DcGenerator>         (apvts);
    builtInGenerators[13] = std::make_unique<SilenceGenerator>    ();
}

void SignalGenerator::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    oversampling.initProcessing ((size_t) juce::jmax (1, maxBlockSize));
    oversampling.reset();

    // Recompile the active definition at the new rate. prepareToPlay is not
    // concurrent with processBlock, so the atomic swap here is uncontended.
    installEngine (definition);

    // The built-in generators render straight at the session rate - each is
    // band-limited by construction, so they bypass the oversampler entirely.
    for (auto& generator : builtInGenerators)
        if (generator != nullptr)
            generator->prepare (sampleRate);
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
                               double startTime, int generatorMode, int builtInGenerator,
                               const float* poolValues, int numPoolValues) noexcept
{
    // Expression mode oversamples - it evaluates arbitrary math and cannot
    // know whether the user wrote a discontinuity-rich waveform. The built-in
    // generators are purpose-written and band-limited by construction, so
    // they render straight at the session rate. Each path owns its own rate
    // handling and writes `out` directly; the two ...Active flags let a mode
    // switch (or a stop) re-prime / re-reset the path resumed into.
    constexpr int kExpression = 0;
    constexpr int kBuiltIn    = 1;

    if (playing && generatorMode == kExpression)
    {
        builtInActive = false;
        renderExpression (out, numSamples, startTime, poolValues, numPoolValues);
    }
    else if (playing && generatorMode == kBuiltIn)
    {
        expressionActive = false;
        renderBuiltIn (out, numSamples, startTime, builtInGenerator);
    }
    else
    {
        juce::FloatVectorOperations::clear (out, numSamples);
        expressionActive = false;
        builtInActive    = false;
    }
}

void SignalGenerator::renderExpression (float* out, int numSamples, double startTime,
                                        const float* poolValues, int numPoolValues) noexcept
{
    auto* engine = activeEngine.load (std::memory_order_acquire);

    // No engine published yet -> silence (and skip the oversampler).
    if (engine == nullptr || ! engine->isCompiled() || numSamples <= 0)
    {
        juce::FloatVectorOperations::clear (out, juce::jmax (0, numSamples));
        lastEngine       = engine;
        expressionActive = false;
        return;
    }

    // A changed engine pointer (a definition load), or the expression path
    // not having rendered the previous block (playback restart, or a switch
    // back from another generator mode), breaks the parameter history -
    // prime paramPrev to the host values rather than ramping from a stale
    // baseline.
    // A transport rewind / loop jumps startTime backwards while the expression
    // path stays active; the engine already resets its phasors on the same
    // backward step, so the FIR must be reset too or the first filter-length
    // samples of each loop blend the pre-rewind signal into the new one (an
    // audible click, and non-reproducible-from-start output).
    const bool rewound = expressionActive && startTime < lastExprStartTime;

    const bool primeParams = (engine != lastEngine) || ! expressionActive || rewound;

    // The oversampler's FIR was not fed while another mode (or silence) was
    // active - reset it so the resumed expression carries no stale state.
    if (! expressionActive || rewound)
        oversampling.reset();

    lastEngine        = engine;
    expressionActive  = true;
    lastExprStartTime = startTime;

    // Oversample up, evaluate the expression into the oversampled buffer,
    // then decimate back down. The expression engine evaluates arbitrary
    // math, so its output is band-limited by oversampling (section 7.4).
    float* channelPtrs[1] = { out };
    juce::dsp::AudioBlock<float> ioBlock (channelPtrs, 1, (size_t) numSamples);

    auto      osBlock      = oversampling.processSamplesUp (ioBlock);
    float*    os           = osBlock.getChannelPointer (0);
    const int osNumSamples = (int) osBlock.getNumSamples();

    if (osNumSamples > 0)
    {
        const int numParams = juce::jmin (engine->getNumParameters(), numPoolValues);

        if (primeParams)
            for (int i = 0; i < numParams; ++i)
                paramPrev[(size_t) i] = poolValues[i];

        const double osRate     = sampleRate * (double) oversampling.getOversamplingFactor();
        const double invOsRate  = 1.0 / osRate;
        const double invOsCount = 1.0 / (double) osNumSamples;

        // Linear-interpolate each declared parameter from its previous-block
        // value toward the host's current value across the block, so
        // automation reads as a smooth ramp rather than a once-per-block
        // staircase. invOsRate doubles as the per-sample time step the engine
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

            // Sanitize the expression result before it enters the decimation
            // FIR: a legal-but-singular expression (log(0), 1/(t-1), tan near
            // pi/2) can yield Inf/NaN for one sample, and an unfiltered Inf/NaN
            // would poison every FIR tap it touches - a burst of garbage output
            // long after the expression itself recovered. Clamp to a sane range.
            const double v = engine->evaluate (startTime + (double) j * invOsRate, invOsRate);
            os[j] = std::isfinite (v) ? (float) juce::jlimit (-4.0, 4.0, v) : 0.0f;
        }

        for (int i = 0; i < numParams; ++i)
            paramPrev[(size_t) i] = poolValues[i];
    }

    oversampling.processSamplesDown (ioBlock);
}

void SignalGenerator::renderBuiltIn (float* out, int numSamples, double startTime,
                                     int generatorIndex) noexcept
{
    BuiltInGenerator* generator =
        (generatorIndex >= 0 && generatorIndex < (int) builtInGenerators.size())
            ? builtInGenerators[(size_t) generatorIndex].get()
            : nullptr;

    // An unimplemented generator slot (or an out-of-range index) is silence.
    if (generator == nullptr || numSamples <= 0)
    {
        juce::FloatVectorOperations::clear (out, juce::jmax (0, numSamples));
        builtInActive = false;
        return;
    }

    // Reset the generator on a fresh start - the first built-in block, a
    // generator change, or a transport rewind (startTime jumped backwards).
    const bool freshStart = ! builtInActive
                         || generatorIndex != lastBuiltInGenerator
                         || startTime < lastBuiltInStartTime;
    if (freshStart)
        generator->reset();

    builtInActive        = true;
    lastBuiltInGenerator = generatorIndex;
    lastBuiltInStartTime = startTime;

    // Built-in generators render straight at the session rate - no oversampler.
    generator->render (out, numSamples, startTime);
}
