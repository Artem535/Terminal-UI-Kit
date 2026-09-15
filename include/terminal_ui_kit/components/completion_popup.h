#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Semantic kind of a completion entry. Used only to pick an icon/color; it has
// no effect on filtering or acceptance. Mirrors PRD section 23's `CompletionKind`.
enum class CompletionKind {
  kUnknown,
  kKeyword,
  kFunction,
  kVariable,
  kType,
  kModule,
  kConstant,
};

// One candidate offered by a completion provider (PRD section 23).
struct CompletionItem {
  std::string label;  // what is displayed in the list
  // Text inserted on acceptance. Falls back to `label` when empty.
  std::string insert_text;
  std::string category;     // optional grouping label, rendered dimmed
  std::string description;  // optional one-line description
  // Optional half-open [begin, end) character range (into the host buffer) that
  // this item replaces. Absent => the completion replaces the typed query.
  std::optional<std::pair<std::size_t, std::size_t>> replacement_range;
  std::shared_ptr<void> metadata;  // optional app-owned payload (safe, owning)
  CompletionKind kind = CompletionKind::kUnknown;

  // The text that gets inserted when this item is accepted.
  std::string EffectiveInsertText() const { return insert_text.empty() ? label : insert_text; }
};

// Input context handed to a provider for a completion request.
struct CompletionContext {
  std::string query;              // text being completed against
  std::size_t cursor_offset = 0;  // cursor position in the host buffer
};

// Result of a completion request, delivered at most once per request.
struct CompletionResult {
  std::vector<CompletionItem> items;
  std::optional<std::string> error;  // non-null => the provider failed

  bool HasError() const { return error.has_value(); }
};

// Base interface for completion providers.
//
// `complete` must invoke `deliver` exactly once with the result tagged with the
// `generation` it was given. A synchronous provider calls it before returning;
// an asynchronous provider may defer it (marshaled to the UI thread by the
// host). The component discards deliveries whose generation no longer matches
// the latest query, so a stale response can never clobber a newer query's
// results. Providers that want to cancel in-flight work can compare the
// generation they received against a shared atomic.
//
// The interface is deliberately not tied to Folly, Asio, or any concrete
// executor: the callback is the only seam (PRD section 23).
//
// THREAD-SAFETY: the component's state is not internally synchronized. An
// asynchronous provider MUST marshal its `deliver` invocation back to the UI /
// main thread (e.g. via a posted event on the host's event loop) before calling
// it. Invoking `deliver` from a worker thread is a data race on the component.
class ICompletionProvider {
 public:
  virtual ~ICompletionProvider() = default;

  virtual void complete(
      const CompletionContext& context, std::uint64_t generation,
      const std::function<void(std::uint64_t generation, CompletionResult result)>& deliver) = 0;
};

// Adapter that turns a plain synchronous function into an ICompletionProvider.
// Exceptions thrown by the function are converted into an error result so a
// failing provider is surfaced through the popup's error state instead of
// propagating out of the component.
class SyncCompletionProvider : public ICompletionProvider {
 public:
  using Fn = std::function<std::vector<CompletionItem>(const CompletionContext&)>;
  explicit SyncCompletionProvider(Fn fn);

  void complete(const CompletionContext& context, std::uint64_t generation,
                const std::function<void(std::uint64_t, CompletionResult)>& deliver) override;

 private:
  Fn fn_;
};

// Options for a CompletionPopup.
struct CompletionPopupOptions {
  std::shared_ptr<ICompletionProvider> provider;
  int max_visible_rows = 8;
  // When true, the popup runs fuzzy filtering over whatever the provider
  // returns before displaying it.
  bool fuzzy_filter = true;
  // Minimum query length before a request is issued (0 = always). Empty and
  // short queries keep the popup hidden.
  std::size_t min_query_length = 0;
  // Invoked with the accepted item (after replacement-range handling). The
  // host is responsible for applying the insertion to its buffer.
  std::function<void(const CompletionItem&)> on_accept;
  const Theme* theme = nullptr;  // null => default_dark_theme()
};

class CompletionPopupImpl;

// Manages the state and view of a completion popup. `CompletionPopup` decorates
// a host input component: it renders the host plus, when visible, the
// completion list positioned above or below the host, and routes navigation /
// accept / cancel keys while the popup is open.
//
// Typical use:
//   CompletionPopup popup(input, options);
//   ftxui::Component root = Container::Vertical({ popup.component(), status });
//   ...on input change: popup.set_query(text, cursor);
//
// The component never retains non-owning views of provider data: items are
// copied into owning containers, and metadata is held via shared_ptr.
class CompletionPopup {
 public:
  // Placement side relative to the host input.
  enum class Placement { kBelow, kAbove };

  explicit CompletionPopup(ftxui::Component host, CompletionPopupOptions options);
  ~CompletionPopup();

  CompletionPopup(const CompletionPopup&) = delete;
  CompletionPopup& operator=(const CompletionPopup&) = delete;
  CompletionPopup(CompletionPopup&&) noexcept = default;
  CompletionPopup& operator=(CompletionPopup&&) noexcept = default;

  // The interactive component for this popup (a focusable decorator over host).
  ftxui::Component component() const;

  // Issue a new completion request for `query`. Any in-flight older request is
  // invalidated. Short (below min_query_length) or empty queries hide the popup.
  void set_query(std::string query, std::size_t cursor_offset);

  // Programmatic open/close. `show()` re-runs the current query (recovering
  // from a loading / no-results / error state when it is not already showing
  // results).
  void show();
  void hide();
  void toggle();
  bool visible() const;

  // The current lifecycle state (exposed for status overlays and tests).
  enum class State {
    kHidden,
    kLoading,
    kResults,
    kNoResults,
    kError,
  };
  State state() const;

  // Keyboard-style navigation over the current results. Both `move_selection`
  // and `select_index` clamp into [0, items().size()). `move_selection` is
  // bounded; callers must not pass INT_MIN (it is not reachable from key
  // events, which use small step sizes).
  void move_selection(int delta);
  void select_index(std::size_t index);
  std::size_t selected_index() const;

  // Accept the selected item (invokes on_accept and hides the popup). Returns
  // false when there are no results to accept.
  bool accept_selected();

  // The currently displayed (post-filter) items.
  const std::vector<CompletionItem>& items() const;

  // Viewport-aware placement: inform the popup how many rows are available
  // below and above the host anchor. Passing a negative value for either
  // resets to auto (defaults to kBelow). Placement is recomputed every frame,
  // so it stays correct across resizes.
  void set_available_space(int available_below, int available_above);
  Placement placement() const;

  // Pure, viewport-aware decision: prefer below, flip above when it fits above
  // but not below, otherwise the side with more room.
  static Placement decide_placement(int available_below, int available_above, int needed_rows);

  // Case-insensitive subsequence fuzzy match of `query` inside `text`.
  static bool FuzzyMatch(const std::string& query, const std::string& text);

  // Applies `item`'s replacement: replaces [begin,end) of `buffer` with the
  // insertion text, or (no range) replaces the `query` region ending at
  // `cursor`. An out-of-range / reversed range leaves `buffer` unchanged.
  static std::string ApplyReplacement(const std::string& buffer, std::size_t cursor,
                                      const std::string& query, const CompletionItem& item);

 private:
  std::shared_ptr<CompletionPopupImpl> impl_;
};

}  // namespace terminal_ui_kit
