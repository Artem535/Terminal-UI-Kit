#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace terminal_ui_kit {

// Broad category of a completion item. Purely informational; used for
// grouped rendering and by hosts that want to style kinds differently.
enum class CompletionKind {
  kUnknown,
  kKeyword,
  kVariable,
  kFunction,
  kType,
  kModule,
  kFile,
  kSnippet,
};

// A byte range (inclusive begin, exclusive end) into the edited line that a
// completion replaces on acceptance. `begin == 0 && end == 0` (the default) is
// treated as "item does not specify a range", in which case the popup falls
// back to the range of the token currently being completed.
struct CompletionRange {
  int begin = 0;
  int end = 0;
};

// A single selectable completion.
struct CompletionItem {
  std::string label;        // Display label; also the inserted text if
                            // insert_text is empty.
  std::string insert_text;  // Text inserted on acceptance.
  std::string description;  // Optional one-line detail shown dimmed.
  std::string category;     // Optional grouping tag shown dimmed.
  CompletionKind kind = CompletionKind::kUnknown;
  CompletionRange range;          // Optional explicit replacement range.
  std::vector<std::string> meta;  // Optional arbitrary metadata.
};

// Everything a provider needs to produce completions for the current input.
struct CompletionContext {
  std::string query;  // The token being completed (trimmed, may be empty).
  std::string text;   // The full edited line.
  int cursor = 0;     // Byte offset of the caret into `text`.
};

// Result of a fuzzy subsequence match.
struct FuzzyMatch {
  bool matched = false;
  int score = 0;
};

// Case-insensitive subsequence match with a small ranking heuristic (earlier
// starts and consecutive runs score higher). `matched` is false unless every
// character of `query` appears in `text` in order.
FuzzyMatch fuzzy_match(const std::string& query, const std::string& text);

// A provider result. `error` non-empty reports a failed provider run.
struct CompletionResult {
  std::vector<CompletionItem> items;
  std::string error;
};

// Provider contract. `complete` may invoke `on_result` synchronously (an
// immediate provider) or later on any thread (an asynchronous provider). A
// newer complete() call supersedes earlier in-flight requests; the popup drops
// results whose generation is no longer current, so a stale response can never
// replace results for a newer query. The API intentionally does not depend on
// Folly, Asio, or any particular executor -- providers choose their own
// scheduling.
class ICompletionProvider {
 public:
  virtual ~ICompletionProvider() = default;
  virtual void complete(const CompletionContext& context,
                        std::function<void(CompletionResult)> on_result) = 0;
};

using CompletionProviderFn = std::function<std::vector<CompletionItem>(const CompletionContext&)>;

// Adapter that wraps a function invoked synchronously. Exceptions thrown by
// the function are reported through CompletionResult::error.
class SyncCompletionProvider : public ICompletionProvider {
 public:
  explicit SyncCompletionProvider(CompletionProviderFn fn);

  void complete(const CompletionContext& context,
                std::function<void(CompletionResult)> on_result) override;

 private:
  CompletionProviderFn fn_;
};

// Adapter that wraps a function invoked asynchronously through a caller-supplied
// scheduler. The scheduler decides when the work runs (a thread pool, the
// event loop, or a deterministic test queue). Cancellation is cooperative:
// superseded requests still run but their results are discarded by the popup.
class AsyncCompletionProvider : public ICompletionProvider {
 public:
  using Scheduler = std::function<void(std::function<void()>)>;

  AsyncCompletionProvider(CompletionProviderFn fn, Scheduler schedule);

  void complete(const CompletionContext& context,
                std::function<void(CompletionResult)> on_result) override;

 private:
  CompletionProviderFn fn_;
  Scheduler schedule_;
};

enum class CompletionState {
  kHidden,     // Popup closed (empty query or after cancel/accept).
  kLoading,    // Request in flight.
  kResults,    // Matches available.
  kNoResults,  // Query completed but nothing matched.
  kError,      // Provider reported an error.
};

enum class PopupPlacement { kBelow, kAbove };

// Decides whether a popup of `needed_rows` fits below the caret row
// (`anchor_row`) inside a `viewport_height`-tall area. Prefers kBelow; falls
// back to kAbove when there is not enough room; on a narrow terminal takes the
// side with more space (ties resolve to kBelow).
PopupPlacement choose_placement(int needed_rows, int anchor_row, int viewport_height);

// Maximum number of rows the popup may actually use for a given anchor and
// viewport height, clamped by the space available on the chosen side.
int clamp_visible_rows(int needed_rows, int anchor_row, int viewport_height);

class CompletionPopupImpl;

// Headless controller and FTXUI view for the completion popup. Wrap a
// CompletionPopupModel for programmatic (non-rendered) use, or get the FTXUI
// component via component() and drive it with an input loop.
class CompletionPopupModel {
 public:
  explicit CompletionPopupModel(std::shared_ptr<ICompletionProvider> provider);
  ~CompletionPopupModel();

  CompletionPopupModel(const CompletionPopupModel&) = delete;
  CompletionPopupModel& operator=(const CompletionPopupModel&) = delete;

  ftxui::Component component() const;

  void set_provider(std::shared_ptr<ICompletionProvider> provider);

  // Update the edited line and caret. Recomputes the token being completed and
  // issues a new request (unless the query is empty, which hides the popup).
  void set_input(std::string text, int cursor);

  void set_max_visible_rows(int rows);
  void set_anchor_row(int row);
  void set_viewport_height(int height);
  void set_default_range(int begin, int end);
  void set_on_accept(std::function<void(const CompletionItem&, const std::string&, int)> on_accept);

  CompletionState state() const;
  std::vector<CompletionItem> items() const;
  std::string error_text() const;
  std::string query() const;
  std::size_t selected() const;

  bool select_index(std::size_t index);
  bool select_relative(int delta);

  // Applies the current selection: computes the replacement range (the item's
  // own range if valid, else the token range) and the resulting line, invokes
  // on_accept, and hides the popup. Returns false when nothing to accept.
  bool accept();
  void cancel();

  PopupPlacement placement() const;
  void recompute_placement();

 private:
  std::shared_ptr<CompletionPopupImpl> impl_;
};

// Convenience factory returning the FTXUI component directly.
ftxui::Component CompletionPopup(std::shared_ptr<ICompletionProvider> provider);

}  // namespace terminal_ui_kit