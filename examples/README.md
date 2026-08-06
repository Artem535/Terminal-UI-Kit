# Examples

Each subdirectory under `examples/` (or standalone source in this directory) is
a runnable application demonstrating one component or model of Terminal UI Kit
in relative isolation.

## `command_history_example.cpp` — CommandHistory

Interactive demo of `terminal_ui_kit::command::CommandHistory`, a bounded,
navigable command-history model with persistence and sensitive-command policy.

```sh
cmake --preset all-features && cmake --build --preset all-features
./build/all-features/examples/terminal_ui_kit_example_command_history
```

What it demonstrates (all through the public `CommandHistory` API):

- Adding commands (`Enter`), with blank / consecutive-duplicate input ignored.
- `Up` / `Down` traversal (`Previous()` / `Next()`) with deterministic
  boundary behavior.
- Substring and prefix search over retained history (`F3` toggles the mode).
- Current history size and configured capacity.
- Clearing history (`F1`).
- Toggling a sensitive-command policy (`F2`: commands beginning with `secret:`
  are kept in memory but not persisted).
- Capacity eviction (the app is seeded past its capacity of 5, so the oldest
  commands are evicted and counted).
- A mock persistence adapter, so the "persistence store" pane shows exactly
  which commands were (and were not) written out.

Controls are rendered in the UI. Exit with `Esc`.
