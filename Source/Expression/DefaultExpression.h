/*
  ==============================================================================

    DefaultExpression.h
    The signal WTGenerator emits before any .xml is loaded - a -6 dBFS 1 kHz
    sine. The plugin therefore makes sound the instant it is inserted and the
    transport rolls, and the declared parameters (freq, amp) plus the phasor
    double as the worked example of the expression mechanism (WTGENERATOR.md
    section 4.3). The oscillator is built on a phasor, so sweeping `freq` is
    click-free.

  ==============================================================================
*/

#pragma once

#include "ExpressionDefinition.h"

//==============================================================================
inline ExpressionDefinition getDefaultExpression()
{
    ExpressionDefinition def;
    def.expression = "amp * sin(phase)";
    def.parameters.push_back ({ "freq", 20.0, 20000.0, 1000.0 });
    def.parameters.push_back ({ "amp",   0.0,     1.0,     0.5 });
    def.phasors.push_back    ({ "phase", "freq", 0.0 });
    return def;
}
