/*
  ==============================================================================

    SineGenerator.cpp
    See SineGenerator.h.

  ==============================================================================
*/

#include "SineGenerator.h"
#include <cmath>

namespace { constexpr double kTwoPi = 6.283185307179586476925; }

//==============================================================================
SineGenerator::SineGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    freqParam  = apvts.getRawParameterValue ("sineFreq");
    levelParam = apvts.getRawParameterValue ("sineLevel");
}

float SineGenerator::currentGain() const noexcept
{
    // sineLevel is dB; kOutputGainMinDb (-100) maps to a linear gain of 0.
    const float db = (levelParam != nullptr) ? levelParam->load() : -6.0f;
    return (float) juce::Decibels::decibelsToGain (db, -100.0f);
}

void SineGenerator::prepare (double rate)
{
    sampleRate = rate;
    reset();
}

void SineGenerator::reset()
{
    phase = 0.0;
    freqRamp.reset ((freqParam != nullptr) ? freqParam->load() : 1000.0f);
    gainRamp.reset (currentGain());
}

void SineGenerator::render (float* out, int numSamples, double /*startTime*/) noexcept
{
    if (numSamples <= 0)
        return;

    freqRamp.setTarget ((freqParam != nullptr) ? freqParam->load() : 1000.0f);
    gainRamp.setTarget (currentGain());

    const double invRate = 1.0 / sampleRate;
    const double invN    = 1.0 / (double) numSamples;

    for (int j = 0; j < numSamples; ++j)
    {
        const double frac = (double) (j + 1) * invN;

        // Integrated phase: changing the frequency steps the rate, never
        // the phase, so an automated frequency is click-free.
        phase += kTwoPi * (double) freqRamp.at (frac) * invRate;
        if (phase >= kTwoPi)
            phase = std::fmod (phase, kTwoPi);

        out[j] = (float) (std::sin (phase) * (double) gainRamp.at (frac));
    }

    freqRamp.finishBlock();
    gainRamp.finishBlock();
}
