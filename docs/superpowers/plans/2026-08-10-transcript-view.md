# TranscriptView Implementation Plan

Status: checked off as completed.

- [x] Design spec: `docs/superpowers/specs/2026-08-10-transcript-view-design.md`
- [x] `include/terminal_ui_kit/components/transcript_model.h` — block types
      (`TextBlock`, `MarkdownBlock`, `CodeBlock`, `LogBlock`, `DiffBlock`,
      `StatusBlock`, `CustomBlock`), `TranscriptItem` variant, `TranscriptModel`
      with mutable tail, cached search index, bookmarks, revision, clear.
- [x] `include/terminal_ui_kit/components/transcript.h` + `transcript.cc` —
      `TranscriptView` wrapping `VirtualList` with follow-end, search input,
      bookmarks, copy/details callbacks, keyboard mapping, fixed-height tail.
- [x] Build wiring: `src/terminal_ui_kit/CMakeLists.txt` (add `transcript.cc`),
      render a fixed-height tail so streaming never invalidates layout.
- [x] Tests: `tests/terminal_ui_kit/unit/transcript_model_test.cc` and
      `tests/terminal_ui_kit/rendering/transcript_test.cc`.
- [x] Benchmark: `benchmarks/transcript_benchmark.cc` (append 100k, streaming
      tail no-new-entry, viewport render, search index).
- [x] Example: `examples/transcript_view_example/transcript_view_example.cpp`
      with a simulated coding-agent session and all documented controls.
- [x] Docs: `examples/README.md`, changelog entry.
- [x] Verification: GCC and Clang strict builds (`-Werror`), ASan/UBSan,
      GoogleTest suite, benchmark run, example run.
- [x] Subagent review and review-fix commit.
- [x] Commit, push, open PR.