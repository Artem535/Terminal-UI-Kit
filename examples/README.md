# Example applications

Standalone runnable demos of the library's components. Each example is a
separate executable and demonstrates only the public API of the component it
exercises.

| Example | Executable | Demonstrates |
| --- | --- | --- |
| `theme_viewer` | `terminal_ui_kit_example_theme_viewer` | Theme roles, light/dark |
| `components_gallery` | `terminal_ui_kit_example_components_gallery` | Component gallery |
| `progress_viewer` | `terminal_ui_kit_example_progress_viewer` | Progress bars / tree |
| `task_dashboard` | `terminal_ui_kit_example_task_dashboard` | ProgressTree dashboard |
| `virtual_list_viewer` | `terminal_ui_kit_example_virtual_list_viewer` | VirtualList |
| `streaming_log_viewer` | `terminal_ui_kit_example_streaming_log_viewer` | StreamingDocument / LogModel |
| `virtual_document_viewer` | `terminal_ui_kit_example_virtual_document_viewer` | VirtualDocument |
| `diff_parser` | `terminal_ui_kit_example_diff_parser` | UnifiedDiffParser |
| `toast_example` | `terminal_ui_kit_example_toast` | ToastManager / ToastView |

## `toast_example`

Interactive toast system demo: timed + persistent notifications, a FIFO queue
with a configurable visible-count limit, optional action callbacks, keyboard
focus with a timeout pause while focused, and a no-color fallback.

Run it with:

```sh
cmake --preset debug --build examples
./build/debug/examples/toast_example/terminal_ui_kit_example_toast
```

Controls:

```text
i          Add info toast
s          Add success toast
w          Add warning toast
e          Add error toast
a          Add toast with action
p          Add persistent toast (dismiss with Delete or c)
c          Clear all toasts
Tab        Move focus to next toast
Shift+Tab  Move focus to previous toast
Enter      Invoke the focused toast's action
Delete     Close the focused toast
t          Toggle color / no-color fallback
q or Esc   Quit
```

Timed toasts show a live countdown and drain automatically under the running
`ScreenInteractive` loop; the focused toast's timeout is paused while it stays
focused. Exceed `max_visible` to see queueing first-in/first-out. Quit with `q`
or `Esc`.
