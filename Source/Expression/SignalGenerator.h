/*
  ==============================================================================

    SignalGenerator.h
    Owns the ExpressionEngine and the oversampler, and turns them into a
    stream of audio: holds the active signal definition, (re)compiles on
    prepare / load, evaluates the expression at an integer multiple of the
    session rate, and decimates back down so any discontinuity-rich waveform
    is band-limited (WTGENERATOR.md section 7.4). The processor handles bus
    routing and output gain; SignalGenerator handles only synthesis.

    Threading: loadDefinition() (message thread) compiles a brand-new engine
    off to the side, then publishes it to the audio thread with a single
    atomic store - it never blocks the audio thread or mutates the engine the
    audio thread is using. The superseded engine is held alive one extra
    generation and freed on the next load; since loads are human-paced and
    a load is thousands of audio blocks apart, the audio thread has long
    since stopped referencing it. No locks, no refcounting.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include "ExpressionEngine.h"
#include "ExpressionDefinition.h"

//==============================================================================
class SignalGenerator
{
public:
    SignalGenerator();

    // Message thread. Sets the sample rate, sizes the oversampler for
    // `maxBlockSize`, and recompiles the active definition. Call from
    // prepareToPlay.
    void prepare (double sampleRate, int maxBlockSize);

    // Message thread. Compiles `definition` into a new engine and atomically
    // swaps it in. Returns false on compile failure - in which case the
    // currently running engine is left untouched and getLastError() says why.
    bool loadDefinition (const ExpressionDefinition& definition);

    // Audio thread. Renders `numSamples` of mono signal into `out`. Writes
    // silence when `playing` is false or no expression is compiled.
    // `startTime` is the elapsed playback time (seconds) of out[0].
    // `poolValues` points at `numPoolValues` pool-slot values (0..1); the
    // active engine's declared parameters are fed from the matching slots.
    void process (float* out, int numSamples, bool playing, double startTime,
                  const float* poolValues, int numPoolValues) noexcept;

    // Latency (base-rate samples) the oversampling anti-aliasing filter
    // adds. The processor reports this to the host via setLatencySamples().
    int getLatencySamples() const noexcept;

    // Message thread. The active definition and the last compile diagnostic.
    const ExpressionDefinition& getDefinition() const noexcept { return definition; }
    juce::String getLastError() const noexcept                 { return lastError; }
    bool isReady() const noexcept                              { return current != nullptr; }

private:
    // Compiles `def` into a fresh engine and, on success, publishes it.
    bool installEngine (const ExpressionDefinition& def);

    // 2 ^ 3 = 8x oversampling. WTGENERATOR.md section 7.4 - the factor is an
    // internal constant; the expression is evaluated at this multiple of the
    // session rate, low-pass filtered and decimated, so a square / saw / fast
    // sweep is band-limited rather than aliasing into the measurement.
    static constexpr size_t kOversamplingFactorLog2 = 3;

    // The engine the audio thread reads, published by installEngine().
    std::atomic<ExpressionEngine*> activeEngine { nullptr };

    // Message-thread ownership: `current` owns the published engine,
    // `previous` keeps the engine from one load ago alive until the next
    // load retires it. See the threading note in the file header.
    std::unique_ptr<ExpressionEngine> current;
    std::unique_ptr<ExpressionEngine> previous;

    // Message-thread copy of the active definition - the editor reads it to
    // build the dynamic parameter UI. The audio path never touches it; the
    // engine carries everything the audio thread needs.
    ExpressionDefinition definition;

    double       sampleRate = 48000.0;
    juce::String lastError;

    // Linear-phase half-band FIR oversampler. Linear phase keeps the test
    // signal's phase relationships intact - it matters for a measurement
    // tool. Integer latency so the host gets a clean sample count.
    juce::dsp::Oversampling<float> oversampling
    {
        1,
        kOversamplingFactorLog2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,
        true
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalGenerator)
};
