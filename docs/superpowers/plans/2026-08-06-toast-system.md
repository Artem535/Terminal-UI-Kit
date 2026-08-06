# Toast System — Implementation Plan

PRD section 40 (Toast). A reusable FTXUI toast-notification component with
deterministic timing, queueing, actions, focus handling, and no-color fallback.

## Architecture

Split retained state from rendering (AGENTS.md architecture rule):

- **`ToastManager`** (`components/toast_manager.{h,cc}`) — pure state, no FTXUI
  types. Owns the visible list + FIFO queue, injected clock, focus index, and
  id counter. Only depends on Core types.
- **`ToastView`** (`components/toast_view.{h,cc}`) — a thin `ftxui::ComponentBase`
  wrapper. Renders the manager's visible toasts each frame, drives
  `ToastManager::Update()` on render (so timed toasts expire naturally under a
  live `ScreenInteractive` loop without creating its own event loop or threads),
  and maps keyboard input to the manager.

## Design decisions

- **Injected clock**: `ToastManager` takes a `std::shared_ptr<const ToastClock>`
  (default `SystemToastClock`). Tests substitute a mutable fake clock, so expiry
  is driven deterministically with no real sleeps.
- **Deterministic ordering**: every toast gets a monotonically increasing
  `uint64_t` id. Visible toasts keep insertion order; a FIFO deque holds
  overflow; expiry/close promotes from the front of the queue to fill a free slot.
- **Timeout pause while focused**: per-toast `remaining` duration decremented on
  each `Update()` by the clock delta. The focused toast's counter is frozen, so
  its timeout is paused while focused.
- **Safe action callbacks**: `action_invoked` guards the at-most-once contract.
  The `std::function` callback and toast id are copied *before* the toast is
  closed; the callback runs after `Close()`, so removing/adding toasts during a
  callback never dereferences freed storage.
- **Focus stability**: focus is an index into the visible list. Removing a toast
  clears/clamps the index; promotion only grows the list; every mutation leaves
  focus valid (no dangling index, no out-of-range).
- **No-color fallback**: `ToastView` renders with `Theme::without_color(theme)`
  when color is toggled off, keeping a severity prefix (`INFO/OK/WARN/ERR`) and
  leading icon so severity stays legible without foreground color. A leader
  marker (`▸`) shows the focused toast; focus remains visual in no-color mode.
- **Narrow terminal**: each toast renders as a full-width box and wraps text at
  the available width; nothing is horizontally clipped.

## Files

- `include/terminal_ui_kit/components/toast_manager.h` — public state API.
- `src/terminal_ui_kit/components/toast_manager.cc` — state implementation.
- `include/terminal_ui_kit/components/toast_view.h` — view API.
- `src/terminal_ui_kit/components/toast_view.cc` — FTXUI component.
- `tests/terminal_ui_kit/rendering/toast_manager_test.cc` — state + fake clock.
- `tests/terminal_ui_kit/rendering/toast_view_test.cc` — rendering / focus.
- `examples/toast_example/CMakeLists.txt` + `main.cc` — interactive demo.
- `examples/README.md` — document the new example.
- `src/terminal_ui_kit/CMakeLists.txt`, `examples/CMakeLists.txt`,
  `tests/terminal_ui_kit/rendering/CMakeLists.txt`, `xmake.lua` — targets.

## Test coverage (required list)

one toast; each severity; queue ordering; visible-count limit; timed expiry;
persistent toast; manual close; action callback; action at most once; focus
navigation (wrap); timeout pause while focused; timeout resume; clear all;
removal during callback; long message; empty message policy; no-color mode;
narrow terminal; deterministic fake clock.

## Build & verify

GCC + Clang with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`
(`TERMINAL_UI_KIT_WARNINGS_AS_ERRORS=ON`), ASan/UBSan preset, run unit/rendering
tests, pty-smoke the example for the main user flows.