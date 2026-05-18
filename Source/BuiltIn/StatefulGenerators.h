/*
  ==============================================================================

    StatefulGenerators.h
    The non-closed-form built-in generators (WTGENERATOR.md section 4.4):
    White / Pink / Brown noise and MLS. These are base-rate-native - defined
    as sample sequences, with running PRNG / filter / LFSR state - so they
    render straight at the session rate and never go through the oversampler.

  ==============================================================================
*/

#pragma once

#include "BuiltInGenerator.h"
#include <cstdint>

//==============================================================================
// Uniform white noise.
class WhiteNoiseGenerator  : public BuiltInGenerator
{
public:
    explicit WhiteNoiseGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* levelParam = nullptr;   // dB
    std::uint32_t rngState = 1u;
    ParamRamp     gainRamp;
};

//==============================================================================
// Pink noise (-3 dB/octave) - white through the Paul Kellet pinking filter.
class PinkNoiseGenerator  : public BuiltInGenerator
{
public:
    explicit PinkNoiseGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* levelParam = nullptr;
    std::uint32_t rngState = 1u;
    double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;   // filter state
    ParamRamp gainRamp;
};

//==============================================================================
// Brown noise (-6 dB/octave) - white through a leaky integrator.
class BrownNoiseGenerator  : public BuiltInGenerator
{
public:
    explicit BrownNoiseGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* levelParam = nullptr;
    std::uint32_t rngState = 1u;
    double    brown = 0.0;
    ParamRamp gainRamp;
};

//==============================================================================
// Maximum-length sequence - a deterministic broadband stimulus. An LFSR of
// the chosen order emits one chip (+/-1) per sample; the sequence repeats
// every 2^order - 1 samples.
class MlsGenerator  : public BuiltInGenerator
{
public:
    explicit MlsGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* orderParam = nullptr;   // Int, 2..20
    std::atomic<float>* levelParam = nullptr;   // dB
    std::uint32_t lfsr = 1u;
    ParamRamp     gainRamp;
};
