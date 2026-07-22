# PR6 Variable-Height Virtualization Implementation Plan

> Execute task-by-task with tests first. The branch starts at PR5 commit
> `5cb8376` and must preserve the fixed-height API and behavior.

## Goal

Extend `VirtualList` to support variable-height rows with cached measurements,
resize invalidation, and scroll anchoring while retaining the PR5 API.

## Architecture

Keep one `VirtualListImpl` and replace its index-only viewport math with a
layout model containing estimated/measured heights and prefix sums. Unknown
rows use `estimate_height(index, width)` or the legacy `item_height`; visible
rows are rendered through FTXUI layout observers, whose assigned boxes update
the cache and request another animation frame. Scroll is a row offset derived
from prefix sums, and an anchor index/row offset compensates changes before the
anchor.

## Global constraints

- C++20 and the repository Google C++ Style configuration.
- Public names use PascalCase for types, snake_case for functions/locals and
  private fields ending in `_`.
- Keep `VirtualListModel`, `VirtualList`, selection callbacks, keyboard events,
  wheel events, and `scroll_to_index` source-compatible.
- No follow-end, streaming, ANSI parsing, LogView, or Xmake changes in PR6.
- Tests render through `terminal_ui_kit::test_support::render_to_screen` and
  do not use `ScreenInteractive`.

### Task 1: Extend options and establish the height model

**Files:**

- Modify `include/terminal_ui_kit/components/virtual_list.h`.
- Modify `src/terminal_ui_kit/components/virtual_list.cc`.
- Modify `tests/terminal_ui_kit/rendering/virtual_list_test.cc`.

Add `std::function<int(std::size_t index, int width)> estimate_height` after
`render_item`, retaining `item_height = 1` as fallback. Add tests proving the
estimate callback is used for initial range calculation, that non-positive
estimates become one row, and that existing fixed-height tests still pass.

The implementation must expose one internal `height_for(index, width)` helper:
return a cached measured value when available, otherwise call
`estimate_height` if set, otherwise use `item_height`, and clamp to one.
Track the current item count and width so a count or width change invalidates
prefix metadata without discarding measured heights for unchanged indices.

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R VirtualList
```

Commit: `Add variable-height estimates to VirtualList`.

### Task 2: Add cached prefix sums and measured row observers

**Files:**

- Modify `src/terminal_ui_kit/components/virtual_list.cc`.
- Modify `tests/terminal_ui_kit/rendering/virtual_list_test.cc`.

Implement a contiguous prefix array where `prefix[0] = 0` and
`prefix[i + 1] = prefix[i] + height_for(i, width)`. Use binary search to map
the current scroll offset to the first visible index and to compute the end
index covering the viewport. Render only that range plus at most one trailing
row. Wrap each row in a custom `ftxui::Node` that observes its assigned box;
when its height changes, update the measured cache, invalidate prefix sums,
and call `ftxui::animation::RequestAnimationFrame()` exactly once for that
layout change.

Add tests that render a 100,000-item list with alternating one-/two-line
elements and assert only viewport rows plus one are materialized; render twice
and assert unchanged measurements do not invoke another invalidation; and
change the backing width to assert the estimate callback receives the new
width while measured heights are refreshed after the follow-up frame.

Run the focused VirtualList tests and `git diff --check` before committing:
`Cache variable row measurements and prefix sums`.

### Task 3: Implement variable-height scrolling, selection visibility, and anchoring

**Files:**

- Modify `src/terminal_ui_kit/components/virtual_list.cc`.
- Modify `tests/terminal_ui_kit/rendering/virtual_list_test.cc`.

Replace index-based `scroll_index_` movement with a clamped row offset. Keep
`scroll_to_index` deterministic: place the requested row at the top when
possible, clamp to the content end otherwise, then normalize selection without
calling `on_select`. Arrow and page navigation must use prefix sums and the
actual viewport height. Wheel events move by three terminal rows, map the new
offset through the prefix array, and never change selection.

Before applying a measured-height update, capture the first visible index and
its pixel offset. If a changed row is before that anchor, add its height delta
to the scroll offset; if it is the anchor or later, leave the offset unchanged.
Always clamp against the new total height. When item count shrinks, preserve
the selected-index callback rule from PR5 and clamp both selection and offset.

Add tests for mixed-height `scroll_to_index`, page navigation, wheel scrolling
without selection callbacks, keeping an anchor row at the same screen y after
an earlier row grows, and count shrink normalization. Render the same list
through `VirtualListModel` to ensure model methods still work.

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R VirtualList
```

Commit: `Add variable-height scrolling and scroll anchoring`.

### Task 4: Update benchmark, viewer example, and documentation

**Files:**

- Modify `benchmarks/virtual_list_benchmark.cc`.
- Modify `examples/virtual_list_viewer/main.cc`.
- Modify `docs/modules/ROOT/pages/changelog.adoc` with the PR6 entry.

Keep the 100,000-item benchmark and add a mixed-height benchmark case using a
deterministic height estimate. Record rendered item count and assert/report
that it remains bounded by the viewport. Extend the viewer with a labelled
“Variable-height mode” section: alternate one-, two-, and three-line rows,
show the selected row and rendered count, and retain the existing controls and
quit behavior. Add the variable-height API and cache behavior to the existing
Unreleased/Added section in `docs/modules/ROOT/pages/changelog.adoc`. Do not
introduce a second example executable.

Build and run:

```bash
cmake --build build-bench --target terminal_ui_kit_virtual_list_benchmark
./build-bench/benchmarks/terminal_ui_kit_virtual_list_benchmark \
  --benchmark_min_time=0.001s --benchmark_repetitions=1
cmake --build build-debug --target terminal_ui_kit_example_virtual_list_viewer
clang-format --dry-run -Werror \
  include/terminal_ui_kit/components/virtual_list.h \
  src/terminal_ui_kit/components/virtual_list.cc \
  tests/terminal_ui_kit/rendering/virtual_list_test.cc \
  benchmarks/virtual_list_benchmark.cc \
  examples/virtual_list_viewer/main.cc
```

Commit: `Demonstrate variable-height virtualization`.

### Task 5: Full verification and handoff

Run a clean build, all CTest suites, benchmark smoke, formatting, and
`git diff --check`. Confirm the worktree is clean, review the complete diff
against `origin/main`, and prepare a PR description listing API compatibility,
cache/anchor behavior, tests, benchmark, and viewer changes.

Expected CTest result is all existing tests plus the expanded VirtualList
suite passing. No merge or branch deletion is performed without explicit user
approval.
