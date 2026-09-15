# Implementation Plan: Searchable TextView (Task M2)

## Goal

Add a reusable, searchable text view component to Terminal-UI-Kit with literal +
regex search, source byte offsets, match navigation (next/prev with wrap),
viewport integration, UTF-8 safety, and rendering tests. Deliver a standalone
example and a benchmark, wired into CMake and Xmake.

## Architecture

Separation of concerns (per requirement):

1. **Search options** — `SearchOptions { case_sensitive, use_regex }`.
2. **Match model** — `TextMatch { line, start_byte, end_byte }` and a
   `MatchNavigator` that owns the ordered match list + active-match cursor
   (next/prev wrap, stable across a shrinking result set).
3. **Search engine** — `SearchEngine::search(lines, query, options, out)` :
   pure data, no FTXUI, no retained views/pointers; returns a `SearchStatus`
   and fills an out-vector of matches. Link only `Core`.
4. **Viewport / navigation** — reused `WrappedDocument` + `VirtualListModel`
   (same pattern as `VirtualDocumentImpl`) for width-based wrapping and
   scrolling; active match auto-scrolls into view.
5. **Rendering** — `SearchableTextView` FTXUI component that slices each
   wrapped display segment at match boundaries and applies distinct styles
   (plain match vs active match), mirroring `VirtualDocumentImpl`.

New files:
- `include/terminal_ui_kit/search/search_engine.h`
- `src/terminal_ui_kit/search/search_engine.cc`
- `include/terminal_ui_kit/components/searchable_text_view.h`
- `src/terminal_ui_kit/components/searchable_text_view.cc`
- `tests/terminal_ui_kit/unit/search_engine_test.cc`
- `tests/terminal_ui_kit/rendering/searchable_text_view_test.cc`
- `benchmarks/search_benchmark.cc`
- `examples/searchable_text_view_example/main.cc`
- `examples/README.md`

New module target `terminal_ui_kit_search` (`TerminalUiKit::Search`), linked by
`Components`. Added to `TerminalUiKit::All` and install/export set, and to the
Xmake module list.

## Matching semantics (documented in the header)

- Matching operates on the **raw UTF-8 bytes** of each source line; `TextMatch`
  offsets are true byte offsets into that line's UTF-8 encoding.
- Literal search uses an ASCII-folding byte comparison (KMP over folded query);
  **ASCII letters are case-folded, non-ASCII code points match in exact case**.
- Regex mode uses `std::regex` (ECMAScript) over the line bytes, with
  `std::regex_constants::icase` in case-insensitive mode. Invalid expressions
  yield `kInvalidRegex`.
- Because a valid UTF-8 pattern matched in valid UTF-8 text aligns on
  codepoint boundaries, byte offsets never split a multi-byte sequence.
- The source document is never modified; search reads it without copying lines.

## Perf guarantee

The view caches the last computed match set, keyed by (document revision,
query, options). `Render()` only reads the cache — it never rebuilds the
search index on a plain re-render. The benchmark asserts/marks this.

## Required behaviors

- Incremental query updates while the search prompt is open (`/`, type,
  Enter/apply); Escape cancels.
- next/prev with wrap; current index + total count surfaced.
- All visible matches highlighted; active match in a distinct style.
- Auto-scroll to active match.
- Invalid-regex and no-results states surfaced via `SearchStatus`.
- UTF-8 preserved end-to-end; no corruption of the source.

## Verification

- GCC + Clang strict (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`).
- Unit + rendering tests; ASan/UBSan (Clang sanitizer build).
- Benchmark on a large document; example run via `scripts/pty-smoke-example.py`.
