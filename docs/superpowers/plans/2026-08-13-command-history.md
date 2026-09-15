# CommandHistory Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans task-by-task.

**Goal:** Add a bounded, navigable CommandHistory model for input and editor components, plus persistence and sensitive-command policy abstractions.

**Architecture:** A standalone, FTXUI-free model in the `editor` module. Commands are owned `std::string`s in a `std::vector` (chronological, newest last). Navigation uses a single `std::optional<size_t>` cursor with deterministic boundary behavior. A `CommandHistoryStore` adapter and a `CommandPersistencePolicy` (with a `SensitiveCommandPolicy`) are attached as `unique_ptr`s; persistence is best-effort and exceptions are swallowed so a failing store never corrupts in-memory state. An interactive FTXUI example demonstrates the full public API.

**Tech Stack:** C++20, FTXUI (example only), GoogleTest, CMake, Xmake (module header mirror).

## Global Constraints

- Model must not depend on FTXUI.
- Retained commands must own their memory (`std::string`, never `string_view`).
- Persistent writes are best-effort; a throwing store must not corrupt history.
- Capacity `0` is handled explicitly and safely.

### Task 1: Model, persistence and policy

**Files:** create `include/terminal_ui_kit/editor/command_history.h`, `src/terminal_ui_kit/editor/command_history.cc`; convert the `editor` module from INTERFACE to compiled in `src/terminal_ui_kit/CMakeLists.txt`.

- [ ] Define `CommandHistory { Add, Previous, Next, Search, SearchPrefix, Clear, Size, Capacity, Current }` plus `CommandHistoryStore`, `CommandPersistencePolicy`, `AlwaysPersistPolicy`, `SensitiveCommandPolicy`.
- [ ] Implement bounded storage, blank/consecutive-duplicate filtering, deterministic Previous/Next boundaries, stable most-recent-first search order, cursor reset on Add, and safe capacity-0 handling.
- [ ] Wire persistence through policy + store with exception swallowing.
- [ ] Add unit tests; build and run them.

### Task 2: Example and docs

**Files:** create `examples/command_history_example/main.cc` + `CMakeLists.txt`; modify `examples/CMakeLists.txt`; create `examples/README.md`.

- [ ] Interactive input demonstrating add, Up/Down navigation, substring and prefix search, size/capacity, clear, sensitive-mode toggle, and capacity eviction — using only the public API.
- [ ] Build examples, run the suite, smoke-test the demo.
- [ ] Commit.

### Task 3: Review, verification, commit

- [ ] GCC + Clang strict builds (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`).
- [ ] ASan/UBSan run.
- [ ] Subagent review; fix important findings; commit.
- [ ] Push and open PR.
