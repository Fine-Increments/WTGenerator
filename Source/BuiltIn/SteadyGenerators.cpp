/*
  ==============================================================================

    SteadyGenerators.cpp
    See SteadyGenerators.h.

  ==============================================================================
*/

#include "SteadyGenerators.h"
#include <cmath>
#include <cstdint>

namespace { constexpr double kTwoPi = 6.283185307179586476925; }

//==============================================================================
// Two-Tone
//==============================================================================
TwoToneGenerator::TwoToneGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    f1Param     = apvts.getRawParameterValue ("twoToneF1");
    f2Param     = apvts.getRawParameterValue ("twoToneF2");
    level1Param = apvts.getRawParameterValue ("twoToneLevel1");
    level2Param = apvts.getRawParameterValue ("twoToneLevel2");
}

void TwoToneGenerator::prepare (double rate)
{
    sampleRate = rate;
    reset();
}

void TwoToneGenerator::reset()
{
    phase1 = phase2 = 0.0;
    freq1Ramp.reset ((f1Param != nullptr) ? f1Param->load() : 1000.0f);
    freq2Ramp.reset ((f2Param != nullptr) ? f2Param->load() : 1100.0f);
    gain1Ramp.reset ((float) juce::Decibels::decibelsToGain (
                         (level1Param != nullptr) ? level1Param->load() : -12.0f, -100.0f));
    gain2Ramp.reset ((float) juce::Decibels::decibelsToGain (
                         (level2Param != nullptr) ? level2Param->load() : -12.0f, -100.0f));
}

void TwoToneGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    freq1Ramp.setTarget ((f1Param != nullptr) ? f1Param->load() : 1000.0f);
    freq2Ramp.setTarget ((f2Param != nullptr) ? f2Param->load() : 1100.0f);
    gain1Ramp.setTarget ((float) juce::Decibels::decibelsToGain (
                            (level1Param != nullptr) ? level1Param->load() : -12.0f, -100.0f));
    gain2Ramp.setTarget ((float) juce::Decibels::decibelsToGain (
                            (level2Param != nullptr) ? level2Param->load() : -12.0f, -100.0f));

    const double invRate = 1.0 / sampleRate;
    const double invN    = 1.0 / (double) numSamples;

    for (int j = 0; j < numSamples; ++j)
    {
        const double frac = (double) (j + 1) * invN;

        phase1 += kTwoPi * (double) freq1Ramp.at (frac) * invRate;
        phase2 += kTwoPi * (double) freq2Ramp.at (frac) * invRate;
        if (phase1 >= kTwoPi) phase1 = std::fmod (phase1, kTwoPi);
        if (phase2 >= kTwoPi) phase2 = std::fmod (phase2, kTwoPi);

        out[j] = (float) (std::sin (phase1) * (double) gain1Ramp.at (frac)
                        + std::sin (phase2) * (double) gain2Ramp.at (frac));
    }

    freq1Ramp.finishBlock(); freq2Ramp.finishBlock();
    gain1Ramp.finishBlock(); gain2Ramp.finishBlock();
}

//==============================================================================
// Multisine
//==============================================================================
MultisineGenerator::MultisineGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    fundamentalParam = apvts.getRawParameterValue ("multisineFundamental");
    maxHarmonicParam = apvts.getRawParameterValue ("multisineMaxHarmonic");
    phaseSchemeParam = apvts.getRawParameterValue ("multisinePhase");
}

void MultisineGenerator::prepare (double rate)
{
    sampleRate = rate;
    reset();
}

