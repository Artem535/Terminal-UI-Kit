# Plan: Fix line-number gutter underflow

## Problem

`CodeView` and `VirtualDocument` format their line-number gutters with:

```cpp
std::string num = std::to_string(line_num);
num = std::string(width - num.size(), ' ') + num;
```

When the digit count exceeds the configured width, `width - num.size()`
underflows (both are `std::size_t`), producing an enormous padding string that
wastes memory and breaks rendering.

## Steps

1. Add a shared `format_line_number` helper in `core` that returns a
   right-aligned number padded to a fixed width, never truncating and never
   underflowing.
2. Use the helper in `CodeView` (both highlighted and plain paths) and
   `VirtualDocument`.
3. Add rendering regression tests covering widths smaller/equal/larger than
   the digit count, width 0, and the required number set.
4. Add a standalone example `examples/line_number_formatting_example`.
5. Register the example in CMake and Xmake; document it in `examples/README.md`.
6. Build and run GCC/Clang strict + ASan/UBSan verification.
7. Run the example and verify flows.
8. Subagent review, then commit, push, and open a PR.
