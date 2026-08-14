# TerminalCapabilities Design

> **Status:** Proposed
> **Date:** 2026-08-13
> **Module:** `terminal_ui_kit/terminal/` (compiled library `TerminalUiKit::Terminal`)

## Problem

Applications built on FTXUI need a deterministic, testable answer to "what does
the terminal I am running in support?" before enabling optional features --
actual color depth, mouse reporting, bracketed paste, hyperlinks, OSC 52
clipboard, and (optionally) image protocols (Kitty graphics, Sixel, iTerm2
inline images), plus a stable identity to key feature-flag logic on.

Today the repository has no such model: the `terminal` module is INTERFACE-only
and holds no code. We add a small, environment-driven capabilities model.

## Goals

- A value-only `TerminalCapabilities` snapshot that is cheap to copy and safe to
  retain (no `string_view`, no borrowed storage).
- Deterministic detection driven by an injectable environment provider, so
  tests never read the real `getenv`.
- Conservative defaults whenever a signal is missing or ambiguous.
- Explicit user overrides that always take precedence over detection.
- Clear separation of: (1) capability data, (2) environment access,
  (3) detection policy, (4) user overrides.

## Non-goals

- Querying the terminal at runtime (DA1 / DA2 / CSI queries). Detection is
  purely static/probabilistic from the environment. Interactive qualification is
  out of scope and left to the application.
- Replacing FTXUI. We return plain data.

## Architecture

Four public headers under `include/terminal_ui_kit/terminal/`:

| Header | Responsibility |
| ------ | -------------- |
| `capabilities.h` | `ColorDepth`, `TerminalCapabilities`, `ColorDepthToName`. |
| `environment.h`  | `EnvironmentProvider` (abstract) + `SystemEnvironment`. |
| `detector.h`     | `DetectTerminalCapabilities`, `ResolveTerminalCapabilities`, `ParseProgramVersion`. |
| `overrides.h`    | `CapabilityOverrides`, `ApplyOverrides`. |

Each header has a matching `.cc` under `src/terminal_ui_kit/terminal/`. The
`terminal` module is promoted from an INTERFACE target to a compiled library in
`src/terminal_ui_kit/CMakeLists.txt`, following the `diff` module pattern. It
depends only on Core (via the install/metadata conventions) and the C++ standard
library -- no FTXUI dependency, so pure-data consumers and tests link it with no
network or terminal backends.

### Capability data

```cpp
enum class ColorDepth { kNone, kAnsi16, kAnsi256, kTrueColor };
struct TerminalCapabilities {
  ColorDepth color_depth = ColorDepth::kNone;
  bool unicode = false;
  bool mouse = false;
  bool bracketed_paste = false;
  bool hyperlinks = false;
  bool kitty_graphics = false;
  bool sixel = false;
  bool iterm_images = false;
  bool osc52 = false;
  bool alternate_screen = false;
  bool tmux = false;
  bool screen = false;
  bool ssh = false;
  std::string terminal_identity;
};
```

A default-constructed snapshot is fully conservative (nothing enabled, no color,
empty identity) and is therefore safe as an "unknown" fallback.

### Environment access

`EnvironmentProvider::Get(name)` returns `std::optional<std::string>`:
`nullopt` means *not set*; an empty `std::string` means *set but empty*. This
distinction matters for signals like `NO_COLOR`, whose spec only disables color
when present **and non-empty**.

`SystemEnvironment` is a thin, non-owning wrapper over `std::getenv` (it
converts the returned `const char*` into an owned `std::string` immediately, so
the provider never leaks a borrowed pointer across a subsequent `getenv`/`setenv`
call). Tests subclass `EnvironmentProvider` with a map-backed snapshot.

### Detection policy

Signals read by `DetectTerminalCapabilities`:

| Signal | Meaning |
| ------ | ------- |
| `TERM` | Baseline terminal type; `"dumb"` is special; suffix/`color` substrings drive color depth. |
| `COLORTERM` | `"truecolor"` / `"24bit"` => truecolor; any other non-empty value => 256-color. |
| `TERM_PROGRAM` | Program identity (kitty, iTerm.app, wezterm, Apple_Terminal, ...). |
| `TERM_PROGRAM_VERSION` | Parsed defensively (never throws); gates iTerm2 inline images. |
| `NO_COLOR` | Present and non-empty => color depth `kNone`. |
| `TMUX` | Present and non-empty => running under tmux. |
| `STY` | Present and non-empty => running under GNU screen. |
| `SSH_CONNECTION` / `SSH_CLIENT` / `SSH_TTY` | Any present and non-empty => SSH session. |

