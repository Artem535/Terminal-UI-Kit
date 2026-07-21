# VirtualList Design

## Goal

Implement PRD section 17's fixed-height `VirtualList` milestone (PR 5): a
focusable FTXUI component that efficiently renders only the visible part of a
large list. It must support 100,000 items without constructing a DOM tree for
every item, keyboard and mouse navigation, retained selection, and
programmatic scrolling. Variable-height layout and follow-end behaviour are
explicitly deferred.

## Scope

Included:

- Fixed-height items only.
- Viewport virtualization with a small configurable overscan.
- Keyboard navigation: Up, Down, PageUp, PageDown, Home, and End.
- Mouse-wheel scrolling.
- Retained selection and an `on_select` callback.
- Programmatic scroll and selection through `VirtualListModel`.
- Viewport-resize invalidation.
- An interactive 100,000-row `virtual_list_viewer` example.

Excluded:

- Variable item heights, layout caching, and scroll anchoring (PR 6).
- Follow-end and streaming append behaviour (PR 7).
- Filtering, searching, and domain-specific row models.

## Public API

```cpp
struct VirtualListOptions {
  std::function<std::size_t()> item_count;
  std::function<ftxui::Element(std::size_t index, int width)> render_item;
  int item_height = 1;
  int overscan = 2;
  std::function<void(std::size_t)> on_select;
};

class VirtualListModel {
 public:
  explicit VirtualListModel(VirtualListOptions options);

  ftxui::Component component() const;
  void scroll_to_index(std::size_t index);
  void select_index(std::size_t index);
  std::optional<std::size_t> selected_index() const;
};

ftxui::Component VirtualList(VirtualListOptions options);
```

`VirtualList(options)` is a convenience factory for callers that do not need
programmatic control. `VirtualListModel` owns the component state and exposes
the same component through `component()`.

`item_count` and `render_item` are callbacks so a caller can change backing
data without rebuilding the component. `render_item` receives the current
viewport width and returns an item element. VirtualList applies
`ftxui::inverted` to the selected item's element, ensuring selection is
visible without complicating the renderer callback.

`item_height <= 0` is normalized to one terminal row. Each returned item
element is constrained to that height. `overscan < 0` is normalized to zero.
An empty list has no selection and `selected_index()` returns
`std::nullopt`; a non-empty list initially selects index zero without calling
`on_select`.

## State and Rendering

The component stores:

- the normalized options;
- `scroll_index_`, the first visible item;
- an optional `selected_index_`;
- the last measured FTXUI viewport box.

It measures the allocated box with `ftxui::reflect()`. When that box changes,
it requests one animation frame so the next render uses the new viewport
height. The render window is calculated from the measured height and
`item_height`, then extended by `overscan` items on both sides. Only indexes
inside that window call `render_item`.

At most `ceil(viewport_height / item_height) + 2 * overscan` items are
rendered per frame, regardless of the total `item_count`. The component uses
flex layout so its reflected box is the allocated viewport rather than merely
the content's natural height.

When `item_count` changes, every public operation and render clamps both
stored indexes to the new valid range. If the list becomes empty, both are
cleared; if it subsequently becomes non-empty, index zero becomes selected
without calling `on_select`. This preserves a selected item whenever its index
remains valid and prevents out-of-range event handling.

## Interaction

- Up/Down move selection one item and scroll just enough to keep it visible.
- PageUp/PageDown move selection by one viewport of items.
- Home/End select and reveal the first or final item.
- A mouse-wheel event moves the viewport by three items; it does not alter
  selection.
- `select_index(index)` clamps the index, changes selection, invokes
  `on_select`, and reveals the item.
- `scroll_to_index(index)` clamps and changes only the viewport; it does not
  change selection.

Changing selection through keyboard or `select_index` invokes `on_select`
exactly once for that change. Events that cannot change state return false.

## Architecture

`VirtualListImpl` is a small `ftxui::ComponentBase` subclass in the
Components library. It is focusable, handles navigation itself, and renders a
`vbox` containing just the current virtual window. The model retains an
`ftxui::Component` pointing at that implementation, mirroring the existing
snapshot-model pattern used by `ProgressTreeModel`.

The initial implementation intentionally does not use `ftxui::frame()` over
the full list: that would create every item element and violate the
virtualization requirement. A custom FTXUI DOM node is also deferred because
the component-plus-reflected-viewport approach is sufficient for fixed-height
items and keeps the variable-height concerns isolated for PR 6.

## Tests and Example

Rendering tests use the existing virtual screen helper and public
`ComponentBase` methods. They cover empty-list normalization, options
normalization, navigation, mouse-wheel scrolling, model control methods,
selection retention after a changing item count, and viewport resize.

A 100,000-item regression test instruments `render_item` and verifies that a
render invokes it only for the viewport window plus overscan. It does not use
a wall-clock assertion, avoiding flaky performance tests while proving the
important complexity property.

`examples/virtual_list_viewer` demonstrates the component with 100,000 fixed
height rows. Its layout has clearly labelled sections explaining the virtual
window, reporting how many rows were rendered for the frame, and listing the
keyboard controls.

## Acceptance Criteria

- Rendering a 100,000-item list does not call `render_item` for every item.
- Selected rows are visibly inverted.
- Keyboard, wheel, and model navigation stay within bounds.
- Callers can retain a `VirtualListModel` while changing the data behind its
  callbacks without recreating the component.
- The example builds with the existing CMake examples option and demonstrates
  the virtualized list interactively.
