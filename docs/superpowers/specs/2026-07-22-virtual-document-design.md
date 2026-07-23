# VirtualDocument Design

## Goal

Implement the missing half of milestone 0.2.0 (PRD sections 18, 57): a
virtualized document component that displays large plain-text documents with
line wrapping, follow mode, and incremental append.

## Scope and boundaries

This work covers the `WrappedDocument` model (FTXUI-free, in `document/`),
the `VirtualDocument` component (in `components/`, wrapping `VirtualList`),
unit and rendering tests, and a `virtual_document_viewer` example. It does
not include `StyledText` rendering, search, highlights, bookmarks, copy
selection, truncation markers, or line-number-based cursor navigation — those
are future iterations.

## WrappedDocument (pure model)

```cpp
struct WrappedLine {
  std::string text;
  std::size_t logical_line;
  std::size_t sub_line;
};

class WrappedDocument {
 public:
  explicit WrappedDocument(int width = 80, int tab_width = 8);

  void rebuild_from(const StreamingDocument& doc);
  void append_from(const StreamingDocument& doc);
  void replace_tail(const StreamingDocument& doc);
  void handle_width_change(int new_width, const StreamingDocument& doc);
  void clear();

  std::size_t display_line_count() const;
  const WrappedLine& display_line_at(std::size_t index) const;
  std::size_t first_display_line_for(std::size_t logical_line) const;

 private:
  int width_;
  int tab_width_;
  std::vector<WrappedLine> lines_;
  std::size_t last_logical_line_count_ = 0;
};
```

`rebuild_from` wraps every logical line in `doc` from scratch, replacing the
full display vector. `append_from` wraps only logical lines whose index is
>= `last_logical_line_count_` and appends the resulting display lines.
`replace_tail` rewraps only the last logical line (the one that was replaced
by `StreamingDocument::replace_tail`). `handle_width_change` calls
`rebuild_from` with the new width. `clear` resets all state.

`first_display_line_for` binary-searches `lines_` for the first display line
whose `logical_line` equals the given index, used by `scroll_to_line`.

Wrapping uses `wrap_plain_text` from core. Wrapped lines are plain strings,
not `StyledText`, matching the plain-text MVP scope.

## VirtualDocument (component)

```cpp
struct VirtualDocumentOptions {
  StreamingDocument* document = nullptr;
  Theme theme = default_dark_theme();
  bool follow = true;
  int tab_width = 8;
  bool show_line_numbers = false;
};

class VirtualDocument {
 public:
  explicit VirtualDocument(VirtualDocumentOptions options);
  ftxui::Component component() const;
  bool follow() const;
  void set_follow(bool follow);
  void scroll_to_bottom();
  void scroll_to_line(std::size_t logical_line_index);
 private:
  std::shared_ptr<VirtualDocumentImpl> impl_;
};
```

Internally, `VirtualDocumentImpl` owns a `WrappedDocument` and a
`VirtualListModel`. On each render frame, it checks the `StreamingDocument`
revision and calls `WrappedDocument::append_from` (or `replace_tail`) if new
lines appeared. If `follow` is true, it calls `VirtualListModel::scroll_to_bottom`
when the revision changes.

`ArrowUp`/`ArrowDown`/wheel disable follow mode. `End` re-enables it and
scrolls to bottom. `show_line_numbers` prepends a formatted line number (or
padding) to the first sub-line of each logical line, using `theme.muted` for
the number style.

## WrappedDocument behaviour

| Scenario | Result |
|---|---|
| Empty doc | 0 display lines |
| Single line shorter than width | 1 display line, logical_line=0, sub_line=0 |
| Single line wider than width | N display lines, sub_line 0..N-1 |
| Append adds new logical lines | Only new lines are wrapped |
| Width changes | Full rebuild, follow scrolls to bottom |
| replace_tail on last line | Only last logical line is rewrapped |
| Logical line = empty string | 1 display line (empty) |

## Tests

### Unit tests (WrappedDocument, no FTXUI)

- empty document has zero display lines
- short line produces one display line
- long line wraps into multiple display lines
- append after rebuild adds only new display lines
- width change rebuilds all display lines
- replace_tail rewraps only the last logical line
- first_display_line_for returns correct index for multi-line wraps
- clear resets all state

### Rendering tests (VirtualDocument, with FTXUI)

- empty document renders empty screen
- short lines display correctly
- wrapped lines display correctly across viewport
- follow mode auto-scrolls on append
- show_line_numbers shows numbers on first sub-line only
- ArrowUp/ArrowDown disable follow

## Example: virtual_document_viewer

A streaming example that generates random multi-line text chunks in a
background thread and displays them in a VirtualDocument. Follow mode is ON
by default. Controls:
- `up/down` — scroll, disables follow
- `end` — re-enable follow
- `q/ESC` — quit
- Status bar shows follow state, total lines, display lines

## Build and testing

`WrappedDocument` is added to `terminal_ui_kit_document` (compiled library).
`VirtualDocument` is added to `terminal_ui_kit_components` (links
`terminal_ui_kit_document`). The example links `TerminalUiKit::Components`.

## Non-goals

No `StyledText` rendering, search, highlights, bookmarks, copy selection,
truncation markers, or cursor navigation within wrapped lines.