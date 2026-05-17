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

The schema is a `<ParameterSet output-mode="expression">` root with one
`<Float name="..." minVal="..." maxVal="..." defaultVal="..."/>` per parameter
and one `<Expression>` element holding the math. The expression is a closed-form
function of `t` (seconds) plus the declared parameters; `sample_rate`, `pi` and
`e` are also bound, and `noise()` / `noise(seed)` give white noise. If the
expression uses `<`, `>` or `&`, wrap it in `<![CDATA[ ... ]]>`.
