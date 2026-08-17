#include "terminal_ui_kit/components/completion_popup.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {

namespace {

constexpr std::size_t kCategoryBudget = 24;
constexpr std::size_t kDescriptionBudget = 40;

std::string Truncate(const std::string& text, std::size_t budget) {
  if (text.size() <= budget) {
    return text;
  }
  if (budget <= 1) {
    return "…";
  }
  std::string out = text.substr(0, budget - 1);
  out += "…";
  return out;
}

// Observer node that records the box FTXUI allocates to the composed output and
// requests a follow-up layout frame when it changes, so placement and the
// visible row count stay correct across resizes (same pattern as VirtualList).
class PopupBoxObserver : public ftxui::Node {
 public:
  PopupBoxObserver(ftxui::Element child, ftxui::Box& observed_box, bool& observed)
      : Node({std::move(child)}), observed_box_(observed_box), observed_(observed) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    const bool changed = observed_box_ != box;
    observed_box_ = box;
    observed_ = true;
    Node::SetBox(box);
    children_[0]->SetBox(box);
    if (changed) {
      ftxui::animation::RequestAnimationFrame();
    }
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  ftxui::Box& observed_box_;
  bool& observed_;
};

}  // namespace

SyncCompletionProvider::SyncCompletionProvider(Fn fn) : fn_(std::move(fn)) {}

void SyncCompletionProvider::complete(
    const CompletionContext& context, std::uint64_t generation,
    const std::function<void(std::uint64_t, CompletionResult)>& deliver) {
  CompletionResult result;
  try {
    result.items = fn_(context);
  } catch (const std::exception& e) {
    result.error = e.what();
  } catch (...) {
    result.error = "unknown provider error";
  }
  deliver(generation, std::move(result));
}

using State = CompletionPopup::State;

// NOTE: CompletionPopupImpl is intentionally defined at `terminal_ui_kit`
// scope (not in an anonymous namespace) so it matches the `class
// CompletionPopupImpl;` forward declaration in the header; GCC rejects
// `std::make_shared<T>` when T is declared at namespace scope but defined in an
// anonymous namespace.
class CompletionPopupImpl : public ftxui::ComponentBase {
 public:
  explicit CompletionPopupImpl(ftxui::Component host, CompletionPopupOptions options)
      : host_(std::move(host)), options_(std::move(options)) {
    theme_ = options_.theme ? *options_.theme : default_dark_theme();
    if (options_.max_visible_rows <= 0) {
      options_.max_visible_rows = 8;
    }
  }

  // Set by the owning CompletionPopup after construction; used to drop late
  // async deliveries once the component has been destroyed.
  std::weak_ptr<CompletionPopupImpl> weak_self_;

  void SetQuery(std::string query, std::size_t cursor_offset) {
    query_ = std::move(query);
    cursor_offset_ = cursor_offset;
    ++generation_;
    if (query_.empty() || query_.size() < options_.min_query_length) {
      state_ = State::kHidden;
      items_.clear();
      selected_ = 0;
      error_message_.clear();
      return;
    }
    state_ = State::kLoading;
    items_.clear();
    selected_ = 0;
    error_message_.clear();
    if (!options_.provider) {
      state_ = State::kError;
      error_message_ = "no completion provider configured";
      return;
    }
    const CompletionContext context{query_, cursor_offset_};
    const std::uint64_t generation = generation_;
    const std::weak_ptr<CompletionPopupImpl> weak = weak_self_;
    options_.provider->complete(
        context, generation,
        [weak, generation](std::uint64_t delivered_at, CompletionResult result) {
          if (delivered_at != generation) {
            return;  // this exact request was superseded or cancelled
          }
          std::shared_ptr<CompletionPopupImpl> self = weak.lock();
          if (!self) {
            return;  // component destroyed; a late callback must not touch it
          }
          self->OnDelivery(generation, std::move(result));
        });
  }

  void OnDelivery(std::uint64_t generation, CompletionResult result) {
    if (generation != generation_) {
      return;  // a newer query superseded this response (stale result)
    }
    if (result.HasError()) {
      state_ = State::kError;
      error_message_ = *result.error;
      items_.clear();
      selected_ = 0;
      return;
    }
    if (options_.fuzzy_filter) {
      items_ = FilterFuzzy(result.items, query_);
    } else {
      items_ = std::move(result.items);
    }
    selected_ = 0;
    state_ = items_.empty() ? State::kNoResults : State::kResults;
  }

