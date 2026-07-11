/*
  ==============================================================================

    TriggeredGenerators.cpp
    See TriggeredGenerators.h.

  ==============================================================================
*/

#include "TriggeredGenerators.h"
#include <cmath>
#include <limits>

namespace
{
    constexpr double kTwoPi   = 6.283185307179586476925;
    constexpr int    kOneShot = 1;   // Repeat Mode: 0 Loop, 1 One-Shot, 2 Periodic

    // Repeat period in samples for Loop / Periodic mode.
    inline juce::int64 repeatPeriod (const std::atomic<float>* rateParam,
                                     double sampleRate) noexcept
    {
        const double rate = juce::jmax (0.01, (rateParam != nullptr)
                                                  ? (double) rateParam->load() : 1.0);
        return (juce::int64) juce::jmax (2.0, sampleRate / rate);
    }
}

//==============================================================================
// Impulse
//==============================================================================
ImpulseGenerator::ImpulseGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    polarityParam = apvts.getRawParameterValue ("impulsePolarity");
    levelParam    = apvts.getRawParameterValue ("impulseLevel");
    oneShotParam  = apvts.getRawParameterValue ("oneShot");
    periodicParam = apvts.getRawParameterValue ("periodicRate");
}

void ImpulseGenerator::prepare (double rate) { sampleRate = rate; reset(); }
void ImpulseGenerator::reset()               { counter = 0; }

void ImpulseGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    const double level = juce::Decibels::decibelsToGain (
                             (levelParam != nullptr) ? levelParam->load() : 0.0f, -100.0f);
    const double sign  = (polarityParam != nullptr && (int) polarityParam->load() == 1)
                            ? -1.0 : 1.0;
    const bool   oneShot = (oneShotParam != nullptr) && ((int) oneShotParam->load() == kOneShot);
    const juce::int64 period = repeatPeriod (periodicParam, sampleRate);

    const float spike = (float) (sign * level);

    for (int j = 0; j < numSamples; ++j)
    {
        const bool trigger = oneShot ? (counter == 0)
                                     : (counter % period == 0);
        out[j] = trigger ? spike : 0.0f;
        ++counter;
    }
}

//==============================================================================
// Step
//==============================================================================
StepGenerator::StepGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    riseTimeParam = apvts.getRawParameterValue ("stepRiseTime");
    levelParam    = apvts.getRawParameterValue ("stepLevel");
    oneShotParam  = apvts.getRawParameterValue ("oneShot");
    periodicParam = apvts.getRawParameterValue ("periodicRate");
}

void StepGenerator::prepare (double rate) { sampleRate = rate; reset(); }
void StepGenerator::reset()               { counter = 0; currentValue = 0.0; }

void StepGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    const double level = juce::Decibels::decibelsToGain (
                             (levelParam != nullptr) ? levelParam->load() : -6.0f, -100.0f);
    const double riseMs = juce::jmax (0.0, (riseTimeParam != nullptr)
                                               ? (double) riseTimeParam->load() : 0.0);
    const bool   oneShot = (oneShotParam != nullptr) && ((int) oneShotParam->load() == kOneShot);
    const juce::int64 period = repeatPeriod (periodicParam, sampleRate);
    const juce::int64 halfPeriod = juce::jmax ((juce::int64) 1, period / 2);

    // Rise Time is in milliseconds, so the edge is the same length at any
    // session rate. In Loop / Periodic mode an edge longer than the half-
    // period would never reach the rails - the square would sag into a low
    // triangle - so clamp it to complete within the half-period.
    double rise = riseMs * sampleRate / 1000.0;
    if (! oneShot)
        rise = juce::jmin (rise, (double) halfPeriod);

    // Per-sample slew so the edge takes `rise` samples; rise 0 is an instant
    // step.
    const double slew = (rise > 0.0) ? level / rise : level;

    for (int j = 0; j < numSamples; ++j)
    {
        double target = level;
        if (! oneShot)
        {
            const bool high = ((counter / halfPeriod) % 2) == 0;
            target = high ? level : 0.0;
        }

        if (rise <= 0.0)
            currentValue = target;
        else if (currentValue < target)
            currentValue = juce::jmin (target, currentValue + slew);
        else if (currentValue > target)
            currentValue = juce::jmax (target, currentValue - slew);

        out[j] = (float) currentValue;
        ++counter;
    }
}

