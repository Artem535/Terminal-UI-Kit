#include "terminal_ui_kit/components/completion_popup.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>

namespace terminal_ui_kit {
namespace {

// Positions its child `row_offset_` rows down within the box it is given
// (clipped to the parent). Used to draw the popup window just below or above
// the caret row inside a full-size overlay box.
class OffsetNode : public ftxui::Node {
 public:
  OffsetNode(ftxui::Element child, int row_offset)
      : Node({std::move(child)}), row_offset_(row_offset) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    Node::SetBox(box);
    ftxui::Box child_box = box;
    child_box.y_min += row_offset_;
    child_box.y_max += row_offset_;
    child_box.y_min = std::clamp(child_box.y_min, box.y_min, box.y_max);
    child_box.y_max = std::clamp(child_box.y_max, box.y_min, box.y_max);
    if (child_box.y_max < child_box.y_min) {
      child_box.y_max = child_box.y_min;
    }
    children_[0]->SetBox(child_box);
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  int row_offset_;
};

// State that is shared between the UI thread and a possibly-asynchronous
// provider callback. The callback never touches `this`; it only mutates this
// structure under `mutex`, which keeps a late result safe even after the popup
// owns no other reference to it.
struct SharedState {
  std::mutex mutex;
  bool dead = false;             // Set by the model destructor.
  std::uint64_t generation = 0;  // Monotonic request counter.
  CompletionState status = CompletionState::kHidden;
  std::vector<CompletionItem> items;
  std::string error;
  std::string query;
  std::size_t selected = 0;
};

// Owns `text` filtering/sorting and is invoked only while `SharedState::mutex`
// is held, so it reads the query snapshot captured at request time.
std::vector<CompletionItem> FilterAndSort(const std::string& query,
                                          std::vector<CompletionItem> items) {
  if (query.empty()) {
    return {};
  }
  struct Scored {
    int score;
    std::size_t order;
    CompletionItem item;
  };
  std::vector<Scored> scored;
  scored.reserve(items.size());
  for (std::size_t i = 0; i < items.size(); ++i) {
    const FuzzyMatch match = fuzzy_match(query, items[i].label);
    if (match.matched) {
      scored.push_back({-match.score, i, std::move(items[i])});
    }
  }
  std::stable_sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
    if (a.score != b.score) {
      return a.score < b.score;
    }
    return a.order < b.order;
  });
  std::vector<CompletionItem> result;
  result.reserve(scored.size());
  for (Scored& entry : scored) {
    result.push_back(std::move(entry.item));
  }
  return result;
}

// Scans `text` backward from `cursor` over word characters ([A-Za-z0-9_]) and
// returns the byte offset where the pending token begins.
int find_word_begin(const std::string& text, int cursor) {
  int begin = cursor;
  while (begin > 0) {
    const unsigned char c = static_cast<unsigned char>(text[static_cast<std::size_t>(begin - 1)]);
    if (!(std::isalnum(c) || c == '_')) {
      break;
    }
    --begin;
  }
  return begin;
}

// Callback invoked by a provider, possibly on another thread. `shared` is a
// shared_ptr captured by the request, and `generation` is the request id. It
// drops stale results and stops immediately once the popup has been destroyed.
void Deliver(const std::shared_ptr<SharedState>& shared, std::uint64_t generation,
             CompletionResult result) {
  std::lock_guard<std::mutex> lock(shared->mutex);
  if (shared->dead || shared->generation != generation) {
    return;
  }
  shared->selected = 0;
  if (!result.error.empty()) {
    shared->status = CompletionState::kError;
    shared->error = std::move(result.error);
    shared->items.clear();
    return;
  }
  shared->error.clear();
  shared->items = FilterAndSort(shared->query, std::move(result.items));
  shared->status = shared->items.empty() ? CompletionState::kNoResults : CompletionState::kResults;
}

}  // namespace

