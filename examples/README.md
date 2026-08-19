# Examples

Standalone runnable applications demonstrating Terminal UI Kit components in
isolation (PRD section 54). Each example builds as its own executable; most
live in a subdirectory, and a few are single files at the top level.

| Example | Source | Demonstrates |
| --- | --- | --- |
| `theme_viewer` | `examples/theme_viewer/` | Theme roles and palettes |
| `components_gallery` | `examples/components_gallery/` | Basic components |
| `progress_viewer` | `examples/progress_viewer/` | Progress indicators |
| `task_dashboard` | `examples/task_dashboard/` | Composite dashboard |
| `virtual_list_viewer` | `examples/virtual_list_viewer/` | Virtualized lists |
| `streaming_log_viewer` | `examples/streaming_log_viewer/` | Streaming document view |
| `virtual_document_viewer` | `examples/virtual_document_viewer/` | Virtual document view |
| `diff_parser` | `examples/diff_parser/` | Unified-diff parser/model (non-interactive) |
| `unified_diff_view` | `examples/unified_diff_view_example.cpp` | Virtualized unified-diff view |

## Building

CMake builds the examples when `TERMINAL_UI_KIT_BUILD_EXAMPLES=ON`, e.g.:

```sh
cmake --preset all-features
cmake --build --preset all-features
```

Xmake mirrors the examples under `--examples` (secondary frontend).

## `unified_diff_view` example

An interactive demo of the virtualized `UnifiedDiffView`. It loads
deterministic in-memory diff scenarios (no network access or external service)
and demonstrates scrolling, hunk/file navigation, collapse/expand, search,
copy, and resize-safe rendering.

### Scenarios (switch with `Tab`)

- Normal single-file diff
- Multi-file diff
- New file
- Deleted file
- Binary file
- Multiple hunks
- Long lines
- Empty diff
- Large diff (100,000 lines)

### Controls

| Key | Action |
| --- | --- |
| `j` / `k` or arrows | Scroll |
| `n` / `N` | Next / previous hunk (or search result while searching) |
| `]` / `[` | Next / previous file |
| `Enter` | Collapse / expand the selected file |
| `/` | Search (type to filter, `Enter` to jump, `Esc` to cancel) |
| `y` | Invoke the copy callback for the selected line |
| `Tab` | Switch scenario |
| `q` / `Esc` | Exit |

The status line reports `File X/N · Hunk Y/M · Visible rows A–B`, e.g.
`File 2/5 · Hunk 3/8 · Visible rows 120–160`, plus the active search match
count when a search is set.