Precedence (highest to lowest):

1. Explicit user overrides (`ApplyOverrides` wins always).
2. `NO_COLOR` / `TERM=dumb` => `kNone` (color never claims more than none here).
3. `COLORTERM` (non-empty) => truecolor / 256.
4. `TERM` color hints (`truecolor`/`direct`, `256color`, `color`) => depth.
5. Fallback => `kAnsi16` (the universal ANSI baseline).

Conventions for the boolean flags:

- **unicode**: on unless `TERM=dumb` (Unicode is effectively universal; being
  conservative here would disable UTF-8 everywhere).
- **Interactive flags**: `mouse`, `bracketed_paste`, `hyperlinks`,
  `alternate_screen`, `osc52` are on only when a *real* terminal is present
  (`TERM` set and not `dumb`). `osc52` is additionally off for
  `Apple_Terminal`, which does not implement OSC 52.
- **Program-specific image protocols** default off and turn on only for a
  positively identified program:
  - `kitty_graphics`: `TERM_PROGRAM == kitty` or `TERM` contains `xterm-kitty`.
  - `iterm_images`: `TERM_PROGRAM == iTerm.app` **and** parsed major version
    `>= 3` (inline images arrived in iTerm2 3.0). A missing/malformed version
    parses to "unknown" and conservatively disables images.
  - `sixel`: positively identified sixel-capable programs (wezterm, xterm,
    kitty) or a `TERM` containing `sixel`. `iTerm.app` also advertises sixel,
    but gated on the same "version parsed and `>= 3`" condition as
    `iterm_images`, so a version-unknown/old iTerm does not over-claim.
- **tmux / screen / ssh**: pure presence signals, independent of the others, so
  "tmux over SSH" and nested tmux+screen+SSH report all flags on.

`ParseProgramVersion` is public and tested so malformed version strings have a
handled, documented path (returns `false`, `major=minor=0`).

### User overrides

`CapabilityOverrides` holds one `std::optional` per capability plus
`std::optional<std::string> terminal_identity`. An engaged override wins over
the detected value; a disengaged one leaves detection intact. This gives both
"explicit enable override" and "explicit disable override" with a single
mechanism. A default-constructed overrides is a no-op.

## Determinism & safety

- All detection is a pure function of the `EnvironmentProvider`, so the same
  provider always yields the same snapshot (repeatable identity; stable
  `terminal_identity`).
- The model retains owned types only (`std::string`, `std::optional`, enums,
  `bool`). No `string_view`/pointer/span is stored beyond the lifetime of its
  source.
- No exceptions on malformed input; `ParseProgramVersion` is `noexcept`.

## Testing

Unit tests use a map-backed `FakeEnvironment : EnvironmentProvider`; no test
reads the real process environment. Coverage targets every scenario in the task
list: empty env, `TERM=dumb`, unknown terminal, 16/256/truecolor, Unicode,
Kitty, iTerm2, Sixel, OSC 52, hyperlinks, tmux, GNU screen, SSH, tmux-over-SSH,
explicit enable/disable overrides, conflicting variables, `NO_COLOR` without
disabling unrelated capabilities, unknown `TERM_PROGRAM`, missing variables,
malformed version strings, nested tmux/SSH, and stable terminal identity.

The capabilities model is pure data with no view, so dedicated rendering or
interaction *tests* do not apply to the library itself; interactive verification
is provided by the runnable example (see below).

## Example

`examples/terminal_capabilities_example.cpp`, a separate FTXUI executable that:
- renders a capability table (color depth, unicode, mouse, bracketed paste,
  hyperlinks, OSC 52, Kitty graphics, Sixel, iTerm images, alternate screen,
  tmux, screen, SSH, terminal identity);
- switches among seven scenarios with `1`..`7`: real environment, `TERM=dumb`,
  Kitty preset, iTerm2 preset, tmux over SSH, unknown terminal, and a custom
  override;
- displays the active synthetic environment, works after a terminal resize, and
  exits with `q`.

The example builds a `SnapshotEnvironment` (an in-file `EnvironmentProvider`
implementation) for the synthetic scenarios and `SystemEnvironment` for the
real one; it uses only the public API.