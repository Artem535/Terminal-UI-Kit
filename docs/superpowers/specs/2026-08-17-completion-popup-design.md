# CompletionPopup Design

## Overview

A reusable completion/autocomplete popup for FTXUI-based terminal applications.
It provides a domain-neutral model + view for a dropdown of completion
candidates driven by pluggable providers (synchronous or asynchronous), with
fuzzy filtering, keyboard selection, stale-result cancellation, viewport-aware
placement, and deterministic, independently testable interaction logic.

This implements PRD section 23 (CompletionPopup) and matches the established
`components` module pattern (a `*Model`-style control object returning an
`ftxui::Component`, as in `VirtualList`/`VirtualListModel`).

## Data model

```cpp
enum class CompletionKind { kUnknown, kKeyword, kFunction, kVariable,
                            kType, kModule, kConstant };

struct CompletionItem {
  std::string label;                       // displayed
  std::string insert_text;                 // inserted on accept (falls back to label)
  std::string category;                    // optional group label
  std::string description;                 // optional one-line description
  std::optional<std::pair<std::size_t, std::size_t>> replacement_range; // [begin,end)
  std::shared_ptr<void> metadata;          // optional app payload (owned, safe)
  CompletionKind kind = CompletionKind::kUnknown;
};

struct CompletionContext { std::string query; std::size_t cursor_offset = 0; };
struct CompletionResult {
  std::vector<CompletionItem> items;
  std::optional<std::string> error;
};
```

Retained data uses owning types (`std::string`, `std::shared_ptr<void>`). No
`string_view`/pointer is retained beyond its source.

## Provider abstraction

```cpp
class ICompletionProvider {
 public:
  virtual ~ICompletionProvider() = default;
  virtual void complete(const CompletionContext& context,
                        std::uint64_t generation,
                        const std::function<void(std::uint64_t, CompletionResult)>& deliver) = 0;
};
```

- Not tied to Folly/Asio/executor: the callback is the only seam.
- A synchronous provider calls `deliver` before `complete` returns; an
  asynchronous provider may defer it (to any thread the host marshals to).
- `generation` is a per-component monotonic counter. Each `set_query` bumps it.
  A delivery tagged with a stale generation is discarded, so a late response
  can never clobber a newer query's results.
- `SyncCompletionProvider` adapts `std::function<std::vector<CompletionItem>(ctx)>`,
  catching exceptions into an error result.

## Component

`CompletionPopup` is a decorator wrapping a host input component (the pattern
used by FTXUI dropdowns). It manages state and exposes an `ftxui::Component`.

```cpp
struct CompletionPopupOptions {
  std::shared_ptr<ICompletionProvider> provider;
  int max_visible_rows = 8;
  bool fuzzy_filter = true;
  std::size_t min_query_length = 0;
  std::function<void(const CompletionItem&)> on_accept;
  const Theme* theme = nullptr;   // null => default_dark_theme()
};

class CompletionPopup {           // PIMPL over a ComponentBase
  CompletionPopup(ftxui::Component host, CompletionPopupOptions options);
  ftxui::Component component() const;
  void set_query(std::string query, std::size_t cursor_offset);
  void show(); void hide(); void toggle(); bool visible() const;
  void move_selection(int delta);
  std::size_t selected_index() const;
  bool accept_selected();
  const std::vector<CompletionItem>& items() const;
  // placement
  enum class Placement { kBelow, kAbove };
  static Placement decide_placement(int below, int above, int needed_rows);
  void set_available_space(int below, int above);  // viewport-aware; -1 = auto
  Placement placement() const;
  static std::string ApplyReplacement(const std::string& buffer, std::size_t cursor,
                                      const std::string& query, const CompletionItem& item);
};
```

### Interaction diagram

- `set_query` -> bump generation -> (validate length, else hide) -> set loading
  state -> `provider->complete(ctx, gen, deliver)`.
- `deliver` (guard: `weak.lock()` dead => drop; `gen != current` => drop):
  fuzzy-filter if enabled -> set items -> resolve state (results / no-results /
  error) -> clamp selection.
- `OnEvent`: when visible, Up/Down/PgUp/PgDn/Home/End navigate, Enter/Tab
  accept, Escape cancels; otherwise forwards to the host input.
- `Render`: host element + (when visible) popup list/composed above or below
  using `decide_placement`, truncating to `max_visible_rows` (narrow fallback).

### Viewport-aware placement

The library cannot know the host's layout, so the host supplies the geometry it
can measure (an example knows the terminal height and the input row). The pure,
testable decision lives in `decide_placement`: prefer below, flip above when it
fits above but not below, else the side with more room. The decorator also
applies a narrow fallback by rendering at most the rows that fit and a
"…" continuation marker. Resize is handled because placement and visible-row
count are recomputed every frame from the observed box and the supplied space.

### Async safety

- Stale results: generation guard.
- Destruction with a request pending: deliveries capture a `std::weak_ptr` to
  the impl; a late callback on a destroyed component is a no-op.
- Providers must marshal deferred callbacks to the UI thread (documented).

## Rendering

Each row: kind icon/color + label (selected row inverted) + dimmed category +
description (truncated to width). Loading, no-results, and error are small
status rows. Styling flows through `Theme` and `to_decorator` (style_bridge) as
in `StatusIndicator`.

## Testing

- Unit (pure logic, no FTXUI calls): fuzzy filter, replacement range incl.
  invalid, `decide_placement`, sync provider, async provider, loading, stale,
  cancellation, destruction-with-pending, provider error, duplicate labels,
  empty query, metadata.
- Rendering (virtual screen): above/below, narrow viewport, resize, loading /
  no-results / error rendering, categories & descriptions, selection
  navigation + accept via Enter/Tab, Escape cancel.

## Example

`examples/completion_popup_example.cpp`: input with two selectable providers
(immediate local, deterministic delayed via a timer), demonstrating loading,
fuzzy filtering, categories, descriptions, acceptance, no-results, stale-result
protection, above/below placement, and narrow-terminal layout. Registered in
CMake and Xmake and documented in `examples/README.md`.
