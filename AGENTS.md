# Repository Guidelines

## Project Structure & Architecture

Terminal UI Kit is a domain-neutral C++20/23 library built on FTXUI. Public
headers are in `include/terminal_ui_kit/`; implementations mirror them in
`src/terminal_ui_kit/`. Keep models separate from views, keep optional features
optional, and compose product-specific widgets outside the library. Tests live
in `tests/terminal_ui_kit/unit` and `tests/terminal_ui_kit/rendering`; examples
are in `examples/`, benchmarks in `benchmarks/`, and published docs in `docs/`.

## Starting a Feature

The issue tracker is the source of truth. Start from an issue and the roadmap
in `docs/modules/ROOT/pages/prd.adoc`; do not implement placeholder example
directories as though they were completed modules. Before creating work, update
`main`, then create an isolated worktree and a focused branch such as
`feat/42-multiline-editor`.

For non-trivial work, write or update a design spec in
`docs/superpowers/specs/` and a checkbox implementation plan in
`docs/superpowers/plans/`. Follow the dependency order in PRD section 63:
foundation/core, basic components, virtualization, streaming/logging, editor,
Markdown, then diff and terminal integrations. Include an example application
when the feature needs interactive verification.

## Build, Test, and Tooling

CMake is authoritative:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Use `all-features` for optional modules/examples and `sanitizers` for ASan/UBSan.
Xmake remains a secondary Linux frontend. Prefer the repository's `rtk` wrapper
for repository and forge operations: `rtk git <subcommand>` and
`rtk proxy gh <subcommand>`. Do not push, create issues, or open PRs unless the
user explicitly asks.

## Style and Tests

Use Google C++ style and C++20. `.clang-format` enforces 100 columns,
left-aligned pointers, and sorted includes; format touched files with
`clang-format -i path/to/file.cc`. Use `PascalCase` types, `snake_case`
functions/locals, `snake_case_` private fields, `kPascalCase` constants, and
`snake_case.h`/`.cc` files. Add focused GoogleTest coverage (`foo_test.cc`) for
every behavior; rendering tests use virtual-screen helpers. Add a benchmark for
large-data components.

## Commits and Pull Requests

Keep commits imperative and scoped: `Add ProgressTree`, `Fix grammar linking`.
Keep PRs small, link their issue, summarize API/behavior and exact verification,
and attach terminal screenshots for visible changes. Add every new primary target
to both CMake and Xmake. Rebase or merge from current `main` before review; do
not merge stale feature branches wholesale when their code is already present.
