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
// A complete expression-mode signal definition. The expression string's
// math is NOT validated by the parser - syntax is checked only when
// ExpressionEngine compiles it against the bound symbols (`t`, sample_rate,
// the declared parameters). The parser guarantees only that the structure
// and the parameter declarations are well-formed.
struct ExpressionDefinition
{
    juce::String                     expression;    // closed-form math in `t`
    std::vector<ExpressionParameter> parameters;
    juce::String                     analysisTag;   // analysis="..." attribute;
                                                    // stored for the v3 sidecar,
                                                    // unused in v1
};