FuzzyMatch fuzzy_match(const std::string& query, const std::string& text) {
  FuzzyMatch result;
  if (query.empty()) {
    result.matched = true;
    return result;
  }
  if (text.empty()) {
    return result;
  }
  std::size_t q = 0;
  int score = 0;
  int previous = -2;
  int first_match = -1;
  for (std::size_t i = 0; i < text.size() && q < query.size(); ++i) {
    const unsigned char tc = static_cast<unsigned char>(text[i]);
    const unsigned char qc = static_cast<unsigned char>(query[q]);
    if (std::tolower(tc) != std::tolower(qc)) {
      continue;
    }
    const int position = static_cast<int>(i);
    if (position - previous == 1) {
      score += 10;  // Consecutive run bonus.
    } else {
      score += 1;
    }
    if (position == 0 || text[i - 1] == '_' || text[i - 1] == ' ' || text[i - 1] == '-' ||
        text[i - 1] == '.') {
      score += 5;  // Word-boundary bonus.
    }
    if (first_match < 0) {
      first_match = position;
      score += (position == 0) ? 8 : 0;
    }
    score -= position / 4;  // Slight preference for earlier matches.
    previous = position;
    ++q;
  }
  if (q != query.size()) {
    return result;
  }
  result.matched = true;
  result.score = score;
  return result;
}

PopupPlacement choose_placement(int needed_rows, int anchor_row, int viewport_height) {
  const int below = viewport_height - (anchor_row + 1);
  const int above = anchor_row;
  if (below >= needed_rows) {
    return PopupPlacement::kBelow;
  }
  if (above >= needed_rows) {
    return PopupPlacement::kAbove;
  }
  return (above > below) ? PopupPlacement::kAbove : PopupPlacement::kBelow;
}

int clamp_visible_rows(int needed_rows, int anchor_row, int viewport_height) {
  if (viewport_height <= 0 || needed_rows <= 0) {
    return 0;
  }
  anchor_row = std::clamp(anchor_row, 0, viewport_height - 1);
  const int below = viewport_height - (anchor_row + 1);
  const int above = anchor_row;
  const int available = std::max({0, above, below});
  return std::min(needed_rows, available);
}

SyncCompletionProvider::SyncCompletionProvider(CompletionProviderFn fn) : fn_(std::move(fn)) {}

void SyncCompletionProvider::complete(const CompletionContext& context,
                                      std::function<void(CompletionResult)> on_result) {
  CompletionResult result;
  try {
    result.items = fn_(context);
  } catch (const std::exception& e) {
    result.error = e.what();
  } catch (...) {
    result.error = "unknown provider error";
  }
  on_result(std::move(result));
}

AsyncCompletionProvider::AsyncCompletionProvider(CompletionProviderFn fn, Scheduler schedule)
    : fn_(std::move(fn)), schedule_(std::move(schedule)) {}

void AsyncCompletionProvider::complete(const CompletionContext& context,
                                       std::function<void(CompletionResult)> on_result) {
  if (!schedule_) {
    CompletionResult result;
    on_result(std::move(result));
    return;
  }
  CompletionProviderFn fn = fn_;
  schedule_([fn = std::move(fn), context, on_result = std::move(on_result)]() {
    CompletionResult result;
    try {
      result.items = fn(context);
    } catch (const std::exception& e) {
      result.error = e.what();
    } catch (...) {
      result.error = "unknown provider error";
    }
    on_result(std::move(result));
  });
}

class CompletionPopupImpl : public ftxui::ComponentBase {
 public:
  explicit CompletionPopupImpl(std::shared_ptr<ICompletionProvider> provider)
      : provider_(std::move(provider)), shared_(std::make_shared<SharedState>()) {}

  ~CompletionPopupImpl() override {
    // Any in-flight or queued callback observes `dead` and stops before
    // touching any member of this (already-destroyed) object.
    std::lock_guard<std::mutex> lock(shared_->mutex);
    shared_->dead = true;
  }