  void Show() {
    // Re-run the current query to recover from a transient loading / no-results
    // / error state. Do not disturb an active results view.
    if (state_ != State::kResults) {
      SetQuery(query_, cursor_offset_);
    }
  }

  void Hide() {
    ++generation_;  // invalidate any in-flight request
    state_ = State::kHidden;
    items_.clear();
    selected_ = 0;
    error_message_.clear();
  }

  void MoveSelection(int delta) {
    if (state_ != State::kResults || items_.empty()) {
      return;
    }
    const std::size_t count = items_.size();
    std::size_t target = selected_;
    if (delta > 0) {
      const std::size_t step = static_cast<std::size_t>(delta);
      target = (target + step < count) ? target + step : count - 1;
    } else if (delta < 0) {
      // Avoid `-delta` signed overflow for the platform-minimum int. This path
      // is not reachable from key events (which use small steps), but
      // move_selection is a public API.
      const std::size_t step = static_cast<std::size_t>(-static_cast<std::int64_t>(delta));
      target = (target > step) ? target - step : 0;
    }
    selected_ = target;
  }

  void SelectIndex(std::size_t index) {
    if (state_ != State::kResults || items_.empty()) {
      return;
    }
    selected_ = std::min(index, items_.size() - 1);
  }

  bool AcceptSelected() {
    if (state_ != State::kResults || items_.empty()) {
      return false;
    }
    const CompletionItem item = items_[selected_];
    state_ = State::kHidden;
    if (options_.on_accept) {
      options_.on_accept(item);
    }
    return true;
  }

  bool Visible() const { return state_ != State::kHidden; }

  CompletionPopup::State State() const { return state_; }

  const std::vector<CompletionItem>& Items() const { return items_; }

  std::size_t SelectedIndex() const { return selected_; }

  void SetAvailableSpace(int below, int above) {
    available_below_ = below;
    available_above_ = above;
  }

  CompletionPopup::Placement Placement() const { return ResolvePlacement(); }

  bool Focusable() const override { return Visible() || (host_ && host_->Focusable()); }

  bool OnEvent(ftxui::Event event) override {
    if (Visible()) {
      // Navigation is only meaningful once results are ready; otherwise the
      // keys fall through to the host (consistent with Enter/Tab).
      const bool has_results = state_ == CompletionPopup::State::kResults && !items_.empty();
      if (has_results && event == ftxui::Event::ArrowUp) {
        MoveSelection(-1);
        return true;
      }
      if (has_results && event == ftxui::Event::ArrowDown) {
        MoveSelection(1);
        return true;
      }
      if (has_results && event == ftxui::Event::PageUp) {
        MoveSelection(-DesiredRows());
        return true;
      }
      if (has_results && event == ftxui::Event::PageDown) {
        MoveSelection(DesiredRows());
        return true;
      }
      if (has_results && event == ftxui::Event::Home) {
        SelectIndex(0);
        return true;
      }
      if (has_results && event == ftxui::Event::End) {
        SelectIndex(items_.size() - 1);
        return true;
      }
      if (event == ftxui::Event::Return || event == ftxui::Event::Tab) {
        if (AcceptSelected()) {
          return true;
        }
      }
      if (event == ftxui::Event::Escape) {
        Hide();
        return true;
      }
    }
    if (host_) {
      return host_->OnEvent(std::move(event));
    }
    return false;
  }

  ftxui::Element Render() override {
    ftxui::Element host_element = host_ ? host_->Render() : ftxui::emptyElement();
    if (state_ == CompletionPopup::State::kHidden) {
      return host_element;
    }
    ftxui::Element popup_element = BuildPopup();
    const CompletionPopup::Placement placement = ResolvePlacement();
    ftxui::Element composed;
    if (placement == CompletionPopup::Placement::kAbove) {
      composed = ftxui::vbox({std::move(popup_element), std::move(host_element)});
    } else {
      composed = ftxui::vbox({std::move(host_element), std::move(popup_element)});
    }
    return std::make_shared<PopupBoxObserver>(std::move(composed), box_, observed_);
  }

