#include "terminal_ui_kit/components/searchable_text_view.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/document/wrapped_document.h"

namespace terminal_ui_kit {
namespace {

// Mirrors VirtualDocumentImpl's WidthTracker: observes the box the widget is
// laid out in so the component can re-wrap the document when the width
// changes.
class WidthTracker : public ftxui::Node {
 public:
  WidthTracker(ftxui::Element child, int& observed_width)
      : Node({std::move(child)}), observed_width_(observed_width) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    observed_width_ = std::max(1, box.x_max - box.x_min + 1);
    Node::SetBox(box);
    children_[0]->SetBox(box);
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  int& observed_width_;
};

struct HighlightRun {
  std::size_t start;  // segment-local byte offset (inclusive)
  std::size_t end;    // segment-local byte offset (exclusive)
  bool active;
};

// Returns `text` with the last UTF-8 code point removed. Multi-byte sequences
// are never split: this steps back over any trailing continuation bytes to the
// leading byte. For a lone/lead byte (or ASCII) exactly one byte is dropped.
std::string_view RemoveLastUtf8Codepoint(std::string_view text) {
  if (text.empty()) {
    return text;
  }
  std::size_t i = text.size() - 1;
  while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0U) == 0x80U) {
    --i;
  }
  return text.substr(0, i);
}

}  // namespace

class SearchableTextViewImpl {
 public:
  explicit SearchableTextViewImpl(SearchableTextViewOptions options)
      : options_(std::move(options)),
        wrapped_(WrappedDocument(80, options_.tab_width)) {
    VirtualListOptions list_opts;
    list_opts.item_count = [this] { return wrapped_.display_line_count(); };
    list_opts.item_height = 1;
    list_opts.render_item = [this](std::size_t index, int width) {
      return render_display_line(index, width);
    };

    model_ = std::make_shared<VirtualListModel>(std::move(list_opts));
    auto list_component = model_->component();

    component_ = ftxui::Renderer(list_component, [this, list_component] {
      check_width();
      return std::make_shared<WidthTracker>(list_component->Render(), current_width_);
    });
    component_ |= ftxui::CatchEvent([this](ftxui::Event event) { return handle_event(event); });
  }

  ~SearchableTextViewImpl() = default;

  ftxui::Component component() const { return component_; }

  void set_lines(const std::vector<std::string>& lines) {
    source_.clear();
    if (!lines.empty()) {
      for (std::size_t i = 0; i < lines.size(); ++i) {
        source_.append(lines[i]);
        if (i + 1 < lines.size()) {
          source_.append("\n");
        }
      }
      source_.finish();
    }
    wrapped_.rebuild_from(source_);
    run_search();
  }

  void open_search() { search_open_ = true; }

  void close_search() { search_open_ = false; }

  bool search_open() const { return search_open_; }

  void set_query(std::string_view query) {
    const std::string next(query);
    if (next == query_ && search_applied_once_) {
      return;  // no change -- do not recompute, do not touch prompt state
    }
    query_ = next;
    search_applied_once_ = true;
    run_search();
  }

  const std::string& query() const { return query_; }

  SearchStatus status() const { return status_; }

  void next_match() {
    navigator_.next();
    scroll_to_current_match();
  }

  void previous_match() {
    navigator_.previous();
    scroll_to_current_match();
  }

  void jump_to_first_match() {
    navigator_.jump_to_first();
    scroll_to_current_match();
  }

  std::size_t match_count() const { return navigator_.count(); }

  std::optional<std::size_t> current_match_index() const { return navigator_.current_index(); }

  bool case_sensitive() const { return search_options_.case_sensitive; }

  void set_case_sensitive(bool value) {
    if (search_options_.case_sensitive == value) {
      return;
    }
    search_options_.case_sensitive = value;
    run_search();
  }

  void toggle_case_sensitive() { set_case_sensitive(!case_sensitive()); }

  bool use_regex() const { return search_options_.use_regex; }

  void set_use_regex(bool value) {
    if (search_options_.use_regex == value) {
      return;
    }
    search_options_.use_regex = value;
    run_search();
  }

  void toggle_regex() { set_use_regex(!use_regex()); }

