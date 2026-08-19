# H1 UnifiedDiffView Implementation Plan

> Benchmark task H1. Branch `hermes-t7-r2` builds on the canonical unified-diff
> parser/model baseline (`diff_parser_test.cc`, `UnifiedDiffParser`, `DiffFile`).
> Goal: a production-oriented, virtualized `UnifiedDiffView` component that
> renders a parsed `std::vector<diff::DiffFile>` and supports navigation, search,
> collapse, copy, resize stability, and 100,000-line performance.

## Goal

Implement a reusable `UnifiedDiffView` in `terminal_ui_kit/components/` that
turns the retained `std::vector<diff::DiffFile>` model (already produced by
`UnifiedDiffParser`) into a scrollable, virtualized FTXUI component. The view is
presentation-only: it never re-parses or re-generates the diff, it never renders
the whole document on a frame, and it never rebuilds its flattened layout when
the model and layout constraints are unchanged.

## Architecture

Build the view on the existing `VirtualListModel` (fixed-height rows) so
scrolling, selection, wheel, and viewport virtualization are inherited and
proven. Maintain a lazily-rebuilt flat row index over the retained `files_`
model:

```
Row { kind: FileHeader | HunkHeader | Line, file_index, hunk_index, line_index }
```

Rows store only indices into the owned `files_` model (no duplicated content
strings, no `std::string_view`/pointer retention). The flat `rows_` vector is
rebuilt only when `SetFiles` is called or a file's collapse state toggles —
never during rendering and never on resize. Fixed-height (1-line) rows plus
long-line truncation make resize a no-op for layout, keeping the selected
file/hunk/row logically stable across resize.

Render only the visible slice through `VirtualListModel` (inherited). The view
wraps the list in a `Renderer` that appends a status bar, and a `CatchEvent`
handler that implements the key bindings and search input.

### Long-line policy (documented)

Content is truncated to the available content width (viewport minus the line
number gutter and the `+`/`-`/space marker column) with a `…` continuation
marker; rows never wrap, so every row is exactly one terminal line and heights
are stable across resize. Narrow terminals shrink/disable the gutter first.

### Interaction map (view-owned)

- `j`/`k` and arrows: scroll / move selection (inherited from VirtualList).
- `n`/`N`: next/previous hunk while no search query is active; otherwise
  next/previous search result.
- `]`/`[`: next/previous file.
- `Enter`: collapse/expand the selected file.
- `/`: enter search mode; characters append, Backspace deletes, Enter jumps to
  the selected result and exits search mode, `Esc` cancels.
- `y`: invoke the copy callback with the selected line's original source text.
- `q`/`Esc`/`Tab` are handled by the example (exit / switch scenario), not the
  view, matching the required example control set.

## Global constraints

- C++20, repository Google C++ Style, `clang-format` (100 cols).
- `PascalCase` types, `snake_case` functions/locals/private fields (`_`).
- The parser/model baseline is not rewritten; only a minimal additive read
  accessor (`visible_range`) is added to `VirtualListModel`.
- Register the new component source in `src/terminal_ui_kit/CMakeLists.txt`
  (into `terminal_ui_kit_components`), tests in the rendering suite, the
  example in `examples/CMakeLists.txt` and `examples/xmake.lua`, plus a new
  benchmark in `benchmarks/CMakeLists.txt`.
- Tests render through `test_support::render_to_screen` / `render_to_text`.

## Task 1: Add `visible_range()` read accessor to VirtualListModel

**Files:** `include/terminal_ui_kit/components/virtual_list.h`,
`src/terminal_ui_kit/components/virtual_list.cc`,
`tests/terminal_ui_kit/rendering/virtual_list_test.cc`.

Add a non-mutating accessor that reports the currently rendered visible item
range so the diff view can build its `Visible rows A–B` status line. Track the
last computed begin/count in `Render`; return `std::nullopt` until laid out.
Add one test that renders a list and asserts the reported range.

## Task 2: Implement `UnifiedDiffView`

**Files:** `include/terminal_ui_kit/components/unified_diff_view.h`,
`src/terminal_ui_kit/components/unified_diff_view.cc`.

Public `UnifiedDiffView` (pimpl, mirroring `VirtualDocument`):

- `UnifiedDiffViewOptions { theme, enable_color, on_copy }`.
- `component()`, `SetFiles(std::vector<diff::DiffFile>)`, `files()`.
- File/hunk counters and `selected_file()`, `selected_hunk()`,
  `visible_range()`.
- Navigation: `next_hunk`, `previous_hunk`, `next_file`, `previous_file`,
  `toggle_collapse`, `collapsed(file)`, `set_search`, `search`, `next_search_result`,
  `previous_search_result`, `copy_selected`, `status_line()`.

Implementation builds `rows_`, computes gutter widths from the model's line
numbers, renders line rows with old/new numbers + marker + styled content
(`to_decorator`), file headers with new/deleted/binary/empty badges, hunk
headers with the raw header, and a status bar. Search is a case-insensitive
substring scan over line content stored as a stable match index list. Collapse
toggles an index set and marks `rows_` dirty for a one-time rebuild, re-anchoring
selection to the same file/hunk.

## Task 3: Rendering/interaction tests

**File:** `tests/terminal_ui_kit/rendering/unified_diff_view_test.cc`,
registered in the rendering `CMakeLists.txt`.

Cover: single file, multiple files, multiple hunks, new file, deleted file,
binary file, empty diff, empty file, long lines, no-color fallback, narrow
terminal, collapse/expand, hunk navigation, file navigation, search, selection,
copy callback, resize, large diff (100,000 lines), stable state after resize.

## Task 4: Performance benchmark + test

**Files:** `benchmarks/unified_diff_view_benchmark.cc`,
`benchmarks/CMakeLists.txt`.

Measure render/layout time and allocation behaviour on a 100,000-line diff;
assert only viewport rows are materialized per frame and that a second render
with unchanged constraints does not rebuild the flat layout.

## Task 5: Example application + docs + build registration

**Files:** `examples/unified_diff_view_example.cpp`,
`examples/CMakeLists.txt`, `examples/xmake.lua`, `examples/README.md`,
`docs/modules/ROOT/pages/changelog.adoc`.

Deterministic scenarios (normal, multi-file, new, deleted, binary, multi-hunk,
long lines, empty diff, 100,000-line), the required controls, a `File 2/5 ·
Hunk 3/8 · Visible rows 120–160` status line, and `Tab`/`q` handling.

## Task 6: Verification

- GCC + Clang strict builds with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
  -Werror`.
- CTest suites; ASan/UBSan (`sanitizers` preset); benchmark smoke run.
- Run the example under a PTY and verify the documented flows.
- Subagent review, then fix important findings in a follow-up commit.

## Task 7: Commit, push, PR

Create the branch off `main`, commit, push, open a PR linked to the benchmark
issue.
