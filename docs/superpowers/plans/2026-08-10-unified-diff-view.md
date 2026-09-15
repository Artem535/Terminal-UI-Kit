# UnifiedDiffView Implementation Plan

Status: Implemented
Date: 2026-08-10

Checklist for implementing a reusable virtualized `UnifiedDiffView` on the
canonical unified-diff parser/model (PRD section 29).

## Model compatibility fix
- [x] Add minimal additive `DiffFile::binary` field + parser detection of
      `Binary files ... differ` (required for binary-file rendering).

## View
- [x] `include/terminal_ui_kit/components/unified_diff_view.h`:
      `UnifiedDiffViewOptions` + `UnifiedDiffView` class.
- [x] `src/terminal_ui_kit/components/unified_diff_view.cc`:
      row flattening, classification, gutter, virtualization via
      `VirtualListModel`, search, navigation, copy, resize stability.

## Rendering coverage
- [x] file headers; hunk headers; context; additions; deletions; old/new line
      numbers; new files; deleted files; binary files; empty files; long lines
      (truncate policy); no-color fallback; narrow-terminal fallback.

## Interaction coverage
- [x] vertical scrolling; next/prev hunk; next/prev file; collapse/expand;
      row/line selection; search; jump to match; copy callback returning
      original source text; stable state after resize.

## Layout coverage
- [x] viewport-based rendering; correct gutter sizing; documented long-line
      policy; stable selection/hunk/file across resize; collapse stable during
      navigation.

## Tests
- [x] `tests/terminal_ui_kit/rendering/unified_diff_view_test.cc` covering:
      single/multi file, multiple hunks, new/deleted/binary/empty file, empty
      diff, long lines, no-color, narrow terminal, collapse/expand, hunk & file
      navigation, search, selection, copy callback, resize, large diff, stable
      state after resize.

## Performance
- [x] `benchmarks/unified_diff_view_benchmark.cc` with a 100,000-line diff:
      render-time measurement and verification that only visible rows render.

## Example
- [x] `examples/unified_diff_view_example.cpp` (registered in CMake + Xmake):
      deterministic scenarios incl. 100,000-line diff, required controls, status
      line, Tab scenario switch, q/Esc exit.

## Build/docs
- [x] CMake source + example + benchmark registration; Xmake registration;
      `examples/README.md` entry; design spec + this plan.

## Verification
- [x] GCC debug build + tests.
- [x] Clang debug build + tests.
- [x] Warnings-as-errors (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion
      -Werror`).
- [x] ASan + UBSan (sanitizers preset) tests.
- [x] Run example, verify main flows.
- [x] Subagent review + fix findings.
- [x] Commit, push, open PR.
