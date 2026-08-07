# CompletionPopup — Design

## 1. Purpose
A domain-neutral, reusable autocomplete popup (PRD §23) that composes over any
input. It supports synchronous and asynchronous providers, fuzzy filtering,
keyboard selection, replacement ranges, viewport-aware placement, and
deterministic interaction tests.

## 2. Public API (`components/completion_popup.h`)

### Data types
```cpp
enum class CompletionKind { kUnknown, kKeyword, kVariable, kFunction, kType,
                            kModule, kFile, kSnippet };

struct CompletionRange { int begin = 0; int end = 0; };  // byte offsets into the line

struct CompletionItem {
  std::string label;        // display label (also insert text if insert_text empty)
  std::string insert_text;  // text inserted on acceptance
  std::string description;  // optional detail line
  std::string category;     // optional grouping tag
  CompletionKind kind = CompletionKind::kUnknown;
  CompletionRange range;    // optional explicit replacement range
  std::vector<std::string> meta;  // optional arbitrary metadata
};

struct CompletionContext {
  std::string query;   // token being completed (already trimmed)
  std::string text;    // full line text
  int cursor = 0;      // byte offset into text
};
```

### Providers
`class ICompletionProvider` — `virtual void complete(const CompletionContext&,
std::function<void(Result)> on_result) = 0;` where
```cpp
struct Result { std::vector<CompletionItem> items; std::string error; };
```
Results may arrive synchronously or later / on another thread. Each new
`complete` call supersedes prior in-flight requests; the model drops stale
results.

`SyncCompletionProvider(std::function<std::vector<CompletionItem>(const
CompletionContext&)>)` — runs immediately, catches exceptions into `Result::error`.

`AsyncCompletionProvider(std::function<std::vector<CompletionItem>(const
CompletionContext&)>, std::function<void(std::function<void()>)> schedule)` —
posts the work through `schedule`; the host chooses a thread pool, event loop,
or a deterministic test queue.

### Fuzzy matching
`struct FuzzyMatch { bool matched; int score; };`
`FuzzyMatch fuzzy_match(const std::string& query, const std::string& text);`
Subsequence match (case-insensitive) with a small scoring heuristic (earlier
start, consecutive runs preferred). Used to filter + sort provider results.

### Model
`class CompletionPopupModel` — headless controller. Holds the provider, the
current input, generation counter, and the shared lifecycle token. Exposes:
* `set_input(std::string text, int cursor)` (recomputes query, re-queries)
* `set_provider(std::shared_ptr<ICompletionProvider>)`
* `state()` → `kHidden/kLoading/kResults/kNoResults/kError`
* `items()`, `error()`, `selected()`, `select_relative(int)`/`select_index`
* `accept()` and `cancel()`; `placement()` + `recompute_placement(...)`
* `on_accept` callback receives `(CompletionItem, std::string new_text, int new_cursor)`
* `set_default_range(int begin, int end)` — the range to replace when an item
  does not provide its own (defaults to the current token range).

### View
`class CompletionPopup : public ftxui::ComponentBase` wraps the model, renders
the list loading/no-results/error banners and the selectable rows (with
category + description, inverse highlight on the selection), and maps keyboard
events (Up/Down, Enter/Tab accept, Escape cancel) to model operations. Placement
(`kAbove`/`kBelow`) is recomputed on every `Render` from `anchor_row` /
`viewport_height`; rows are drawn at the top or bottom region of the overlay box.

## 3. Async safety model
FTXUI 5 `ComponentBase` has no `enable_shared_from_this`. The model stores a
`std::shared_ptr<LifecycleToken>` that the async callback captures; the token
holds the generation counter, the delivered results, and an atomic `dead`
flag set by the model destructor. Late callbacks mutate only the token (guarded
by a mutex) and bail on `dead`/generation mismatch before touching any `this`
member. Destruction is therefore always safe, including for a genuinely
threaded scheduler.

## 4. Placement
`Placement compute_placement(int needed, int anchor_row, int viewport_height)`:
* `below = viewport_height - (anchor_row + 1)`, `above = anchor_row`.
* If `below >= needed` → kBelow; else if `above >= needed` → kAbove;
* else take the side with `max(above, below)` and clamp rows (narrow fallback;
  tie → kBelow).

## 5. Replacement
On accept, the effective range is the item's `range` if valid
(`begin>=0 && end<=text_len && begin<=end`), else the default range (token
start..cursor). `new_text` = line with `[begin,end)` replaced by
non-empty `insert_text` (falling back to `label`), `new_cursor` =
`begin + insert_text.size()`. The host applies `new_text`/`new_cursor` and the
`on_accept` callback also receives the raw item and range.

## 6. Tests
`tests/terminal_ui_kit/rendering/completion_popup_test.cc` covers: sync
provider; async provider; loading; no-results; provider error; fuzzy filtering;
keyboard navigation; acceptance; cancellation; replacement range; invalid
replacement range; stale response; duplicate labels; categories; popup above;
popup below; narrow viewport; resize; empty query; destruction with a pending
request.

## 7. Example
`examples/completion_popup_example.cpp` — a fullscreen FTXUI app with a text
input and two selectable providers (immediate local, deterministic delayed via
a frame-drained queue — no network/threads). Shows loading, fuzzy filtering,
categories, descriptions, acceptance, no-results, stale-result protection,
above/below placement, and a narrow-terminal layout, with a documented quit key.
</content>