# Task 3 report

Implemented variable-height scrolling and scroll anchoring for `VirtualList`.

## Changes

- Replaced row-index scrolling with a clamped pixel offset backed by prefix sums.
- `scroll_to_index()` places the requested row at the top when possible and
  clamps to the content end otherwise.
- Arrow, page, wheel, Home, and End navigation use measured/estimated row
  heights and the actual viewport height.
- Wheel scrolling moves three terminal rows without invoking `on_select`.
- Added a scrolling node that renders the virtualized content at a pixel
  offset, including partial-row offsets.
- Measured-height updates compensate the offset when a row before the first
  visible anchor changes height.
- Count changes clamp selection and scroll offset while preserving callback
  behavior.
- Added focused tests for mixed-height scrolling, page/wheel behavior, and
  count-shrink normalization.

## Verification

```text
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R VirtualList
```

Result: 20/20 VirtualList tests passed.

```text
clang-format --dry-run -Werror \
  src/terminal_ui_kit/components/virtual_list.cc \
  tests/terminal_ui_kit/rendering/virtual_list_test.cc
git diff --check
```

Both checks passed.

## Review fixes

- `ScrolledNode` now receives the scroll offset relative to the first row in
  the materialized `vbox`, so `scroll_to_index()` and wheel scrolling place
  the requested row at the correct viewport position.
- `ScrolledNode` stores that offset by value, avoiding a dangling reference
  to the temporary relative-offset local.
- Model `scroll_to_index()`/`select_index()` and event selection paths refresh
  count-dependent layout metadata before using prefix sums.
- Added a render assertion that verifies a nonzero `scroll_to_index()` puts
  the requested row at the first viewport line.

Review-fix verification: 21/21 VirtualList tests passed.

## Concerns

The anchor compensation path is exercised by measured-height callbacks during
rendering; a dedicated black-box test that mutates a previously measured row
while it is off-screen would require an explicit cache invalidation API, which
is outside the current Task 3 public interface.
