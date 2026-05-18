/*
  ==============================================================================

    SineGenerator.h
    Built-in sine generator (WTGENERATOR.md section 4.4) - frequency and
    level, both automatable. The phase is integrated, so sweeping the
    frequency is click-free; the parameters are per-block linear-ramped, so
    a swept frequency glides smoothly.

  ==============================================================================
*/

#pragma once

#include "BuiltInGenerator.h"

//==============================================================================
class SineGenerator  : public BuiltInGenerator
{
public:
    explicit SineGenerator (juce::AudioProcessorValueTreeState& apvts);

    void prepare (double sampleRate) override;
    void reset() override;
    void render (float* out, int numSamples, double startTime) noexcept override;

private:
    float currentGain() const noexcept;   // sineLevel (dB) -> linear

    std::atomic<float>* freqParam  = nullptr;   // "sineFreq"  (Hz)
    std::atomic<float>* levelParam = nullptr;   // "sineLevel" (dB)

    double    sampleRate = 48000.0;
    double    phase      = 0.0;                 // radians, 0 .. 2*pi
    ParamRamp freqRamp;
    ParamRamp gainRamp;
};
