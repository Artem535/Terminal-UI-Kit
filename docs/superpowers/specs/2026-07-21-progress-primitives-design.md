# Progress primitives design

## Scope

This PR adds `ProgressBar` and `IndeterminateProgress`. `ProgressTree` and
`TaskList` are deliberately deferred to a later PR because they need their
own state model.

## ProgressBar

`ProgressBar(double fraction, const Theme&, ProgressBarOptions = {})` is the
canonical API. A `value, total` overload normalizes and delegates to it.
Invalid inputs, including non-finite values and non-positive totals, resolve
to zero; all fractions are clamped to `[0, 1]`.

Options have width 20 and `show_percentage = true` defaults. Width zero
renders an empty element. Styles are Unicode blocks (default), ASCII, dots,
and Braille. Filled/track/percentage use accent/muted/secondary theme roles.

## IndeterminateProgress

`IndeterminateProgress` is a non-focusable animated FTXUI component. It uses
the same width/style options, default segment width 4 and 80 ms frame time.
Its accent segment moves left-to-right and wraps. Segment width is capped at
the available width; zero width renders empty. Labels are composed by callers.

## Verification

Rendering tests cover normalization, styles, width boundaries, colors and
percentage visibility. Animation tests cover focusability, initial render,
frame timing and wrapping. Tests require no physical terminal.

## Example application

`examples/progress_viewer` demonstrates both components together. It shows
determinate and indeterminate rows in every supported style, lets the user
cycle style and percentage, and documents its key bindings with `KeyHintBar`.
