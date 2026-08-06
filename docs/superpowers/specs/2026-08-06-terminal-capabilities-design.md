# TerminalCapabilities Design

A deterministic terminal-capability model for Terminal UI Kit
(PRD section 63, "terminal integrations"). It answers "what can this terminal
display/do?" so callers can disable or degrade features (Unicode glyphs,
truecolor, hyperlinks, kitty/sixel graphics, OSC 52, images, etc.) without
probing a live terminal.

## Layering

Four separable concerns, each expressed as its own header so requirements — and
tests — can exercise them independently:

| Concern | Header | Contents |
| --- | --- | --- |
| 1. Capability data | `terminal_capabilities.h` | `ColorDepth`, `TerminalCapabilities` (pure data) |
| 2. Environment access | `environment.h` | `EnvironmentProvider` interface, `ProcessEnvironment`, `MapEnvironment` |
| 3. Detection policy | `terminal_detector.h` | `TriState`, `CapabilityOverrides`, `TerminalDetector::Detect` |
| 4. User overrides | `terminal_detector.h` | `CapabilityOverrides` (`kEnable`/`kDisable`) |

Detection never touches the real process. `Detect` takes an
`EnvironmentProvider` and a `CapabilityOverrides`; tests pass a `MapEnvironment`
and so are fully deterministic. `ProcessEnvironment` is the one adapter that
queries `std::getenv`, and only the example uses it for the "real environment"
preset.

## Data model

```cpp
enum class ColorDepth { kNone, k16Color, k256Color, kTrueColor };

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

All windows default to the most conservative state; detection raises them.

## Detection policy

`TerminalDetector::Detect(env, overrides)` is a deterministic, side-effect-free
function. Signals, in increasing weight:

1. **Container context** — `tmux` (env `TMUX` non-empty or `TERM` contains
   `"tmux"`), `screen` (env `STY` non-empty or `TERM` contains `"screen"`),
   `ssh` (any of `SSH_TTY`, `SSH_CLIENT`, `SSH_CONNECTION` non-empty). These are
   orthogonal and composable (tmux-over-SSH).
2. **Program identity + capability defaults** — a conservative preset table
   keyed by program/terminal. Modern programs (kitty, iTerm2, WezTerm, foot,
   vscode) grant truecolor + Unicode + mouse + bracketed paste + OSC 52 +
   alternate screen; kitty additionally grants kitty graphics and hyperlinks;
   iTerm2 grants iTerm images and sixel; `xterm`-class defaults to 16-color,
   `-256color` raises to 256. Unknown terminals inherit the conservative base
   (no color, no extras).
3. **`COLORTERM`** — `truecolor`/`24bit` → `kTrueColor`; `256color` →
   `k256Color`. Overrides the `TERM`-derived guess upward.
4. **`NO_COLOR`** — when set (any value), forces `color_depth` to `kNone`.
   Affects only color; every unrelated capability is untouched.
5. **Explicit overrides** (highest) — `CapabilityOverrides` with `kEnable`/
   `kDisable` for each boolean and an optional color-depth pin. An explicit
   override wins over `NO_COLOR`, `COLORTERM`, and every preset default.

`terminal_identity` is the most specific known name: the normalized
`TERM_PROGRAM` when present, else the normalized `TERM` lowercased (e.g.
`xterm-256color`), else empty.

## Precedence summary (documented on the API)

```
override > NO_COLOR > COLORTERM > TERM base/preset > container context > conservative default
```

`NO_COLOR` and overrides are the only two signals that pull *down*; everything
else is additive/monotonic.

## Testability

Because detection is a pure function of an injectable provider, the test suite
covers every required environment with a literal `MapEnvironment`, including
conflicting variables (`COLORTERM=truecolor` + `TERM=dumb`), `NO_COLOR`
isolation, unknown `TERM_PROGRAM`, override precedence, malformed version
strings, nested tmux/SSH, and a stable identity.

## Example

`examples/terminal_capabilities_example` renders the capability table for a
selectable preset: real env, `TERM=dumb`, kitty, iTerm2, tmux-over-SSH, unknown,
and a custom override. `q` exits; the layout is resize-safe.