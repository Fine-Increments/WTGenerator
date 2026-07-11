/*
  ==============================================================================

    StatefulGenerators.cpp
    See StatefulGenerators.h.

  ==============================================================================
*/

#include "StatefulGenerators.h"

namespace
{
    // Fixed PRNG seed - the noise generators are reproducible run to run.
    constexpr std::uint32_t kNoiseSeed = 0x1234567u;

    // xorshift32 -> a uniform sample in [-1, 1).
    inline double nextWhite (std::uint32_t& s) noexcept
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (double) s * (2.0 / 4294967296.0) - 1.0;
    }

    inline float gainFor (const std::atomic<float>* levelParam, float fallbackDb) noexcept
    {
        const float db = (levelParam != nullptr) ? levelParam->load() : fallbackDb;
        return (float) juce::Decibels::decibelsToGain (db, -100.0f);
    }

    // Galois LFSR toggle masks - the XAPP052 maximal-length polynomials in
    // right-shifting Galois form, indexed by order. When the chip shifted
    // out is 1, the register is XORed with this mask.
    const std::uint32_t kMlsTaps[21] =
    {
        0u, 0u,
        0x3u, 0x6u, 0xCu, 0x14u, 0x30u, 0x60u, 0xB8u, 0x110u, 0x240u, 0x500u,
        0x829u, 0x100Du, 0x2015u, 0x6000u, 0xD008u, 0x12000u, 0x20400u,
        0x40023u, 0x90000u
    };
}

//==============================================================================
// White noise
//==============================================================================
WhiteNoiseGenerator::WhiteNoiseGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    levelParam = apvts.getRawParameterValue ("whiteNoiseLevel");
}

void WhiteNoiseGenerator::prepare (double) { reset(); }

void WhiteNoiseGenerator::reset()
{
    rngState = kNoiseSeed;
    gainRamp.reset (gainFor (levelParam, -12.0f));
}

void WhiteNoiseGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    gainRamp.setTarget (gainFor (levelParam, -12.0f));
    const double invN = 1.0 / (double) numSamples;

    for (int j = 0; j < numSamples; ++j)
        out[j] = (float) (nextWhite (rngState) * (double) gainRamp.at ((double) (j + 1) * invN));

    gainRamp.finishBlock();
}

//==============================================================================
// Pink noise - Paul Kellet's pinking filter
//==============================================================================
PinkNoiseGenerator::PinkNoiseGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    levelParam = apvts.getRawParameterValue ("pinkNoiseLevel");
}

void PinkNoiseGenerator::prepare (double) { reset(); }

void PinkNoiseGenerator::reset()
{
    rngState = kNoiseSeed;
    b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0;
    gainRamp.reset (gainFor (levelParam, -12.0f));
}

void PinkNoiseGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    gainRamp.setTarget (gainFor (levelParam, -12.0f));
    const double invN = 1.0 / (double) numSamples;

    for (int j = 0; j < numSamples; ++j)
    {
        const double white = nextWhite (rngState);

        b0 = 0.99886 * b0 + white * 0.0555179;
        b1 = 0.99332 * b1 + white * 0.0750759;
        b2 = 0.96900 * b2 + white * 0.1538520;
        b3 = 0.86650 * b3 + white * 0.3104856;
        b4 = 0.55000 * b4 + white * 0.5329522;
        b5 = -0.7616 * b5 - white * 0.0168980;

        const double pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362) * 0.11;
        b6 = white * 0.115926;

        out[j] = (float) (pink * (double) gainRamp.at ((double) (j + 1) * invN));
    }

    gainRamp.finishBlock();
}

//==============================================================================
// Brown noise - white through a leaky integrator
//==============================================================================
BrownNoiseGenerator::BrownNoiseGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    levelParam = apvts.getRawParameterValue ("brownNoiseLevel");
}

void BrownNoiseGenerator::prepare (double) { reset(); }

void BrownNoiseGenerator::reset()
{
    rngState = kNoiseSeed;
    brown    = 0.0;
    gainRamp.reset (gainFor (levelParam, -12.0f));
}

void BrownNoiseGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    gainRamp.setTarget (gainFor (levelParam, -12.0f));
    const double invN = 1.0 / (double) numSamples;

    for (int j = 0; j < numSamples; ++j)
    {
        // Leaky integration of white noise: -6 dB/octave, bounded by the
        // leak (and a hard clamp as a safety net at the extremes).
        brown = juce::jlimit (-1.0, 1.0, brown * 0.998 + nextWhite (rngState) * 0.02);
        out[j] = (float) (brown * (double) gainRamp.at ((double) (j + 1) * invN));
    }

    gainRamp.finishBlock();
}

//==============================================================================
// MLS - maximum-length sequence
//==============================================================================
MlsGenerator::MlsGenerator (juce::AudioProcessorValueTreeState& apvts)
{
    orderParam = apvts.getRawParameterValue ("mlsOrder");
    levelParam = apvts.getRawParameterValue ("mlsLevel");
}

void MlsGenerator::prepare (double) { reset(); }

void MlsGenerator::reset()
{
    lfsr      = 1u;
    lastOrder = -1;
    gainRamp.reset (gainFor (levelParam, -12.0f));
}

void MlsGenerator::render (float* out, int numSamples, double) noexcept
{
    if (numSamples <= 0)
        return;

    const int order = juce::jlimit (2, 20,
        (orderParam != nullptr) ? (int) orderParam->load() : 16);
    const std::uint32_t mask = kMlsTaps[order];

    // Reseed when the order changes: a smaller mask never XOR-clears bits set
    // above the new order, so the register would emit a constant (-A) for up to
    // `order` samples until they shift out. Reseeding restarts a clean sequence.
    if (order != lastOrder)
    {
        lfsr      = 1u;
        lastOrder = order;
    }

    gainRamp.setTarget (gainFor (levelParam, -12.0f));
    const double invN = 1.0 / (double) numSamples;

    for (int j = 0; j < numSamples; ++j)
    {
        // Galois LFSR: the bit shifted out is the chip (0 -> -1, 1 -> +1);
        // when it is 1, the tap mask is XORed back in. With the XAPP052
        // Galois masks a non-zero register cycles through all 2^order - 1
        // states - a true maximum-length sequence - and never collapses to
        // zero, so no all-zero guard is needed.
        const std::uint32_t chip = lfsr & 1u;
        lfsr >>= 1;
        if (chip != 0u)
            lfsr ^= mask;

        const double s = (chip != 0u) ? 1.0 : -1.0;
        out[j] = (float) (s * (double) gainRamp.at ((double) (j + 1) * invN));
    }

    gainRamp.finishBlock();
}
