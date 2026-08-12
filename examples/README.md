# Examples

Standalone, runnable example applications that demonstrate `Terminal-UI-Kit`
components in isolation. Each example is a separate executable, behaves
deterministically, requires no network access, and exits through a documented
key.

## Line Number Formatting (`terminal_ui_kit_example_line_number_formatting`)

Source: `line_number_formatting_example.cpp`

Demonstrates the line-number gutter of `CodeView` and `VirtualDocument` after
the unsigned-underflow fix:

- numbers shorter than the configured gutter width are right-aligned;
- numbers exactly equal to the width render without padding;
- numbers longer than the width render in full — no truncation and no enormous
  padding run;
- a gutter width of `0` is handled safely.

The document has 100,000 lines, so the gutter shows line numbers from 1 digit
up to 6 digits (`1`, `9`, `99`, `9999`, `10000`, `99999`, `100000`). Both views
are shown side by side; resizing the terminal re-renders without rebuilding.

Controls:

| Key    | Action           |
| ------ | ---------------- |
| `+`/`-`| adjust gutter width |
| `1..9` | set gutter width |
| `q`/`ESC` | quit          |

Build (CMake, authoritative):

```sh
cmake --preset all-features
cmake --build --preset all-features --target terminal_ui_kit_example_line_number_formatting
./build/all-features/examples/terminal_ui_kit_example_line_number_formatting
```

## Other examples

- `theme_viewer` — browse the built-in themes.
- `components_gallery` — gallery of basic components.
- `progress_viewer` — progress bar / indeterminate progress.
- `task_dashboard` — composite dashboard.
- `virtual_list_viewer` — virtualized list.
- `streaming_log_viewer` — streaming log view.
- `virtual_document_viewer` — virtualized wrapped document.
- `diff_parser` — unified diff parser demo.
- `markdown_viewer` — Markdown rendering (built when Markdown is enabled).