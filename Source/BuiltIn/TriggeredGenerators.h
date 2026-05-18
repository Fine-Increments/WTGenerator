/*
  ==============================================================================

    TriggeredGenerators.h
    The triggered built-in generators (WTGENERATOR.md sections 4.4, 5.3):
    Impulse, Step and Tone Burst. Each watches the Repeat Mode parameter -
    One-Shot fires once per playback start; Loop / Periodic re-fire every
    1 / periodicRate seconds. An internal sample counter, reset on playback
    start, drives the timing.

  ==============================================================================
*/

#pragma once

#include "BuiltInGenerator.h"

//==============================================================================
// A single-sample impulse - One-Shot fires one spike at playback start;
// Loop / Periodic emit a spike every 1 / periodicRate seconds.
class ImpulseGenerator  : public BuiltInGenerator
{
public:
    explicit ImpulseGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* polarityParam = nullptr;   // 0 Positive, 1 Negative
    std::atomic<float>* levelParam    = nullptr;   // dB
    std::atomic<float>* oneShotParam  = nullptr;
    std::atomic<float>* periodicParam = nullptr;   // periodicRate (Hz)

    double      sampleRate = 48000.0;
    juce::int64 counter     = 0;
};

//==============================================================================
// A step: output rises 0 -> level over `riseTime` samples. One-Shot rises
// once and holds; Loop / Periodic alternate up / down every half period.
class StepGenerator  : public BuiltInGenerator
{
public:
    explicit StepGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* riseTimeParam = nullptr;   // samples
    std::atomic<float>* levelParam    = nullptr;   // dB
    std::atomic<float>* oneShotParam  = nullptr;
    std::atomic<float>* periodicParam = nullptr;

    double      sampleRate   = 48000.0;
    juce::int64 counter      = 0;
    double      currentValue = 0.0;
};

//==============================================================================
// A sine gated by an ADSR envelope. One-Shot plays one burst; Loop / Periodic
// re-fire the envelope every 1 / periodicRate seconds.
class ToneBurstGenerator  : public BuiltInGenerator
{
public:
    explicit ToneBurstGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* freqParam    = nullptr;
    std::atomic<float>* levelParam   = nullptr;   // dB - burst peak
    std::atomic<float>* attackParam  = nullptr;   // ms
    std::atomic<float>* decayParam   = nullptr;   // ms
    std::atomic<float>* sustainParam = nullptr;   // dB - sustain level
    std::atomic<float>* releaseParam = nullptr;   // ms
    std::atomic<float>* gateParam    = nullptr;   // ms - hold at sustain
    std::atomic<float>* oneShotParam = nullptr;
    std::atomic<float>* periodicParam = nullptr;

    double      sampleRate = 48000.0;
    double      phase      = 0.0;
    juce::int64 counter    = 0;
};
