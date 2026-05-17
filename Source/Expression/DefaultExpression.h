/*
  ==============================================================================

    DefaultExpression.h
    The signal WTGenerator emits before any .xml is loaded - a -6 dBFS 1 kHz
    sine. The plugin therefore makes sound the instant it is inserted and the
    transport rolls, and the two declared parameters (freq, amp) double as the
    worked example of the expression parameter mechanism (WTGENERATOR.md
    section 4.3).

  ==============================================================================
*/

#pragma once

#include "ExpressionDefinition.h"

//==============================================================================
inline ExpressionDefinition getDefaultExpression()
{
    ExpressionDefinition def;
    def.expression = "amp * sin(2 * pi * freq * t)";
    def.parameters.push_back ({ "freq", 20.0, 20000.0, 1000.0 });
    def.parameters.push_back ({ "amp",   0.0,     1.0,     0.5 });
    return def;
}
