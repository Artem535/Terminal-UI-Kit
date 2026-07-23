# Syntax Highlighting Design

## Goal

Add syntax highlighting to CodeView using Tree-sitter (PRD section 27).
Integrates with the existing CodeView component and MarkdownView's
fenced code blocks.

## Scope and boundaries

This work covers:
- Tree-sitter integration via vcpkg
- In-tree grammars for 10 languages (statically linked)
- SyntaxHighlighter class (highlight → StyledText)
- Built-in highlights.scm queries per language
- Fixed capture → Theme role mapping
- Lazy parser creation
- Integration with CodeView (auto-highlight when language is set)
- Tests
- Update markdown_viewer example

Out of scope: custom queries per theme, incremental parsing, dynamic
grammar loading, all languages beyond the initial 10.

## Dependencies

Tree-sitter is added as a vcpkg feature:

```json
"tree-sitter": {
  "description": "Tree-sitter syntax highlighting",
  "dependencies": ["tree-sitter"]
}
```

CMake option: `TERMINAL_UI_KIT_ENABLE_TREE_SITTER` (already exists in
TerminalUiKitOptions.cmake). When enabled, `terminal_ui_kit_syntax`
links `tree-sitter::tree-sitter`.

## Grammars

Each grammar is a git repository containing a `src/parser.c` file.
Connected via FetchContent as static libraries:

| Language | Repository |
|---|---|
| C | tree-sitter/tree-sitter-c |
| C++ | tree-sitter/tree-sitter-cpp |
| Python | tree-sitter/tree-sitter-python |
| JSON | tree-sitter/tree-sitter-json |
| YAML | tree-sitter/tree-sitter-yaml |
| Shell | tree-sitter/tree-sitter-bash |
| Markdown | tree-sitter/tree-sitter-markdown |
| Diff | tree-sitter/tree-sitter-diff |
| Rust | tree-sitter/tree-sitter-rust |
| JS/TS | tree-sitter/tree-sitter-javascript |

Each grammar is compiled as a static library and linked into
`terminal_ui_kit_syntax`. The Tree-sitter runtime provides
`ts_parser_new()`, `ts_parser_parse()`, `ts_query_new()`, etc.

## SyntaxHighlighter

```cpp
class SyntaxHighlighter {
 public:
  // Highlight a code string for the given language.
  // Returns StyledText with TextStyle applied to each token.
  // If language is unknown or tree-sitter is unavailable,
  // returns StyledText with a single unstyled span.
  static StyledText highlight(
      std::string_view code,
      std::string_view language,
      const Theme& theme);
};
```

Internally:
1. Look up language name → TSLanguage* function (e.g., "cpp" → tree_sitter_cpp)
2. Create TSParser (lazy, cached per language)
3. Parse code → TSTree
4. Run built-in highlights.scm query → captures
5. Map captures → TextStyle using fixed table
6. Build StyledText from spans

## Capture → Theme mapping

Fixed table (no configuration for MVP):

| Capture pattern | Theme role |
|---|---|
| @keyword, @keyword.return, @keyword.type | theme.accent |
| @string, @string.special | theme.success |
| @number, @float | theme.warning |
| @comment | theme.muted |
| @type, @type.builtin | theme.code |
| @function, @function.builtin | theme.primary |
| @variable, @parameter | theme.primary |
| @operator, @punctuation | theme.secondary |
| @property | theme.secondary |
| @constant, @constant.builtin | theme.warning |
| @constructor | theme.accent |
| @attribute | theme.muted |
| @tag | theme.accent |
| @escape | theme.error |

## Built-in queries

Each language has a `highlights.scm` query string stored as
`constexpr const char*` in a header file:

```
include/terminal_ui_kit/syntax/
  queries/
    c_highlights.h
    cpp_highlights.h
    python_highlights.h
    json_highlights.h
    yaml_highlights.h
    bash_highlights.h
    markdown_highlights.h
    diff_highlights.h
    rust_highlights.h
    javascript_highlights.h
```

Each file contains:
```cpp
constexpr const char* kCppHighlights = R"(
"if" @keyword
"return" @keyword.return
"type" @keyword.type
(string_literal) @string
(number_literal) @number
(comment) @comment
// ...
)";
```

## CodeView integration

CodeViewOptions remains unchanged. When `options.language` is non-empty
and Tree-sitter is available, CodeView calls
`SyntaxHighlighter::highlight(code, options.language, options.theme)`
and renders the resulting StyledText instead of plain text.

If Tree-sitter is not available (TERMINAL_UI_KIT_ENABLE_TREE_SITTER=OFF),
CodeView falls back to plain text with theme.code style.

## Files

```
include/terminal_ui_kit/syntax/
  syntax_highlighter.h
  queries/
    c_highlights.h
    cpp_highlights.h
    python_highlights.h
    json_highlights.h
    yaml_highlights.h
    bash_highlights.h
    markdown_highlights.h
    diff_highlights.h
    rust_highlights.h
    javascript_highlights.h

src/terminal_ui_kit/syntax/
  syntax_highlighter.cc

tests/terminal_ui_kit/unit/
  syntax_highlighter_test.cc
```

## Tests

### Unit tests (SyntaxHighlighter, no FTXUI rendering)

- C keywords are highlighted with accent style
- C strings are highlighted with success style
- C comments are highlighted with muted style
- Unknown language returns unstyled StyledText
- Empty code returns empty StyledText
- Multiple languages can be highlighted independently

## Build

When `TERMINAL_UI_KIT_ENABLE_TREE_SITTER=ON`:
- `terminal_ui_kit_syntax` links `tree-sitter::tree-sitter`
- Each grammar is fetched via FetchContent and compiled as static lib
- `terminal_ui_kit_syntax` links all grammar libraries
- CodeView conditionally calls SyntaxHighlighter

## Design decisions

| Decision | Choice |
|---|---|
| Dependencies | vcpkg feature |
| Grammars | Static linking, FetchContent |
| Languages | All 10 |
| Rendering | StyledText |
| Integration | CodeView calls SyntaxHighlighter internally |
| Queries | Built-in (constexpr strings) |
| Capture mapping | Fixed table (capture → Theme role) |
| Lazy loading | Parser created on first use |