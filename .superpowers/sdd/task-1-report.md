# Task 1 report: Compile Document target and add StreamingDocument

## Status

Complete. `TerminalUiKit::Document` is now a compiled C++20 target with public
Core linkage. `StreamingDocument` incrementally decodes UTF-8 chunks, retains
incomplete byte suffixes, splits newline-delimited records, strips CRLF
carriage returns, replaces malformed bytes with U+FFFD, supports tail
replacement/finalization/clear, and tracks one revision increment per mutating
call. Bounds-checked `line_at()` throws `std::out_of_range`.

## Commit

The implementation is committed as `Add StreamingDocument model` at
`ed7aaec`.

## Verification

Commands run in the streaming-model worktree:

```text
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug \
  -DTERMINAL_UI_KIT_BUILD_TESTS=ON \
  -DFETCHCONTENT_SOURCE_DIR_FTXUI=/home/a.durynin/Projects/C++/terminal_ui_kit/build-debug/_deps/ftxui-src
```

Configure completed successfully using the existing local FTXUI checkout.

```text
cmake --build build-debug --target terminal_ui_kit_unit_tests -j2
```

Completed successfully; Core, compiled Document, and the unit executable
linked cleanly.

```text
ctest --test-dir build-debug --output-on-failure -R StreamingDocument
```

Result: 10/10 StreamingDocument tests passed.

```text
ctest --test-dir build-debug --output-on-failure -R 'TextPosition|TextRange|StyledText|TextWrap|Theme'
```

Result: 20/20 pre-existing unit tests passed.

```text
clang-format --dry-run -Werror \
  include/terminal_ui_kit/document/streaming_document.h \
  src/terminal_ui_kit/document/streaming_document.cc \
  tests/terminal_ui_kit/unit/streaming_document_test.cc
git diff --check
```

Both checks passed.

## Concerns

No FTXUI headers or linkage were introduced into Document; FTXUI was needed
only by the repository's existing Components target during configure.

## Review fix

The decoder now validates every continuation byte already available before
retaining an incomplete suffix. An invalid lead such as `E2 61` or `E2 0A`
is replaced immediately, then decoding continues with the following ASCII
byte or newline instead of dropping it on `finish()`.

Added regression coverage for invalid continuation bytes before ASCII and
newline boundaries. The focused suite now passes 12/12 tests.
