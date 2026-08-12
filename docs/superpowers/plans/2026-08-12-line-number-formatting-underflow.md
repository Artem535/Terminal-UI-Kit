# Fix Line-Number Formatting Underflow — Implementation Plan

> **Task:** E1 — Fix unsigned underflow and incorrect padding when a rendered
> line number contains more digits than the configured gutter width.

## Problem

Three call sites format a line-number gutter as:

```cpp
std::string num = std::to_string(line_num);
num = std::string(width - num.size(), ' ') + num;   // width is 4 or 5
```

`num.size()` is `std::size_t` (unsigned). When the digit count exceeds `width`,
`width - num.size()` underflows to a huge `std::size_t`, so
`std::string(huge, ' ')` attempts an enormous (effectively unbounded)
allocation — a resource/DoS bug and an incorrect-padding bug.

Sites:
- `src/terminal_ui_kit/components/code_view.cc:43,84` (width 4).
- `src/terminal_ui_kit/components/virtual_document.cc:127` (width 5).

## Fix

### 1. Shared helper (Core, header-only inline)
Add `include/terminal_ui_kit/core/padded_text.h`:

```cpp
// Right-aligns `text` in a `width`-column field by left-padding with ASCII
// spaces. When `text` is longer than `width` (including `width == 0`) the
// text is returned unchanged: never truncated, and no padding is added, so
// there is no unsigned underflow of `width - text.size()`.
std::string pad_left_to_width(std::string_view text, std::size_t width);
```

The `width - text.size()` subtraction is guarded by `if (text.size() < width)`,
so it can never underflow. Exact-fit (`size() == width`) adds no padding.

### 2. Configurable gutter width
- `CodeViewOptions::std::size_t gutter_width = 4;`
- `VirtualDocumentOptions::std::size_t gutter_width = 5;`

Defaults match current hardcoded values, so ordinary documents render
byte-for-byte as before. `code_view.cc` uses `options.gutter_width`;
`virtual_document.cc` uses `options_.gutter_width`.

VirtualDocument continuation (sub_line != 0) lines are indented
`gutter_width + 1` spaces so they align with the number column's text start
(`gutter_width` digits + 1 separator space); at the default width 5 this is 6
spaces, and the first-sub-line gutter is also 6 (`5` + separator) — making the
gutter internally consistent.

### 3. Tests
- Unit (`unit/padded_text_test.cc`): covers `1`, `9`, `99`, `9999`, `10000`,
  `99999`, `100000`, a large valid number, width `0`, width < digits, width ==
  digits, width > digits.
- Rendering (`rendering/code_view_test.cc`, extend `rendering/virtual_document_test.cc`):
  verify long line numbers render fully (no truncation, no huge padding) and
  short numbers stay right-aligned, in both `CodeView` and `VirtualDocument`.

### 4. Example
`examples/line_number_formatting_example.cpp` — a standalone interactive app
that renders the required document (`1, 9, 99, 9999, 10000, 99999, 100000`)
through both `CodeView` and `VirtualDocument` with `show_line_numbers`, lets
the user switch gutter widths (`<`/`>` or `1`–`9`), and exits on `q`/`ESC`.
Registered in `examples/CMakeLists.txt` and documented in `examples/README.md`.

### 5. Build wiring
- CMake: add the new example target; register new test sources.
- Xmake: register the example target (Xmake is secondary; CMake is
  authoritative).

## Acceptance
- Shorter-than-width: right-aligned (unchanged).
- Exactly width: no padding.
- Longer-than-width: full, safe, no truncation, no huge allocation.
- Width 0: handled safely.
- No unsigned underflow.
- Ordinary documents unchanged (defaults preserved).
- Tests fail on the old code, pass on the new.