  void set_provider(std::shared_ptr<ICompletionProvider> provider) {
    provider_ = std::move(provider);
    if (!query().empty()) {
      request();
    } else {
      hide();
    }
  }

  void set_input(std::string text, int cursor) {
    text_ = std::move(text);
    cursor_ = std::clamp(cursor, 0, static_cast<int>(text_.size()));
    word_begin_ = find_word_begin(text_, cursor_);
    if (word_begin_ > cursor_) {
      word_begin_ = cursor_;
    }
    query_ = text_.substr(static_cast<std::size_t>(word_begin_),
                          static_cast<std::size_t>(cursor_ - word_begin_));
    update_query();
    if (query_.empty()) {
      hide();
      return;
    }
    request();
  }

  void set_max_visible_rows(int rows) {
    max_visible_rows_ = std::max(1, rows);
    recompute_placement();
  }

  void set_anchor_row(int row) {
    anchor_row_ = row;
    recompute_placement();
  }

  void set_viewport_height(int height) {
    viewport_height_ = std::max(0, height);
    recompute_placement();
  }

  void set_default_range(int begin, int end) {
    default_range_begin_ = begin;
    default_range_end_ = end;
  }

  void set_on_accept(
      std::function<void(const CompletionItem&, const std::string&, int)> on_accept) {
    on_accept_ = std::move(on_accept);
  }

  CompletionState state() const {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    return shared_->status;
  }

  std::vector<CompletionItem> items() const {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    return shared_->items;
  }

  std::string error_text() const {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    return shared_->error;
  }

  std::string query() const {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    return shared_->query;
  }

  std::size_t selected() const {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    return shared_->selected;
  }

  bool select_index(std::size_t index) {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    if (shared_->status != CompletionState::kResults || shared_->items.empty()) {
      return false;
    }
    shared_->selected = std::min(index, shared_->items.size() - 1);
    return true;
  }

  bool select_relative(int delta) {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    if (shared_->status != CompletionState::kResults || shared_->items.empty()) {
      return false;
    }
    if (delta > 0) {
      shared_->selected =
          std::min(shared_->selected + static_cast<std::size_t>(delta), shared_->items.size() - 1);
    } else if (delta < 0) {
      const std::size_t step = static_cast<std::size_t>(-delta);
      shared_->selected = (step > shared_->selected) ? 0 : shared_->selected - step;
    }
    return true;
  }

  bool accept() {
    const std::string line = text_;
    const int cursor = cursor_;
    CompletionItem item;
    bool has_item = false;
    {
      std::lock_guard<std::mutex> lock(shared_->mutex);
      if (shared_->status == CompletionState::kResults && !shared_->items.empty()) {
        item = shared_->items[std::min(shared_->selected, shared_->items.size() - 1)];
        has_item = true;
      }
    }
    if (!has_item) {
      return false;  // Nothing selectable; do not alter state or notify.
    }
    hide();

    // Clamp against the actual line length so an item with a range that
    // extends past the end cannot read out of bounds.
    const int text_len = static_cast<int>(line.size());
    const bool item_range_valid =
        item.range.end > item.range.begin && item.range.begin >= 0 && item.range.end <= text_len;
    const bool default_range_valid = default_range_begin_ >= 0 &&
                                     default_range_end_ > default_range_begin_ &&
                                     default_range_end_ <= text_len;
    int begin = item_range_valid ? item.range.begin
                                 : (default_range_valid ? default_range_begin_ : word_begin_);
    int end =
        item_range_valid ? item.range.end : (default_range_valid ? default_range_end_ : cursor);
    begin = std::clamp(begin, 0, text_len);
    end = std::clamp(std::max(begin, end), begin, text_len);
    if (begin > end) {
      begin = cursor;
      end = cursor;
    }

    const std::string insert = item.insert_text.empty() ? item.label : item.insert_text;
    std::string new_line = line.substr(0, static_cast<std::size_t>(begin)) + insert +
                           line.substr(static_cast<std::size_t>(end));
    const int new_cursor = begin + static_cast<int>(insert.size());
    if (on_accept_) {
      on_accept_(item, new_line, new_cursor);
    }
    return true;
  }

