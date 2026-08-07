# Examples

Standalone, runnable example applications built on Terminal UI Kit (PRD
section 54). Each is a separate `add_executable` target added through
`examples/CMakeLists.txt`. Build them with:

```sh
cmake --preset all-features   # includes TERMINAL_UI_KIT_BUILD_EXAMPLES=ON
cmake --build build/all-features
```

## searchable_text_view_example

Demonstrates the reusable, searchable text view component
(`terminal_ui_kit/components/searchable_text_view.h`) with a deterministic,
sizeable sample document (repeated tokens plus UTF-8 Cyrillic lines):

- literal and regular-expression search
- case-sensitive / case-insensitive matching
- incremental query updates while typing
- previous/next match navigation with wrap-around
- current-match index and total match count
- invalid-regex and no-results states
- UTF-8 source, unchanged; all visible matches highlighted, active match
  in a distinct style
- automatic scrolling to the active match; re-wrap on terminal resize

Controls:

| Key   | Action                            |
|-------|-----------------------------------|
| `/`   | Open search                       |
| type  | Edit the query (incremental)      |
| Enter | Apply the query / close prompt    |
| `n`   | Next match                        |
| `N`   | Previous match                    |
| `c`   | Toggle case sensitivity           |
| `r`   | Toggle regex mode                 |
| Esc   | Close / cancel search             |
| `q`   | **Quit** (documented exit key)    |

Build and run:

```sh
cmake --build build/all-features --target terminal_ui_kit_example_searchable_text_view
./build/all-features/examples/searchable_text_view_example/terminal_ui_kit_example_searchable_text_view
```