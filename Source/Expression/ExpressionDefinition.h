/*
  ==============================================================================

    ExpressionDefinition.h
    The parsed, validated form of an expression-mode signal definition
    (WTGENERATOR.md section 4.3) - the math string plus the parameters it
    declares. Produced by ExpressionXmlParser; consumed by ExpressionEngine
    and the editor's dynamic parameter UI.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
// Hard cap on declared parameters. This is the size of the fixed generic
// APVTS parameter pool (Param 01 .. Param 32) that expression parameters map
// onto - see WTGeneratorAudioProcessor::createParameterLayout. The pool is
// declared once, at construction, and cannot grow without invalidating saved
// host state, so the cap is fixed here and every consumer references this
// one symbol.
inline constexpr int kMaxExpressionParameters = 32;

//==============================================================================
// One declared expression parameter. v1 supports Float parameters only -
// every WTGENERATOR.md section 4.3 example is Float; Int / Bool / Choice
// arrive with the full script-driven parameter UI in v3.
struct ExpressionParameter
{
    juce::String name;
    double       minValue     = 0.0;
    double       maxValue     = 1.0;
    double       defaultValue = 0.0;
};

//==============================================================================
// A declared phasor: an engine-integrated running phase, in radians [0, 2*pi).
// Its name becomes a variable in the expression; the engine advances it each
// sample by 2*pi * freq / sample_rate. Because the phase is the integral of
// frequency, changing the driving frequency never jumps the phase - an
// oscillator built on a phasor is click-free under parameter automation,
// where a raw `sin(2*pi*freq*t)` is not. See WTGENERATOR.md section 4.3.
struct ExpressionPhasor
{
    juce::String name;             // the variable the expression references
    juce::String freqParam;        // driving parameter's name, or empty for a constant
    double       freqConstant = 0.0;   // driving frequency when freqParam is empty
};

//==============================================================================
// A complete expression-mode signal definition. The expression string's
// math is NOT validated by the parser - syntax is checked only when
// ExpressionEngine compiles it against the bound symbols (`t`, sample_rate,
// the declared parameters, the declared phasors). The parser guarantees only
// that the structure and the declarations are well-formed.
struct ExpressionDefinition
{
    juce::String                     expression;    // closed-form math in `t` / phasors
    std::vector<ExpressionParameter> parameters;
    std::vector<ExpressionPhasor>    phasors;
    juce::String                     analysisTag;   // analysis="..." attribute;
                                                    // stored for the v3 sidecar,
                                                    // unused in v1
};
