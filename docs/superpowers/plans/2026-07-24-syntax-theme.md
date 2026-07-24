# GitHub Dark Syntax Theme Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand Tree-sitter highlighting with a dedicated GitHub Dark-like syntax palette and broader capture coverage without applying UI status backgrounds to code tokens.

**Architecture:** Add a `SyntaxTheme` value type and derive it from the existing terminal `Theme` in `SyntaxHighlighter`. Keep syntax styles separate from component roles, normalize syntax styles to foreground/attributes only, and map capture families to dedicated roles. Update all pinned grammar queries and validate representative colors and background behavior through rendering tests.

**Tech Stack:** C++20, Tree-sitter, FTXUI, GoogleTest, CMake.

## Global Constraints

- Preserve the existing `Theme` API for ordinary components.
- Keep `SyntaxHighlighter::highlight(std::string_view, std::string_view, const Theme&)` source-compatible.
- Syntax styles must not inherit `Theme::code.background`.
- Preserve source bytes exactly, including when Tree-sitter captures overlap.
- Keep queries compatible with grammar revisions pinned in `cmake/TerminalUiKitDependencies.cmake`.
- Use Google C++ style and run `clang-format-18 --dry-run --Werror` on changed C++ files.
- Validate with the full CMake build, all CTest tests, and the syntax example executable.

---

### Task 1: Add the syntax palette type and GitHub Dark defaults

**Files:**
- Create: `include/terminal_ui_kit/syntax/syntax_theme.h`
- Create: `src/terminal_ui_kit/syntax/syntax_theme.cc`
- Modify: `src/terminal_ui_kit/CMakeLists.txt:119-121`
- Test: `tests/terminal_ui_kit/unit/syntax_theme_test.cc`
- Modify: `tests/terminal_ui_kit/unit/CMakeLists.txt`

**Interfaces:**
- Consumes: `terminal_ui_kit::Theme`, `terminal_ui_kit::TextStyle`.
- Produces: `struct SyntaxTheme`, `SyntaxTheme default_dark_syntax_theme(const Theme&)`, and `SyntaxTheme default_light_syntax_theme(const Theme&)`.

- [ ] **Step 1: Write the failing tests**

Add tests that assert the dark palette gives distinct foreground colors for keyword, type, function, string, number, comment, property, namespace, and macro, and that every syntax role has no background:

```cpp
TEST(SyntaxTheme, DarkPaletteUsesDistinctSemanticColors) {
  SyntaxTheme syntax = default_dark_syntax_theme(default_dark_theme());
  EXPECT_NE(syntax.keyword.foreground, syntax.type.foreground);
  EXPECT_NE(syntax.type.foreground, syntax.function.foreground);
  EXPECT_NE(syntax.function.foreground, syntax.string.foreground);
  EXPECT_NE(syntax.string.foreground, syntax.number.foreground);
  EXPECT_NE(syntax.number.foreground, syntax.comment.foreground);
}

TEST(SyntaxTheme, SyntaxRolesDoNotHaveBackgrounds) {
  SyntaxTheme syntax = default_dark_syntax_theme(default_dark_theme());
  for (const TextStyle* style : syntax.roles()) {
    EXPECT_FALSE(style->background.has_value());
  }
}
```

Expose `roles()` as a const collection of pointers only if that keeps the test simple; otherwise assert each named role explicitly.

- [ ] **Step 2: Run the tests and verify they fail**

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_unit_tests
ctest --test-dir build-debug --output-on-failure -R SyntaxTheme
```

Expected: compile failure because `SyntaxTheme` and its factories do not exist.

- [ ] **Step 3: Implement the palette**

Define:

```cpp
struct SyntaxTheme {
  TextStyle keyword;
  TextStyle type;
  TextStyle function;
  TextStyle variable;
  TextStyle string;
  TextStyle number;
  TextStyle comment;
  TextStyle operator_style;
  TextStyle property;
  TextStyle namespace_style;
  TextStyle macro;
  TextStyle constant;
};

