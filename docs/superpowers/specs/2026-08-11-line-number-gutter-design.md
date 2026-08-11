# Design: line-number gutter formatting

## Context

`CodeView` and `VirtualDocument` render line-number gutters by computing
`width - num.size()` padding with `std::size_t` arithmetic. When the number's
digit count exceeds the configured width the subtraction underflows and the
gutter requests an enormous allocation.

## Decision

Add one shared, header-only helper in `terminal_ui_kit/core`:

```cpp
std::string format_line_number(std::size_t number, std::size_t width);
```

It right-aligns `number` to `width` columns with leading spaces. When the digit
count already reaches or exceeds `width` (including width 0) it returns the
unpadded number unchanged, so valid long line numbers are never truncated and
no underflow can occur. The helper is used by every gutter-formatting site.

## Consequences

- Short numbers keep their existing right alignment.
- Numbers equal to the width render without padding.
- Numbers wider than the width render fully.
- Width 0 is safe.
- Alignment behavior for ordinary documents is unchanged.
