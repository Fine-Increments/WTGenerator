# Example expression definitions

Self-contained expression-mode signal definitions for WTGenerator. Each is a
plain `.xml` - no Python, no rendering. Load one with the **Load Expression...**
button; its declared parameters appear as sliders in the plugin window and as
`Param NN` automation targets in the host.

| File | Signal | Notes |
|---|---|---|
| `square.xml`   | Band-limited square wave        | Exercises the 8x anti-aliasing |
| `saw.xml`      | Sawtooth (fractional-part)      | Also discontinuity-rich |
| `pwm.xml`      | Pulse wave, adjustable duty     | Automate `duty` for PWM |
| `fm.xml`       | FM oscillator                   | Automate `mod_index` / `mod_hz` at audio rate |
| `am_noise.xml` | Amplitude-modulated white noise | Shows the `noise()` function |
| `chirp.xml`    | Linear chirp                    | Aperiodic; best driven by the transport |

## Writing your own

The schema is a `<ParameterSet output-mode="expression">` root containing:

- one `<Float name="..." minVal="..." maxVal="..." defaultVal="..."/>` per
  automatable parameter;
- a `<Phasor name="..." freq="..."/>` for each oscillator - `freq` names a
  declared parameter (or is a numeric constant). A phasor is an
  engine-integrated phase running `0 .. 2*pi`, so `sin(name)` is a sine and
  `name/pi - 1` a sawtooth. Build any oscillator whose pitch you might
  automate on a phasor, or changing the frequency will click;
- one `<Expression>` element holding the math.

The expression is a function of the declared parameters and phasors, plus the
bound names `t` (seconds), `sample_rate`, `pi`, `e`, and `noise()` /
`noise(seed)` for white noise. Reach for raw `t` only for genuinely
time-based math - chirps, envelopes, ramps - as in `chirp.xml`. If the
expression uses `<`, `>` or `&`, wrap it in `<![CDATA[ ... ]]>`.