SyntaxTheme default_dark_syntax_theme(const Theme& theme);
SyntaxTheme default_light_syntax_theme(const Theme& theme);
```

Use explicit GitHub Dark-like RGB colors. Preserve useful attributes from the base theme where appropriate, but clear `background` for every syntax role. Keep the factories pure and do not mutate the supplied `Theme`.

- [ ] **Step 4: Register and run the tests**

Add `syntax_theme.cc` to the syntax target and the new test to the unit target. Run the focused tests and expect all SyntaxTheme tests to pass.

- [ ] **Step 5: Commit**

```bash
git add include/terminal_ui_kit/syntax/syntax_theme.h src/terminal_ui_kit/syntax/syntax_theme.cc src/terminal_ui_kit/syntax/CMakeLists.txt tests/terminal_ui_kit/unit/syntax_theme_test.cc tests/terminal_ui_kit/unit/CMakeLists.txt
git commit -m "Add dedicated syntax highlighting theme"
```

### Task 2: Map captures to syntax roles

**Files:**
- Modify: `src/terminal_ui_kit/syntax/syntax_highlighter.cc`
- Modify: `tests/terminal_ui_kit/unit/syntax_highlighter_test.cc`

**Interfaces:**
- Consumes: `SyntaxTheme` factories from Task 1.
- Produces: capture-to-style behavior used by all existing `SyntaxHighlighter` callers.

- [ ] **Step 1: Add failing mapping tests**

Highlight a C++ sample containing a keyword, type, function call, string, number, comment, namespace, and operator. Assert the corresponding spans have distinct foreground colors and no background. Add equivalent representative checks for Python and Rust to ensure language-specific captures use the same semantic palette.

- [ ] **Step 2: Run the focused tests and verify failure**

```bash
cmake --build build-debug --target terminal_ui_kit_unit_tests
ctest --test-dir build-debug --output-on-failure -R 'SyntaxHighlighter'
```

Expected: new color assertions fail because `style_for_capture()` still returns ordinary `Theme` roles.

- [ ] **Step 3: Implement mapping**

Construct the appropriate `SyntaxTheme` once per `highlight()` call and change `style_for_capture()` to accept it. Map capture families exactly as specified:

```text
keyword.* -> keyword
type.* -> type
function.*, method, constructor -> function
variable, parameter -> variable
string, escape -> string
number, float -> number
constant.* -> constant
property, field -> property
namespace -> namespace_style
macro, attribute, decorator -> macro
comment -> comment
operator, punctuation.* -> operator_style
```

Unknown captures use `syntax.variable` or the primary foreground style with no background. Keep the existing non-overlapping interval algorithm unchanged.

- [ ] **Step 4: Run focused tests**

Run all SyntaxHighlighter tests and expect them to pass, including the overlap regression and empty-code behavior.

- [ ] **Step 5: Commit**

```bash
git add src/terminal_ui_kit/syntax/syntax_highlighter.cc tests/terminal_ui_kit/unit/syntax_highlighter_test.cc
git commit -m "Map Tree-sitter captures to syntax palette"
```

### Task 3: Expand and validate grammar queries

**Files:**
- Modify: `include/terminal_ui_kit/syntax/queries/c_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/cpp_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/python_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/rust_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/javascript_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/bash_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/markdown_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/yaml_highlights.h`
- Modify: `include/terminal_ui_kit/syntax/queries/diff_highlights.h`
- Modify: `tests/terminal_ui_kit/unit/syntax_highlighter_test.cc`

**Interfaces:**
- Consumes: pinned grammar revisions from `cmake/TerminalUiKitDependencies.cmake`.
- Produces: current-node queries that emit the capture families consumed by Task 2.

- [ ] **Step 1: Add language coverage tests**

For each supported grammar, add a compact source sample and assert that highlighting produces at least one span in each expected semantic category. Include the newer node forms already required by the pinned versions: Python f-strings and booleans, C++ namespace identifiers and booleans, JavaScript function expressions and `null`/`this`, Rust lifetimes/macros, and Markdown block nodes.

- [ ] **Step 2: Run tests before query changes**

```bash
cmake --build build-debug --target terminal_ui_kit_unit_tests
ctest --test-dir build-debug --output-on-failure -R SyntaxHighlighter
```

Expected: newly added language assertions fail for captures not emitted by the current query text.

- [ ] **Step 3: Update queries**

Use only node names present in the pinned grammar sources. Add or adjust captures for the semantic families in the spec; remove obsolete node names that cause `TSQueryErrorNodeType`. Keep query strings embedded in the existing generated-header style and do not change grammar dependency versions in this task.

- [ ] **Step 4: Run all syntax tests and inspect query errors**

```bash
ctest --test-dir build-debug --output-on-failure -R SyntaxHighlighter
```

Expected: all language tests pass and no query parse errors occur.

- [ ] **Step 5: Commit**

```bash
git add include/terminal_ui_kit/syntax/queries tests/terminal_ui_kit/unit/syntax_highlighter_test.cc
git commit -m "Expand Tree-sitter syntax captures for current grammars"
```

### Task 4: Verify CodeView and examples visually and by regression tests

**Files:**
- Modify: `tests/terminal_ui_kit/rendering/markdown_view_test.cc`
- Modify: `examples/components_gallery/main.cc` to include representative C++, Python, and Rust snippets

**Interfaces:**
- Consumes: updated `SyntaxHighlighter` and syntax queries.
- Produces: examples that visibly demonstrate semantic colors without duplicated text or background artifacts.

- [ ] **Step 1: Add a rendering regression for syntax backgrounds**

Render a code block containing a type and namespace, inspect the corresponding `ftxui::Pixel` values, and assert the code token foreground is set while its background remains `ftxui::Color::Default`.

- [ ] **Step 2: Build and run the rendering test**

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R 'MarkdownView|CodeView'
```

- [ ] **Step 3: Build every example**

```bash
cmake --build build-debug --parallel 4
```

Expected: `components_gallery`, `markdown_viewer`, and all existing examples build successfully.

The gallery must show at least one string, number, type, function, comment, and
namespace in its existing language samples so the expanded palette is visible
without adding a second executable.

- [ ] **Step 4: Run the Markdown example and check source preservation**

```bash
TERM=xterm timeout 2s ./build-debug/examples/markdown_viewer/terminal_ui_kit_example_markdown_viewer > /tmp/markdown_viewer.syntax.out 2>&1 || true
! rg -a -n 'fibonaccifibonaccifibonacci|fibonaccifibonacci|mainmain' /tmp/markdown_viewer.syntax.out
```

- [ ] **Step 5: Commit**

```bash
git add tests/terminal_ui_kit/rendering/markdown_view_test.cc examples/components_gallery/main.cc
git commit -m "Validate syntax theme rendering in examples"
```

### Task 5: Full verification

- [ ] **Step 1: Format and check the diff**

```bash
clang-format-18 --dry-run --Werror $(git diff --name-only HEAD~4..HEAD -- '*.cc' '*.h')
git diff HEAD~4..HEAD --check
```

- [ ] **Step 2: Run the complete build and test suite**

```bash
cmake --build build-debug --parallel 4
ctest --test-dir build-debug --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 3: Inspect final status**

```bash
git status --short
git log -5 --oneline
```

Expected: clean worktree and the syntax-theme commits visible in history.