 private:
  CompletionPopup::Placement ResolvePlacement() const {
    if (available_below_ >= 0 && available_above_ >= 0) {
      return CompletionPopup::decide_placement(available_below_, available_above_, DesiredRows());
    }
    return CompletionPopup::Placement::kBelow;
  }

  int DesiredRows() const {
    if (state_ != CompletionPopup::State::kResults) {
      return 1;  // loading / no-results / error status rows are single line
    }
    const std::size_t wanted = std::min<std::size_t>(
        items_.size(), static_cast<std::size_t>(std::max(1, options_.max_visible_rows)));
    return std::max(1, static_cast<int>(wanted));
  }

  int AvailableRowsForPopup() const {
    if (!observed_) {
      return -1;  // box not measured yet: do not clip on the first frame
    }
    const int height = std::max(1, box_.y_max - box_.y_min + 1);
    return std::max(1, height - 1);  // reserve one row for the host input
  }

  int ShownRows() const {
    const int desired = DesiredRows();
    const int available = AvailableRowsForPopup();
    if (available > 0 && available < desired) {
      return std::max(1, available);  // narrow-terminal / short-viewport fallback
    }
    return desired;
  }

  ftxui::Element BuildPopup() const {
    ftxui::Elements rows;
    switch (state_) {
      case CompletionPopup::State::kLoading:
        rows.push_back(ftxui::text("Loading…") | to_decorator(theme_.muted));
        break;
      case CompletionPopup::State::kNoResults:
        rows.push_back(ftxui::text("No matches for \"" + query_ + "\"") |
                       to_decorator(theme_.muted));
        break;
      case CompletionPopup::State::kError:
        rows.push_back(ftxui::text("Error: " + error_message_) | to_decorator(theme_.error));
        break;
      case CompletionPopup::State::kResults: {
        const int shown = ShownRows();
        const std::size_t limit =
            std::min<std::size_t>(items_.size(), static_cast<std::size_t>(shown));
        for (std::size_t index = 0; index < limit; ++index) {
          rows.push_back(BuildItemRow(items_[index], index == selected_));
        }
        if (items_.size() > limit) {
          rows.push_back(ftxui::text("…") | ftxui::dim);
        }
      } break;
      case CompletionPopup::State::kHidden:
        break;
    }
    return ftxui::vbox(std::move(rows));
  }

  ftxui::Element BuildItemRow(const CompletionItem& item, bool selected) const {
    ftxui::Element line = ftxui::text(item.label) | KindDecorator(item.kind);
    if (!item.category.empty()) {
      ftxui::Element category =
          ftxui::text(" (" + Truncate(item.category, kCategoryBudget) + ")") | ftxui::dim;
      line = ftxui::hbox({std::move(line), std::move(category)});
    }
    if (!item.description.empty()) {
      ftxui::Element description =
          ftxui::text("  ·  " + Truncate(item.description, kDescriptionBudget)) | ftxui::dim;
      line = ftxui::hbox({std::move(line), std::move(description)});
    }
    if (selected) {
      return line | ftxui::inverted;
    }
    return line;
  }

  ftxui::Decorator KindDecorator(CompletionKind kind) const {
    switch (kind) {
      case CompletionKind::kKeyword:
        return to_decorator(theme_.accent);
      case CompletionKind::kVariable:
        return to_decorator(theme_.secondary);
      case CompletionKind::kType:
        return to_decorator(theme_.success);
      case CompletionKind::kModule:
        return to_decorator(theme_.muted);
      case CompletionKind::kConstant:
        return to_decorator(theme_.warning);
      case CompletionKind::kFunction:
      case CompletionKind::kUnknown:
        return to_decorator(theme_.primary);
    }
    // All enumerators are covered above; the trailing return keeps GCC's
    // -Wreturn-type satisfied across toolchains.
    return to_decorator(theme_.primary);
  }

