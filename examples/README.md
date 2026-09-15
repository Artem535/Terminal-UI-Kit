# Examples

Each subdirectory or single-file example demonstrates one or more Terminal UI
Kit components in isolation. Examples are built with CMake when
`TERMINAL_UI_KIT_BUILD_EXAMPLES=ON`, and mirrored for the developer-facing
Xmake frontend.

| Example | Component(s) | Description |
| --- | --- | --- |
| `theme_viewer` | Theme | Browse and compare themes. |
| `components_gallery` | Multiple | Interactive gallery of components. |
| `progress_viewer` | Progress | Progress bars and indeterminate spinners. |
| `task_dashboard` | Progress tree / status | A task dashboard UI. |
| `virtual_list_viewer` | VirtualList | Virtualized list with scrolling. |
| `streaming_log_viewer` | LogView | Streaming log with follow/pause. |
| `virtual_document_viewer` | VirtualDocument | Streaming wrapped document viewer. |
| `diff_parser` | UnifiedDiffParser | Parse unified diffs from stdin. |
| `line_number_formatting_example.cpp` | CodeView, VirtualDocument, `format_line_number` | Right-aligned line-number gutters under configurable widths. |

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

Build and run:

```sh
cmake --preset all-features
cmake --build --preset all-features --target terminal_ui_kit_example_line_number_formatting
./build/all-features/examples/terminal_ui_kit_example_line_number_formatting
```
