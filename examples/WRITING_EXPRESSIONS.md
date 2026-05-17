# Writing expression definitions

An expression definition is a self-contained `.xml` file describing a test
signal as a closed-form math expression. WTGenerator parses it, exposes its
parameters to the host, and evaluates the math live on the audio thread - no
Python, no rendering. Load one with the **Load Expression...** button.

The `examples/` folder has six worked definitions; this guide is the
reference for writing your own.

## The file

```xml
<?xml version="1.0" encoding="UTF-8"?>

<ParameterSet output-mode="expression">
  <Float  name="freq" minVal="20" maxVal="20000" defaultVal="1000"/>
  <Float  name="amp"  minVal="0"  maxVal="1"     defaultVal="0.5"/>
  <Phasor name="phase" freq="freq"/>
  <Expression>amp * sin(phase)</Expression>
</ParameterSet>
```

- **`<ParameterSet output-mode="expression">`** - the root. `output-mode="expression"`
  is required. An optional `analysis="..."` attribute is carried through to
  the sidecar (unused today).
- **`<Float name min max default>`** - one automatable parameter. Up to 32 per
  definition. (Int / Bool / Choice parameter types arrive in a later version;
  expression mode is Float-only for now.)
- **`<Phasor name="..." freq="...">`** - an oscillator phase. See below.
- **`<Expression>`** - the math. If it contains `<`, `>` or `&`, wrap it in
  `<![CDATA[ ... ]]>`.

## Phasors - and why oscillators need them

Writing an oscillator as `sin(2 * pi * freq * t)` clicks whenever `freq` is
automated: the phase `freq * t` jumps by `change-in-freq * t`, and `t` (elapsed
playback time) is large, so a small frequency nudge throws the phase by many
cycles.

A `<Phasor>` fixes this. It declares a phase variable the engine integrates -
it advances by `2 * pi * freq / sample_rate` every sample, so the phase stays
continuous when the frequency changes. `freq` is a declared parameter (the
usual case, so it is automatable) or a numeric constant.

```xml
<Phasor name="osc" freq="freq"/>
```

The phasor runs `0 .. 2*pi`. So `sin(osc)` is a sine, `sgn(sin(osc))` a
square, `osc/pi - 1` a sawtooth. For FM, declare two phasors and add one
phase into the other: `sin(carrier + index * sin(modulator))`.

**Rule of thumb:** build any oscillator whose pitch may be automated on a
`<Phasor>`. Use raw `t` only for genuinely time-based math - chirps,
envelopes, ramps - where the phase is closed-form and continuous by
construction (see `chirp.xml`).

## Expression vocabulary

**Bound names**

- `t` - elapsed playback time in seconds (resets to 0 on transport rewind).
- `sample_rate` - the session sample rate.
- `pi`, `e` - constants.
- every declared `<Float>` parameter, by name.
- every declared `<Phasor>`, by name (its current phase, `0 .. 2*pi`).

**`noise(seed)`** - a uniform random sample in `[-1, 1]`. The integer `seed`
selects a reproducible stream; any value works for plain white noise. There
is no zero-argument `noise()` form - always pass a seed, e.g. `noise(1)`.

**Operators** - `+ - * /`, `^` (power), comparisons `== != < <= > >=`,
logical `and or not nand nor xor`, and the conditional `cond ? a : b`.

**Functions** (a useful subset; ExprTk provides more):

| Group | Functions |
|---|---|
| Rounding | `floor` `ceil` `round` `trunc` `frac` |
| Powers / roots | `sqrt` `pow` `exp` `log` `log2` `log10` `root` |
| Trig | `sin` `cos` `tan` `asin` `acos` `atan` `atan2` `sinh` `cosh` `tanh` |
| Range / sign | `abs` `sgn` `min` `max` `clamp` `hypot` |
| Other | `mod` `avg` `sum` `if(cond, a, b)` |

The full function set and language reference is ExprTk's - see
<https://github.com/ArashPartow/exprtk>.

## Rules

- **Identifiers** (parameter and phasor names): letters, digits and
  underscore; must not start with a digit.
- **Reserved names:** `t`, `sample_rate`, `pi`, `e`, `noise` are bound by the
  engine - do not name a parameter or phasor after them.
- **No built-in clashes:** a parameter or phasor name must not be a built-in
  function name - `mod`, `min`, `sin`, `frac`, etc. are off limits.
- **No per-sample state in the expression** - no integrators, filters or
  delays. The one piece of running state is the `<Phasor>`, and the engine
  owns it. Stateful signals (pink/brown noise, MLS) are built-in generators,
  not expressions.

## Troubleshooting

When a definition fails to load, the editor's status line shows why. There
are two kinds of message:

**WTGenerator validation** - structural problems found while reading the
`.xml`. These messages state the fix directly: `Phasor name "mod" is
reserved...`, `... is driven by "X", which is not a declared parameter`,
`minVal >= maxVal`, `output-mode is "..."`. Read the message and correct
the file.

**`Expression error: ...`** - the math itself would not compile. The text
describes the problem (an undefined name, mismatched parentheses, a bad
call). It may carry an `ERRnnn` code from the expression compiler; the code
is an internal index - read the description after it, not the number.

Common cases:

| Message / symptom | Cause | Fix |
|---|---|---|
| `Expression error: ... undefined symbol 'X'` | `X` is used but not declared | add a `<Float>` or `<Phasor>` named `X`, or fix a typo |
| `... name 'X' is not usable - it collides with a name reserved by the expression language` | a parameter / phasor is named after a built-in function | rename it |
| `Phasor "X" is driven by "Y", which is not a declared parameter` | the `freq` attribute names a missing parameter | declare `Y`, or give `freq` a numeric constant |
| `output-mode is "..."; expression mode requires output-mode="expression"` | wrong / missing root attribute | set `output-mode="expression"` |
| Clicks when a frequency is swept | the oscillator uses `sin(2*pi*freq*t)` | drive it from a `<Phasor>` |
| Loads, but silent | an amplitude parameter sits at 0, or the expression evaluates to 0 | check parameter defaults and the math |
