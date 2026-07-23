# MarkdownView Design

## Goal

Implement MarkdownView and CodeView (PRD sections 25-26, milestone 0.3.0):
a Markdown parser adapter using cmark-gfm, a MarkdownDocument RAII wrapper,
a MarkdownView component that renders Markdown as FTXUI elements, and a
reusable CodeView component for fenced code blocks.

## Scope and boundaries

This work covers:
- cmark-gfm integration via vcpkg
- MarkdownDocument (RAII wrapper over cmark AST with unique_ptr)
- MarkdownView component (recursive visitor: cmark_node → ftxui::Element)
- CodeView component (reusable, in components/)
- P0 Markdown elements (PRD section 25.2)
- Hyperlinks (ftxui::hyperlink + on_link callback)
- Unit tests (MarkdownDocument parsing) + golden tests (rendering)
- Example: markdown_viewer

Out of scope: P1 elements (tables, task lists, footnotes), Tree-sitter
syntax highlighting, selectable text in code blocks.

## Dependencies

cmark-gfm is added as a vcpkg feature:

```json
"cmark-gfm": {
  "description": "GitHub-flavored Markdown parser",
  "dependencies": ["cmark-gfm"]
}
```

CMake option: `TERMINAL_UI_KIT_ENABLE_MARKDOWN` (already exists in
TerminalUiKitOptions.cmake). When enabled, `terminal_ui_kit_markdown`
links `cmark-gfm::cmark-gfm`.

## MarkdownDocument (RAII wrapper)

```cpp
struct CmarkNodeDeleter {
  void operator()(cmark_node* node) const;
};
using CmarkNodePtr = std::unique_ptr<cmark_node, CmarkNodeDeleter>;

class MarkdownDocument {
 public:
  explicit MarkdownDocument(std::string_view markdown);
  ~MarkdownDocument() = default;

  cmark_node* root() const;

 private:
  CmarkNodePtr root_;
};
```

Constructor calls `cmark_parse_document()` with `CMARK_OPT_DEFAULT`.
The unique_ptr ensures `cmark_node_free()` is called on destruction.
Move-only semantics.

## MarkdownView (component)

```cpp
struct MarkdownViewOptions {
  Theme theme = default_dark_theme();
  std::function<void(std::string url)> on_link;
};

ftxui::Component MarkdownView(
    std::shared_ptr<MarkdownDocument> document,
    MarkdownViewOptions options);
```

Rendering is a recursive visitor over the cmark AST:

| cmark node type | FTXUI rendering |
|---|---|
| CMARK_NODE_DOCUMENT | `ftxui::vbox(children)` |
| CMARK_NODE_PARAGRAPH | `ftxui::paragraph(inline_children)` |
| CMARK_NODE_HEADING | `ftxui::text(text) \| bold` (h1-h6 sizes) |
| CMARK_NODE_EMPH | `ftxui::text(text) \| italic` |
| CMARK_NODE_STRONG | `ftxui::text(text) \| bold` |
| CMARK_NODE_CODE | `ftxui::text(text) \| code_style` |
| CMARK_NODE_CODE_BLOCK | `CodeView(code, language)` |
| CMARK_NODE_LIST | `ftxui::vbox(item_elements)` with bullets/numbers |
| CMARK_NODE_ITEM | `ftxui::hbox({bullet, content})` |
| CMARK_NODE_BLOCKQUOTE | `ftxui::vbox(children) \| border_left` |
| CMARK_NODE_HORIZONTAL_RULE | `ftxui::separator()` |
| CMARK_NODE_LINK | `ftxui::hyperlink(url, text)` + on_link callback |
| CMARK_NODE_SOFTBREAK | `ftxui::text(" ")` |
| CMARK_NODE_LINEBREAK | `ftxui::text("")` |

Inline elements are collected into `ftxui::hbox()` for each paragraph.

## CodeView (reusable component)

```cpp
struct CodeViewOptions {
  std::string language;
  bool show_line_numbers = false;
  Theme theme = default_dark_theme();
};

ftxui::Component CodeView(std::string code, CodeViewOptions options);
```

CodeView renders a fenced code block:
- Monospace text (theme.code style)
- Optional line numbers (gray, right-aligned)
- Horizontal scrolling via ftxui::hbox with scroll offset
- Language label in top-right corner
- No syntax highlighting for MVP (future: Tree-sitter)

## Files

```
include/terminal_ui_kit/markdown/
  markdown_document.h
  markdown_view.h

src/terminal_ui_kit/markdown/
  markdown_document.cc
  markdown_view.cc

include/terminal_ui_kit/components/
  code_view.h

src/terminal_ui_kit/components/
  code_view.cc

tests/terminal_ui_kit/unit/
  markdown_document_test.cc

tests/terminal_ui_kit/rendering/
  markdown_view_test.cc
  code_view_test.cc

examples/markdown_viewer/
  main.cc
  CMakeLists.txt
```

## Tests

### Unit tests (MarkdownDocument, no FTXUI)

- Parses headings (h1-h6)
- Parses paragraphs
- Parses bold/italic/inline code
- Parses fenced code blocks with language
- Parses unordered/ordered lists
- Parses blockquotes
- Parses horizontal rules
- Parses links
- Empty document
- Invalid UTF-8 handling

### Golden tests (MarkdownView/CodeView, FTXUI)

- Heading renders with bold
- Paragraph renders wrapped text
- Code block renders with monospace style
- List renders with bullets
- Link renders with hyperlink decoration
- CodeView renders line numbers when enabled

## Example: markdown_viewer

Loads a Markdown file or uses built-in sample, renders it with
MarkdownView. Shows headings, paragraphs, code blocks, lists, links.
Controls: up/down scroll, q/ESC quit.

## Build

When `TERMINAL_UI_KIT_ENABLE_MARKDOWN=ON`:
- `terminal_ui_kit_markdown` links `cmark-gfm::cmark-gfm`, `terminal_ui_kit_core`, `terminal_ui_kit_components`
- `terminal_ui_kit_components` gains `code_view.cc`

## Design decisions

| Decision | Choice |
|---|---|
| Parser | cmark-gfm |
| Model | RAII wrapper, unique_ptr with custom deleter |
| Rendering | Recursive visitor (cmark_node → Element) |
| CodeView | Separate component, no syntax highlighting for MVP |
| File placement | MarkdownView in markdown/, CodeView in components/ |
| Dependencies | vcpkg feature |
| Inline rendering | ftxui::hbox |
| Hyperlinks | ftxui::hyperlink + on_link callback |
| Tests | Unit + golden |
| Constructor | MarkdownDocument(std::string_view) |