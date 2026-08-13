# Terminal UI Kit Examples

Each example is a standalone executable that demonstrates one or more library
components in isolation. Build with CMake:

```sh
cmake --preset all-features
cmake --build --preset all-features
```

## toast_example — Toast Viewer

Demonstrates the reusable toast-notification component: every severity,
queueing and the configurable max-visible limit, timed and persistent toasts,
action callbacks, keyboard focus with timeout pause, no-color fallback,
clear-all, and terminal resize.

Controls:

```text
i            Add an info toast
s            Add a success toast
w            Add a warning toast
e            Add an error toast
a            Add a toast with an action ("Revert")
p            Add a persistent toast (never expires)
c            Clear all toasts
n            Toggle no-color mode
Tab          Move focus to the next toast
Shift+Tab    Move focus to the previous toast
Enter        Invoke the focused toast's action
Delete       Close the focused toast
q / Esc      Exit
```

Key behaviours to observe:

- **Timeout** — timed toasts auto-expire after their duration.
- **Queueing** — with more toasts than `max_visible`, extras are queued and
  slide in as earlier ones close or expire.
- **Focus pause** — while a toast is focused (Tab), its timeout is paused;
  moving focus off the last toast resumes it. The status line shows
  `[focus paused]`.
- **Actions** — press `a` then focus the toast and press `Enter` to fire its
  `Revert` action exactly once (the toast closes; the counter increments).
- **No-color** — press `n` to strip color (icons and bold remain).
- **Resize** — the view re-flows on terminal resize; long messages wrap in a
  narrow column.