//==============================================================================
// Tone Burst
//==============================================================================
ToneBurstGenerator::ToneBurstGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    freqParam     = apvts.getRawParameterValue ("burstFreq");
    levelParam    = apvts.getRawParameterValue ("burstLevel");
    attackParam   = apvts.getRawParameterValue ("burstAttack");
    decayParam    = apvts.getRawParameterValue ("burstDecay");
    sustainParam  = apvts.getRawParameterValue ("burstSustain");
    releaseParam  = apvts.getRawParameterValue ("burstRelease");
    gateParam     = apvts.getRawParameterValue ("burstGate");
    oneShotParam  = apvts.getRawParameterValue ("oneShot");
    periodicParam = apvts.getRawParameterValue ("periodicRate");
}

void ToneBurstGenerator::prepare (double rate) { sampleRate = rate; reset(); }
void ToneBurstGenerator::reset()               { phase = 0.0; counter = 0; }

void ToneBurstGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    const double freq = (freqParam  != nullptr) ? (double) freqParam->load()  : 1000.0;
    const double peak = juce::Decibels::decibelsToGain (
                            (levelParam != nullptr) ? levelParam->load() : -6.0f, -100.0f);
    const double sus  = juce::Decibels::decibelsToGain (
                            (sustainParam != nullptr) ? sustainParam->load() : -6.0f, -100.0f);

    const double msToSamples = sampleRate / 1000.0;
    // Floor the attack to one sample: a literal 0 ms attack would jump the
    // envelope straight to full amplitude on the burst's first sample - a click
    // on the continuous-phase sine. One sample of ramp is inaudible but declicks.
    const double aLen = juce::jmax (1.0, ((attackParam != nullptr) ? (double) attackParam->load() : 5.0) * msToSamples);
    const double dLen = juce::jmax (0.0, (decayParam   != nullptr) ? (double) decayParam->load()   : 50.0) * msToSamples;
    const double gLen = juce::jmax (0.0, (gateParam    != nullptr) ? (double) gateParam->load()    : 200.0) * msToSamples;
    const double rLen = juce::jmax (0.0, (releaseParam != nullptr) ? (double) releaseParam->load() : 50.0) * msToSamples;
    const double burstLen = aLen + dLen + gLen + rLen;

    const bool oneShot = (oneShotParam != nullptr) && ((int) oneShotParam->load() == kOneShot);
    const juce::int64 period = oneShot
        ? std::numeric_limits<juce::int64>::max()
        : juce::jmax ((juce::int64) (burstLen + 1.0), repeatPeriod (periodicParam, sampleRate));

    const double invRate = 1.0 / sampleRate;

    for (int j = 0; j < numSamples; ++j)
    {
        // Position within the current burst.
        const double pos = (double) (oneShot ? counter : (counter % period));

        // ADSR envelope: attack 0->1, decay 1->sustain, hold, release ->0.
        double env = 0.0;
        if (pos < aLen)
            env = (aLen > 0.0) ? pos / aLen : 1.0;
        else if (pos < aLen + dLen)
            env = (dLen > 0.0) ? 1.0 + (sus - 1.0) * (pos - aLen) / dLen : sus;
        else if (pos < aLen + dLen + gLen)
            env = sus;
        else if (pos < burstLen)
            env = (rLen > 0.0) ? sus * (1.0 - (pos - aLen - dLen - gLen) / rLen) : 0.0;
        // else env stays 0 - the gap before the next burst

        phase += kTwoPi * freq * invRate;
        if (phase >= kTwoPi)
            phase = std::fmod (phase, kTwoPi);

        out[j] = (float) (std::sin (phase) * peak * env);
        ++counter;
    }
}
