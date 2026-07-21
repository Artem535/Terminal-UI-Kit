# Design: Theme System

Date: 2026-07-21
Status: Approved

## Context

PRD §14 defines `Theme` only as a struct of `TextStyle` fields (`primary`,
`secondary`, `muted`, `success`, `warning`, `error`, `accent`, `code`,
`addition`, `deletion`, `border`, `selected`, `focused`) and lists
capabilities: dark/light themes, no-color fallback, 16/256/true color
adaptation, user overrides, adaptation to terminal capabilities.

`core/text_style.h` already provides `Color` and `TextStyle` — this design
defines `Theme` on top of them.

Theme is a blocker for PR4 (Basic Components: StatusIndicator, Spinner,
CollapsiblePanel, ModalStack, KeyHintBar), all of which need to style
themselves from a shared theme.

Full `ColorDepth`/`TerminalCapabilities`-based adaptation (PRD §33) is out of
scope here — that's planned as part of PR7 (Terminal Integration). This
design covers only what unblocks PR4: the `Theme` struct, two built-in
themes, and a no-color fallback transform.

## Scope

In scope:

- `Theme` struct (13 `TextStyle` fields, per PRD §14.1).
- `default_dark_theme()` / `default_light_theme()` built-in themes.
- `without_color(const Theme&)` no-color fallback transform.
- User overrides via plain struct copy/modify (no dedicated API needed).

Out of scope (deferred to PR7 / Terminal Integration):

- `ColorDepth`-aware adaptation (16/256/true color degradation).
- Terminal-capability-driven theme selection.

## API

Location: `include/terminal_ui_kit/theme/theme.h`, `namespace
terminal_ui_kit` (matching `TextStyle`'s placement directly in the
`terminal_ui_kit` namespace — no dedicated `theme` sub-namespace, consistent
with PRD §46 which does not list one).

```cpp
namespace terminal_ui_kit {

struct Theme {
  TextStyle primary;
  TextStyle secondary;
  TextStyle muted;
  TextStyle success;
  TextStyle warning;
  TextStyle error;
  TextStyle accent;
  TextStyle code;
  TextStyle addition;
  TextStyle deletion;
  TextStyle border;
  TextStyle selected;
  TextStyle focused;

  friend bool operator==(const Theme&, const Theme&) = default;
};

const Theme& default_dark_theme();
const Theme& default_light_theme();

// Strips foreground/background from every role, keeping bold/italic/
// underline/dim/strikethrough untouched. Consumers that need visual
// distinction without color (StatusIndicator icons, diff +/- markers)
// already carry that meaning outside of color, so without_color() does not
// need to fabricate extra attributes to compensate.
Theme without_color(const Theme& theme);

}  // namespace terminal_ui_kit
```

### User overrides

No dedicated override API. `Theme` is a plain aggregate; callers copy a
built-in theme and modify individual fields:

```cpp
auto custom = terminal_ui_kit::default_dark_theme();
custom.accent.foreground = terminal_ui_kit::Color{...};
```

This satisfies the "user overrides" capability from PRD §14.2 without
additional surface area.

## Default theme values

Inspired by the GitHub Primer dark/light palettes. Values are approximate,
not verified byte-for-byte against Primer's published design tokens — they
are a reasonable default, not a compatibility guarantee.

| Role | Dark fg / bg | Light fg / bg | Notes |
|---|---|---|---|
| primary | `#e6edf3` | `#1f2328` | |
| secondary | `#848d97` | `#656d76` | |
| muted | `#6e7681` | `#8c959f` | `dim = true` |
| success | `#3fb950` | `#1a7f37` | |
| warning | `#d29922` | `#9a6700` | |
| error | `#f85149` | `#d1242f` | `bold = true` |
| accent | `#58a6ff` | `#0969da` | |
| code | `#e6edf3` / `#161b22` | `#1f2328` / `#f6f8fa` | |
| addition | `#3fb950` / `#0f2c1e` | `#1a7f37` / `#e6ffec` | |
| deletion | `#f85149` / `#331418` | `#d1242f` / `#ffebe9` | |
| border | `#30363d` | `#d1d9e0` | |
| selected | — / `#142f60` | — / `#ddf4ff` | background only |
| focused | `#58a6ff` | `#0969da` | `bold = true` |

Fields not listed above (e.g. `primary.background`) are left unset
(`std::nullopt`).

## Build integration

- Add `theme/theme.cc` to the `terminal_ui_kit_core` target's sources in
  `src/terminal_ui_kit/CMakeLists.txt` (Theme has no dedicated CMake target
  per that file's existing header comment — it's exposed through
  `TerminalUiKit::Core`).
- No new CMake target, no new module registration.

## Testing

Unit tests in `tests/terminal_ui_kit/unit/theme_test.cc` (registered in
`tests/terminal_ui_kit/unit/CMakeLists.txt`):

- `default_dark_theme()` and `default_light_theme()` are internally stable
  (repeated calls return equal values) and differ from each other.
- `without_color()` clears `foreground`/`background` on all 13 roles.
- `without_color()` leaves non-color attributes untouched (e.g.
  `error.bold`, `focused.bold`, `muted.dim` survive the transform).

## Non-goals

- No `ColorDepth` or `TerminalCapabilities` integration (PR7).
- No theme registry, theme switching mechanism, or serialization — those
  aren't requested by PRD §14 and aren't needed by PR4's consumers.
