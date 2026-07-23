# Task 3 report: retained and filtered `LogModel`

## Status

Complete. Added a Core-only `LogModel` to the compiled Document target. It
stores entries in a deque, applies oldest-first retention, preserves the
active filter across `clear()`, and maintains a source-index vector for the
visible filtered order. Substring filtering joins all `StyledText` spans and
is case-sensitive. `revision()` increments once per mutating public call;
`at()` throws `std::out_of_range` for an invalid visible index.

## Commit

`Add retained LogModel`

## Verification

```text
cmake --build build-debug --target terminal_ui_kit_unit_tests -j2
```

Completed successfully; the compiled Document target and unit executable
linked cleanly.

```text
ctest --test-dir build-debug --output-on-failure -R 'LogModel|AnsiParser|StreamingDocument'
```

Result: 25/25 focused tests passed.

```text
ctest --test-dir build-debug --output-on-failure
```

All 50 built unit tests passed. The separate rendering executable was not
built in this worktree, so CTest reported its existing `_NOT_BUILT` placeholder
as not run.

```text
clang-format --dry-run -Werror \
  include/terminal_ui_kit/document/log_model.h \
  src/terminal_ui_kit/document/log_model.cc \
  tests/terminal_ui_kit/unit/log_model_test.cc
git diff --check
```

Both checks passed.

## Concerns

Retention limit `0` means unlimited storage. Severity ordering follows the
declared enum order from trace through error. Filtering is rebuilt eagerly on
append, clear, and filter changes; this is intentional for the small MVP and
keeps `at()` constant-time for the visible sequence.
