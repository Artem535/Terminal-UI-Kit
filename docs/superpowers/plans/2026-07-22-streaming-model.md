# PR7A Streaming Model Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the FTXUI-independent `StreamingDocument`, ANSI parser,
and retained/filterable `LogModel` as the model half of PR7.

**Architecture:** Convert `terminal_ui_kit_document` from an INTERFACE target
into a compiled library linked publicly to Core. `StreamingDocument` stores
completed lines plus an incomplete UTF-8 buffer; `parse_ansi` produces existing
Core `StyledText` spans; `LogModel` stores source entries in a deque and builds
a filtered index without deleting source records.

**Tech Stack:** C++20, GoogleTest/CTest, CMake, existing Core `StyledText` and
`TextStyle` types. No FTXUI dependency and no Xmake source wiring in PR7A.

## Global Constraints

- Public model code is FTXUI-independent and links only against
  `TerminalUiKit::Core`.
- C++20 and the repository Google C++ Style configuration.
- Types use `PascalCase`, functions/locals use `snake_case`, and private fields
  end in `_`; files use `snake_case.h`/`.cc`.
- `StreamingDocument` and `LogModel` are synchronous; no thread dispatcher or
  locking is added.
- Invalid UTF-8 is replaced by U+FFFD; malformed ANSI controls are consumed
  without throwing.
- `retention_limit == 0` means unlimited; positive retention evicts oldest
  entries first.
- PR7A contains no `LogView`, follow UI, FTXUI rendering, search UI, or
  interactive example app. Those belong to PR7B.

---

### Task 1: Compile the Document target and implement StreamingDocument

**Files:**

- Create `include/terminal_ui_kit/document/streaming_document.h`.
- Create `src/terminal_ui_kit/document/streaming_document.cc`.
- Modify `src/terminal_ui_kit/CMakeLists.txt` to replace the Document
  INTERFACE target with a compiled target containing the new source.
- Modify `tests/terminal_ui_kit/unit/CMakeLists.txt` to link
  `TerminalUiKit::Document`.
- Create `tests/terminal_ui_kit/unit/streaming_document_test.cc`.

**Interface:**

```cpp
class StreamingDocument {
 public:
  void append(std::string_view chunk);
  void replace_tail(std::string_view tail);
  void finish();
  void clear();
  std::size_t line_count() const;
  std::string_view line_at(std::size_t index) const;
  std::uint64_t revision() const;
};
```

- [ ] **Step 1: Add failing tests** for empty state, newline chunks,
  split UTF-8 codepoints, `replace_tail`, `finish`, `clear`, invalid-byte
  replacement, revision increments, and out-of-range `line_at` throwing
  `std::out_of_range`.
- [ ] **Step 2: Confirm the tests fail** because the header/target do not yet
  exist:
  `cmake --build build-debug --target terminal_ui_kit_unit_tests`.
- [ ] **Step 3: Implement** the pending byte buffer and UTF-8 decoder. Decode
  complete codepoints while appending; preserve an incomplete suffix; emit
  U+FFFD for invalid leading/continuation bytes; split on `\n` and strip a
  preceding `\r` from each completed line. `replace_tail` replaces the current
  unterminated line, `finish` appends it even when empty, and `clear` removes
  lines and pending bytes. Increment `revision_` once per mutating call.
- [ ] **Step 4: Build and run** the focused tests, then the existing unit suite;
  expected all StreamingDocument tests and all pre-existing tests pass.
- [ ] **Step 5: Commit** as `Add StreamingDocument model`.

### Task 2: Implement ANSI parsing into StyledText

**Files:**

- Create `include/terminal_ui_kit/document/ansi_parser.h`.
- Create `src/terminal_ui_kit/document/ansi_parser.cc`.
- Modify `src/terminal_ui_kit/CMakeLists.txt` to add `ansi_parser.cc`.
- Create `tests/terminal_ui_kit/unit/ansi_parser_test.cc` and register it.

**Interface:**

```cpp
StyledText parse_ansi(std::string_view text);
```

- [ ] **Step 1: Add failing tests** for plain text, reset, bold/dim/
  underline/strikethrough, standard and bright foreground/background colors,
  `38;2`/`48;2` RGB colors, `38;5`/`48;5` indexed colors, adjacent equal-style
  span coalescing, unsupported CSI/OSC removal, and a literal ESC-free result.
