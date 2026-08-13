# Toast System — Implementation Plan (E3)

## Goal

A reusable FTXUI toast-notification component with deterministic timing,
queueing, actions, focus handling, and no-color fallback.

## Architecture (model/view split)

Keep retained state separate from rendering, mirroring the repo's
"models separate from views" rule (AGENTS.md).

### `ToastManager` (model — no FTXUI dependency)

Pure state + logic, unit-tested with an injected fake clock.

- Enum `ToastSeverity { kInfo, kSuccess, kWarning, kError }`.
- `ToastClock` = `std::function<TimePoint()>` where `TimePoint =
  std::chrono::steady_clock::time_point`; defaults to `steady_clock::now`.
- `ToastAction { label, callback }` (`std::function<void()>`).
- `Toast` = `{ id, message, severity, expires_at?, action? }`.
  - `expires_at` = `std::optional<TimePoint>`; `nullopt` ⇒ persistent.
  - Owner of `std::string message` and `std::function` — no retained
    `string_view`/pointer/reference past their source (benchmark rule).
- `ToastOptions { message, severity, duration? (steady_clock::duration,
  nullopt ⇒ persistent), action? }`.
- `ToastManagerOptions { max_visible, clock }`.

Behavior:
- `show(options) -> id`: appends a toast; ordering is **FIFO by insertion
  id** (deterministic). Visible = first `max_visible` active toasts; the
  rest form the queue and slide in as earlier ones are removed.
- `close(id)`, `clear_all()`.
- `tick()`: called on a timer from the view/example; uses the clock to
  compute elapsed since last tick (stored `last_tick_`). While a toast is
  focused (pause active) time is **frozen**: `last_tick_` advances but no
  toast's remaining time decrements ⇒ timeout pause/resume is deterministic.
  Expired non-persistent toasts are removed.
- Focus state: `set_focused(id)` / `move_focus(delta)` / `focused_id()` /
  `has_focus()`. Focus index is stored as a position into the active list
  and **clamped on removal** so it stays valid after expiry/close.
- `invoke_focused_action()`: invokes the focused toast's action **at most
  once** and closes the toast; safe if the callback removes more toasts
  (callback works on a stored copy; removal is deferred-safe).
- `close_focused()`.
- `visible()`: `const std::vector<Toast>&` for the view.

### `ToastView` (FTXUI `ComponentBase`)

Thin view over a `ToastManager&`:

- `Render()`: `vbox` of the visible toasts. Each toast = severity icon +
  message + optional action label; styled via `to_decorator(theme.<role>)`.
  Focused toast gets a `focused`/inverted indicator. Supports `no_color`
  mode (use `without_color(theme)` or rely on icons/bold).
  Narrow terminals: message rendered with `ftxui::paragraph` (wraps), box
  constrained to available width via `size(ftxui::WIDTH, ftxui::LESS_THAN, w)`
  so a single toast survives resize to a narrow column.
- `OnEvent()`: `Tab`/`Shift+Tab` → `move_focus`; `Enter` → `invoke_focused_action`;
  `Delete`/`Backspace` → `close_focused`. No own event loop, no threads.
- `Focusable()` → true. Keyboard focus drives pause automatically via the model.

## Files

- `include/terminal_ui_kit/components/toast.h`
- `src/terminal_ui_kit/components/toast.cc`
- `tests/terminal_ui_kit/unit/toast_test.cc` (manager: every required case)
- `tests/terminal_ui_kit/rendering/toast_test.cc` (rendering/focus/no-color/narrow)
- `examples/toast_example/main.cc` + `examples/toast_example/CMakeLists.txt`
- `examples/toast_example/xmake.lua` (or register in `examples/xmake.lua`)
- register source in `src/.../components/CMakeLists.txt`, test in
  unit+rendering CMakeLists, example in `examples/CMakeLists.txt`
- `examples/README.md` (new)
- docs plan (this file)

## Required tests (from task E3)

one toast; each severity; queue ordering; visible-count limit; timed expiry;
persistent; manual close; action callback; callback at-most-once; focus
navigation; timeout pause while focused; timeout resume; clear all; removal
during callback; long message; empty message policy; no-color; narrow
terminal; deterministic fake clock.

## Verification

GCC + Clang strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`)
per-file and via CMake; ASan/UBSan via Clang (`sanitizers`); run example
under a PTY smoke test; commit, push, PR.
