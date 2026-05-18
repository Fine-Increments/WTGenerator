/*
  ==============================================================================

    BuiltInGenerator.h
    Base class for WTGenerator's built-in C++ signal generators
    (WTGENERATOR.md section 4.4). One concrete subclass per generator;
    SignalGenerator owns the set and dispatches to the selected one. Each
    generator caches its own APVTS parameter pointers at construction.

    Built-in generators render straight at the session sample rate - unlike
    the expression engine they do not go through the oversampler. Each is
    purpose-written and band-limited by construction (a sine is a sine; a
    multisine sums only sub-Nyquist harmonics) or base-rate-native (noise,
    MLS, impulse, step), so there is nothing to anti-alias. Oversampling
    noise / MLS would in fact corrupt them.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// Per-block linear parameter ramp. A parameter is interpolated from its
// previous-block value to the host's current value across the block, so an
// automated parameter reads as a smooth ramp rather than a once-per-block
// staircase - the same scheme the expression path uses. `at(frac)` takes a
// fractional block position in (0, 1]; the value reaches the target at the
// block's last sample.
struct ParamRamp
{
    void  reset      (float v) noexcept       { prev = target = v; }
    void  setTarget  (float v) noexcept       { target = v; }
    float at         (double frac) const noexcept
    {
        return (float) ((double) prev + ((double) target - (double) prev) * frac);
    }
    void  finishBlock() noexcept              { prev = target; }

    float prev   = 0.0f;
    float target = 0.0f;
};

//==============================================================================
class BuiltInGenerator
{
public:
    virtual ~BuiltInGenerator() = default;

    // Message thread. `sampleRate` is the session sample rate render() runs at.
    virtual void prepare (double sampleRate) = 0;

    // Audio thread. Playback (re)started, the generator was just selected,
    // or the transport rewound - reset running phase / ramp state.
    virtual void reset() = 0;

    // Audio thread. Fills `numSamples` of mono signal into `out` at the
    // session sample rate. `startTime` is the playback time (seconds) of
    // out[0]. The generator reads its parameters from its cached APVTS
    // pointers.
    virtual void render (float* out, int numSamples, double startTime) noexcept = 0;
};
