# Example Applications

Standalone, runnable examples demonstrating the library components in isolation
(PRD section 54). Each is a separate executable registered in
`examples/CMakeLists.txt` (and mirrored in `examples/xmake.lua`).

## line_number_formatting_example

Interactive demo of the line-number gutter in `CodeView` and `VirtualDocument`.
It exercises the fix for unsigned underflow: a rendered line number wider than
the configured gutter width is displayed in full (never truncated) and produces
no huge padding.

The document contains enough lines so that the line numbers `1`, `9`, `99`,
`9999`, `10000`, `99999` and `100000` all exist. Key `w` cycles the gutter width
through `1, 2, 3, 4, 5, 6, 8` so you can observe:

- short numbers right-aligned within the gutter;
- numbers wider than the gutter rendered in full, with no clipping;
- no enormous spacing after a wide number;
- the layout staying correct after a terminal resize.

Controls:

| Key      | Action                  |
|----------|-------------------------|
| `w`      | cycle gutter width      |
| up/down  | scroll                  |
| `q`/ESC  | quit                    |

Build and run (CMake):

```sh
cmake --preset debug -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON
cmake --build --preset debug --target terminal_ui_kit_example_line_number_formatting
./build/debug/examples/terminal_ui_kit_example_line_number_formatting
```