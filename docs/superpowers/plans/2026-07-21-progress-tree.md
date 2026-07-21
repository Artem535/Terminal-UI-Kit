# ProgressTree Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans task-by-task.

**Goal:** Add navigable ProgressTree, flat TaskList, and task dashboard example.

**Architecture:** A value-only ProgressTask snapshot feeds a ComponentBase that owns selection/expansion keyed by stable IDs. TaskList delegates to the same renderer.

**Tech Stack:** C++20, FTXUI, GoogleTest, CMake.

## Global Constraints

- Tests are terminal-free rendering/event tests; no Xmake changes.
- ProgressTask snapshots are immutable inputs; UI state stores only IDs and selection.
- Existing StatusIndicator, ProgressBar, Spinner and Theme must be reused.

### Task 1: Model and static tree rendering

**Files:** create `progress_tree.h/.cc`, `progress_tree_test.cc`; modify Components/rendering CMake lists.

- [ ] Write failing tests for one root row, nested indentation, status icon, known fraction bar, and running unknown fraction spinner.
- [ ] Implement `ProgressTask { id, label, status, optional<double> fraction, detail, children }` and focusable `ProgressTree` ComponentBase renderer.
- [ ] Build focused tests and full suite; commit `Add ProgressTree rendering`.

### Task 2: Navigation, expansion and TaskList

**Files:** modify progress tree header/source/tests.

- [ ] Write failing event tests for Up/Down, Home/End, Space/Enter expansion, and expansion persistence when a new snapshot is supplied.
- [ ] Implement flattened visible rows, stable selected ID, expanded ID set, and `TaskList(std::vector<ProgressTask>, const Theme&)` factory.
- [ ] Run focused/full tests; commit `Add ProgressTree navigation and TaskList`.

### Task 3: task_dashboard example

**Files:** create `examples/task_dashboard/main.cc`, CMake file; modify examples CMake list.

- [ ] Show a build/workflow hierarchy with every row affordance and KeyHintBar control guide.
- [ ] Add executable target, build examples, run full suite and terminal smoke test.
- [ ] Commit `Add task dashboard example`.
