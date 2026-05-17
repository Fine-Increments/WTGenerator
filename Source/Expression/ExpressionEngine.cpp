/*
  ==============================================================================

    ExpressionEngine.cpp
    The ONLY translation unit that includes exprtk.hpp. See ExpressionEngine.h
    for the threading contract.

  ==============================================================================
*/

#include "ExpressionEngine.h"
#include "ExpressionDefinition.h"

// Relative include - resolves against this file's location, so no Projucer
// header-search-path entry is needed. exprtk.hpp is vendored, not a Projucer
// source file (a 1.66 MB header has no business in the IDE source tree).
#include "../ThirdParty/exprtk/exprtk.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

//==============================================================================
namespace
{
    // Default stream identifier for the no-argument noise() call.
    constexpr std::uint64_t kDefaultNoiseSeed = 0x6A09E667F3BCC908ull;

    // SplitMix64 finaliser, mapped to a uniform sample in [-1, 1). White
    // noise is a pure function of (seed, sample index) - no retained PRNG
    // state - so noise() and noise(seed) are reproducible run to run: the
    // same expression at the same playback position yields the same value.
    inline double hashToUnit (std::uint64_t x) noexcept
    {
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        // Top 53 bits -> [0, 1) at double precision, then -> [-1, 1).
        return (double) (x >> 11) * (2.0 / 9007199254740992.0) - 1.0;
    }

    //==========================================================================
    // The noise() / noise(seed) expression function (WTGENERATOR.md section
    // 4.3). Registered as a generic (variadic) function so one object serves
    // both the zero-argument and seeded forms. The "PRNG state" is just the
    // playback position: the sample index is derived from the engine's bound
    // t and sample_rate, so nothing is retained between calls.
    struct NoiseFunction : public exprtk::igeneric_function<double>
    {
        typedef exprtk::igeneric_function<double> Base;
        typedef Base::parameter_list_t            parameter_list_t;
        typedef Base::generic_type                generic_type;
        typedef generic_type::scalar_view         scalar_t;

        NoiseFunction() : Base() {}   // default ctor -> fully variadic

        // Pointers into the owning Impl's bound t / sample_rate. Set once,
        // after Impl construction; the targets outlive this object.
        const double* timePtr = nullptr;
        const double* ratePtr = nullptr;

        inline double operator() (parameter_list_t parameters) override
        {
            const double tNow = (timePtr != nullptr) ? *timePtr : 0.0;
            const double sr   = (ratePtr != nullptr) ? *ratePtr : 48000.0;
            const auto index  = (std::uint64_t) (std::int64_t) std::llround (tNow * sr);

            std::uint64_t seed = kDefaultNoiseSeed;
            if (parameters.size() >= 1)
                seed = (std::uint64_t) (std::int64_t) scalar_t (parameters[0])();

            return hashToUnit (seed * 0xD1B54A32D192ED03ull
                               + index * 0x9E3779B97F4A7C15ull);
        }
    };
}

//==============================================================================
struct ExpressionEngine::Impl
{
    // Evaluation precision is double (PRINCIPLES.md section 1 - double where
    // it matters); SignalGenerator casts to float at the output boundary.
    exprtk::symbol_table<double> symbolTable;
    exprtk::expression<double>   expression;
    exprtk::parser<double>       parser;

    double               t          = 0.0;        // reserved variable
    double               sampleRate = 48000.0;     // reserved variable
    std::vector<double>  paramValues;               // bound declared parameters
    std::vector<double>  paramMin;                  // per-parameter [min,max] for
    std::vector<double>  paramSpan;                 // denormalising 0..1 pool values
    NoiseFunction        noiseFn;

    // Declared phasors: phasorPhase[i] is the bound, engine-integrated phase
    // variable; the frequency comes from paramValues[phasorFreqParamIndex[i]]
    // when that index is >= 0, otherwise from phasorFreqConstant[i].
    std::vector<double>  phasorPhase;
    std::vector<int>     phasorFreqParamIndex;
    std::vector<double>  phasorFreqConstant;
    double               lastT = 0.0;               // for rewind detection

    bool        compiled = false;
    std::string lastError;

    Impl()
    {
        noiseFn.timePtr = &t;
        noiseFn.ratePtr = &sampleRate;
    }
};

//==============================================================================
ExpressionEngine::ExpressionEngine() : impl (std::make_unique<Impl>()) {}
ExpressionEngine::~ExpressionEngine() = default;

