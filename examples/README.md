# Examples

Terminal UI Kit ships focused example applications that demonstrate individual
components in isolation (PRD section 54). Each example lives in its own
subdirectory and is a standalone `add_executable` target; each is registered
in [`CMakeLists.txt`](CMakeLists.txt) and built as part of the `examples`
CMake subtarget.

The examples are interactive terminal applications built on FTXUI, with two
exceptions noted below. Quit an interactive example by pressing the `q` key
(or the key shown in its on-screen controls). Examples require no network
access and use only deterministic, built-in sample data.

## Building

Examples are built when `TERMINAL_UI_KIT_BUILD_EXAMPLES` is enabled:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON \
  -DTERMINAL_UI_KIT_BUILD_TESTS=ON
cmake --build build --parallel
```

Binaries are written under `build/examples/<name>/` with the prefix
`terminal_ui_kit_example_`.

## List of examples

| Directory | Executable | Description | Interactive |
| --- | --- | --- | --- |
| `components_gallery/` | `terminal_ui_kit_example_components_gallery` | Status components, panels, modals, themes, and code rendering. | Yes |
| `theme_viewer/` | `terminal_ui_kit_example_theme_viewer` | Semantic theme roles (`default_dark_theme`, `default_light_theme`). | Yes |
| `progress_viewer/` | `terminal_ui_kit_example_progress_viewer` | Determinate and indeterminate progress. | Yes |
| `task_dashboard/` | `terminal_ui_kit_example_task_dashboard` | Hierarchical task state (ProgressTree / TaskList). | Yes |
| `virtual_list_viewer/` | `terminal_ui_kit_example_virtual_list_viewer` | A virtualized 100,000-row list with variable row heights. | Yes |
| `streaming_log_viewer/` | `terminal_ui_kit_example_streaming_log_viewer` | Live structured logs with ANSI styling. | Yes |
| `virtual_document_viewer/` | `terminal_ui_kit_example_virtual_document_viewer` | An incrementally updated wrapped text document. | Yes |
| `searchable_text_view_example/` | `terminal_ui_kit_example_searchable_text_view` | Literal/regex search over a document with highlighting and navigation. | Yes |
| `diff_parser/` | `terminal_ui_kit_example_diff_parser` | Parses a unified diff from stdin and prints a plain-text summary. | No (stdin) |
| `toast_example/` | `terminal_ui_kit_example_toast` | ToastManager / ToastView: timed and persistent notifications with a FIFO queue. | Yes |
| `command_history_example/` | `terminal_ui_kit_example_command_history` | Bounded, navigable command history with search, persistence, and sensitive-command policy. | Yes |
| `terminal_capabilities_example.cpp` | `terminal_ui_kit_example_terminal_capabilities` | Terminal-capability detection across presets. | Yes |
| `markdown_viewer/` | `terminal_ui_kit_example_markdown_viewer` | Markdown rendering. Only built when `TERMINAL_UI_KIT_ENABLE_MARKDOWN=ON`. | Yes |
| `line_number_formatting_example.cpp` | `terminal_ui_kit_example_line_number_formatting` | Right-aligned line-number gutters under configurable widths. | Yes |
| `completion_popup_example.cpp` | `terminal_ui_kit_example_completion_popup` | Fuzzy completion with synchronous and delayed providers. | Yes |
| `unified_diff_view_example.cpp` | `terminal_ui_kit_example_unified_diff_view` | Virtualized diff rendering and navigation. | Yes |
| `multiline_editor/` | `terminal_ui_kit_example_multiline_editor` | Multiline editing, submission, and history recall. | Yes |
| `transcript_view_example/` | `terminal_ui_kit_example_transcript_view` | Streaming transcript with heterogeneous blocks. | Yes |

## searchable_text_view_example

Demonstrates the reusable, searchable text view component
(`terminal_ui_kit/components/searchable_text_view.h`) with a deterministic,
sizeable sample document (repeated tokens plus UTF-8 Cyrillic lines):

- literal and regular-expression search
- case-sensitive / case-insensitive matching
- incremental query updates while typing
- previous/next match navigation with wrap-around
- current-match index and total match count
- invalid-regex and no-results states
- UTF-8 source, unchanged; all visible matches highlighted, active match
  in a distinct style
- automatic scrolling to the active match; re-wrap on terminal resize

Controls:

| Key   | Action                            |
|-------|-----------------------------------|
| `/`   | Open search                       |
| type  | Edit the query (incremental)      |
| Enter | Apply the query / close prompt    |
| `n`   | Next match                        |
| `N`   | Previous match                    |
| `c`   | Toggle case sensitivity           |
| `r`   | Toggle regex mode                 |
| Esc   | Close / cancel search             |
| `q`   | **Quit** (documented exit key)    |

Run it:

```sh
./build/examples/searchable_text_view_example/terminal_ui_kit_example_searchable_text_view
```

## toast_example

Interactive toast system demo: timed + persistent notifications, a FIFO queue
with a configurable visible-count limit, optional action callbacks, keyboard
focus with a timeout pause while focused, and a no-color fallback.

Run it with:

```sh
./build/examples/toast_example/terminal_ui_kit_example_toast
```

Controls:

```text
i          Add info toast
s          Add success toast
w          Add warning toast
e          Add error toast
a          Add toast with action
p          Add persistent toast (dismiss with Delete or c)
c          Clear all toasts
Tab        Move focus to next toast
Shift+Tab  Move focus to previous toast
Enter      Invoke the focused toast's action
Delete     Close the focused toast
t          Toggle color / no-color fallback
q or Esc   Quit
```

Timed toasts show a live countdown and drain automatically under the running
`ScreenInteractive` loop; the focused toast's timeout is paused while it stays
focused. Exceed `max_visible` to see queueing first-in/first-out. Quit with `q`
or `Esc`.

## command_history_example

A bounded, navigable `CommandHistory` model for input and editor components.
The demo drives a small `CommandHistory` (capacity 8) through its public API:

- adding commands (and watching empty / whitespace-only / consecutive-duplicate
  input get ignored);
- `Up` / `Down` navigation over previous and next entries;
- substring search (a left panel) and prefix search (a right panel), both
  showing the most-recent-first ordering;
- current history size and capacity, including capacity eviction of the oldest
  command once the bound is reached;
- clearing the history;
- toggling sensitive mode, which blocks `secret` and `password` from reaching
  the attached persistence store (they stay in memory); the on-screen
  "Persisted" counter stops increasing for those commands.

A counting store is attached to prove that persistence activity (and its
suppression for sensitive commands) really happens.

Run it:

```sh
./build/examples/command_history_example/terminal_ui_kit_example_command_history
```

Controls:

| Key | Action |
| --- | --- |
| `Enter` | Add the typed command to history |
| `Up` / `Down` | Navigate previous / next history entries |
| `c` | Clear history (when the command box is not focused) |
| `t` | Toggle sensitive mode (when the command box is not focused) |
| `q` | Quit (when the command box is not focused) |
| `Esc` | Quit (from anywhere) |
| `Tab` | Move focus between the command and search inputs |

The single-letter hotkeys are gated on the command box not being focused so
that typing a command containing `c`, `t` or `q` is never swallowed.

The underlying model is tested independently of the terminal in
`tests/terminal_ui_kit/unit/command_history_test.cc`, which covers navigation
boundaries, capacity eviction, duplicate filtering, search ordering, capacity
`0`, persistence failure, and sensitive-command persistence suppression.

## terminal_capabilities

An interactive FTXUI demo of the `TerminalUiKit::Terminal` module
(`terminal_ui_kit/terminal/*.h`). It renders a table of detected capabilities —
color depth, Unicode, mouse, bracketed paste, hyperlinks, OSC 52, Kitty
graphics, Sixel, iTerm images, alternate screen, tmux, screen, SSH, and terminal
identity — and lets you switch between environments:

| Key | Scenario |
| --- | -------- |
| `1` | Real process environment (`SystemEnvironment`) |
| `2` | `TERM=dumb` |
| `3` | Kitty preset |
| `4` | iTerm2 preset |
| `5` | tmux over SSH |
| `6` | Unknown terminal |
| `7` | Custom `CapabilityOverrides` scenario |
| `q` | Quit |

The example uses only the public Terminal module API; synthetic environments are
supplied through an in-file `EnvironmentProvider` implementation and explicit
`CapabilityOverrides`, so every non-live scenario is fully deterministic. The
layout is an ordinary FTXUI tree and reflows on terminal resize.

Run it:

```sh
./build/examples/terminal_ui_kit_example_terminal_capabilities
```

## line_number_formatting_example

Demonstrates the shared line-number gutter helper and both gutter-rendering
components. It shows the required document (lines 1, 9, 99, 9999, 10000,
99999, 100000) and lets you switch gutter widths to confirm that:

- short numbers stay right-aligned;
- numbers wider than the gutter render fully (never clipped);
- no underflow produces enormous padding;
- rendering stays correct after a terminal resize.

Controls: `+`/`-` change gutter width, `c` toggles CodeView/VirtualDocument,
`q`/`Esc` quits.

Run it:

```sh
./build/examples/terminal_ui_kit_example_line_number_formatting
```

## completion_popup_example

Demonstrates the `CompletionPopup` component: an input field with fuzzy
autocomplete backed by two selectable providers.

- **Immediate** provider — a synchronous local `SyncCompletionProvider` that
  resolves as you type.
- **Delayed** provider — a deterministic asynchronous provider that resolves
  after a fixed 250 ms delay, showing the loading state and the component's
  stale-result protection: type quickly and only the latest query's results
  appear; the older (stale) response is discarded.

Features shown: loading state, fuzzy filtering, category and description
rendering, acceptance (replacement-range aware), no-results state, provider
switching, and viewport-aware placement (the popup renders below the input when
there is room, and flips above / degrades to fewer rows on a narrow terminal —
resize the terminal to see it adapt).

Controls:

| Key       | Action                          |
|-----------|---------------------------------|
| type      | fuzzy-filter the dataset        |
| up/down   | move selection                  |
| enter/tab | accept the selected completion  |
| esc       | cancel the popup                |
| m         | toggle immediate / delayed mode |
| q         | quit                            |

Exit by pressing `q`.

## unified_diff_view

`unified_diff_view_example.cpp` — an interactive viewer for the
`UnifiedDiffView` component. It parses a unified diff with the canonical
`UnifiedDiffParser` and displays it as a virtualized, scrolling diff view. All
sample data is deterministic and self-contained (no network access).

Scenarios (cycle with `Tab`): normal single-file diff, multi-file diff, new
file, deleted file, binary file, multiple hunks, long lines, empty diff, and a
~100,000-line diff.

Controls:

```text
j/k or ↑↓   scroll / select a row
n/N         next / previous hunk
]/[         next / previous file
Enter       collapse / expand the current file
/           search (Enter cycles matches, Esc closes)
y           copy the selected line's source text
Tab         switch scenario
q/Esc       quit
```

The status line reports `File X/Y · Hunk A/B · Visible rows C–D of N`. The view
is virtualized, so the 100,000-line scenario stays responsive.

## multiline_editor

A full-screen multiline editor backed by `MultilineEditor`
(`include/terminal_ui_kit/editor/multiline_editor.h`), which internally owns an
`EditorDocument` and optionally integrates a `CommandHistory`.

```sh
# after configuring with examples enabled
./build/all-features/examples/multiline_editor/terminal_ui_kit_example_multiline_editor
```

The status bar shows the current line/column, total line count, viewport
position, history status and the last submitted value. Controls (defaults, all
configurable via `EditorKeyBindings`):

- Arrows / `Home` / `End` — move the cursor
- `Ctrl+Left` / `Ctrl+Right` — word navigation
- `Ctrl+Up` / `Ctrl+Down` — history previous / next (recall mode)
- `Backspace` / `Delete` — delete backward / forward
- `Enter` — insert a newline
- `Ctrl+Enter` — submit (calls `on_submit`, adds to `CommandHistory`)
- `Esc` — exit

> Note: `Ctrl+Enter` is bound to the kitty keyboard-protocol CSI-u sequence
> (`\x1b[13;5u`). Terminals that advertise and send that sequence distinguish it
> from Enter; terminals that do not will treat it as Enter (a newline). Rebind
> `EditorKeyBindings::submit` to a key your terminal emits (e.g.
> `\x1b[13;3u` for Alt+Enter) if needed.

## TranscriptView — `transcript_view_example`

Simulates an interactive coding-agent session rendered in a virtualized,
follow-end transcript. Demonstrates all primary `TranscriptView` behaviors on
deterministic, locally-generated data (no network access):

- heterogeneous blocks (text, Markdown, code, logs, status, diff, custom);
- a live mutable streaming tail that updates in place;
- follow-end auto-scroll that disables on manual scroll and resumes on `f`;
- mixed-height blocks;
- search with next/previous result navigation;
- bookmarks;
- copy and open-details callbacks;
- terminal resize handling;
- a 100,000-block large-data mode;
- streaming start/stop.

### Build

```sh
cmake --preset all-features
cmake --build --preset all-features --target terminal_ui_kit_example_transcript_view
build/all-features/examples/transcript_view_example/terminal_ui_kit_example_transcript_view
```

> The `all-features` preset builds optional Markdown/tree-sitter too. The
> example itself only requires the base `Components` module; to build it with
> just CMake defaults (plus examples) configure with
> `-DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON`.

### Controls

| Key       | Action                                    |
| --------- | ----------------------------------------- |
| `Space`   | Start / stop streaming                    |
| `f`       | Toggle follow-end                         |
| `/`       | Search (type a query; `Enter`/`Esc` close)|
| `n` / `N` | Next / previous search result             |
| `b`       | Add / remove bookmark on the active block |
| `Enter`   | Open block details                        |
| `y`       | Copy the active block's plain text        |
| `g` / `G` | Jump to beginning / end                   |
| `l`       | Generate a 100,000-block transcript       |
| `q` / `Esc`| Exit                                      |

### Behavior worth watching

- `Space` streams tokens into a single tail block; watch the block counter,
  which stays constant while the tail grows.
- Scroll up (arrow keys or wheel) turns `follow` from `FOLLOW` to `manual`;
  press `f` to resume following.
- Press `/`, type a few letters, then `n`/`N` to walk matches; matching blocks
  show a `●` gutter marker.
- Press `b` on an active block to bookmark it (gutter shows `◆`), `y` to copy,
  `Enter` to open details (status line reports both).
- Press `l` to render a 100,000-block transcript and verify navigation stays
  responsive.