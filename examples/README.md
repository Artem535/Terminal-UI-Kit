# Examples

Each example is a standalone executable demonstrating one component or family
of components in isolation. Build them with CMake:

```sh
cmake -S . -B build/examples -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON
cmake --build build/examples -j
```

## unified_diff_view

`unified_diff_view_example.cpp` — an interactive viewer for the
`UnifiedDiffView` component. It parses a unified diff with the canonical
`UnifiedDiffParser` and displays it as a virtualized, scrolling diff view. All
sample data is deterministic and self-contained (no network access).

Scenarios (cycle with `Tab`): normal single-file diff, multi-file diff, new
file, deleted file, binary file, multiple hunks, long lines, empty diff, and a
~100,000-line diff.

Controls:

```text
j/k or ↑↓   scroll / select a row
n/N         next / previous hunk
]/[         next / previous file
Enter       collapse / expand the current file
/           search (Enter cycles matches, Esc closes)
y           copy the selected line's source text
Tab         switch scenario
q/Esc       quit
```

The status line reports `File X/Y · Hunk A/B · Visible rows C–D of N`. The view
is virtualized, so the 100,000-line scenario stays responsive.

## Other examples

* `theme_viewer` — browse the built-in themes.
* `components_gallery` — a gallery of basic components.
* `progress_viewer` / `progress` — progress-bar and progress-tree demos.
* `task_dashboard` — a composite dashboard of several components.
* `virtual_list_viewer` — the virtualized list component.
* `streaming_log_viewer` — the streaming log view.
* `virtual_document_viewer` — the virtualized document view.
* `diff_parser` — a non-interactive dump of the unified-diff parser/model.
* `multiline_editor` — the multiline editor.
* `markdown_viewer` — the Markdown view (built when Markdown is enabled).
