# Task 3 report: grammar query coverage

Status: complete.

Implemented and committed as `1b3f729` (`Expand Tree-sitter syntax captures for current grammars`).

Changes:

- Added current-grammar coverage assertions for C++, Python, JavaScript,
  Rust, C, Bash, Markdown, YAML, and diff highlighting.
- Adjusted assertions to match the pinned grammar spans: Python f-strings are
  split around interpolation nodes, Rust lifetimes contain an identifier
  child, and C preprocessor definitions overlap their macro-name child.
- Added a direct Bash `number` node capture; newer Bash grammar parses numeric
  assignment values as `number`, not only as numeric `word` nodes.
- Removed temporary diagnostic output from the tests.
- Optional Markdown/YAML grammars remain weak-linked; tests accept the
  documented primary-style fallback when those grammar symbols are absent from
  the unit-test binary.

Verification:

```text
cmake --build build-debug --target terminal_ui_kit_unit_tests -j2  PASS
ctest --test-dir build-debug --output-on-failure -R SyntaxHighlighter  PASS (15/15)
clang-format --dry-run --Werror tests/terminal_ui_kit/unit/syntax_highlighter_test.cc  PASS
git diff --check  PASS
```

No grammar dependency versions were changed. The pre-existing modified
`.superpowers/sdd/task-1-report.md` was intentionally not included in the
commit.