  void cancel() {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    ++shared_->generation;  // Invalidate any in-flight request.
    shared_->status = CompletionState::kHidden;
    shared_->items.clear();
    shared_->error.clear();
    shared_->selected = 0;
  }

  PopupPlacement placement() const { return placement_; }

  void recompute_placement() {
    placement_ = choose_placement(max_visible_rows_, anchor_row_, viewport_height_);
    popup_rows_ = clamp_visible_rows(max_visible_rows_, anchor_row_, viewport_height_);
  }

  ftxui::Element Render() override {
    recompute_placement();
    const CompletionState status = state();
    if (status == CompletionState::kHidden) {
      return ftxui::text("");
    }
    const int rows = std::max(1, popup_rows_);

    ftxui::Elements contents;
    contents.push_back(ftxui::text("Completion") | ftxui::bold | ftxui::dim);
    std::vector<CompletionItem> current_items;
    std::string current_error;
    std::size_t current_selected = 0;
    {
      std::lock_guard<std::mutex> lock(shared_->mutex);
      current_items = shared_->items;
      current_error = shared_->error;
      current_selected = shared_->selected;
    }
    switch (status) {
      case CompletionState::kLoading:
        contents.push_back(ftxui::text("Loading\u2026") | ftxui::dim);
        break;
      case CompletionState::kError:
        contents.push_back(ftxui::text("Error: " + current_error) | ftxui::dim);
        break;
      case CompletionState::kNoResults:
        contents.push_back(ftxui::text("No matches") | ftxui::dim);
        break;
      case CompletionState::kResults:
        for (std::size_t i = 0; i < current_items.size(); ++i) {
          const CompletionItem& item = current_items[i];
          ftxui::Elements row = {ftxui::text(item.label.empty() ? "?" : item.label)};
          if (!item.category.empty()) {
            row.push_back(ftxui::text("  [" + item.category + "]") | ftxui::dim);
          }
          if (!item.description.empty()) {
            row.push_back(ftxui::text("  " + item.description) | ftxui::dim);
          }
          ftxui::Element line = ftxui::hbox(std::move(row));
          if (i == current_selected) {
            line = line | ftxui::inverted;
          }
          contents.push_back(std::move(line));
        }
        break;
      case CompletionState::kHidden:
        break;
    }
    ftxui::Element window = ftxui::vbox(std::move(contents)) | ftxui::yframe | ftxui::border |
                            ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, rows);
    if (viewport_height_ <= 0) {
      // Viewport not configured yet; render the window in place.
      return window;
    }
    int offset = (placement_ == PopupPlacement::kAbove) ? anchor_row_ - rows + 1 : anchor_row_ + 1;
    return ftxui::Element(std::make_shared<OffsetNode>(std::move(window), offset));
  }

  bool OnEvent(ftxui::Event event) override {
    if (event == ftxui::Event::ArrowDown) {
      const bool was_results = state() == CompletionState::kResults;
      select_relative(1);
      return was_results;
    }
    if (event == ftxui::Event::ArrowUp) {
      const bool was_results = state() == CompletionState::kResults;
      select_relative(-1);
      return was_results;
    }
    if (event == ftxui::Event::Tab) {
      const bool consumed = state() == CompletionState::kResults;
      if (consumed) {
        return accept();
      }
      return false;
    }
    if (event == ftxui::Event::Return) {
      const bool consumed = state() == CompletionState::kResults;
      if (consumed) {
        return accept();
      }
      return false;
    }
    if (event == ftxui::Event::Escape) {
      if (state() == CompletionState::kHidden) {
        return false;
      }
      cancel();
      return true;
    }
    return false;
  }

  bool Focusable() const override { return true; }

 private:
  void update_query() {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    shared_->query = query_;
  }

  void hide() {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    ++shared_->generation;  // Invalidate any in-flight request.
    shared_->status = CompletionState::kHidden;
    shared_->items.clear();
    shared_->error.clear();
    shared_->selected = 0;
  }

  void request() {
    const CompletionContext context{query_, text_, cursor_};
    const std::uint64_t generation = [this] {
      std::lock_guard<std::mutex> lock(shared_->mutex);
      shared_->query = query_;
      shared_->status = CompletionState::kLoading;
      shared_->items.clear();
      shared_->error.clear();
      return ++shared_->generation;
    }();
    if (!provider_) {
      CompletionResult result;
      Deliver(shared_, generation, std::move(result));
      return;
    }
    provider_->complete(context, [shared = shared_, generation](CompletionResult result) {
      Deliver(shared, generation, std::move(result));
    });
  }

  std::shared_ptr<ICompletionProvider> provider_;
  std::shared_ptr<SharedState> shared_;

  // UI-thread-only members (never read by an async callback).
  std::string text_;
  int cursor_ = 0;
  int word_begin_ = 0;
  std::string query_;
  int max_visible_rows_ = 8;
  int anchor_row_ = 0;
  int viewport_height_ = 0;
  int default_range_begin_ = -1;
  int default_range_end_ = -1;
  PopupPlacement placement_ = PopupPlacement::kBelow;
  int popup_rows_ = 8;
  std::function<void(const CompletionItem&, const std::string&, int)> on_accept_;
};

