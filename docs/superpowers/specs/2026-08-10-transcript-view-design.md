# TranscriptView Design

## Problem

Coding-agent CLIs need a transcript of heterogeneous blocks — plain text,
Markdown, code, logs, diffs, status — with a live streaming tail that updates
in place, follow-end behaviour, search, bookmarks, and copy/details actions,
all backed by a virtualized viewport so a 100,000-block session stays
interactive.

## Architecture

`TranscriptView` reuses the existing `VirtualList` component for virtualization
(mixed-height, prefix-sum layout, viewport-only rendering) exactly as `LogView`
does. A separate `TranscriptModel` holds the data (blocks, the mutable tail,
search index, bookmarks) so the model stays testable without a terminal, per
AGENTS.md "keep models separate from views".

```
TranscriptModel (data, tail, search, bookmarks)
      ↓  block_count / block_at / block_lines
TranscriptView wraps VirtualList (render_item, estimate_height)
      ↓  follow, search, bookmarks, copy/details events
FTXUI Component
```

## Block types

A `TranscriptItem` is a `std::variant` of owning, immutable-after-append value
types. All data is `std::string`/enum owning types — no retained `string_view`
or pointers. The tail is *one* variant stored in the model; appending a chunk
mutates that single entry rather than inserting a new one.

```cpp
using TranscriptItem = std::variant<
    TextBlock, MarkdownBlock, CodeBlock, LogBlock, DiffBlock, StatusBlock, CustomBlock>;
```

## Mutable tail

- `begin_tail(item)` appends one block and records its index as the tail.
- `append_tail(chunk)` / `replace_tail(content)` mutate that single block in
  place (no new transcript entry, no change to `block_count()`).
- `finalize_tail()` clears the tail flag; the block becomes an ordinary
  completed block.
- The view renders the tail at a **fixed** `tail_display_height` (last N lines,
  padded), so appending never changes the block's measured height and never
  invalidates the virtual list prefix sums → O(1) per appended token, no
  full-transcript relayout.

## Search, bookmarks, actions

- `find(query)` lazily rebuilds a cached `matches_` index of block indices,
  keyed by `(query, revision)`; it is not rebuilt per frame unless the query or
  data revision changed.
- `next_match` / `previous_match` navigate the cached hit list (wrapping).
- Bookmarks are a `std::set<size_t>`; `toggle_bookmark` / `is_bookmarked`.
- The view's `CatchEvent` layer maps keys: arrows wheel → follow off; `End`/`f`
  → follow on; `/` search input; `n`/`N` next/previous; `b` bookmark; `y` copy;
  `Enter` details; `g`/`G` beginning/end.

## Performance

- 100k blocks: viewport renders only visible cells; prefix sums built once.
- Streaming tail: fixed display height keeps prefix sums valid; block count
  constant → no full rebuild per token.
- Search: cached on `(query, revision)`.
- Documented in `benchmarks/transcript_benchmark.cc`.

## Dependencies

Lives in `components/` and links `TerminalUiKit::Components` +
`TerminalUiKit::Document` (for `LogSeverity`). Rendering is self-contained
(ftxui primitives + `StatusIndicator`); Markdown/diff/syntax are not required so
the component works in the minimal FTXUI-only configuration.