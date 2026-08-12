# Fix line-number formatting underflow

## Problem

The line-number gutter in `CodeView` and `VirtualDocument` builds the padded
number with `std::string(N - num.size(), ' ')` where `N` is a hard-coded
`int` (4 in `CodeView`, 5 in `VirtualDocument`) and `num.size()` is `std::size_t`
(64-bit unsigned). When a rendered line number has more digits than `N`,
`N - num.size()` underflows to a huge `std::size_t` value, so `std::string(...)`
allocates and renders an enormous padding region.

Affected sites (identical expression `std::string(N - num.size(), ' ') + num`):

- `src/terminal_ui_kit/components/code_view.cc` line 43 (highlighted path).
- `src/terminal_ui_kit/components/code_view.cc` line 84 (plain-text path).
- `src/terminal_ui_kit/components/virtual_document.cc` line 127.

## Approach

1. Extract a shared, header-only helper `format_line_number(std::size_t, std::size_t)`
   in `include/terminal_ui_kit/components/line_number_format.h`. The logic is
   genuinely duplicated across three call sites, so a shared helper is justified.
   The helper only pads when `num.size() < width`, making the subtraction always
   non-negative and eliminating underflow. Right-alignment (leading spaces) is
   preserved, and numbers wider than the gutter render in full with no padding.
   Header-only (inline) keeps the change out of CMake/Xmake source lists and the
   install target set.

2. Make the gutter width configurable so the behaviour can be exercised and tested:
   - add `std::size_t line_number_width = 4` to `CodeViewOptions`;
   - add `std::size_t line_number_width = 5` to `VirtualDocumentOptions`.
   Defaults equal today's hard-coded widths, so ordinary documents render exactly
   as before.

3. Use the helper at all three sites.

4. In `VirtualDocument`, the wrapped-line continuation gutter is hard-coded to
   `"       "` (7 spaces). Make it `line_number_width + 1` spaces so continuation
   lines stay aligned with the number column regardless of width. Default width 5
   gives 6 spaces (previously 7 — the continuation column was offset one column
   further than the number column; this is corrected as part of the gutter-width
   adaptation and is documented in the PR).

## Tests

- New rendering test file `tests/terminal_ui_kit/rendering/line_number_format_test.cc`
  covering the helper directly and both views:
  - numbers `1`, `9`, `99`, `9999`, `10000`, `99999`, `100000`, plus a large
    line number (e.g. `1000000`);
  - widths `0`, smaller than digit count, equal to digit count, larger than
    digit count;
  - assert full number renders (never truncated), no huge padding, and short
    numbers remain right-aligned.
- Render tests in `virtual_document_test.cc` for wrapped continuation alignment.

## Example

- `examples/line_number_formatting_example.cpp` — standalone interactive app
  using only the public `CodeView`/`VirtualDocument` API, showing the required
  sample document (lines `1`, `9`, `99`, `9999`, `10000`, `99999`, `100000`),
  with keys to cycle gutter width and to quit.
- Registered in `examples/CMakeLists.txt`, documented in `examples/README.md`.

## Build/verification

- CMake `debug` preset with GCC and Clang, warnings-as-errors.
- Sanitizers (ASan/UBSan) preset.
- Run ctest; run the example.
