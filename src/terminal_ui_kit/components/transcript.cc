#include "terminal_ui_kit/components/transcript.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/components/status_indicator.h"
#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/core/text_style.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

namespace {

std::string lowercase(std::string text) {
  for (char& c : text) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return text;
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::string current;
  current.reserve(text.size() / 8 + 1);
  for (const char c : text) {
    if (c == '\n') {
      lines.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  lines.push_back(current);
  // A trailing '\n' does not introduce an extra (phantom) blank display line.
  if (!lines.empty() && !text.empty() && text.back() == '\n') {
    lines.pop_back();
  }
  return lines;
}

// Returns a pointer to the primary text field of any block kind, so the tail
// can be streamed in place regardless of which variant it is.
std::string* mutable_item_text(TranscriptItem& item) {
  return std::visit(
      [](auto& block) -> std::string* {
        using T = std::decay_t<decltype(block)>;
        if constexpr (std::is_same_v<T, TextBlock>) {
          return &block.text;
        } else if constexpr (std::is_same_v<T, MarkdownBlock>) {
          return &block.markdown;
        } else if constexpr (std::is_same_v<T, CodeBlock>) {
          return &block.code;
        } else if constexpr (std::is_same_v<T, LogBlock>) {
          return &block.message;
        } else if constexpr (std::is_same_v<T, DiffBlock>) {
          return &block.diff;
        } else if constexpr (std::is_same_v<T, StatusBlock>) {
          return &block.text;
        } else {
          return &block.content;
        }
      },
      item);
}

std::string_view item_text(const TranscriptItem& item) {
  return std::visit(
      [](const auto& block) -> std::string_view {
        using T = std::decay_t<decltype(block)>;
        if constexpr (std::is_same_v<T, TextBlock>) {
          return block.text;
        } else if constexpr (std::is_same_v<T, MarkdownBlock>) {
          return block.markdown;
        } else if constexpr (std::is_same_v<T, CodeBlock>) {
          return block.code;
        } else if constexpr (std::is_same_v<T, LogBlock>) {
          return block.message;
        } else if constexpr (std::is_same_v<T, DiffBlock>) {
          return block.diff;
        } else if constexpr (std::is_same_v<T, StatusBlock>) {
          return block.text;
        } else {
          return block.content;
        }
      },
      item);
}

TextStyle log_severity_style(LogSeverity severity, const Theme& theme) {
  switch (severity) {
    case LogSeverity::kTrace:
      return theme.muted;
    case LogSeverity::kDebug:
      return theme.code;
    case LogSeverity::kInfo:
      return theme.accent;
    case LogSeverity::kWarning:
      return theme.warning;
    case LogSeverity::kError:
      return theme.error;
  }
  return theme.primary;
}

ftxui::Element render_diff_line(const std::string& line, const Theme& theme) {
  if (!line.empty() && line.front() == '+') {
    return ftxui::text(line) | to_decorator(theme.addition);
  }
  if (!line.empty() && line.front() == '-') {
    return ftxui::text(line) | to_decorator(theme.deletion);
  }
  return ftxui::text(line) | to_decorator(theme.primary);
}

}  // namespace

// ---------------------------------------------------------------------------
// TranscriptModel
// ---------------------------------------------------------------------------

std::size_t TranscriptModel::append(TranscriptItem item) {
  if (tail_) {
    finalize_tail();
  }
  blocks_.push_back(std::move(item));
  ++revision_;
  return blocks_.size() - 1;
}

std::size_t TranscriptModel::begin_tail(TranscriptItem initial) {
  if (tail_) {
    finalize_tail();
  }
  const std::size_t index = append(std::move(initial));
  tail_ = index;
  return index;
}

bool TranscriptModel::has_tail() const { return tail_.has_value(); }

std::optional<std::size_t> TranscriptModel::tail_index() const { return tail_; }

void TranscriptModel::append_tail(std::string_view chunk) {
  if (!tail_) {
    return;
  }
  mutable_item_text(blocks_[*tail_])->append(chunk.data(), chunk.size());
  ++revision_;
}

void TranscriptModel::replace_tail(std::string_view content) {
  if (!tail_) {
    return;
  }
  mutable_item_text(blocks_[*tail_])->assign(content.data(), content.size());
  ++revision_;
}

void TranscriptModel::finalize_tail() {
  if (!tail_) {
    return;
  }
  tail_.reset();
  ++revision_;
}

std::size_t TranscriptModel::block_count() const { return blocks_.size(); }

const TranscriptItem& TranscriptModel::block_at(std::size_t index) const {
  return blocks_.at(index);
}

std::string TranscriptModel::block_plain_text(std::size_t index) const {
  return std::string(item_text(blocks_.at(index)));
}

std::vector<std::string> TranscriptModel::block_lines(std::size_t index) const {
  return split_lines(std::string(item_text(blocks_.at(index))));
}

std::vector<std::string> TranscriptModel::tail_visible_lines(std::size_t max_lines) const {
  std::vector<std::string> out;
  if (!tail_) {
    return out;
  }
  const std::string_view text = item_text(blocks_[*tail_]);
  if (text.empty()) {
    return out;
  }
  const std::size_t limit = std::max<std::size_t>(1, max_lines);
  std::size_t pos = text.size();
  if (text.back() == '\n') {
    --pos;  // drop a single trailing newline: no phantom blank line
  }
  // Scan backward from the end, collecting at most `limit` single lines. Each
  // rfind('\n', pos-1) yields the last newline before pos, so text[pos, newline)
  // is exactly one line; when npos is returned, text[0, pos) has no newline and
  // is the final single line. Cost is O(limit) lines, not O(total tail).
  while (pos > 0 && out.size() < limit) {
    const std::size_t nl = text.rfind('\n', pos - 1);
    if (nl == std::string_view::npos) {
      out.push_back(std::string(text.substr(0, pos)));
      pos = 0;
    } else {
      out.push_back(std::string(text.substr(nl + 1, pos - nl - 1)));
      pos = nl;
    }
  }
  std::reverse(out.begin(), out.end());
  return out;
}

void TranscriptModel::clear() {
  blocks_.clear();
  tail_.reset();
  bookmarks_.clear();
  search_query_.clear();
  matches_.clear();
  search_revision_ = 0;
  ++revision_;
}

std::optional<std::size_t> TranscriptModel::find(const std::string& query) {
  if (query != search_query_ || search_revision_ != revision_) {
    search_query_ = query;
    rebuild_search_index();
    search_revision_ = revision_;
  }
  if (matches_.empty()) {
    return std::nullopt;
  }
  return std::optional<std::size_t>{matches_.front()};
}

std::optional<std::size_t> TranscriptModel::next_match(std::size_t after) const {
  const auto it = std::upper_bound(matches_.begin(), matches_.end(), after);
  if (it == matches_.end()) {
    return std::nullopt;
  }
  return std::optional<std::size_t>{*it};
}

std::optional<std::size_t> TranscriptModel::previous_match(std::size_t before) const {
  const auto it = std::lower_bound(matches_.begin(), matches_.end(), before);
  if (it == matches_.begin()) {
    return std::nullopt;
  }
  return std::optional<std::size_t>{*(it - 1)};
}

void TranscriptModel::toggle_bookmark(std::size_t index) {
  const auto it = bookmarks_.find(index);
  if (it == bookmarks_.end()) {
    bookmarks_.insert(index);
  } else {
    bookmarks_.erase(it);
  }
}

bool TranscriptModel::is_bookmarked(std::size_t index) const { return bookmarks_.count(index) > 0; }

std::vector<std::size_t> TranscriptModel::bookmarks() const {
  return std::vector<std::size_t>(bookmarks_.begin(), bookmarks_.end());
}

void TranscriptModel::rebuild_search_index() {
  matches_.clear();
  if (search_query_.empty()) {
    return;
  }
  const std::string needle = lowercase(search_query_);
  for (std::size_t i = 0; i < blocks_.size(); ++i) {
    if (lowercase(std::string(item_text(blocks_[i]))).find(needle) != std::string::npos) {
      matches_.push_back(i);
    }
  }
}

// ---------------------------------------------------------------------------
// TranscriptView
// ---------------------------------------------------------------------------

class TranscriptViewImpl {
 public:
  TranscriptViewImpl(TranscriptModel* model, TranscriptViewOptions options)
      : model_(model), options_(std::move(options)), follow_(options_.follow) {
    assert(model_ != nullptr);
    VirtualListOptions list_options;
    list_options.item_count = [this] { return item_count(); };
    list_options.render_item = [this](std::size_t index, int width) {
      return render_block(index, width);
    };
    list_options.estimate_height = [this](std::size_t index, int) {
      return estimate_block_height(index);
    };

    list_ = std::make_shared<VirtualListModel>(std::move(list_options));
    auto list_component = list_->component();

    component_ = ftxui::Renderer(list_component, [this, list_component] {
      check_follow();
      return list_component->Render();
    });
    component_ |= ftxui::CatchEvent([this](ftxui::Event event) { return handle_event(event); });
  }

  ftxui::Component component() const { return component_; }

  bool follow() const { return follow_; }

  void set_follow(bool follow) {
    follow_ = follow;
    if (follow_) {
      list_->scroll_to_bottom();
    }
  }

  void scroll_to_bottom() { list_->scroll_to_bottom(); }

  void scroll_to_index(std::size_t index) { list_->scroll_to_index(index); }

  bool find(const std::string& query) {
    search_query_ = query;
    const std::optional<std::size_t> match = model_->find(query);
    if (match) {
      current_match_ = match;
      list_->select_index(*match);
      // Searching implies the user wants to inspect a location, not ride the
      // streaming tail; disable follow so the next append does not snap back.
      follow_ = false;
      return true;
    }
    current_match_.reset();
    return false;
  }

  bool find_next() {
    // Rebuild the cached hit list if the query or the data revision changed
    // since the last find() (no-op when nothing changed).
    model_->find(search_query_);
    if (search_query_.empty() || model_->match_count() == 0) {
      return false;
    }
    std::optional<std::size_t> next =
        current_match_ ? model_->next_match(*current_match_) : std::nullopt;
    if (!next) {
      next = model_->matches().front();
    }
    current_match_ = *next;
    list_->select_index(*current_match_);
    follow_ = false;
    return true;
  }

  bool find_previous() {
    model_->find(search_query_);
    if (search_query_.empty() || model_->match_count() == 0) {
      return false;
    }
    std::optional<std::size_t> previous =
        current_match_ ? model_->previous_match(*current_match_) : std::nullopt;
    if (!previous) {
      previous = model_->matches().back();
    }
    current_match_ = *previous;
    list_->select_index(*current_match_);
    follow_ = false;
    return true;
  }

  std::optional<std::size_t> current_match() const { return current_match_; }

  bool search_mode() const { return search_mode_; }

 private:
  std::size_t item_count() const { return model_ ? model_->block_count() : 0; }

  void check_follow() {
    if (!follow_) {
      return;
    }
    const std::uint64_t revision = model_->revision();
    if (revision != last_revision_) {
      last_revision_ = revision;
      list_->scroll_to_bottom();
    }
  }

  int estimate_block_height(std::size_t index) {
    if (model_->tail_index() && *model_->tail_index() == index) {
      return std::max(1, options_.tail_display_height);
    }
    // StatusBlock is rendered as a single StatusIndicator row regardless of
    // the text length, so its height is always one.
    if (std::holds_alternative<StatusBlock>(model_->block_at(index))) {
      return 1;
    }
    const std::size_t line_count = model_->block_lines(index).size();
    return static_cast<int>(std::max<std::size_t>(1, line_count));
  }

  ftxui::Element render_block(std::size_t index, int width) {
    const bool is_tail = model_->tail_index() == index;
    std::vector<std::string> lines;
    if (is_tail) {
      // Clip to the fixed-height tail window without splitting the whole
      // (possibly very large) streaming content.
      lines = model_->tail_visible_lines(
          static_cast<std::size_t>(std::max(1, options_.tail_display_height)));
      const std::size_t height =
          static_cast<std::size_t>(std::max(1, options_.tail_display_height));
      while (lines.size() < height) {
        lines.push_back("");
      }
    } else {
      lines = model_->block_lines(index);
    }
    ftxui::Element content = render_content(model_->block_at(index), lines);
    std::string marker = " ";
    if (model_->is_bookmarked(index)) {
      marker = "\u25C6";  // ◆
    } else if (current_match_ && *current_match_ == index) {
      marker = "\u25CF";  // ●
    }
    content = ftxui::hbox({ftxui::text(marker + " ") | ftxui::dim, content});
    (void)width;
    return content;
  }

  ftxui::Element render_content(const TranscriptItem& item, const std::vector<std::string>& lines) {
    const Theme& theme = options_.theme;
    ftxui::Elements rows;
    rows.reserve(lines.size());
    switch (item.index()) {
      case 0: {  // TextBlock
        for (const auto& line : lines) {
          rows.push_back(ftxui::text(line) | to_decorator(theme.primary));
        }
        break;
      }
      case 1: {  // MarkdownBlock
        for (const auto& line : lines) {
          rows.push_back(ftxui::text(line) | to_decorator(theme.secondary));
        }
        break;
      }
      case 2: {  // CodeBlock
        const CodeBlock& block = std::get<CodeBlock>(item);
        for (std::size_t i = 0; i < lines.size(); ++i) {
          if (i == 0 && !block.language.empty()) {
            rows.push_back(ftxui::hbox({
                ftxui::text(block.language) | to_decorator(theme.muted),
                ftxui::text(" "),
                ftxui::text(lines[i]) | to_decorator(theme.code),
            }));
          } else {
            rows.push_back(ftxui::text(lines[i]) | to_decorator(theme.code));
          }
        }
        break;
      }
      case 3: {  // LogBlock
        const LogBlock& block = std::get<LogBlock>(item);
        const TextStyle style = log_severity_style(block.severity, theme);
        for (const auto& line : lines) {
          rows.push_back(ftxui::text(line) | to_decorator(style));
        }
        break;
      }
      case 4: {  // DiffBlock
        for (const auto& line : lines) {
          rows.push_back(render_diff_line(line, theme));
        }
        break;
      }
      case 5: {  // StatusBlock
        const StatusBlock& block = std::get<StatusBlock>(item);
        rows.push_back(StatusIndicator(block.status, block.text, theme));
        break;
      }
      case 6: {  // CustomBlock
        const CustomBlock& block = std::get<CustomBlock>(item);
        for (std::size_t i = 0; i < lines.size(); ++i) {
          if (i == 0 && !block.label.empty()) {
            rows.push_back(ftxui::hbox({
                ftxui::text(block.label) | to_decorator(theme.accent),
                ftxui::text(" "),
                ftxui::text(lines[i]) | to_decorator(theme.primary),
            }));
          } else {
            rows.push_back(ftxui::text(lines[i]) | to_decorator(theme.primary));
          }
        }
        break;
      }
      default:
        break;
    }
    return ftxui::vbox(std::move(rows));
  }

  bool handle_event(ftxui::Event event) {
    if (event.is_mouse()) {
      const ftxui::Mouse& mouse = event.mouse();
      if (mouse.button == ftxui::Mouse::WheelUp || mouse.button == ftxui::Mouse::WheelDown) {
        follow_ = false;
      }
      return false;  // let VirtualList process the wheel
    }
    if (search_mode_) {
      return handle_search_event(event);
    }
    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::ArrowDown ||
        event == ftxui::Event::PageUp || event == ftxui::Event::PageDown ||
        event == ftxui::Event::Home) {
      follow_ = false;
      return false;  // VirtualList owns navigation
    }
    if (event == ftxui::Event::End) {
      set_follow(true);
      return true;
    }
    if (event == ftxui::Event::Return) {
      open_details_selected();
      return true;
    }
    if (event.is_character()) {
      const std::string& key = event.character();
      if (key == "f" || key == "F") {
        set_follow(!follow_);
        return true;
      }
      if (key == "g") {
        list_->scroll_to_index(0);
        follow_ = false;
        return true;
      }
      if (key == "G") {
        follow_ = true;
        list_->scroll_to_bottom();
        return true;
      }
      if (key == "b") {
        toggle_bookmark_selected();
        return true;
      }
      if (key == "y") {
        copy_selected();
        return true;
      }
      if (key == "/") {
        begin_search();
        return true;
      }
      if (key == "n") {
        find_next();
        return true;
      }
      if (key == "N") {
        find_previous();
        return true;
      }
    }
    return false;
  }

  bool handle_search_event(const ftxui::Event& event) {
    if (event == ftxui::Event::Escape || event == ftxui::Event::Return) {
      search_mode_ = false;
      return true;
    }
    if (event == ftxui::Event::Backspace && !search_query_.empty()) {
      search_query_.pop_back();
      find(search_query_);
      return true;
    }
    if (event.is_character() && !event.character().empty()) {
      const std::string& key = event.character();
      // Let n/N navigate hits while the search input is open.
      if (key == "n") {
        return find_next();
      }
      if (key == "N") {
        return find_previous();
      }
      search_query_.append(key);
      find(search_query_);
      return true;
    }
    return false;
  }

  void begin_search() {
    search_mode_ = true;
    search_query_.clear();
    current_match_.reset();
  }

  void toggle_bookmark_selected() {
    const std::optional<std::size_t> selected = list_->selected_index();
    if (selected) {
      model_->toggle_bookmark(*selected);
    }
  }

  void copy_selected() {
    const std::optional<std::size_t> selected = list_->selected_index();
    if (!selected || !options_.on_copy) {
      return;
    }
    options_.on_copy(model_->block_plain_text(*selected));
  }

  void open_details_selected() {
    const std::optional<std::size_t> selected = list_->selected_index();
    if (!selected || !options_.on_open_details) {
      return;
    }
    options_.on_open_details(*selected);
  }

  TranscriptModel* model_;
  TranscriptViewOptions options_;
  std::shared_ptr<VirtualListModel> list_;
  ftxui::Component component_;
  bool follow_;
  std::uint64_t last_revision_ = 0;
  bool search_mode_ = false;
  std::string search_query_;
  std::optional<std::size_t> current_match_;
};

TranscriptView::TranscriptView(TranscriptModel* model, TranscriptViewOptions options)
    : impl_(std::make_shared<TranscriptViewImpl>(model, std::move(options))) {}

ftxui::Component TranscriptView::component() const { return impl_->component(); }
bool TranscriptView::follow() const { return impl_->follow(); }
void TranscriptView::set_follow(bool follow) { impl_->set_follow(follow); }
void TranscriptView::scroll_to_bottom() { impl_->scroll_to_bottom(); }
void TranscriptView::scroll_to_index(std::size_t index) { impl_->scroll_to_index(index); }
bool TranscriptView::find(const std::string& query) { return impl_->find(query); }
bool TranscriptView::find_next() { return impl_->find_next(); }
bool TranscriptView::find_previous() { return impl_->find_previous(); }
std::optional<std::size_t> TranscriptView::current_match() const { return impl_->current_match(); }
bool TranscriptView::search_mode() const { return impl_->search_mode(); }

}  // namespace terminal_ui_kit