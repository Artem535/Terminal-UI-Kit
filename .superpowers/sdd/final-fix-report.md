# Final review fixes: Tree-sitter syntax theme

## Changes

- Replaced weak references for optional YAML, Markdown, and Diff grammars with
  CMake-provided compile-time feature macros and strong references. A grammar
  is advertised by `supports_language()` exactly when its grammar target is
  linked into `terminal_ui_kit_syntax`.
- Extended capture-family matching to dotted captures (`property.*`,
  `field.*`, `namespace.*`, `macro.*`, `attribute.*`, `decorator.*`,
  `comment.*`, `operator.*`, and `punctuation.*`).
- Updated representative queries to use dotted capture names and added a
  regression fixture covering property, attribute, and field roles.

## Verification

Commands run from this worktree:

```text
cmake --build build-debug --target terminal_ui_kit_unit_tests --parallel 4
  Built terminal_ui_kit_unit_tests successfully.

ctest --test-dir build-debug --output-on-failure -R 'SyntaxHighlighter\\.(DottedCaptureFamiliesUseSemanticRoles|CoversYamlBlockNodes|CoversDiffBlockNodes)'
  3/3 tests passed; Markdown was skipped because its grammar fetch was
  unavailable in this configure (the source directory already existed but
  was incomplete).

clang-format-18 --dry-run --Werror src/terminal_ui_kit/syntax/syntax_highlighter.cc tests/terminal_ui_kit/unit/syntax_highlighter_test.cc
  Passed.

git diff --check
  Passed.
```

## Concern

The current worktree's Markdown grammar checkout is incomplete, so the
Markdown availability test remains independently skipped. Once the grammar
checkout is restored or fetched, the compile-time macro wiring will exercise
it through a strong linker reference like YAML and Diff.