- [ ] **Step 2: Confirm focused compile/test failure** because the parser
  header is absent.
- [ ] **Step 3: Implement** a byte scanner that emits ordinary text into a
  current span, recognizes `ESC [` CSI sequences ending in a final byte, and
  recognizes `ESC ] ... BEL`/`ESC ] ... ESC \\` OSC sequences. Parse semicolon
  parameters with defaults; mutate a `TextStyle`; map ANSI 16-color and
  xterm-256 indexes to RGB; flush spans only when style changes. Ignore
  unsupported controls without throwing.
- [ ] **Step 4: Run parser and full unit tests**; expected all pass.
- [ ] **Step 5: Run clang-format and commit** as `Add ANSI StyledText parser`.

### Task 3: Implement retained and filtered LogModel

**Files:**

- Create `include/terminal_ui_kit/document/log_model.h`.
- Create `src/terminal_ui_kit/document/log_model.cc`.
- Modify `src/terminal_ui_kit/CMakeLists.txt` to add `log_model.cc`.
- Create `tests/terminal_ui_kit/unit/log_model_test.cc` and register it.

**Interfaces:**

```cpp
enum class LogSeverity { kTrace, kDebug, kInfo, kWarning, kError };

struct LogEntry {
  std::string timestamp;
  LogSeverity severity = LogSeverity::kInfo;
  StyledText message;
};

struct LogFilter {
  std::optional<LogSeverity> minimum_severity;
  std::string substring;
};

class LogModel {
 public:
  explicit LogModel(std::size_t retention_limit = 0);
  void append(LogEntry entry);
  void clear();
  void set_filter(LogFilter filter);
  std::size_t size() const;
  const LogEntry& at(std::size_t index) const;
  std::uint64_t revision() const;
};
```

- [ ] **Step 1: Add failing tests** for append/order, unlimited retention,
  oldest-first eviction, clear preserving filter, minimum-severity filtering,
  case-sensitive substring search over concatenated message spans, filtered
  order, revision changes, empty/fully filtered state, and out-of-range `at`.
- [ ] **Step 2: Confirm focused failure** because the model header is absent.
- [ ] **Step 3: Implement** a `std::deque<LogEntry>` source store, a vector of
  source indexes for the visible filtered sequence, and a rebuild helper called
  after append/clear/filter/eviction. Convert `StyledText` to plain text only
  while rebuilding the filter index. Keep the active filter across `clear` and
  increment revision once per mutating public operation.
- [ ] **Step 4: Run LogModel, ANSI, StreamingDocument, and complete unit tests**;
  expected all pass under C++20 with no FTXUI linkage.
- [ ] **Step 5: Run clang-format/diff-check and commit** as `Add retained LogModel`.

### Task 4: Documentation and public target verification

**Files:**

- Modify `docs/modules/ROOT/pages/changelog.adoc` under `[Unreleased]` →
  `Added`.
- No index page change is required because the current documentation does not
  describe the Document target as header-only.

- [ ] **Step 1: Document** StreamingDocument, ANSI parsing, retention and
  filtering as PR7A model capabilities, explicitly noting that LogView and the
  interactive streaming example are PR7B.
- [ ] **Step 2: Configure/build** with tests disabled and verify the compiled
  Document target links only against Core and includes no FTXUI headers. The
  repository may still fetch FTXUI because Components unconditionally requires
  it; this task only verifies that Document adds no FTXUI dependency.
- [ ] **Step 3: Run the complete unit and CTest suites, clang-format,
  `git diff --check`, and inspect the installed/exported Document target.
- [ ] **Step 4: Commit** as `Document streaming model in changelog`.

### Task 5: Final verification and handoff

- [ ] **Step 1:** Run clean Debug build and `ctest --test-dir build-debug
  --output-on-failure`.
- [ ] **Step 2:** Run the no-tests/no-examples configure/build and confirm the
  Document target itself remains FTXUI-independent even though Components may
  fetch FTXUI as an existing unconditional dependency.
- [ ] **Step 3:** Run clang-format with `-Werror`, `git diff --check`, and
  inspect the complete diff against `origin/main`.
- [ ] **Step 4:** Prepare a PR with the spec/plan links, API summary, exact test
  counts, and explicit note that LogView/example app are deferred to PR7B.
