# PR7A Streaming Model Design

## Goal

Implement the model half of PR7: an FTXUI-independent `StreamingDocument`,
ANSI-to-`StyledText` parsing, and a retained/filterable `LogModel` that the
future `LogView` can consume without rebuilding complete histories.

## Scope and boundaries

This work is the first half of PR7 and contains no FTXUI component. The
document module becomes a compiled target with public Core linkage. The later
PR7B adds `LogView`, follow/pause UI, virtualization wiring, and the
`streaming_log_viewer` application. Thread-safe mutation and a UI dispatcher
are explicitly deferred; all model methods are synchronous.

## StreamingDocument

Public API:

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

The model stores completed lines separately from a pending byte buffer. `append`
splits only on complete newline-delimited records and retains an incomplete
UTF-8 suffix until the next chunk. Invalid UTF-8 bytes are replaced with the
UTF-8 encoding of `U+FFFD`; a malformed continuation does not corrupt later
valid text. `replace_tail` replaces the current pending line, `finish` commits
pending text as the last line, and `clear` removes all data. Every mutating
operation increments `revision` once, including operations that produce an
empty final line; read methods never mutate.

## ANSI parser

```cpp
StyledText parse_ansi(std::string_view text);
```

The parser recognizes CSI SGR sequences for reset (`0`), bold (`1`), dim
(`2`), underline (`4`), strikethrough (`9`), standard/bright 16-color
foreground (`30`-`37`, `90`-`97`) and background (`40`-`47`, `100`-`107`),
24-bit foreground/background (`38;2;r;g;b`, `48;2;r;g;b`), and indexed 256-color
forms (`38;5;n`, `48;5;n`) mapped to RGB. Unsupported CSI/OSC sequences are
consumed and omitted from output. Plain text is emitted as coalesced spans;
adjacent text with equal `TextStyle` is one span. Hyperlinks are not parsed in
PR7A.

## LogModel

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

`retention_limit == 0` means unlimited. Positive limits evict oldest entries
first. Filtering never deletes source entries: `size` and `at` address the
currently visible filtered sequence in stable source order. The substring
filter is case-sensitive and searches the concatenated plain text of message
spans; minimum severity is inclusive. `clear` preserves the active filter and
increments revision. Out-of-range `at` uses the repository's existing bounds
policy for model access and is covered by tests.

## Build and testing

`terminal_ui_kit_document` changes from an INTERFACE target to a compiled
library containing `streaming_document.cc`, `ansi_parser.cc`, and
`log_model.cc`, linking `terminal_ui_kit_core` publicly. Unit tests remain
FTXUI-free and cover chunk boundaries, invalid UTF-8, revision semantics,
ANSI styles, retention, filtering, and empty states. A model-only
`streaming_log_model` example is not added; the interactive example belongs to
PR7B.

## Non-goals

No `LogView`, FTXUI rendering, follow/pause state, asynchronous dispatcher,
thread synchronization, search UI, or Xmake source wiring is included.
