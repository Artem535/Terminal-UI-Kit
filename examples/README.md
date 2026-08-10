# Examples

Runnable example applications demonstrate library components in isolation. Each
is a standalone `add_executable` target; build them with the `all-features` CMake
preset (or enable `TERMINAL_UI_KIT_BUILD_EXAMPLES=ON`).

| Example | Target | Demonstrates |
| --- | --- | --- |
| `theme_viewer` | `terminal_ui_kit_example_theme_viewer` | Theme system |
| `components_gallery` | `terminal_ui_kit_example_components_gallery` | Component gallery |
| `progress_viewer` | `terminal_ui_kit_example_progress_viewer` | Progress primitives |
| `task_dashboard` | `terminal_ui_kit_example_task_dashboard` | Composed dashboard |
| `virtual_list_viewer` | `terminal_ui_kit_example_virtual_list_viewer` | Virtualized lists |
| `streaming_log_viewer` | `terminal_ui_kit_example_streaming_log_viewer` | Streaming logs |
| `virtual_document_viewer` | `terminal_ui_kit_example_virtual_document_viewer` | Virtual document |
| `diff_parser` | `terminal_ui_kit_example_diff_parser` | Unified diff parser |
| `multiline_editor` | `terminal_ui_kit_example_multiline_editor` | MultilineEditor MVP |

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
