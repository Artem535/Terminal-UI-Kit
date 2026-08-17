# Examples

Standalone runnable applications that demonstrate individual components in
isolation. Each example uses only the public API of the library and requires
no network access. Build them with CMake (authoritative) or Xmake:

```sh
cmake --preset all-features && cmake --build --preset all-features
# or
xmake f --examples=y && xmake
```

## completion_popup_example

Demonstrates the `CompletionPopup` component: an input field with fuzzy
autocomplete backed by two selectable providers.

- **Immediate** provider — a synchronous local `SyncCompletionProvider` that
  resolves as you type.
- **Delayed** provider — a deterministic asynchronous provider that resolves
  after a fixed 250 ms delay, showing the loading state and the component's
  stale-result protection: type quickly and only the latest query's results
  appear; the older (stale) response is discarded.

Features shown: loading state, fuzzy filtering, category and description
rendering, acceptance (replacement-range aware), no-results state, provider
switching, and viewport-aware placement (the popup renders below the input when
there is room, and flips above / degrades to fewer rows on a narrow terminal —
resize the terminal to see it adapt).

Controls:

| Key       | Action                          |
|-----------|---------------------------------|
| type      | fuzzy-filter the dataset        |
| up/down   | move selection                  |
| enter/tab | accept the selected completion  |
| esc       | cancel the popup                |
| m         | toggle immediate / delayed mode |
| q         | quit                            |

Exit by pressing `q`.
