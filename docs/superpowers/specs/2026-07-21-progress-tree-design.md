# ProgressTree and TaskList design

## Model

`ProgressTask` is an immutable snapshot value with a stable string `id`,
label, `Status`, optional fraction, optional right-side detail, and children.
Callers replace snapshots; the component owns only expansion and selection.

## Components

`ProgressTree(std::vector<ProgressTask>, const Theme&)` renders a focusable,
navigable tree. It keeps expanded IDs across updated snapshots. `TaskList` is
a thin factory over the same renderer for flat root tasks.

Rows indent by depth and contain a chevron for parents, a StatusIndicator,
label, determinate ProgressBar when fraction is known, Spinner for running
tasks with no fraction, and a right-aligned detail string. Navigation is
Up/Down, Home/End, Space/Enter toggle selected parents.

## Testing and example

Rendering tests cover hierarchy, status/progress choices, collapse state and
key navigation without ScreenInteractive. `examples/task_dashboard` displays
a build/workflow snapshot and explains controls.
