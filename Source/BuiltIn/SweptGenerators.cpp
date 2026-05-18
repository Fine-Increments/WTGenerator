/*
  ==============================================================================

    SweptGenerators.cpp
    See SweptGenerators.h.

  ==============================================================================
*/

#include "SweptGenerators.h"
#include <cmath>

namespace
{
    constexpr double kTwoPi   = 6.283185307179586476925;
    constexpr int    kOneShot = 1;   // Repeat Mode: 0 Loop, 1 One-Shot, 2 Periodic

    // Raised-cosine taper length applied to each end of the Chirp: the lesser
    // of a fixed time and a fraction of the sweep, so a short chirp never
    // tapers more than a tenth of its length end to end.
    constexpr double kChirpFadeSeconds  = 0.015;
    constexpr double kChirpFadeFraction = 0.05;

    // One sample of a swept sine. `pos` is the sweep position in seconds;
    // advances `phase` by the instantaneous frequency. Returns the sample.
    inline float sweepSample (double pos, double dur, double f0, double f1,
                              bool logCurve, double invRate, double& phase) noexcept
    {
        const double u    = juce::jlimit (0.0, 1.0, pos / dur);
        const double freq = logCurve ? f0 * std::pow (f1 / f0, u)
                                      : f0 + (f1 - f0) * u;

        phase += kTwoPi * freq * invRate;
        if (phase >= kTwoPi)
            phase = std::fmod (phase, kTwoPi);

        return (float) std::sin (phase);
    }

    // Raised-cosine envelope: 0 -> 1 over the first `fade` seconds of the
    // sweep, 1 -> 0 over the last `fade`, unity between. Tapering the ends
    // stops the abrupt start / stop from splattering broadband energy across
    // the spectrum - the property a measurement chirp needs and the bare
    // sine sweep does not bother with.
    inline double chirpEnvelope (double pos, double dur, double fade) noexcept
    {
        if (fade <= 0.0)
            return 1.0;

        if (pos < fade)
            return 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * pos / fade);

        if (pos > dur - fade)
            return 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * (dur - pos) / fade);

        return 1.0;
    }
}

//==============================================================================
// Sine Sweep
//==============================================================================
SineSweepGenerator::SineSweepGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    startParam    = apvts.getRawParameterValue ("sweepStartHz");
    endParam      = apvts.getRawParameterValue ("sweepEndHz");
    durationParam = apvts.getRawParameterValue ("sweepDuration");
    curveParam    = apvts.getRawParameterValue ("sweepCurve");
    oneShotParam  = apvts.getRawParameterValue ("oneShot");
}

void SineSweepGenerator::prepare (double rate)
{
    sampleRate = rate;
    reset();
}

void SineSweepGenerator::reset()
{
    phase          = 0.0;
    elapsedSamples = 0.0;
}

void SineSweepGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    const double f0       = (startParam    != nullptr) ? (double) startParam->load()    : 20.0;
    const double f1       = (endParam      != nullptr) ? (double) endParam->load()      : 20000.0;
    const double dur      = juce::jmax (0.01, (durationParam != nullptr)
                                                  ? (double) durationParam->load() : 10.0);
    const bool   logCurve = (curveParam   != nullptr) && ((int) curveParam->load() == 1);
    const bool   oneShot  = (oneShotParam != nullptr) && ((int) oneShotParam->load() == kOneShot);
    const double invRate  = 1.0 / sampleRate;

    for (int j = 0; j < numSamples; ++j)
    {
        double pos = elapsedSamples * invRate;
        elapsedSamples += 1.0;

        if (oneShot && pos >= dur)
        {
            out[j] = 0.0f;          // one-shot sweep finished
            continue;
        }

        if (! oneShot)
            pos = std::fmod (pos, dur);   // repeating sweep

        out[j] = sweepSample (pos, dur, f0, f1, logCurve, invRate, phase);
    }
}

//==============================================================================
// Chirp (Farina log sweep)
//==============================================================================
ChirpGenerator::ChirpGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    startParam    = apvts.getRawParameterValue ("chirpStartHz");
    endParam      = apvts.getRawParameterValue ("chirpEndHz");
    durationParam = apvts.getRawParameterValue ("chirpDuration");
    oneShotParam  = apvts.getRawParameterValue ("oneShot");
}

void ChirpGenerator::prepare (double rate)
{
    sampleRate = rate;
    reset();
}

void ChirpGenerator::reset()
{
    phase          = 0.0;
    elapsedSamples = 0.0;
}

void ChirpGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    const double f0      = (startParam    != nullptr) ? (double) startParam->load()    : 20.0;
    const double f1      = (endParam      != nullptr) ? (double) endParam->load()      : 20000.0;
    const double dur     = juce::jmax (0.01, (durationParam != nullptr)
                                                 ? (double) durationParam->load() : 5.0);
    const bool   oneShot = (oneShotParam != nullptr) && ((int) oneShotParam->load() == kOneShot);
    const double invRate = 1.0 / sampleRate;

    // Taper length, clamped so the two ends never overlap on a short chirp.
    const double fade = juce::jmin (kChirpFadeSeconds, dur * kChirpFadeFraction);

    for (int j = 0; j < numSamples; ++j)
    {
        double pos = elapsedSamples * invRate;
        elapsedSamples += 1.0;

        if (oneShot && pos >= dur)
        {
            out[j] = 0.0f;
            continue;
        }

        if (! oneShot)
            pos = std::fmod (pos, dur);   // each repeat re-tapers in and out

        out[j] = (float) (sweepSample (pos, dur, f0, f1, /*logCurve*/ true, invRate, phase)
                            * chirpEnvelope (pos, dur, fade));
    }
}
