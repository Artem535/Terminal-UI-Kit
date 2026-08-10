# UnifiedDiffView Design

Status: Implemented
Date: 2026-08-10
Scope: `include/terminal_ui_kit/components/unified_diff_view.h`,
`src/terminal_ui_kit/components/unified_diff_view.cc`, tests, example, benchmark.

## Objective

Provide a reusable, virtualized, interactive unified-diff view on top of the
canonical `UnifiedDiffParser` / `DiffFile` model (PRD section 28). The view is
display-only: it never generates diffs and depends on FTXUI only for rendering
and event capture.

## Data / ownership

The view takes a `std::vector<DiffFile>` by value (move-constructed) and
retains it. All cross references (file / hunk / line) are stored as `std::size_t`
indices into that retained model — no `string_view`, span, pointer, or reference
to a caller-owned buffer is retained, satisfying the benchmark's safe-owning-type
rule.

The parser's model cannot distinguish a binary file (whose `Binary files ...
differ` notice is currently discarded) from an empty / unchanged file. The
rendering requirements demand a distinct binary-file notice, so a minimal,
additive compatibility field `bool binary = false;` is added to `DiffFile` and
the parser sets it when it encounters a `Binary files` body line. This is the
smallest possible extension of the canonical model, clearly required by the task.

## Virtualization

Diff rows are single-screen-height by construction: long lines are truncated
(documented policy, see "Long lines"), and file / hunk / notice headers are one
line each. The flattened presentation is therefore a fixed-height row list
`rows_`, built once per model + collapse-state change, never per frame.

The existing `VirtualListModel` provides the virtualized windowing (prefix-sum
scrolling, mouse wheel, page up/down, home/end, selection, and resize-stable
scroll offsets) over that fixed-height row list. Because it virtualizes by row
index over a stable layout, a 100,000-line diff renders only the visible
rows and does not rebuild the flattened layout while model and collapse state
are unchanged.

## Flattened layout (`rows_`)

`DiffRowKind`: `kFileHeader`, `kNewFileNotice`, `kDeletedFileNotice`,
`kBinaryNotice`, `kEmptyNotice`, `kHunkHeader`, `kLine`.

File-level classification:
* old\_path == `/dev/null` → new file (additions only).
* new\_path == `/dev/null` → deleted file (deletions only).
* `binary` flag → binary notice, no hunks.
* no hunks and not binary → empty-file notice.
* otherwise → hunks (hunk header row + one row per line).

Gutter: old/new 1-based line numbers, each right-aligned in a fixed-width column
derived from the widest line number in the model; blank where a line has no
counterpart (added lines have no old number, deleted lines no new number).

Long-line policy: content is truncated with a single trailing `…` when it
exceeds the available content width; it is never wrapped, keeping row height at
1. The policy is documented and surfaced in the header.

Narrow-terminal fallback: when the viewport is too narrow for the gutter plus a
minimum content width, line numbers are hidden entirely (the remaining width is
given to content). This keeps single-line rows and navigation stable.

## Interaction

`UnifiedDiffView` wraps `VirtualListModel` and adds a `CatchEvent` layer:

* `j`/`k` and up/down — scroll/select (VirtualList).
* `n`/`N` — next/previous hunk (scroll to and select the hunk header row).
* `]`/`[` — next/previous file.
* Enter — collapse/expand the current file (rebuilds `rows_`, preserves the
  selected file and best-effort selection).
* `/` — enter search (see below).
* `y` — invoke the copy callback with the selected line's original source text
  (plain, marker-stripped content; `content` spans concatenated).
* Mouse wheel, PageUp/Down, Home/End — delegated to VirtualList.

Collapsed state is a `std::vector<bool>` over files, stored independently of the
row list, so it remains stable during navigation and is consulted when `rows_`
is rebuilt.

## Search

`set_search(query)` scans the retained model once, collecting for each hunk line
whose content contains the query a `{row_index, line_index}` match. The current
match advances with `jump_to_next_match` / `jump_to_prev_match` and the
`scroll_to_match` keeps the matched row visible. The match set is rebuilt only on
`set_search` or on layout change, not per frame.

## Resize stability

Row heights are 1 for every row and independent of width, so the VirtualList
scroll offset (in rows) — and therefore the selected file / hunk / row — is
unchanged across a width or height change. Gutter visibility may flip at the
narrow threshold, but selection and scroll position are preserved.

## Status

`Status { file_index, file_count, hunk_index, hunk_count, first_visible,
last_visible, row_count }` is exposed so the example can render the required
`File 2/5 · Hunk 3/8 · Visible rows 120–160` status line.

## No-color

`UnifiedDiffViewOptions::color` (default true) selects the theme; when false the
view renders through `without_color(theme)` so diff meaning is carried by the
`+`/`-` markers and the gutter, not by color.

## Performance

`benchmarks/unified_diff_view_benchmark.cc` builds a ~100,020-row diff (ten
files, 100,000 diff lines) and measures render+paint time and navigation cost at
a 120x40 viewport. Measured on the development machine (16x 3.8 GHz, GCC 16,
`-O2`):

| Benchmark                             | Per-iteration time | row_count |
|--------------------------------------|--------------------|-----------|
| RenderAndPaint (full viewport)       | ~0.7–2.0 ms        | 100.02k   |
| Navigation (scroll jump + render)    | ~0.7–2.1 ms        | 100.02k   |

Only the visible rows (~40) are rendered per frame; the flattened row list is
built once per model+collapse change and never rebuilt per frame. Both numbers
are well within a 16 ms frame budget, so interaction with the large diff remains
responsive.
