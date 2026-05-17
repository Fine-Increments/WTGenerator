/*
  ==============================================================================

    ExpressionEngine.h
    PIMPL wrapper around ExprTk - WTGenerator's closed-form expression
    evaluator (WTGENERATOR.md section 4.3).

    ExprTk's exprtk.hpp is ~1.66 MB / ~46k lines and slow to compile. It is
    #included by ExpressionEngine.cpp and NOWHERE else, so exactly one
    translation unit pays that cost. No ExprTk type appears in this header.

    Threading: compile() is message-thread only - it parses, allocates and
    builds the AST. evaluate() and parameterBuffer() are audio-thread safe
    after a successful compile (allocation-free). The two are never called
    concurrently on one engine instance - the message thread hands a freshly
    compiled engine to the audio thread via an atomic swap (step 7).

  ==============================================================================
*/

#pragma once

#include <memory>
#include <string>

struct ExpressionDefinition;   // Expression/ExpressionDefinition.h

//==============================================================================
class ExpressionEngine
{
public:
    ExpressionEngine();
    ~ExpressionEngine();

    ExpressionEngine (const ExpressionEngine&) = delete;
    ExpressionEngine& operator= (const ExpressionEngine&) = delete;

    // Builds the symbol table for `definition` (binds t, sample_rate, the
    // declared parameters and the noise() function) and compiles its
    // expression string. Message-thread only. Returns true on success; on
    // failure the engine is left un-compiled and getLastError() carries a
    // human-readable diagnostic.
    bool compile (const ExpressionDefinition& definition, double sampleRate);

    bool isCompiled() const noexcept;

    // Parser / binding diagnostic from the most recent failed compile();
    // empty after a successful compile.
    const std::string& getLastError() const noexcept;

    //==============================================================================
    // Number of declared parameters bound by the last successful compile.
    int getNumParameters() const noexcept;

    // Audio thread. Sets declared parameter `index` from a 0..1 value,
    // denormalised internally through the parameter's [min,max] captured at
    // compile time. Out-of-range indices and an un-compiled engine are
    // ignored. Allocation-free.
    void setParameterNormalised (int index, double norm01) noexcept;

    // Evaluates the expression for one output sample. `t` is elapsed playback
    // time (seconds); `dt` is the time step since the previous evaluate()
    // call - the engine integrates each declared phasor by 2*pi*freq*dt. A
    // backward jump in `t` (transport rewind, playback restart) resets the
    // phasor accumulators. Allocation-free and audio-thread safe after a
    // successful compile; returns 0.0 when no expression is compiled.
    double evaluate (double t, double dt) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
