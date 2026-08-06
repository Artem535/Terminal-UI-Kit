# Examples

Each example is a standalone, runnable application that demonstrates one
component or workflow of Terminal UI Kit (PRD section 54). Examples are
registered in `examples/CMakeLists.txt` under CMake (authoritative) and
mirrored for discoverability in `examples/xmake.lua`.

## Running

```sh
cmake --preset all-features      # builds tests, examples, and benchmarks
# or, to build examples only:
cmake -S . -B build-examples -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON
cmake --build build-examples
```

Then run any example binary from its directory under `build*/examples/`.

## List

| Example | Description |
| --- | --- |
| `theme_viewer` | Browse and previews the built-in dark/light themes. |
| `components_gallery` | Gallery of the basic components. |
| `progress_viewer` | Deterministic and indeterminate progress bars. |
| `task_dashboard` | Task list dashboard built from progress components. |
| `virtual_list_viewer` | Virtualized fixed/variable-height list. |
| `streaming_log_viewer` | Streaming, wrapped, follow-mode log view. |
| `virtual_document_viewer` | Streaming wrapped document with selection and follow. |
| `diff_parser` | Unified-diff parser (pure data, no TUI). |
| `markdown_viewer` | Markdown rendering (enabled with `TERMINAL_UI_KIT_ENABLE_MARKDOWN`). |
| `line_number_formatting_example` | Configurable line-number gutter in `CodeView` and `VirtualDocument`, incl. numbers wider than the gutter and zero-width. |

## line_number_formatting_example

Demonstrates the configurable line-number gutter of `CodeView` and
`VirtualDocument`. It renders a sample document whose lines are

```text
1
9
99
9999
10000
99999
100000
```

and — in a second virtualized pane — a 100000-line document whose gutter line
numbers reach 100000. It demonstrates:

- right-alignment of numbers shorter than the gutter width;
- numbers equal to the gutter width rendering without padding;
- numbers wider than the gutter rendering fully (no clipping, no enormous
  padding);
- a zero gutter width being handled safely.

**Controls** (displayed in the UI):

- `[1]`–`[8]` — set the line-number gutter width;
- `[0]` — set a zero gutter width;
- `q` / `ESC` — quit.

The layout reflows on terminal resize. All sample data is deterministic and
local; no network access or external service is used.

```sh
./build*/examples/line_number_formatting_example/terminal_ui_kit_example_line_number_formatting
```