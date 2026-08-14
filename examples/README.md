# Examples

Each example is a standalone executable demonstrating one component (or a small
group) in isolation. Only examples registered in
[`examples/CMakeLists.txt`](CMakeLists.txt) are built; enable them with the
`TERMINAL_UI_KIT_BUILD_EXAMPLES` option (or the `all-features` CMake preset).

Build and run (typical):

```sh
cmake --preset all-features
cmake --build --preset all-features
./build/all-features/examples/<name>/<executable>
```

| Example | Executable | What it demonstrates |
| ------- | ---------- | -------------------- |
| `components_gallery` | `terminal_ui_kit_example_components_gallery` | Status components, panels, modals, themes, code rendering |
| `theme_viewer` | `terminal_ui_kit_example_theme_viewer` | Semantic theme roles |
| `progress_viewer` | `terminal_ui_kit_example_progress_viewer` | Determinate and indeterminate progress |
| `task_dashboard` | `terminal_ui_kit_example_task_dashboard` | Hierarchical task state |
| `virtual_list_viewer` | `terminal_ui_kit_example_virtual_list_viewer` | A virtualized 100,000-row list |
| `streaming_log_viewer` | `terminal_ui_kit_example_streaming_log_viewer` | Live structured logs with ANSI styling |
| `virtual_document_viewer` | `terminal_ui_kit_example_virtual_document_viewer` | Incrementally updated wrapped text |
| `diff_parser` | `terminal_ui_kit_example_diff_parser` | Unified diff parsing to a plain-text model |
| `terminal_capabilities` | `terminal_ui_kit_example_terminal_capabilities` | Terminal-capability detection across presets |
| `markdown_viewer` (optional) | `terminal_ui_kit_example_markdown_viewer` | Markdown rendering (when the Markdown feature is enabled) |

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

```sh
cmake --preset all-features
cmake --build --preset all-features --target terminal_ui_kit_example_terminal_capabilities
./build/all-features/examples/terminal_ui_kit_example_terminal_capabilities
```