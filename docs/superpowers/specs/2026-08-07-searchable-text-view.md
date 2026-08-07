= Searchable Text View — Design Spec

== Summary

A reusable, searchable text view component (`SearchableTextView`) and a
pure-data search engine (`SearchEngine`) for Terminal-UI-Kit. Supports literal
and regular-expression search, case-sensitive/insensitive modes, match
navigation with wrap-around, per-line UTF-8-safe byte offsets, viewport
integration (auto-scroll to the active match, re-wrap on resize), and
incremental query updates.

== Motivation (PRD)

Editor-like find-in-text is a recurring need for log viewers, code views, and
markdown/text browsers. This delivers a domain-neutral, reusable find
experience without coupling it to any single existing component.

== Architecture

Separated concerns (each owned by a distinct type):

* `SearchOptions` — `case_sensitive`, `use_regex`.
* `TextMatch` — `{ line, start_byte, end_byte }` (byte offsets into a line).
* `SearchEngine::search` — pure function over `span<const string_view>`:
  produces an ordered match list and a `SearchStatus`.
* `MatchNavigator` — owns the match list + active-match cursor (next/prev with
  wrap, cursor preserved across a shrinking result set).
* `SearchableTextView` — FTXUI component. Wraps the source with
  `WrappedDocument`, virtualizes with `VirtualListModel`, drives the search
  prompt and navigation, and slices rendered segments at match boundaries.

`SearchEngine` links only `Core` (no FTXUI), so it is unit-testable and
reusable without a terminal.

== Matching semantics

* Byte-oriented over each line's UTF-8 bytes; offsets never split a UTF-8
  sequence for a valid pattern matched in valid text.
* Literal: ASCII-case folding for case-insensitive matches; non-ASCII code
  points match in exact case. Matches are non-overlapping.
* Regex: `std::regex` (ECMAScript) with `std::regex_constants::icase`;
  invalid patterns yield `kInvalidRegex`. Zero-length matches are skipped.

== Performance

The view caches the match set computed when the query is applied; `Render()`
only reads the cache and never re-runs the search. A benchmark
(`benchmarks/search_benchmark.cc`) and a test (status-callback counter) verify
this.

== Files

New module `search/` (engine), component `searchable_text_view.h/.cc` in
`components/`, unit + rendering tests, `benchmarks/search_benchmark.cc`, and
`examples/searchable_text_view_example/`.
