# Examples

Standalone runnable applications demonstrating each Terminal UI Kit component
in isolation. Each directory is an independent `add_executable` target seen by
`cmake --preset all-features` (which enables examples) and mirrors in
`examples/xmake.lua`.

## TranscriptView — `transcript_view_example`

Simulates an interactive coding-agent session rendered in a virtualized,
follow-end transcript. Demonstrates all primary `TranscriptView` behaviors on
deterministic, locally-generated data (no network access):

- heterogeneous blocks (text, Markdown, code, logs, status, diff, custom);
- a live mutable streaming tail that updates in place;
- follow-end auto-scroll that disables on manual scroll and resumes on `f`;
- mixed-height blocks;
- search with next/previous result navigation;
- bookmarks;
- copy and open-details callbacks;
- terminal resize handling;
- a 100,000-block large-data mode;
- streaming start/stop.

### Build

```sh
cmake --preset all-features
cmake --build --preset all-features --target terminal_ui_kit_example_transcript_view
build/all-features/examples/transcript_view_example/terminal_ui_kit_example_transcript_view
```

> The `all-features` preset builds optional Markdown/tree-sitter too. The
> example itself only requires the base `Components` module; to build it with
> just CMake defaults (plus examples) configure with
> `-DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON`.

### Controls

| Key       | Action                                    |
| --------- | ----------------------------------------- |
| `Space`   | Start / stop streaming                    |
| `f`       | Toggle follow-end                         |
| `/`       | Search (type a query; `Enter`/`Esc` close)|
| `n` / `N` | Next / previous search result             |
| `b`       | Add / remove bookmark on the active block |
| `Enter`   | Open block details                        |
| `y`       | Copy the active block's plain text        |
| `g` / `G` | Jump to beginning / end                   |
| `l`       | Generate a 100,000-block transcript       |
| `q` / `Esc`| Exit                                      |

### Behavior worth watching

- `Space` streams tokens into a single tail block; watch the block counter,
  which stays constant while the tail grows.
- Scroll up (arrow keys or wheel) turns `follow` from `FOLLOW` to `manual`;
  press `f` to resume following.
- Press `/`, type a few letters, then `n`/`N` to walk matches; matching blocks
  show a `●` gutter marker.
- Press `b` on an active block to bookmark it (gutter shows `◆`), `y` to copy,
  `Enter` to open details (status line reports both).
- Press `l` to render a 100,000-block transcript and verify navigation stays
  responsive.