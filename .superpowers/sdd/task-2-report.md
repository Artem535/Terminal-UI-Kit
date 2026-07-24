Status: DONE_WITH_CONCERNS

## Changes

- Added focused C++, Python, and Rust capture-to-role assertions.
- Mapped capture families to the dedicated `SyntaxTheme` roles, including
  dotted keyword/type/function/variable/constant/punctuation captures.
- Selected the dark or light syntax palette from the supplied built-in theme,
  while keeping the public `SyntaxHighlighter::highlight` signature intact.
- Preserved the existing non-overlapping interval resolution algorithm.

## Commits

Pending commit in this worktree.

## Verification

- `g++ -std=c++20 -Iinclude -I.../tree-sitter-src/lib/include -I.../tree-sitter-src/lib/src -c src/terminal_ui_kit/syntax/syntax_highlighter.cc` — PASS.
- `clang-format --dry-run -Werror` on changed C++ files — PASS after formatting.
- `git diff --check` — PASS.
- Full CMake/CTest execution was unavailable because this worktree's configure
  attempted to fetch FTXUI and network DNS could not reach GitHub.

## Self-review and concerns

- Custom themes that are not byte-for-byte equal to the built-in light theme
  use the dark syntax palette; the API does not expose an explicit light/dark
  mode, so this is the safest backward-compatible inference.
- Runtime tests should be rerun by the parent agent using its cached dependency
  build; the focused assertions are intentionally strict about semantic role
  colors and background absence.

## Review follow-up

- Updated string/escape and number/float handling to use the same dotted-family
  matching as keywords, types, and other capture groups.
- Extended the Python and Rust checks with float and escape captures.
- `g++` standalone compile, clang-format, and `git diff --check` pass. Full
  CTest remains unavailable in this worktree because dependency fetching is
  blocked by network DNS.
