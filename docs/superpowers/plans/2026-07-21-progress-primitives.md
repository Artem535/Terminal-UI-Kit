# Progress Primitives Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add determinate and indeterminate progress primitives plus an interactive viewer example.

**Architecture:** `ProgressBar` is a pure `ftxui::Element` function with shared options and styles. `IndeterminateProgress` is a non-focusable `ComponentBase` that reuses the same style rendering rules and advances a wrapping segment. The example composes both with existing `KeyHintBar`.

**Tech Stack:** C++20, FTXUI, GoogleTest, CMake.

## Global Constraints

- C++20; PascalCase types and snake_case functions/files.
- Rendering tests use `test_support::render_to_screen`/`render_to_text`, never a physical terminal.
- No Xmake changes.
- Four styles: Unicode blocks, ASCII, dots, Braille; default width 20, Unicode blocks, visible percentage.

---

### Task 1: ProgressBar and shared options

**Files:** create `include/terminal_ui_kit/components/progress_bar.h`, `src/terminal_ui_kit/components/progress_bar.cc`, `tests/terminal_ui_kit/rendering/progress_bar_test.cc`; modify Components and rendering CMake lists.

**Interfaces:** define `enum class ProgressStyle { kUnicodeBlocks, kAscii, kDots, kBraille };` and `struct ProgressBarOptions { std::size_t width = 20; ProgressStyle style = ProgressStyle::kUnicodeBlocks; bool show_percentage = true; };`. Provide `ProgressBar(double fraction, const Theme&, ProgressBarOptions = {})` and `ProgressBar(double value, double total, const Theme&, ProgressBarOptions = {})`.

- [ ] Write failing tests for 50% Unicode output, ASCII/dots/Braille styles, clamp of negative/over-one/non-finite values, non-positive total yielding 0%, zero width empty output, hidden percentage, and accent/muted/secondary pixels.
- [ ] Build `terminal_ui_kit_rendering_tests`; confirm missing `progress_bar.h` failure.
- [ ] Implement normalization using `std::isfinite`, clamp to `[0,1]`, per-style glyph generation, width-zero empty element, and themed filled/track/percentage hbox.
- [ ] Build and run `ctest --test-dir build-debug --output-on-failure -R ProgressBar`; run the full suite.
- [ ] Commit: `Add ProgressBar styles and normalization`.

### Task 2: IndeterminateProgress

**Files:** create `include/terminal_ui_kit/components/indeterminate_progress.h`, `src/terminal_ui_kit/components/indeterminate_progress.cc`, `tests/terminal_ui_kit/rendering/indeterminate_progress_test.cc`; modify Components and rendering CMake lists.

**Interfaces:** provide `ftxui::Component IndeterminateProgress(const Theme&, ProgressBarOptions = {}, std::size_t segment_width = 4, std::chrono::milliseconds frame_duration = 80ms);`.

- [ ] Write failing tests for non-focusability, initial segment, frame advance only after duration, wrap-around, zero width, and segment clamping to width.
- [ ] Build target and confirm missing-header RED failure.
- [ ] Implement a `ComponentBase` that requests frames from `Render()`, accumulates duration, moves a segment left-to-right with modulo wrap, and uses shared style glyph rules.
- [ ] Run focused `IndeterminateProgress` tests and the full suite.
- [ ] Commit: `Add IndeterminateProgress`.

### Task 3: progress_viewer example

**Files:** create `examples/progress_viewer/main.cc`, `examples/progress_viewer/CMakeLists.txt`; modify `examples/CMakeLists.txt`.

- [ ] Create a fullscreen viewer listing all four styles, each with a determinate bar and an indeterminate bar.
- [ ] Add keys `left`/`right` to change percentage by 5%, `s` to cycle style, `q` to quit; render these through `KeyHintBar`.
- [ ] Add executable `terminal_ui_kit_example_progress_viewer` linked to `TerminalUiKit::Components` and FTXUI libraries; register the subdirectory.
- [ ] Configure examples, build `terminal_ui_kit_example_progress_viewer`, run full tests, and smoke-run it in a pseudo-terminal.
- [ ] Commit: `Add progress viewer example`.

## Self-review

- Task 1 covers all determinate options, safety semantics, styles and colors.
- Task 2 covers animation, narrow widths and component lifecycle.
- Task 3 covers the required interactive example.