  void scroll_to_current_match() {
    const std::optional<TextMatch> current = navigator_.current();
    if (!current) {
      return;
    }
    const std::optional<std::size_t> display =
        display_line_containing(current->line, current->start_byte);
    if (!display) {
      return;
    }
    model_->scroll_to_index(*display);
    model_->select_index(*display);
  }

 private:
  void run_search() {
    // Build the search input as a local so the view list borrows into
    // `source_` only for the duration of this call. `source_` is not mutated
    // in between, but keeping the borrowed views local (rather than a cached
    // member) removes any risk of them surviving a `source_` reallocation.
    std::vector<std::string_view> search_lines;
    search_lines.resize(source_.line_count());
    for (std::size_t i = 0; i < source_.line_count(); ++i) {
      search_lines[i] = source_.line_at(i);
    }

    std::vector<TextMatch> matches;
    status_ = SearchEngine::search(search_lines, query_, search_options_, matches);
    navigator_.set_matches(std::move(matches));
    jump_to_first_match();
    if (options_.on_status_change) {
      options_.on_status_change(status_);
    }
  }

  std::optional<std::size_t> display_line_containing(std::size_t logical, std::size_t byte) const {
    const std::size_t count = wrapped_.display_line_count();
    for (std::size_t i = 0; i < count; ++i) {
      const WrappedLine& line = wrapped_.display_line_at(i);
      if (line.logical_line != logical) {
        continue;
      }
      const std::size_t seg_end = line.byte_offset + line.text.size();
      if (byte >= line.byte_offset && byte < seg_end) {
        return i;
      }
      if (line.text.empty() && byte == line.byte_offset) {
        return i;
      }
      if (line.byte_offset > byte) {
        break;  // passed the byte; cannot be in a later segment of this line
      }
    }
    return std::nullopt;
  }

  void check_width() {
    if (current_width_ != last_width_ && current_width_ > 0) {
      last_width_ = current_width_;
      wrapped_.handle_width_change(last_width_, source_);
    }
  }

  ftxui::Element render_display_line(std::size_t index, int /*width*/) {
    const WrappedLine& line = wrapped_.display_line_at(index);
    const std::size_t seg_start = line.byte_offset;
    const std::size_t seg_end = seg_start + line.text.size();

    ftxui::Elements parts;
    if (options_.show_line_numbers) {
      parts.push_back(render_line_number(line));
    }

    const std::vector<HighlightRun> hits =
        highlight_runs_for(line.logical_line, seg_start, seg_end);
    std::size_t cursor = 0;
    for (const HighlightRun& run : hits) {
      if (run.start > cursor) {
        parts.push_back(ftxui::text(line.text.substr(cursor, run.start - cursor)));
      }
      const ftxui::Decorator decor = run.active ? active_match_decor() : match_decor();
      parts.push_back(ftxui::text(line.text.substr(run.start, run.end - run.start)) | decor);
      cursor = run.end;
    }
    if (cursor < line.text.size()) {
      parts.push_back(ftxui::text(line.text.substr(cursor)));
    }
    if (parts.empty()) {
      parts.push_back(ftxui::text(""));
    }
    return ftxui::hbox(std::move(parts));
  }

  ftxui::Element render_line_number(const WrappedLine& line) {
    // Numbered rows render "<5-wide number> ", i.e. 6 leading columns.
    // Continuation rows emit 6 spaces so wrapped content lines up.
    if (line.sub_line == 0) {
      std::string num = std::to_string(line.logical_line + 1);
      num = std::string(5 - std::min<std::size_t>(5, num.size()), ' ') + num;
      return ftxui::text(num + " ") | ftxui::color(ftxui::Color::GrayDark);
    }
    return ftxui::text(std::string(6, ' ')) | ftxui::color(ftxui::Color::GrayDark);
  }

  // Computes the highlight runs for one display segment, intersecting the
  // logical line's byte-range matches with this segment's byte range.
  std::vector<HighlightRun> highlight_runs_for(std::size_t logical, std::size_t seg_start,
                                               std::size_t seg_end) const {
    const std::optional<TextMatch> active = navigator_.current();
    std::vector<HighlightRun> runs;
    for (const TextMatch& match : navigator_.matches()) {
      if (match.line != logical) {
        continue;
      }
      if (match.end_byte <= seg_start || match.start_byte >= seg_end) {
        continue;  // no intersection with this segment
      }
      const std::size_t a = match.start_byte > seg_start ? match.start_byte - seg_start : 0;
      const std::size_t b =
          match.end_byte < seg_end ? match.end_byte - seg_start : seg_end - seg_start;
      if (a >= b) {
        continue;
      }
      const bool is_active = active && active->line == match.line &&
                             active->start_byte == match.start_byte &&
                             active->end_byte == match.end_byte;
      runs.push_back(HighlightRun{a, b, is_active});
    }
    return runs;
  }

