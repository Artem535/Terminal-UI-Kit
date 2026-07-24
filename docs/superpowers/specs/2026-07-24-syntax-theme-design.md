# Syntax Highlighting Theme Design

## Goal

Improve Tree-sitter syntax highlighting so the examples resemble GitHub Dark:
more semantic categories receive distinct colors, syntax highlighting no longer
reuses status-oriented UI colors, and syntax spans do not render accidental
background blocks.

## Scope

The change covers the syntax theme model, capture-to-style mapping, and query
coverage for the currently supported grammars. It does not change the public
`Theme` roles used by ordinary components, Markdown layout, or Tree-sitter
range normalization.

## Syntax palette

Add a dedicated `SyntaxTheme` containing independent `TextStyle` roles:

- `keyword`
- `type`
- `function`
- `variable`
- `string`
- `number`
- `comment`
- `operator_style`
- `property`
- `namespace_style`
- `macro`
- `constant`

The default dark syntax palette follows GitHub Dark proportions: blue
keywords, cyan types and namespaces, yellow functions, green strings, orange
numbers/constants, muted gray comments, neutral operators/punctuation, and a
pink accent for macros/decorators. Syntax roles have no background color;
`Theme::code.background` remains available for code blocks and is not applied
to type or namespace captures.

## Capture mapping

`style_for_capture()` maps Tree-sitter capture names to `SyntaxTheme` roles:

| Capture family | Syntax role |
| --- | --- |
| `keyword.*` | `keyword` |
| `type.*` | `type` |
| `function.*`, `method`, `constructor` | `function` |
| `variable`, `parameter` | `variable` |
| `string`, `escape` | `string` |
| `number`, `float` | `number` |
| `constant.*` | `constant` |
| `property`, `field` | `property` |
| `namespace` | `namespace_style` |
| `macro`, `attribute`, `decorator` | `macro` |
| `comment` | `comment` |
| `operator`, `punctuation.*` | `operator_style` |

Unknown capture names fall back to the primary syntax text style.

## Query coverage

Update the C++, Python, Rust, JavaScript, C, Bash, YAML, Markdown, and Diff
queries to use current grammar node names and expose the capture families
above where the grammar provides them. Query changes must remain compatible
with the pinned grammar revisions in `TerminalUiKitDependencies.cmake`.

## API and data flow

`SyntaxHighlighter::highlight()` obtains the syntax theme from the selected
terminal theme, parses the source, resolves captures into non-overlapping byte
intervals, and emits `TextSpan` values using syntax styles. Existing callers
continue to pass `Theme`; no caller-specific or agent-specific types are added
to the public API.

## Validation

Add or update tests that verify:

1. source text is preserved exactly when captures overlap;
2. representative captures map to distinct syntax colors;
3. syntax spans do not carry a background color;
4. the Markdown and gallery examples render without duplicated text or
   regressions.

Run the full CMake build, all CTest tests, and the example binaries after the
change.
