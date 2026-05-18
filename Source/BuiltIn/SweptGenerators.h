/*
  ==============================================================================

    SweptGenerators.h
    The swept built-in generators (WTGENERATOR.md section 4.4): the Sine Sweep
    (linear or logarithmic) and the Farina log Chirp. Both integrate the phase
    per sample, so changing a parameter mid-sweep never clicks; the sweep
    position is driven by an internal sample counter, reset on playback start.

  ==============================================================================
*/

#pragma once

#include "BuiltInGenerator.h"

//==============================================================================
// A sine sweeping start -> end Hz over `duration`, linear or log. With Repeat
// Mode = One-Shot it sweeps once then falls silent; otherwise it repeats.
class SineSweepGenerator  : public BuiltInGenerator
{
public:
    explicit SineSweepGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* startParam    = nullptr;
    std::atomic<float>* endParam      = nullptr;
    std::atomic<float>* durationParam = nullptr;
    std::atomic<float>* curveParam    = nullptr;   // 0 Linear, 1 Log
    std::atomic<float>* oneShotParam  = nullptr;   // Repeat Mode

    double sampleRate     = 48000.0;
    double phase          = 0.0;
    double elapsedSamples = 0.0;
};

//==============================================================================
// A Farina log chirp - the swept stimulus for impulse-response deconvolution.
// Always logarithmic, and raised-cosine tapered at both ends so the start /
// stop transients do not splatter broadband energy across the spectrum. That
// end taper is what distinguishes it from a plain logarithmic sine sweep.
class ChirpGenerator  : public BuiltInGenerator
{
public:
    explicit ChirpGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    std::atomic<float>* startParam    = nullptr;
    std::atomic<float>* endParam      = nullptr;
    std::atomic<float>* durationParam = nullptr;
    std::atomic<float>* oneShotParam  = nullptr;

    double sampleRate     = 48000.0;
    double phase          = 0.0;
    double elapsedSamples = 0.0;
};
