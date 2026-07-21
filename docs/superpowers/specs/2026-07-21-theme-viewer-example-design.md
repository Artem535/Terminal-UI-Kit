# Design: `theme_viewer` example app

Date: 2026-07-21
Status: Approved

## Context

The user asked for an example app demonstrating the Theme system's work, and
stated this will be a standing convention: every future task/PR gets its own
demonstrating example. `examples/` currently contains only `.gitkeep`
placeholders (`chat`, `diff_viewer`, `image_viewer`, `log_viewer`,
`markdown_viewer`, `multiline_editor` — PRD §54 categories tied to future
components) and no `examples/CMakeLists.txt` exists yet anywhere in the
repo, in either this worktree or the original checkout's `main`. This is
therefore the first example app and the first examples build wiring in the
project.

## Scope

One new example, `examples/theme_viewer`: an interactive FTXUI app listing
all 13 `Theme` roles with sample styled text, letting the user toggle
dark/light and with/without-color at runtime.

Decided during brainstorming:
- **One example per task** (not a single growing "showcase"). Each future
  PR gets its own `examples/<name>/` demonstrating just that task's work.
- **Interactive**, not a one-shot static print: keys `t` (toggle dark/
  light), `c` (toggle color/no-color via `without_color()`), `q`/`Esc`
  (quit).
- **CMake only.** Xmake currently builds neither tests nor examples in this
  repo (no `tests/terminal_ui_kit/xmake.lua`, no FTXUI package wired into
  `xmake.lua` at all) — wiring FTXUI into Xmake is a separate, larger, pre-
  existing gap unrelated to this example and out of scope here.
- **No new library API.** The `TextStyle` → FTXUI `Decorator` conversion is
  local, throwaway code inside the example's `main.cc`, not part of the
  public library surface. PRD §8.4/§8.5 keep Core FTXUI-agnostic; if a real
  component (PR4+) later needs the same conversion for its own rendering,
  that's the point to promote it into the library — not before, on
  spec/YAGNI grounds.

## Behavior

```
Terminal UI Kit - Theme Viewer

[t] toggle dark/light   [c] toggle color   [q] quit

primary     Sample text
secondary   Sample text
muted       Sample text
success     Sample text
warning     Sample text
error       Sample text
accent      Sample text
code        Sample text
addition    + added line
deletion    - removed line
border      Sample text
selected    Sample text
focused     Sample text

Theme: Dark | Color: On
```

Each role's sample text is rendered with that role's `TextStyle` from the
active `Theme` (`default_dark_theme()` or `default_light_theme()`, passed
through `without_color()` when color is toggled off).

## Implementation notes

- `ftxui::Decorator style_to_decorator(const TextStyle&)`, local to
  `main.cc`: composes `ftxui::bold`/`dim`/`underlined`/`strikethrough` (each
  applied conditionally) and `ftxui::color`/`bgcolor` (from `foreground`/
  `background` via `ftxui::Color::RGB`), starting from `ftxui::nothing` and
  chaining with `Decorator operator|(Decorator, Decorator)`.
- **`TextStyle::italic` is not represented.** FTXUI's `elements.hpp` has no
  `italic` decorator at all (only bold/dim/underlined/underlinedDouble/
  blink/strikethrough/color/bgcolor) — there is nothing to map it to. No
  built-in theme role currently sets `italic`, so this has no visible
  effect today; noted as a FTXUI limitation, not a bug in this example.
- Role list built as `std::vector<RoleDemo>` with `TextStyle Theme::*`
  pointer-to-member, name, and sample text — one place enumerating all 13
  roles, iterated for rendering.
- Standard FTXUI idiom, matching existing patterns in FTXUI's own examples
  (`ScreenInteractive::Fullscreen()`, `Renderer(...)`, `CatchEvent(...)`,
  `screen.Loop(...)`), no new abstractions.

## Build wiring

- `examples/CMakeLists.txt` (new): `add_subdirectory(theme_viewer)`. Future
  examples add their own `add_subdirectory(...)` line here.
- `examples/theme_viewer/CMakeLists.txt` (new): `add_executable`, linked
  against `TerminalUiKit::Core` and `ftxui::screen`/`ftxui::dom`/
  `ftxui::component` (already available once `TERMINAL_UI_KIT_BUILD_EXAMPLES`
  triggers `terminal_ui_kit_require_ftxui()` in
  `src/terminal_ui_kit/CMakeLists.txt`, which runs before
  `add_subdirectory(examples)` in the root `CMakeLists.txt`), plus
  `target_terminal_ui_kit_warnings(...)`.
- No Xmake changes (see Scope).

## Non-goals

- No library-level `TextStyle`→FTXUI bridge.
- No Xmake example wiring.
- No automated test/CI check of this example's output (it's a manual demo;
  existing rendering-test infrastructure in `tests/` already covers
  `Theme`'s actual behavior).
