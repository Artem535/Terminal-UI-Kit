# Task 1 report: Extend options and establish the height model

## Status

Complete. `VirtualListOptions` now accepts an optional height-estimate
callback. Rendering uses the estimate for the initial viewport range, clamps
non-positive values to one row, and retains the legacy `item_height` fallback.
The implementation has a single `height_for(index, width)` helper and keeps
measured-height storage across item-count and width metadata invalidation for
the next cache/prefix-sum task.

## Commit

- `Add variable-height estimates to VirtualList`

## Verification

Commands run in the variable-height worktree:

```text
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug \
  -DTERMINAL_UI_KIT_BUILD_TESTS=ON \
  -DFETCHCONTENT_SOURCE_DIR_FTXUI=/home/a.durynin/Projects/C++/terminal_ui_kit/build-debug/_deps/ftxui-src
```

Configure completed successfully using the existing local FTXUI checkout.

```text
cmake --build build-debug --target terminal_ui_kit_rendering_tests
```

Completed successfully.

```text
ctest --test-dir build-debug --output-on-failure -R VirtualList
```

Result: 13/13 VirtualList tests passed, including the two new estimate tests.

```text
clang-format --dry-run -Werror \
  include/terminal_ui_kit/components/virtual_list.h \
  src/terminal_ui_kit/components/virtual_list.cc \
  tests/terminal_ui_kit/rendering/virtual_list_test.cc
git diff --check
```

Both checks passed.

## Concerns

Prefix sums and measured row observers are intentionally not implemented in
this task; the stored height metadata and invalidation state are scaffolding
for Task 2. Legacy fixed-height viewport/event behavior remains in place.
