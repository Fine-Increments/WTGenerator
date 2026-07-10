/*
  ==============================================================================

    PresetLibrary.cpp
    See PresetLibrary.h.

  ==============================================================================
*/

#include "PresetLibrary.h"

namespace BuiltInPresets
{
    //==========================================================================
    // Choice-parameter index constants, mirroring the StringArrays in
    // WTGeneratorAudioProcessor::createParameterLayout. Named here so the
    // preset table below reads as intent rather than bare numbers.
    namespace
    {
        constexpr float kModeBuiltIn = 1.0f;          // generatorMode

        // builtInGenerator
        constexpr float kSine      =  0.0f, kSineSweep =  1.0f, kTwoTone   =  2.0f,
                        kMultisine =  3.0f, kChirp     =  4.0f, kImpulse   =  5.0f,
                        kStep      =  6.0f, kToneBurst =  7.0f, kWhite     =  8.0f,
                        kPink      =  9.0f, kBrown     = 10.0f, kMls       = 11.0f,
                        kDc        = 12.0f, kSilence   = 13.0f;

        constexpr float kLoop = 0.0f, kOneShot = 1.0f, kPeriodic = 2.0f;   // oneShot
        constexpr float kLinear = 0.0f, kLogarithmic = 1.0f;               // sweepCurve
        constexpr float kSchroeder = 2.0f;                                 // multisinePhase
        constexpr float kPositive  = 0.0f;                                 // impulsePolarity
    }

    //==========================================================================
    const std::vector<Preset>& library()
    {
        static const std::vector<Preset> presets
        {
            //------------------------------------------------------------------
            { "Sine", "1 kHz Sine, -6 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSine },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "sineFreq", 1000.0f }, { "sineLevel", -6.0f } } },

            { "Sine", "1 kHz Sine, -20 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSine },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "sineFreq", 1000.0f }, { "sineLevel", -20.0f } } },

            { "Sine", "100 Hz Sine, -6 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSine },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "sineFreq", 100.0f }, { "sineLevel", -6.0f } } },

            { "Sine", "10 kHz Sine, -6 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSine },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "sineFreq", 10000.0f }, { "sineLevel", -6.0f } } },

            //------------------------------------------------------------------
            { "Sweeps & Chirps", "Log Sine Sweep 20 Hz-20 kHz, 10 s", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSineSweep },
                { "oneShot", kOneShot }, { "outputGain", -6.0f },
                { "sweepStartHz", 20.0f }, { "sweepEndHz", 20000.0f },
                { "sweepDuration", 10.0f }, { "sweepCurve", kLogarithmic } } },

            { "Sweeps & Chirps", "Linear Sine Sweep 20 Hz-20 kHz, 5 s", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSineSweep },
                { "oneShot", kOneShot }, { "outputGain", -6.0f },
                { "sweepStartHz", 20.0f }, { "sweepEndHz", 20000.0f },
                { "sweepDuration", 5.0f }, { "sweepCurve", kLinear } } },

            { "Sweeps & Chirps", "Farina Log Chirp 20 Hz-20 kHz, 5 s", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kChirp },
                { "oneShot", kOneShot }, { "outputGain", -6.0f },
                { "chirpStartHz", 20.0f }, { "chirpEndHz", 20000.0f },
                { "chirpDuration", 5.0f } } },

