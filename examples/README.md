# Examples

Standalone applications demonstrating each Terminal UI Kit component in
isolation. Each is a separate executable; build them with CMake or Xmake.

## Building

```sh
cmake --preset all-features     # or your own build dir with examples ON
cmake --build --preset all-features --target terminal_ui_kit_example_<name>
```

Every example is registered in both `examples/CMakeLists.txt` (CMake) and
`xmake.lua` (Xmake). Run an example directly in a terminal; most are
fullscreen/alternate-screen apps that restore your terminal on exit.

## Available examples

| Name                          | Executable                                | What it demonstrates                     |
| ----------------------------- | ----------------------------------------- | ---------------------------------------- |
| `theme_viewer`                | `terminal_ui_kit_example_theme_viewer`    | Color themes and styling.                |
| `components_gallery`          | `terminal_ui_kit_example_components_gallery` | A gallery of built-in widgets.        |
| `progress_viewer`             | `terminal_ui_kit_example_progress_viewer` | Progress bars and indeterminate progress.|
| `task_dashboard`              | `terminal_ui_kit_example_task_dashboard`  | Task/dashboard composition.              |
| `virtual_list_viewer`         | `terminal_ui_kit_example_virtual_list_viewer` | Virtualized large lists.              |
| `streaming_log_viewer`        | `terminal_ui_kit_example_streaming_log_viewer` | Streaming log lines in real time.    |
| `virtual_document_viewer`     | `terminal_ui_kit_example_virtual_document_viewer` | Virtualized large documents.       |
| `diff_parser`                 | `terminal_ui_kit_example_diff_parser`     | Unified diff parsing (non-interactive, reads stdin). |
| `completion_popup`            | `terminal_ui_kit_example_completion_popup`| Autocomplete popup with sync + async providers. |

## `completion_popup`

An input field with two selectable completion providers:

1. **Immediate** local provider — synchronous filtering over a deterministic
   vocabulary.
2. **Delayed** provider — asynchronous (completes ~50 ms later from a
   background thread), demonstrating the loading state, the async provider
   adapter, and stale-result protection (typing faster than the provider
   supersedes older in-flight requests).

Demonstrates fuzzy filtering, categories, descriptions, `Enter`/`Tab`
acceptance, `Escape` dismissal, and viewport-aware above/below placement that
adjusts when the terminal is resized. No network access.

**Controls:** type to filter · `up`/`down` select · `enter`/`tab` accept ·
`escape` dismiss · `1` immediate provider · `2` delayed provider · `q` quit.