  ftxui::Decorator match_decor() const { return to_decorator(options_.theme.selected); }

  ftxui::Decorator active_match_decor() const {
    return to_decorator(options_.theme.error) | ftxui::inverted | ftxui::bold;
  }

  bool handle_event(ftxui::Event event) {
    if (search_open_) {
      if (event == ftxui::Event::Return) {
        search_open_ = false;
        return true;
      }
      if (event == ftxui::Event::Backspace) {
        if (!query_.empty()) {
          set_query(RemoveLastUtf8Codepoint(query_));
        }
        return true;
      }
      if (event == ftxui::Event::Escape) {
        set_query("");
        search_open_ = false;
        return true;
      }
      if (event.is_character()) {
        std::string next = query_;
        next += event.character();
        set_query(next);
        return true;
      }
      return false;
    }

    if (event == ftxui::Event::Character('/')) {
      open_search();
      return true;
    }
    if (event == ftxui::Event::Character('n')) {
      next_match();
      return true;
    }
    if (event == ftxui::Event::Character('N')) {
      previous_match();
      return true;
    }
    if (event == ftxui::Event::Character('c')) {
      toggle_case_sensitive();
      return true;
    }
    if (event == ftxui::Event::Character('r')) {
      toggle_regex();
      return true;
    }
    return false;
  }

  SearchableTextViewOptions options_;
  StreamingDocument source_;
  WrappedDocument wrapped_;
  std::shared_ptr<VirtualListModel> model_;
  ftxui::Component component_;
  int current_width_ = 0;
  int last_width_ = 0;

  std::string query_;
  SearchOptions search_options_;
  SearchStatus status_ = SearchStatus::kEmptyQuery;
  bool search_open_ = false;
  bool search_applied_once_ = false;
  MatchNavigator navigator_;
};

SearchableTextView::SearchableTextView(SearchableTextViewOptions options)
    : impl_(std::make_shared<SearchableTextViewImpl>(std::move(options))) {}

SearchableTextView::~SearchableTextView() = default;

SearchableTextView::SearchableTextView(SearchableTextView&&) noexcept = default;
SearchableTextView& SearchableTextView::operator=(SearchableTextView&&) noexcept = default;

ftxui::Component SearchableTextView::component() const { return impl_->component(); }

void SearchableTextView::set_lines(const std::vector<std::string>& lines) {
  impl_->set_lines(lines);
}

void SearchableTextView::open_search() { impl_->open_search(); }

void SearchableTextView::close_search() { impl_->close_search(); }

bool SearchableTextView::search_open() const { return impl_->search_open(); }

void SearchableTextView::set_query(std::string_view query) { impl_->set_query(query); }

const std::string& SearchableTextView::query() const { return impl_->query(); }

SearchStatus SearchableTextView::status() const { return impl_->status(); }

void SearchableTextView::next_match() { impl_->next_match(); }

void SearchableTextView::previous_match() { impl_->previous_match(); }

void SearchableTextView::jump_to_first_match() { impl_->jump_to_first_match(); }

std::size_t SearchableTextView::match_count() const { return impl_->match_count(); }

std::optional<std::size_t> SearchableTextView::current_match_index() const {
  return impl_->current_match_index();
}

bool SearchableTextView::case_sensitive() const { return impl_->case_sensitive(); }

void SearchableTextView::set_case_sensitive(bool value) { impl_->set_case_sensitive(value); }

void SearchableTextView::toggle_case_sensitive() { impl_->toggle_case_sensitive(); }

bool SearchableTextView::use_regex() const { return impl_->use_regex(); }

void SearchableTextView::set_use_regex(bool value) { impl_->set_use_regex(value); }

void SearchableTextView::toggle_regex() { impl_->toggle_regex(); }

void SearchableTextView::scroll_to_current_match() { impl_->scroll_to_current_match(); }

}  // namespace terminal_ui_kit
