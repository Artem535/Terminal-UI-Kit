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
| `diff_parser/` | `terminal_ui_kit_example_diff_parser` | Parses a unified diff from stdin and prints a plain-text summary. | No (stdin) |
| `command_history_example/` | `terminal_ui_kit_example_command_history` | Bounded, navigable command history with search, persistence, and sensitive-command policy. | Yes |
| `markdown_viewer/` | `terminal_ui_kit_example_markdown_viewer` | Markdown rendering. Only built when `TERMINAL_UI_KIT_ENABLE_MARKDOWN=ON`. | Yes |

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
| `c` | Clear history |
| `t` | Toggle sensitive mode |
| `q` | Quit |

The underlying model is tested independently of the terminal in
`tests/terminal_ui_kit/unit/command_history_test.cc`, which covers navigation
boundaries, capacity eviction, duplicate filtering, search ordering, capacity
`0`, persistence failure, and sensitive-command persistence suppression.