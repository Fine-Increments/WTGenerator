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

Pre-alpha. **v0 skeleton** - the plugin loads, reports one stereo output bus
and an optional MIDI input, and emits silence. No signal generation yet; the
responsive UI shell and the state-persistence path (APVTS) are in place so
later milestones are additive.

## Roadmap

- **v0** - Skeleton plugin (this milestone): one output bus, empty
  processBlock, no parameters.
- **v1** - Expression mode: closed-form math expressions evaluated live in
  C++, every declared parameter exposed as a DAW-automatable APVTS value,
  oversampled for aliasing protection. The primary signal-authoring path.
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
`JuceLibraryCode/`; both are gitignored - only `WTGenerator.jucer` and
`Source/` are tracked.

The Standalone build runs without a host - useful for offline measurement
workflows such as batch IR captures and calibration runs.

## Relationship to WTSynth

[WTSynth](https://github.com/getdunne/WTSynth) (Shane Dunne) is the wavetable
synthesizer that currently serves as the test-signal source for the WTAnalyzer
workflow. WTGenerator is a strict superset of WTSynth's analysis-relevant
capabilities: it reads the same `wavetable.wav` files and runs the same Python
scripts (`<script>.py` plus a sibling `<script>.xml`) unchanged, so existing
assets keep working. What it adds is free-run transport playback, a neutral
signal path by default, sample-rate-locked playback, aliasing protection,
native sidecar JSON, and the expression engine.
