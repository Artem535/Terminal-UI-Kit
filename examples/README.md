# Examples

Each subdirectory is a standalone example application demonstrating one
library component in isolation (PRD section 54). Examples are only built when
`TERMINAL_UI_KIT_BUILD_EXAMPLES` is ON (the `all-features` CMake preset
enables it).

| Example | Component | Description |
| --- | --- | --- |
| `theme_viewer` | Theme | Browse the built-in themes and color swatches. |
| `components_gallery` | Components | Gallery of shared components. |
| `progress_viewer` | Progress | Interactive determinate/indeterminate progress. |
| `task_dashboard` | Progress tree | Navigable task/progress tree. |
| `virtual_list_viewer` | Virtual list | Virtualized large list rendering. |
| `streaming_log_viewer` | Streaming/log | Streaming log viewer. |
| `virtual_document_viewer` | Virtual document | Virtualized document rendering. |
| `diff_parser` | Diff | Parse `git diff --no-color` output from stdin. |
| `markdown_viewer` | Markdown | Markdown document viewer. |
| `terminal_capabilities_example` | Terminal | Detected terminal-capability table with selectable presets. |

## Run

Build with the `all-features` preset, then run the binary directly. The
interactive examples are driven by their on-screen key hints; `q` quits.

```sh
cmake --preset all-features
cmake --build --preset all-features
build/all-features/examples/terminal_capabilities_example/terminal_ui_kit_example_terminal_capabilities
```