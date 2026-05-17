/*
  ==============================================================================

    Colors.h
    Centralised colour palette for WTGenerator's UI. Anything that should look
    consistent across components lives here so a single edit retones the
    whole plugin.

    Discipline carried over from WTAnalyzer (PRINCIPLES.md, and the
    feedback-color-semantics memory): there are NO per-mode accent colours.
    A signal is a signal regardless of which generator mode produced it
    (Expression / Built-in / Wavetable / Render), so it gets one colour.
    WTAnalyzer's pre/post/analysis hue roles do not apply here - WTGenerator
    emits a single signal, it does not compare two. As concrete UI lands
    (waveform preview, level meter, sweep controls) the specific roles get
    added here; v0 only needs the frame colours.

    Compile-time `inline const` constants; if runtime theming is ever needed,
    wrap these in a class with a singleton accessor. Until then the intended
    change path is edit-and-rebuild.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace WTColors
{
    // ---- Window / surface ---------------------------------------------------
    inline const juce::Colour background { 0xff111213 };  // window fill
    inline const juce::Colour panel      { 0xff1a1c1f };  // raised panel fill
    inline const juce::Colour outline    { 0xff2c2f33 };  // panel / control borders

    // ---- Text ---------------------------------------------------------------
    inline const juce::Colour text       { 0xfff5f5f5 };  // primary text (whitesmoke)
    inline const juce::Colour textDim    { 0xff808080 };  // secondary / inactive text

    // ---- Signal -------------------------------------------------------------
    // The one colour for emitted signal: the future waveform / spectrum
    // preview, level meter fill, anything that represents "the test signal
    // WTGenerator is producing". One signal, one colour - see file header.
    inline const juce::Colour signal     { 0xff30e8a0 };  // mint / emerald

    // ---- Optional: dim variant ---------------------------------------------
    inline juce::Colour dim (juce::Colour c, float alpha = 0.5f) noexcept
    {
        return c.withAlpha (alpha);
    }
}