CompletionPopupModel::CompletionPopupModel(std::shared_ptr<ICompletionProvider> provider)
    : impl_(ftxui::Make<CompletionPopupImpl>(std::move(provider))) {}

CompletionPopupModel::~CompletionPopupModel() = default;

ftxui::Component CompletionPopupModel::component() const { return impl_; }

void CompletionPopupModel::set_provider(std::shared_ptr<ICompletionProvider> provider) {
  impl_->set_provider(std::move(provider));
}

void CompletionPopupModel::set_input(std::string text, int cursor) {
  impl_->set_input(std::move(text), cursor);
}

void CompletionPopupModel::set_max_visible_rows(int rows) { impl_->set_max_visible_rows(rows); }

void CompletionPopupModel::set_anchor_row(int row) { impl_->set_anchor_row(row); }

void CompletionPopupModel::set_viewport_height(int height) { impl_->set_viewport_height(height); }

void CompletionPopupModel::set_default_range(int begin, int end) {
  impl_->set_default_range(begin, end);
}

void CompletionPopupModel::set_on_accept(
    std::function<void(const CompletionItem&, const std::string&, int)> on_accept) {
  impl_->set_on_accept(std::move(on_accept));
}

CompletionState CompletionPopupModel::state() const { return impl_->state(); }

std::vector<CompletionItem> CompletionPopupModel::items() const { return impl_->items(); }

std::string CompletionPopupModel::error_text() const { return impl_->error_text(); }

std::string CompletionPopupModel::query() const { return impl_->query(); }

std::size_t CompletionPopupModel::selected() const { return impl_->selected(); }

bool CompletionPopupModel::select_index(std::size_t index) { return impl_->select_index(index); }

bool CompletionPopupModel::select_relative(int delta) { return impl_->select_relative(delta); }

bool CompletionPopupModel::accept() { return impl_->accept(); }

void CompletionPopupModel::cancel() { impl_->cancel(); }

PopupPlacement CompletionPopupModel::placement() const { return impl_->placement(); }

void CompletionPopupModel::recompute_placement() { impl_->recompute_placement(); }

ftxui::Component CompletionPopup(std::shared_ptr<ICompletionProvider> provider) {
  return ftxui::Make<CompletionPopupImpl>(std::move(provider));
}

}  // namespace terminal_ui_kit