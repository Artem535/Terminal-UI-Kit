# Plan: Fix line-number formatting underflow (benchmark E1)

## Problem

`CodeView` (`src/terminal_ui_kit/components/code_view.cc`) and
`VirtualDocument` (`src/terminal_ui_kit/components/virtual_document.cc`)
pad the line-number gutter with:

    std::string(fixed_width - num.size(), ' ') + num;

When a rendered line number has more digits than the fixed gutter width,
`fixed_width - num.size()` underflows `std::size_t` to a huge value, so the
view allocates/renders an enormous amount of padding (or crashes).

## Fix

- Add a shared core helper `FormatLineNumber(number, width)` that
  right-aligns a number to `width` columns but returns it unpadded (never
  truncated, no huge padding) when `digits.size() >= width`. It performs the
  subtraction only after the `>=` guard, so no underflow; width `0` is safe
  (always returns the bare number).
- Make the gutter width configurable:
  - `CodeViewOptions::line_number_width = 4` (matches current hard-coded 4).
  - `VirtualDocumentOptions::line_number_width = 5` (matches current).
- Replace both underflow sites in `code_view.cc` and the one in
  `virtual_document.cc` with the helper. Keep continuation (wrapped) sub-line
  indent tracking the width while preserving current visual output.

## Steps

- [x] Add `include/terminal_ui_kit/core/line_number.h` inline helper.
- [x] Add `line_number_width` to `CodeViewOptions`; use helper in `code_view.cc`.
- [x] Add `line_number_width` to `VirtualDocumentOptions`; use helper in
      `virtual_document.cc`; make continuation indent width-aware.
- [x] Add unit test `line_number_test.cc` for the helper (incl. required values,
      width 0, width < / == / > digit count).
- [x] Add rendering test `code_view_test.cc` + extend VirtualDocument rendering
      tests for wide numbers.
- [x] Add example `examples/line_number_formatting_example/` with required
      sample lines and switchable gutter widths.
- [x] Register example/test targets in CMake; register example in Xmake.
- [x] Build with GCC and Clang, strict warnings as errors.
- [x] Run ASan/UBSan.
- [x] Run example, verify user flows.
- [ ] Subagent review, fix findings.
- [ ] Commit, push, open PR.

## Out-of-scope pre-existing build fixes (required by -Werror/Clang mandate)

- `CMakeLists.txt`: resolve FTXUI at top level so the imported `ftxui::*`
  targets are visible to sibling `examples/`/`benchmarks/` (pre-existing
  configure failure for example targets).
- `src/terminal_ui_kit/core/text_wrap.cc`: add explicit casts for Clang
  `-Wsign-conversion` (pre-existing warnings).
- `tests/.../virtual_list_test.cc`, `style_bridge_test.cc`: complete aggregate
  initializers (pre-existing `-Wmissing-field-initializers` warnings).
