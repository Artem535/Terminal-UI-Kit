# CompletionPopup — Implementation Plan

## Goal
Add a reusable autocomplete / completion popup component to
`src/terminal_ui_kit/components/`, with unit + rendering tests, a standalone
example, CMake/Xmake wiring, and docs (PRD §23).

## Design summary
* `CompletionItem` — data (label, insert_text, description, category, kind,
  replacement range, metadata).
* `ICompletionProvider` — callback-based `complete(context, on_result)` that
  must not tie the library to Folly/Asio/one executor. Results may arrive
  synchronously or later (any thread).
* `SyncCompletionProvider` — adapter running a function immediately.
* `AsyncCompletionProvider` — adapter that posts through a caller-supplied
  scheduler `std::function<void(std::function<void()>)>`.
* `CompletionPopupModel` — owns provider, generation counter, and a shared
  lifecycle token so a late async result never touches freed state.
* `CompletionPopup` — FTXUI `ComponentBase` rendering the list / loading /
  no-results / error states, handling Up/Down/Enter/Tab/Escape, computing the
  replacement range on accept, and deciding above/below placement.

## Files
- include/terminal_ui_kit/components/completion_popup.h
- src/terminal_ui_kit/components/completion_popup.cc
- tests/terminal_ui_kit/rendering/completion_popup_test.cc
- examples/completion_popup_example.cc
- examples/README.md (new)
- CMakeLists: components sources, rendering tests, examples
- xmake.lua: headers only (follows existing convention)

## Async safety (the tricky part)
FTXUI v5 `ComponentBase` has no `enable_shared_from_this`, so a weak-`this`
pattern is replaced by a `std::shared_ptr<LifecycleToken>` owned by the model:
the token has an atomic `dead` flag and the generation counter. Async results
capture `(shared token, generation)` and only mutate data *inside the token*;
`dead` is set in the model destructor. A late callback sees `dead` and stops
before touching any `this` member. A mutex guards the token's result vector so
a genuinely threaded provider is safe. Tests are deterministic: the async
provider's scheduler is a manually-drained queue, so tests control exactly when
results land.

## Placement
`compute_placement(needed, anchor_row, viewport_height)` returns `kAbove` or
`kBelow` based on available space; narrow terminals take whichever side has
more room and clamp the visible rows. `recompute_placement()` is called every
`Render`, so resizing re-evaluates automatically.

## Verification
- GCC + Clang strict: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`.
- ASan/UBSan for the widget + parser via the sanitiser preset / Clang build.
- Example built and smoke-tested with `scripts/pty-smoke-example.py`.
- Render the popup in an off-screen `ftxui::Screen` for golden tests.
</content>
</invoke>