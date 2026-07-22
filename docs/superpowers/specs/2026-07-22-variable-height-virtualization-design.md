# Variable-Height Virtualization Design

## Goal

Extend the existing fixed-height `VirtualList` with variable item heights,
layout caching, resize invalidation, and scroll anchoring while preserving the
PR5 API and behavior for existing callers.

## Scope

This PR covers only the virtualization primitive and its tests, benchmark, and
viewer. Streaming documents, follow-end mode, retention, ANSI parsing, and log
rendering remain PR7 work.

## Public API

`VirtualListOptions` gains:

```cpp
std::function<int(std::size_t index, int width)> estimate_height;
```

`item_height` remains as a compatibility fallback. If `estimate_height` is
provided, it supplies the initial height estimate; otherwise the existing
positive `item_height` is used. Estimated heights are clamped to at least one
row. The callback is never used to replace measured heights once layout data
exists.

No new public component type is introduced. `VirtualListModel` retains its
existing selection, scrolling, and callback semantics.

## Layout and rendering

The component maintains a per-index height cache plus prefix sums for the
current item count and width. Unknown items use `estimate_height`. Rendering
starts at the item containing the scroll offset and includes items until the
viewport is covered, with one extra item when available. Only those items are
materialized through `render_item`.

Each materialized element is wrapped in a layout observer. After FTXUI assigns
its box, the observed row height updates the cache. A changed measurement
invalidates prefix sums and requests another animation frame. A stable cache
is reused across renders; changing width or item count invalidates only the
affected layout data.

## Scroll anchoring

The scroll position is represented as a pixel/row offset rather than only an
item index. When a row before the visible anchor changes height, the offset is
adjusted by the same delta so the anchor remains at the same screen row. If
the changed row is at or after the anchor, no compensation is applied. On
count shrink, selection is clamped and the offset is clamped to the new total
content height without invoking `on_select` for normalization.

## Interaction

Arrow, page, home/end, wheel, programmatic selection, and
`scroll_to_index()` keep their PR5 meanings. Page movement uses the current
measured/estimated viewport range. Selecting an item makes it visible using
the cached prefix sums; it does not force rendering of unrelated items.

## Testing and examples

Rendering tests cover mixed measured heights, estimate fallback, cache reuse,
width invalidation, count shrink, selection visibility, and anchor stability.
The 100,000-item benchmark continues to assert bounded materialization. The
viewer gains a clearly labelled mixed-height mode so the behavior can be
verified interactively. All code follows the repository's Google C++ Style
configuration and existing naming deviations.

## Non-goals

No follow-end mode, append/retention model, ANSI parser, variable-height
document abstraction, or changes to Xmake wiring are included.
