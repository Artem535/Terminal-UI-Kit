= MultilineEditor MVP — design

:toc:
:source-highlighter: coderay

== Overview

Implement a self-contained multiline-editor MVP with a terminal-agnostic
document model, an FTXUI view, configurable key bindings, command-history
recall, bracketed-paste handling and large-buffer efficiency. Scope is limited
per PRD section 21.2/21.3: no undo/redo, selection, mouse, clipboard,
completion, syntax highlighting, or IME.

== Architecture

The `editor/` module is converted from an INTERFACE-only target to a compiled
library with a clear model/view split:

* `EditorDocument` — pure model. Owns `std::vector<std::string>` lines and a
  logical cursor. No FTXUI dependency (depends only on Core `TextPosition`).
* `CommandHistory` — ring-buffer history with recall navigation.
* `EditorKeyBindings` — configurable `ftxui::Event` → command mapping.
* `MultilineEditor` + `MultilineEditorOptions` — the FTXUI Component view that
  owns an `EditorDocument` and translates events into model commands.

== Model invariants

* The document always has >= 1 line (empty buffer = a single `""` line).
* The cursor is always a valid `(line, column)` where the column is a byte
  offset at a UTF-8 code-point boundary — pasted/typed text is never split at a
  mid-code-point byte.
* After every edit, cursor movement and viewport resize, the viewport contains
  the cursor.
* Preferred column is deterministic and preserved across vertical movement.
* Empty-document operations are safe.
* Submit passes a copy of the buffer to the callback; it does not mutate the
  buffer.
* Retained text (lines, history entries, submitted values) uses owning types
  (`std::string` / `std::vector`), never borrowed views.

== Editing model

`insert_text` splits its input on `\n` and splices the resulting line block
into the line vector in a single pass (O(n)), so a large bracketed paste is not
quadratic. `insert_newline`, `delete_backward`, `delete_forward` and the
navigation commands follow conventional editor semantics; `delete_backward` at
a line start joins the previous line, `delete_forward` at a line end joins the
next line.

== Viewport

The model owns a viewport (width x height in cells) plus a scroll origin
(`scroll_top` line index, `scroll_left` byte offset). Every mutation calls
`ensure_cursor_visible()`, which scrolls just enough to bring the cursor inside
the viewport. The FTXUI view observes its allocated box each frame and calls
`set_viewport_size`, so terminal resizes are handled and the horizontal scroll
origin is always advanced to a code-point boundary.

== History semantics

Described in detail on `CommandHistory`. Recall starts by stashing the draft;
`history_previous` walks older, `history_next` walks newer and restores the
draft at the newest entry. Any edit exits history mode.

== Key bindings

Defaults: arrows/Home/End for movement, Ctrl+Left/Right for words,
Ctrl+Up/Down for history, Backspace/Delete, Return for newline, Ctrl+Enter
(CSI-u `\x1b[13;5u`) for submit. All fields are reassignable.

== Submitting

`on_submit(std::string)` receives a copy of the final buffer. The example adds
the submitted value to the `CommandHistory`.
