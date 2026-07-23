# Text Selection Design

## Goal

Add keyboard-driven text selection to VirtualDocument: Shift+Arrow navigation,
visual highlighting of selected spans, and copy via Ctrl+C (PRD sections 31,
49.3). Selection is the last missing piece of milestone 0.2.0 (PRD section 57).

## Scope and boundaries

This work covers an FTXUI-free `SelectionManager` model in core/, an updated
`WrappedDocument` that tracks byte offsets for span-level highlighting, and
selection support in `VirtualDocument`. It does not include mouse selection,
OSC 52 clipboard integration, styled-text span-level selection (future P1
features per PRD section 58), or integration with LogView/other components.

## SelectionManager (pure model, core/)

```cpp
class SelectionManager {
 public:
  SelectionManager();

  void start(TextPosition pos);
  void extend_to(TextPosition pos);
  void clear();
  bool has_selection() const;
  TextRange range() const;
  bool is_selecting() const;

  std::string selected_text(const StreamingDocument& doc) const;

  bool handle_key_event(ftxui::Event event, TextPosition cursor);

 private:
  std::optional<TextPosition> start_;
  std::optional<TextPosition> end_;
  bool selecting_ = false;
};
```

Coordinates use logical (line, column) — mapping from display positions is
the component's responsibility. `selected_text` extracts the raw bytes from
a StreamingDocument by iterating over the range [start, end). Lines are
joined with newlines. `handle_key_event` processes Shift+Arrow keys and
returns true if the event was consumed.

## WrappedDocument changes

Add `byte_offset` to `WrappedLine` so VirtualDocument can split a display
line into selected/unselected spans:

```cpp
struct WrappedLine {
  std::string text;
  std::size_t logical_line = 0;
  std::size_t sub_line = 0;
  std::size_t byte_offset = 0;  // first byte's position in logical line
};
```

Add `wrap_plain_text_with_offsets` to core/text_wrap.h that returns the
byte offset of each wrapped segment within the source text:

```cpp
using WrappedSegment = std::pair<std::string, std::size_t>;
std::vector<WrappedSegment> wrap_plain_text_with_offsets(
    std::string_view text, int width);
```

Update `WrappedDocument::wrap_logical_line` to use the new function and
populate `byte_offset`.

## VirtualDocument selection integration

VirtualDocumentImpl gains a `SelectionManager` member. The renderer splits
each display line into at most three spans (before selection, selected,
after selection) using the `byte_offset` and selection range.

### Keyboard handling

| Keys | Behaviour |
|---|---|
| Shift+Left | Extend selection one codepoint left (or to previous line) |
| Shift+Right | Extend selection one codepoint right (or to next line) |
| Shift+Up | Extend selection to previous display line, mapped to logical coords |
| Shift+Down | Extend selection to next display line, mapped to logical coords |
| Ctrl+C | Copy selected text via `on_copy` callback, clear selection |
| Arrow (no Shift) | Clear selection, move cursor |

Only `Ctrl+C` fires the copy callback; the callback is optional in
`VirtualDocumentOptions`. If no callback is set, `Ctrl+C` still clears
selection but does not copy.

`handle_key_event` in `handle_event` runs before follow-mode checks so that
selection does not re-enable follow.

### Rendering

For each display line, compute whether it intersects the selection:

```text
line before selection  |  selected span  |  line after selection
       normal          |     inverted    |       normal
```

If no intersection, render the full line as normal. If the line is fully
selected, invert the entire line. If partially selected, create three
text elements and join with `hbox`.

## VirtualDocumentOptions changes

```cpp
struct VirtualDocumentOptions {
  StreamingDocument* document = nullptr;
  Theme theme = default_dark_theme();
  bool follow = true;
  int tab_width = 8;
  bool show_line_numbers = false;
  std::function<void(std::string)> on_copy;  // NEW
};
```

## Tests

### Unit tests (SelectionManager, no FTXUI)

- starts with no selection
- start sets the anchor
- extend_to sets the range
- clear resets state
- selected_text returns correct content for single-line range
- selected_text returns correct content for multi-line range
- handle_key_event with Shift+Arrow extends selection

### Unit tests (wrap_plain_text_with_offsets, no FTXUI)

- each wrapped segment has correct byte offset
- hard-broken words track offsets correctly

### Rendering tests (VirtualDocument with selection, FTXUI)

- selected text appears inverted
- Ctrl+C calls on_copy callback with correct text
- Arrow without Shift clears selection
- Shift+Arrow extends selection

## Build

SelectionManager and wrap_plain_text_with_offsets land in
`terminal_ui_kit_core`. No new target dependencies.

## Non-goals

Mouse selection, OSC 52, styled-text span selection, LogView integration,
cross-component selection sharing.

## Design decisions

| Decision | Choice |
|---|---|
| Model | Separate SelectionManager class |
| Coordinates | Logical (line, column) |
| Wrapped navigation | Display-level, mapped to logical |
| Span rendering | byte_offset in WrappedLine + wrap_plain_text_with_offsets |
| Copy trigger | Ctrl+C (only when selection active, fires on_copy) |
| Arrow without Shift | Clears selection |
| File location | core/ |