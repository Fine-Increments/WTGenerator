/*
  ==============================================================================

    SteadyGenerators.h
    The steady (continuous, non-triggered) built-in generators:
    Two-Tone, Multisine, DC offset and Silence (WTGENERATOR.md section 4.4).
    Sine itself lives in SineGenerator.h.

  ==============================================================================
*/

#pragma once

#include "BuiltInGenerator.h"
#include <array>

//==============================================================================
// Two independent sine partials summed - the IMD two-tone stimulus.
class TwoToneGenerator  : public BuiltInGenerator
{
public:
    explicit TwoToneGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* f1Param     = nullptr;
    std::atomic<float>* f2Param     = nullptr;
    std::atomic<float>* level1Param = nullptr;
    std::atomic<float>* level2Param = nullptr;

    double    sampleRate = 48000.0;
    double    phase1     = 0.0;
    double    phase2     = 0.0;
    ParamRamp freq1Ramp, freq2Ramp, gain1Ramp, gain2Ramp;
};

//==============================================================================
// A harmonic series 1..maxHarmonic of a fundamental, each partial at a phase
// set by the phase scheme (Zero / Random / Schroeder). Harmonics above
// Nyquist are dropped, so the signal is band-limited by construction.
class MultisineGenerator  : public BuiltInGenerator
{
public:
    static constexpr int kMaxHarmonics = 256;

    explicit MultisineGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    // Builds the per-harmonic phase offsets for `scheme`. The offsets do NOT
    // depend on the harmonic count - Schroeder uses the full kMaxHarmonics -
    // so changing the count never shifts the existing partials.
    void rebuildPhases (int scheme);

    std::atomic<float>* fundamentalParam = nullptr;
    std::atomic<float>* maxHarmonicParam = nullptr;
    std::atomic<float>* phaseSchemeParam = nullptr;

    double    sampleRate = 48000.0;
    double    phase      = 0.0;            // fundamental phasor
    ParamRamp freqRamp;
    ParamRamp countRamp;                   // smoothed harmonic count

    std::array<double, kMaxHarmonics> harmonicPhase {};   // per-harmonic offsets
    int       builtScheme = -1;
};

//==============================================================================
// A constant DC offset.
class DcGenerator  : public BuiltInGenerator
{
public:
    explicit DcGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* levelParam = nullptr;   // "dcLevel" (linear -1..1)
    ParamRamp levelRamp;
};

//==============================================================================
// Silence - the noise-floor reference (WTGENERATOR.md section 4.4).
class SilenceGenerator  : public BuiltInGenerator
{
public:
    void prepare (double) override {}
    void reset() override {}
    void render (float* out, int numSamples, double) noexcept override
    {
        juce::FloatVectorOperations::clear (out, juce::jmax (0, numSamples));
    }
};