bool ExpressionEngine::compile (const ExpressionDefinition& definition, double sampleRate)
{
    auto& m = *impl;

    m.compiled = false;
    m.lastError.clear();
    m.sampleRate = sampleRate;
    m.t          = 0.0;

    // Bound storage for the declared parameters. Sized once, here; never
    // resized afterwards, so the by-reference bindings below stay valid for
    // the engine's lifetime. paramMin / paramSpan capture each parameter's
    // range so setParameterNormalised can denormalise a 0..1 pool value.
    const auto numParams = definition.parameters.size();
    m.paramValues.assign (numParams, 0.0);
    m.paramMin.assign    (numParams, 0.0);
    m.paramSpan.assign   (numParams, 1.0);
    for (size_t i = 0; i < numParams; ++i)
    {
        const auto& p = definition.parameters[i];
        m.paramValues[i] = p.defaultValue;
        m.paramMin[i]    = p.minValue;
        m.paramSpan[i]   = p.maxValue - p.minValue;
    }

    // Fresh symbol table - a recompile may declare a different parameter set.
    m.symbolTable = exprtk::symbol_table<double>();
    m.symbolTable.add_constants();                            // pi, epsilon, infinity
    m.symbolTable.add_constant ("e", 2.71828182845904523536); // Euler's number
    m.symbolTable.add_variable ("t", m.t);
    m.symbolTable.add_variable ("sample_rate", m.sampleRate);

    for (size_t i = 0; i < numParams; ++i)
    {
        const auto name = definition.parameters[i].name.toStdString();

        // add_variable fails if the name collides with an ExprTk built-in
        // (sin, min, ...) - names the XML parser's reserved-set check cannot
        // know. Surface it as an honest, actionable error (PRINCIPLES 1).
        if (! m.symbolTable.add_variable (name, m.paramValues[i]))
        {
            m.lastError = "Parameter name '" + name + "' is not usable - it "
                          "collides with a name reserved by the expression "
                          "language.";
            return false;
        }
    }

    // Declared phasors: one engine-integrated phase accumulator each, bound
    // into the symbol table as a variable. Sized once here so the bindings
    // stay valid; the resolved frequency source is captured for evaluate().
    const auto numPhasors = definition.phasors.size();
    m.phasorPhase.assign          (numPhasors, 0.0);
    m.phasorFreqParamIndex.assign (numPhasors, -1);
    m.phasorFreqConstant.assign   (numPhasors, 0.0);

    for (size_t i = 0; i < numPhasors; ++i)
    {
        const auto& phasor = definition.phasors[i];

        if (phasor.freqParam.isNotEmpty())
        {
            for (size_t p = 0; p < numParams; ++p)
                if (definition.parameters[p].name == phasor.freqParam)
                {
                    m.phasorFreqParamIndex[i] = (int) p;
                    break;
                }
        }
        else
        {
            m.phasorFreqConstant[i] = phasor.freqConstant;
        }

        if (! m.symbolTable.add_variable (phasor.name.toStdString(), m.phasorPhase[i]))
        {
            m.lastError = "Phasor name '" + phasor.name.toStdString()
                        + "' is not usable - it collides with a name reserved "
                          "by the expression language.";
            return false;
        }
    }

    m.lastT = 0.0;

    m.symbolTable.add_function ("noise", m.noiseFn);

    // Fresh expression bound to the rebuilt table.
    m.expression = exprtk::expression<double>();
    m.expression.register_symbol_table (m.symbolTable);

    m.compiled = m.parser.compile (definition.expression.toStdString(), m.expression);
    if (! m.compiled)
        m.lastError = m.parser.error();

    return m.compiled;
}

bool ExpressionEngine::isCompiled() const noexcept
{
    return impl->compiled;
}

const std::string& ExpressionEngine::getLastError() const noexcept
{
    return impl->lastError;
}

int ExpressionEngine::getNumParameters() const noexcept
{
    return impl->compiled ? (int) impl->paramValues.size() : 0;
}

void ExpressionEngine::setParameterNormalised (int index, double norm01) noexcept
{
    if (! impl->compiled)
        return;

    if (index < 0 || index >= (int) impl->paramValues.size())
        return;

    impl->paramValues[(size_t) index] = impl->paramMin[(size_t) index]
                                      + norm01 * impl->paramSpan[(size_t) index];
}

double ExpressionEngine::evaluate (double t, double dt) noexcept
{
    if (! impl->compiled)
        return 0.0;

    auto& m = *impl;

    // A backward step in t - transport rewind, or a fresh playback start -
    // resets the phasor accumulators, so a phasor-built signal is
    // reproducible from playback start.
    if (t < m.lastT)
        std::fill (m.phasorPhase.begin(), m.phasorPhase.end(), 0.0);
    m.lastT = t;

    // Integrate each phasor: phase += 2*pi * freq * dt, wrapped to [0, 2*pi).
    // The integral is continuous in freq, so an automated driving frequency
    // steps the phase RATE but never jumps the phase itself - no click.
    constexpr double twoPi = 6.283185307179586476925;
    for (size_t i = 0; i < m.phasorPhase.size(); ++i)
    {
        const double freq = (m.phasorFreqParamIndex[i] >= 0)
            ? m.paramValues[(size_t) m.phasorFreqParamIndex[i]]
            : m.phasorFreqConstant[i];

        double phase = std::fmod (m.phasorPhase[i] + twoPi * freq * dt, twoPi);
        if (phase < 0.0)
            phase += twoPi;
        m.phasorPhase[i] = phase;
    }

    m.t = t;
    return m.expression.value();
}