  std::vector<CompletionItem> FilterFuzzy(const std::vector<CompletionItem>& source,
                                          const std::string& query) const {
    std::vector<CompletionItem> out;
    out.reserve(source.size());
    for (const CompletionItem& item : source) {
      if (CompletionPopup::FuzzyMatch(query, item.label)) {
        out.push_back(item);
      }
    }
    return out;
  }

  ftxui::Component host_;
  CompletionPopupOptions options_;
  Theme theme_ = default_dark_theme();
  std::string query_;
  std::size_t cursor_offset_ = 0;
  std::uint64_t generation_ = 0;
  CompletionPopup::State state_ = CompletionPopup::State::kHidden;
  std::vector<CompletionItem> items_;
  std::size_t selected_ = 0;
  std::string error_message_;
  int available_below_ = -1;
  int available_above_ = -1;
  ftxui::Box box_;
  bool observed_ = false;
};

CompletionPopup::CompletionPopup(ftxui::Component host, CompletionPopupOptions options)
    : impl_(std::make_shared<CompletionPopupImpl>(std::move(host), std::move(options))) {
  impl_->weak_self_ = impl_;
}

CompletionPopup::~CompletionPopup() = default;

ftxui::Component CompletionPopup::component() const { return impl_; }

void CompletionPopup::set_query(std::string query, std::size_t cursor_offset) {
  impl_->SetQuery(std::move(query), cursor_offset);
}

void CompletionPopup::show() { impl_->Show(); }

void CompletionPopup::hide() { impl_->Hide(); }

void CompletionPopup::toggle() {
  if (visible()) {
    hide();
  } else {
    show();
  }
}

bool CompletionPopup::visible() const { return impl_->Visible(); }

CompletionPopup::State CompletionPopup::state() const { return impl_->State(); }

void CompletionPopup::move_selection(int delta) { impl_->MoveSelection(delta); }

void CompletionPopup::select_index(std::size_t index) { impl_->SelectIndex(index); }

std::size_t CompletionPopup::selected_index() const { return impl_->SelectedIndex(); }

bool CompletionPopup::accept_selected() { return impl_->AcceptSelected(); }

const std::vector<CompletionItem>& CompletionPopup::items() const { return impl_->Items(); }

void CompletionPopup::set_available_space(int available_below, int available_above) {
  impl_->SetAvailableSpace(available_below, available_above);
}

CompletionPopup::Placement CompletionPopup::placement() const { return impl_->Placement(); }

CompletionPopup::Placement CompletionPopup::decide_placement(int available_below,
                                                             int available_above, int needed_rows) {
  const int needed = std::max(1, needed_rows);
  if (available_below >= needed) {
    return Placement::kBelow;
  }
  if (available_above >= needed) {
    return Placement::kAbove;
  }
  return available_above > available_below ? Placement::kAbove : Placement::kBelow;
}

bool CompletionPopup::FuzzyMatch(const std::string& query, const std::string& text) {
  if (query.empty()) {
    return true;
  }
  std::size_t query_index = 0;
  for (std::size_t text_index = 0; text_index < text.size() && query_index < query.size();
       ++text_index) {
    const char text_char = text[text_index];
    const char query_char = query[query_index];
    if (std::tolower(static_cast<unsigned char>(text_char)) ==
        std::tolower(static_cast<unsigned char>(query_char))) {
      ++query_index;
    }
  }
  return query_index == query.size();
}

std::string CompletionPopup::ApplyReplacement(const std::string& buffer, std::size_t cursor,
                                              const std::string& query,
                                              const CompletionItem& item) {
  const std::string insert = item.EffectiveInsertText();
  std::size_t begin = 0;
  std::size_t end = 0;
  if (item.replacement_range) {
    begin = item.replacement_range->first;
    end = item.replacement_range->second;
    if (begin > end || end > buffer.size()) {
      return buffer;  // invalid range: leave the buffer untouched
    }
  } else {
    const std::size_t safe_cursor = std::min(cursor, buffer.size());
    const std::size_t query_len = std::min(query.size(), safe_cursor);
    begin = safe_cursor - query_len;
    end = safe_cursor;
  }
  std::string result;
  result.reserve(buffer.size() - (end - begin) + insert.size());
  result += buffer.substr(0, begin);
  result += insert;
  result += buffer.substr(end);
  return result;
}

}  // namespace terminal_ui_kit