            //------------------------------------------------------------------
            { "Two-Tone & Multisine", "CCIF Twin-Tone 19 + 20 kHz", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kTwoTone },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "twoToneF1", 19000.0f }, { "twoToneF2", 20000.0f },
                { "twoToneLevel1", -12.0f }, { "twoToneLevel2", -12.0f } } },

            { "Two-Tone & Multisine", "SMPTE IMD 60 Hz + 7 kHz (4:1)", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kTwoTone },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "twoToneF1", 60.0f }, { "twoToneF2", 7000.0f },
                { "twoToneLevel1", -6.0f }, { "twoToneLevel2", -18.0f } } },

            { "Two-Tone & Multisine", "Two-Tone 1 kHz + 1.2 kHz", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kTwoTone },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "twoToneF1", 1000.0f }, { "twoToneF2", 1200.0f },
                { "twoToneLevel1", -12.0f }, { "twoToneLevel2", -12.0f } } },

            { "Two-Tone & Multisine", "Multisine 32 Harmonics, Schroeder", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kMultisine },
                { "oneShot", kLoop }, { "outputGain", -6.0f },
                { "multisineFundamental", 100.0f }, { "multisineMaxHarmonic", 32.0f },
                { "multisinePhase", kSchroeder } } },

            { "Two-Tone & Multisine", "Multisine 64 Harmonics, Schroeder", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kMultisine },
                { "oneShot", kLoop }, { "outputGain", -6.0f },
                { "multisineFundamental", 50.0f }, { "multisineMaxHarmonic", 64.0f },
                { "multisinePhase", kSchroeder } } },

            //------------------------------------------------------------------
            { "Transients", "Unit Impulse, Positive", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kImpulse },
                { "oneShot", kOneShot }, { "outputGain", 0.0f },
                { "impulsePolarity", kPositive }, { "impulseLevel", 0.0f } } },

            { "Transients", "Impulse Train, 2 Hz", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kImpulse },
                { "oneShot", kPeriodic }, { "periodicRate", 2.0f }, { "outputGain", 0.0f },
                { "impulsePolarity", kPositive }, { "impulseLevel", -6.0f } } },

            { "Transients", "Unit Step, -6 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kStep },
                { "oneShot", kOneShot }, { "outputGain", 0.0f },
                { "stepRiseTime", 0.0f }, { "stepLevel", -6.0f } } },

            { "Transients", "Tone Burst 1 kHz, ADSR", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kToneBurst },
                { "oneShot", kOneShot }, { "outputGain", 0.0f },
                { "burstFreq", 1000.0f }, { "burstLevel", -6.0f },
                { "burstAttack", 5.0f }, { "burstDecay", 50.0f },
                { "burstSustain", -6.0f }, { "burstRelease", 50.0f },
                { "burstGate", 200.0f } } },

            { "Transients", "Tone Burst Train 1 kHz, 2 Hz", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kToneBurst },
                { "oneShot", kPeriodic }, { "periodicRate", 2.0f }, { "outputGain", 0.0f },
                { "burstFreq", 1000.0f }, { "burstLevel", -6.0f },
                { "burstAttack", 5.0f }, { "burstDecay", 50.0f },
                { "burstSustain", -6.0f }, { "burstRelease", 50.0f },
                { "burstGate", 200.0f } } },

            //------------------------------------------------------------------
            { "Noise & MLS", "White Noise, -12 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kWhite },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "whiteNoiseLevel", -12.0f } } },

            { "Noise & MLS", "Pink Noise, -12 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kPink },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "pinkNoiseLevel", -12.0f } } },

            { "Noise & MLS", "Brown Noise, -12 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kBrown },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "brownNoiseLevel", -12.0f } } },

            { "Noise & MLS", "MLS, Order 16, -12 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kMls },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "mlsOrder", 16.0f }, { "mlsLevel", -12.0f } } },

            { "Noise & MLS", "MLS, Order 18, -12 dBFS", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kMls },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "mlsOrder", 18.0f }, { "mlsLevel", -12.0f } } },

            //------------------------------------------------------------------
            { "Reference", "DC Offset, +0.5", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kDc },
                { "oneShot", kLoop }, { "outputGain", 0.0f },
                { "dcLevel", 0.5f } } },

            { "Reference", "Silence (Noise Floor)", {
                { "generatorMode", kModeBuiltIn }, { "builtInGenerator", kSilence },
                { "oneShot", kLoop }, { "outputGain", 0.0f } } },
        };

        return presets;
    }

    //==========================================================================
    void apply (juce::AudioProcessorValueTreeState& apvts, int index)
    {
        const auto& presets = library();
        if (index < 0 || index >= (int) presets.size())
            return;

        for (const auto& s : presets[(size_t) index].settings)
            if (auto* param = apvts.getParameter (s.paramID))
                param->setValueNotifyingHost (param->convertTo0to1 (s.value));
    }
}
