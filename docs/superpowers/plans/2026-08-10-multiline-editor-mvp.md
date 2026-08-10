# MultilineEditor MVP — implementation plan

Date: 2026-08-10
Branch: `hermes-t8`
PRD reference: sections 21 (MultilineEditor) and 22 (CommandHistory).

## Goal

Deliver a self-contained multiline-editor MVP with a terminal-agnostic document
model, an FTXUI view, configurable key bindings, command history integration, a
bracketed-paste path, and randomized invariant tests.

## Architecture

Follow the repository's model/view split (like `LogModel`/`LogView`,
`VirtualListModel`). The `editor/` module currently is INTERFACE-only; convert
it to a compiled library.

Public headers under `include/terminal_ui_kit/editor/`:

- `editor_document.h` — `EditorDocument`: pure document model (lines, cursor,
  preferred column, viewport, revision). No FTXUI dependency (links Core for
  `TextPosition`).
- `command_history.h` — `CommandHistory`: ring buffer + recall navigation.
- `editor_key_bindings.h` — `EditorKeyBindings`: configurable `ftxui::Event` ->
  command mapping (view-level configuration).
- `multiline_editor.h` — `MultilineEditor` + `MultilineEditorOptions`: the
  FTXUI component (view/controller).

Internal helper under `src/terminal_ui_kit/editor/`:
- `utf8_util.h` — small UTF-8 helpers shared by model and view.

## Model invariants

- Cursor is always clamped to a valid line/column (byte offset at a UTF-8
  code-point boundary).
- Document always has >= 1 line ("" for empty).
- After every edit/navigation/resize the viewport contains the cursor.
- Preferred column is deterministic and preserved across vertical movement.
- Empty-document operations are safe.
- Submit copies the buffer (retained owning type), it does not mutate it.
- `insert_text` splices line runs in O(n) — no accidental quadratic behavior
  on bracketed paste / large buffers.

## Key semantics (defaults)

- move_left/right/up/down : arrows; Home/End; Ctrl+Left/Right = word nav;
  Ctrl+Up/Ctrl+Down = history previous/next; Backspace/Delete; Return = newline;
  Ctrl+Enter (`\x1b[13;5u`) = submit; Escape = exit (example level).
- History: first `history_previous` stashes the draft; each subsequent step
  recalls older entries; `history_next` walks newer and restores the draft at
  the newest. Any edit exits history mode.

## Files to change

- `src/terminal_ui_kit/CMakeLists.txt` — editor -> compiled library (needs FTXUI).
- `include/terminal_ui_kit/editor/*.h`, `src/terminal_ui_kit/editor/*.cc`.
- `tests/terminal_ui_kit/unit/{editor_document_test,command_history_test}.cc`
  (+ CMakeLists).
- `tests/terminal_ui_kit/rendering/multiline_editor_test.cc` (+ CMakeLists).
- `benchmarks/multiline_editor_benchmark.cc` (+ CMakeLists).
- `examples/multiline_editor/{main.cc,CMakeLists.txt}` (+ examples/CMakeLists).
- `examples/README.md`, `xmake.lua` (example registration), docs plan/spec.

## Verification

- GCC + Clang, `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`.
- ASan/UBSan preset for tests + randomized sequences.
- ctest debug + sanitizers; benchmark run.
- Run the example and exercise main flows.
