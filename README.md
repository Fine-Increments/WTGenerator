# WTGenerator

A JUCE-based VST3/AU/Standalone test-signal generator plugin. WTGenerator
emits a clean, reproducible, transport-driven reference signal for measuring
what an audio effect does to it.

It is the source side of a precision DSP test bench: WTGenerator produces the
test signal, the effect under test processes it, and
[WTAnalyzer](https://github.com/Fine-Increments/WTAnalyzer) sits downstream
measuring the result. WTGenerator also stands on its own as a precision
signal source for effect demos, IR captures, A/B references, and calibration
- it does not depend on WTAnalyzer being present.

WTGenerator is not a musical instrument. It has no filter, no LFO, no
envelope, no polyphony. Its job is a clean reference signal and nothing else.

## Status

Pre-alpha. **v1 - expression mode** is working: the plugin generates a live
test signal from a math expression, with every declared parameter exposed to
the host for automation.

What works today:

- **Expression mode** - a test signal defined as a closed-form math
  expression in a small `.xml` file, evaluated live in C++. No Python, no
  rendering.
- **Phasors** - declared oscillator phases the engine integrates, so an
  oscillator's frequency can be automated without clicks.
- **Automatable parameters** - each declared parameter maps to a host
  automation slot; sweep it from the DAW at audio rate.
- **Oversampled anti-aliasing** - the signal is evaluated at 8x and
  decimated, so discontinuity-rich waveforms (square, saw) stay band-limited.
- **Transport playback** - free-run, transport-driven; the Standalone build
  free-runs without a host.
- **Output gain**, and session save / restore of the loaded definition.

Build it, drop it on a track, and it emits a 1 kHz sine out of the box. Load
one of the [examples](examples/) to hear more, or write your own - see
[examples/WRITING_EXPRESSIONS.md](examples/WRITING_EXPRESSIONS.md).

## Expression mode

A test signal is a closed-form function of time, written as plain math:

```xml
<ParameterSet output-mode="expression">
  <Float  name="freq" minVal="20" maxVal="20000" defaultVal="1000"/>
  <Float  name="amp"  minVal="0"  maxVal="1"     defaultVal="0.5"/>
  <Phasor name="phase" freq="freq"/>
  <Expression>amp * sin(phase)</Expression>
</ParameterSet>
```

WTGenerator parses it, creates a host parameter per `<Float>`, integrates each
`<Phasor>`, and evaluates the expression per sample on the audio thread. Any
parameter is automatable; anyone who can write a formula can ship a new test
signal as a tiny `.xml` with no plugin rebuild. The
[examples](examples/) folder has square, saw, PWM, FM, AM-noise and chirp
definitions; [WRITING_EXPRESSIONS.md](examples/WRITING_EXPRESSIONS.md) is the
authoring reference.

## Roadmap

- **v0** - Skeleton plugin. *(done)*
- **v1** - Expression mode: closed-form math, phasors, automatable
  parameters, oversampled anti-aliasing, transport playback. *(done)*
- **v2** - Built-in generators: sine, sweeps, two-tone, multisine, Farina
  chirp, impulse, step, tone burst, white/pink/brown noise, MLS, DC, silence,
  plus a curated preset library.
- **v3** - WTSynth-compatibility layer: wavetable mode, the Python script
  runner, native sidecar JSON emission, render-mode playback.
- **v4** - Internal auto-sweep and script-parameter stepping; aliasing
  protection visuals for wavetable mode.
- **v5** - Pitch-shifted compat mode and MIDI-driven playback.
- **v6** - Stereo wavetable support.

## Build

WTGenerator is a [Projucer](https://juce.com/download/) project (JUCE 8+,
C++20). Open `WTGenerator.jucer` in the Projucer, then build the exported
project (Xcode on macOS). The Projucer regenerates `Builds/` and
`JuceLibraryCode/`; both are gitignored - only `WTGenerator.jucer`, `Source/`
and `examples/` are tracked.

The Standalone build runs without a host - useful for offline measurement
workflows such as batch IR captures and calibration runs.

## Relationship to WTSynth

[WTSynth](https://github.com/getdunne/WTSynth) (Shane Dunne) is the wavetable
synthesizer that currently serves as the test-signal source for the WTAnalyzer
workflow. WTGenerator is a strict superset of WTSynth's analysis-relevant
capabilities: it will read the same `wavetable.wav` files and run the same
Python scripts unchanged (the wavetable / script paths arrive in v3), so
existing assets keep working. What it adds is free-run transport playback, a
neutral signal path by default, sample-rate-locked playback, aliasing
protection, native sidecar JSON, and the expression engine.