void MultisineGenerator::rebuildPhases (int scheme)
{
    // scheme: 0 Zero, 1 Random, 2 Schroeder. The offsets are computed for the
    // full kMaxHarmonics set and never depend on the active harmonic count, so
    // changing Max Harmonic leaves every existing partial's phase untouched.
    if (scheme == 1)
    {
        // Reproducible random phases (fixed seed).
        std::uint32_t s = 0x9E3779B9u;
        for (int k = 0; k < kMaxHarmonics; ++k)
        {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            harmonicPhase[(size_t) k] = (double) s * (kTwoPi / 4294967296.0);
        }
    }
    else if (scheme == 2)
    {
        // Schroeder phases - low crest factor. phase_k = -pi * k * (k-1) / N,
        // with N fixed at kMaxHarmonics so the count never shifts a partial.
        for (int k = 0; k < kMaxHarmonics; ++k)
        {
            const double h = (double) (k + 1);
            harmonicPhase[(size_t) k] = std::fmod (
                -juce::MathConstants<double>::pi * h * (h - 1.0) / (double) kMaxHarmonics,
                kTwoPi);
        }
    }
    else
    {
        harmonicPhase.fill (0.0);   // Zero - all partials in phase
    }

    builtScheme = scheme;
}

void MultisineGenerator::reset()
{
    phase = 0.0;
    freqRamp.reset ((fundamentalParam != nullptr) ? fundamentalParam->load() : 100.0f);

    const int maxHarmonic = juce::jlimit (1, kMaxHarmonics,
        (maxHarmonicParam != nullptr) ? (int) maxHarmonicParam->load() : 32);
    const int scheme      = (phaseSchemeParam != nullptr)
                              ? (int) phaseSchemeParam->load() : 2;

    countRamp.reset ((float) maxHarmonic);
    rebuildPhases (scheme);
}

void MultisineGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    freqRamp.setTarget ((fundamentalParam != nullptr) ? fundamentalParam->load() : 100.0f);

    const int maxHarmonic = juce::jlimit (1, kMaxHarmonics,
        (maxHarmonicParam != nullptr) ? (int) maxHarmonicParam->load() : 32);
    const int scheme      = (phaseSchemeParam != nullptr)
                              ? (int) phaseSchemeParam->load() : 2;

    // The phase offsets no longer depend on the count, so a rebuild is needed
    // only when the scheme itself changes.
    if (scheme != builtScheme)
        rebuildPhases (scheme);

    countRamp.setTarget ((float) maxHarmonic);

    const double invRate = 1.0 / sampleRate;
    const double invN    = 1.0 / (double) numSamples;
    const double nyquist = 0.5 * sampleRate;

    for (int j = 0; j < numSamples; ++j)
    {
        const double frac = (double) (j + 1) * invN;
        const double fund = (double) freqRamp.at (frac);

        phase += kTwoPi * fund * invRate;
        if (phase >= kTwoPi) phase = std::fmod (phase, kTwoPi);

        // Smoothly ramp the harmonic count, then clamp it to the highest
        // harmonic that stays below Nyquist for the current fundamental.
        double count = (double) countRamp.at (frac);
        if (fund > 0.0)
            count = juce::jmin (count, nyquist / fund);
        else
            count = 0.0;

        // Whole harmonics sum at unit gain; the harmonic crossing the count
        // boundary fades in at fractional gain so a changing count never adds
        // or drops a partial as a step. Output is normalised by the same
        // smooth count, keeping the sum bounded for any phase scheme.
        const int    whole    = (int) count;
        const double boundary = count - (double) whole;

        double sum = 0.0;
        for (int k = 1; k <= whole; ++k)
            sum += std::sin ((double) k * phase + harmonicPhase[(size_t) (k - 1)]);

        if (boundary > 0.0 && whole < kMaxHarmonics)
            sum += boundary * std::sin ((double) (whole + 1) * phase
                                        + harmonicPhase[(size_t) whole]);

        out[j] = (float) (count > 0.0 ? sum / count : 0.0);
    }

    freqRamp.finishBlock();
    countRamp.finishBlock();
}

//==============================================================================
// DC offset
//==============================================================================
DcGenerator::DcGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    levelParam = apvts.getRawParameterValue ("dcLevel");
}

void DcGenerator::prepare (double)
{
    reset();
}

void DcGenerator::reset()
{
    levelRamp.reset ((levelParam != nullptr) ? levelParam->load() : 0.0f);
}

void DcGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    levelRamp.setTarget ((levelParam != nullptr) ? levelParam->load() : 0.0f);

    const double invN = 1.0 / (double) numSamples;
    for (int j = 0; j < numSamples; ++j)
        out[j] = levelRamp.at ((double) (j + 1) * invN);

    levelRamp.finishBlock();
}
