#include "terminal_ui_kit/components/unified_diff_view.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {
namespace {

// Number of decimal digits in a non-negative integer (gutter column width).
int digit_count(int value) {
  int digits = 1;
  while (value >= 10) {
    value /= 10;
    ++digits;
  }
  return digits;
}

// Concatenates the plain text of every span; diff lines are always a single
// span, so this returns the source line text.
std::string plain_text(const StyledText& text) {
  std::string result;
  for (const TextSpan& span : text.spans()) result += span.text;
  return result;
}

// Truncates `text` to at most `max_cells` cells, appending a single trailing
// ellipsis when the source is longer. Chopping is done on a UTF-8 code point
// boundary so a multi-byte sequence is never split in the middle. The cell
// width of wide characters is approximated by their byte length (documented
// limitation).
std::string truncate(std::string_view text, int max_cells) {
  if (max_cells <= 0) return "";
  if (text.empty()) return "";
  if (static_cast<int>(text.size()) <= max_cells) return std::string(text);
  std::size_t keep = static_cast<std::size_t>(max_cells) - 1;  // room for '…'
  // Back up over a UTF-8 continuation byte so we never split a code point.
  while (keep > 0 && (static_cast<unsigned char>(text[keep]) & 0xC0) == 0x80) {
    --keep;
  }
  std::string out(text.substr(0, keep));
  out += "\xE2\x80\xA6";  // '…'
  return out;
}

// Node decorator that records the box allocated to its child, so the view can
// track the current viewport width and height across frames.
class ObservingBoxDecorator : public ftxui::Node {
 public:
  // `observed_box` is Impl::box_. This decorator only lives for the duration of
  // a single frame's element tree, while Impl (and its box_) is owned by the
  // shared_ptr held by UnifiedDiffView/component, so the reference is valid for
  // the decorator's whole lifetime. Do not point it at a frame-local object.
  ObservingBoxDecorator(ftxui::Element child, ftxui::Box& observed_box)
      : Node({std::move(child)}), observed_box_(observed_box) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    const bool box_changed = observed_box_ != box;
    observed_box_ = box;
    Node::SetBox(box);
    children_[0]->SetBox(box);
    if (box_changed) {
      // The viewport changed: request a follow-up frame so the visible-row
      // window is recomputed with the new dimensions.
      ftxui::animation::RequestAnimationFrame();
    }
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  ftxui::Box& observed_box_;
};

}  // namespace

namespace diffview_detail {

// One row of the flattened, fixed-height layout. All cross references are
// indices into the retained model, satisfying the safe-owning-type rule.
enum class DiffRowKind {
  kFileHeader,
  kNewFileNotice,
  kDeletedFileNotice,
  kBinaryNotice,
  kEmptyNotice,
  kHunkHeader,
  kLine,
};

struct DiffRow {
  DiffRowKind kind = DiffRowKind::kLine;
  std::size_t file = 0;
  std::size_t hunk = 0;
  std::size_t line = 0;
  // 0-based ordinal of this row's file among all files (for status).
  std::size_t file_ordinal = 0;
  // 0-based ordinal of this row's hunk among all hunk-header rows (for status).
  std::size_t hunk_ordinal = 0;
  std::string header;  // cached display text for header/notice rows
  std::string old_no;  // right-aligned old line number (gutter)
  std::string new_no;  // right-aligned new line number (gutter)
};

}  // namespace diffview_detail

class UnifiedDiffView::Impl : public ftxui::ComponentBase {
 public:
  explicit Impl(std::vector<diff::DiffFile> files, UnifiedDiffViewOptions options)
      : files_(std::move(files)), options_(std::move(options)) {
    theme_ = options_.color ? options_.theme : without_color(options_.theme);
    collapsed_.assign(files_.size(), false);
    compute_gutter_width();
    rebuild_rows();
    if (!rows_.empty()) {
      selected_row_ = 0;
    }
  }

  ftxui::Element Render() override {
    if (rows_.empty()) {
      return std::make_shared<ObservingBoxDecorator>(ftxui::text(empty_message()) | ftxui::dim,
                                                     box_);
    }
    clamp_scroll();
    ensure_visible(selected_row_);
    const int width = box_width();
    const int height = box_height();
    const std::size_t begin = static_cast<std::size_t>(scroll_offset_);
    const std::size_t end = std::min(rows_.size(), begin + static_cast<std::size_t>(height));
    ftxui::Elements rows;
    for (std::size_t i = begin; i < end; ++i) {
      rows.push_back(render_row(i, width));
    }
    ftxui::Element content = ftxui::vbox(std::move(rows)) | ftxui::yflex;
    return std::make_shared<ObservingBoxDecorator>(std::move(content), box_);
  }

  bool Focusable() const override { return !rows_.empty(); }

  bool OnEvent(ftxui::Event event) override {
    if (event.is_mouse()) {
      return on_mouse_event(event);
    }
    if (rows_.empty()) {
      return false;
    }
    if (event == ftxui::Event::ArrowDown || event.input() == "j") {
      return select_row(selected_row_ + 1);
    }
    if (event == ftxui::Event::ArrowUp || event.input() == "k") {
      return select_row(selected_row_ == 0 ? 0 : selected_row_ - 1);
    }
    if (event == ftxui::Event::PageDown) {
      select_row(
          std::min(rows_.size() - 1, selected_row_ + static_cast<std::size_t>(box_height())));
      return true;
    }
    if (event == ftxui::Event::PageUp) {
      const std::size_t target = (selected_row_ > static_cast<std::size_t>(box_height()))
                                     ? selected_row_ - static_cast<std::size_t>(box_height())
                                     : 0;
      return select_row(target);
    }
    if (event == ftxui::Event::Home) {
      return select_row(0);
    }
    if (event == ftxui::Event::End) {
      return select_row(rows_.size() - 1);
    }
    if (event.input() == "]") {
      next_file();
      return true;
    }
    if (event.input() == "[") {
      prev_file();
      return true;
    }
    if (event.input() == "n") {
      next_hunk();
      return true;
    }
    if (event.input() == "N") {
      prev_hunk();
      return true;
    }
    if (event == ftxui::Event::Return) {
      toggle_collapse_current_file();
      return true;
    }
    if (event.input() == "y") {
      return copy_selection();
    }
    return false;
  }

  void next_file() {
    if (rows_.empty()) return;
    const std::size_t current = rows_[selected_row_].file_ordinal;
    if (current + 1 >= files_.size()) return;
    select_file_ordinal(current + 1);
  }

  void prev_file() {
    if (rows_.empty()) return;
    const std::size_t current = rows_[selected_row_].file_ordinal;
    if (current == 0) return;
    select_file_ordinal(current - 1);
  }

  void next_hunk() {
    const std::size_t target = next_hunk_row(selected_row_ + 1);
    if (target != rows_.size()) {
      select_row(target);
    }
  }

  void prev_hunk() {
    const std::size_t target = prev_hunk_row(selected_row_);
    if (target != rows_.size()) {
      select_row(target);
    }
  }

  void toggle_collapse_current_file() {
    if (rows_.empty()) return;
    const std::size_t file = rows_[selected_row_].file;
    collapsed_[file] = !collapsed_[file];
    rebuild_rows();
    // Keep the toggled file's header selected so the user stays in context.
    select_row(row_of_file_header(file));
  }

  void collapse_file(std::size_t file_index) {
    if (file_index >= collapsed_.size() || collapsed_[file_index]) return;
    collapsed_[file_index] = true;
    rebuild_rows();
    select_row(row_of_file_header(file_index));
  }

  void expand_file(std::size_t file_index) {
    if (file_index >= collapsed_.size() || !collapsed_[file_index]) return;
    collapsed_[file_index] = false;
    rebuild_rows();
    select_row(row_of_file_header(file_index));
  }

  bool is_collapsed(std::size_t file_index) const {
    return file_index < collapsed_.size() && collapsed_[file_index];
  }

  void set_search(const std::string& query) {
    search_query_ = query;
    rebuild_matches();
    if (!matches_.empty()) {
      current_match_ = 0;
      select_row(matches_[0]);
    }
  }

  void jump_to_next_match() {
    if (matches_.empty()) return;
    current_match_ = (current_match_ + 1) % matches_.size();
    select_row(matches_[current_match_]);
  }

  void jump_to_prev_match() {
    if (matches_.empty()) return;
    current_match_ = (current_match_ + matches_.size() - 1) % matches_.size();
    select_row(matches_[current_match_]);
  }

  bool has_matches() const { return !matches_.empty(); }
  std::size_t match_count() const { return matches_.size(); }
  std::optional<std::size_t> current_match() const {
    if (matches_.empty()) return std::nullopt;
    return current_match_;
  }

  bool select_row(std::size_t index) {
    if (rows_.empty()) return false;
    index = std::min(index, rows_.size() - 1);
    if (index == selected_row_) {
      ensure_visible(selected_row_);
      return false;
    }
    selected_row_ = index;
    ensure_visible(selected_row_);
    return true;
  }

  void scroll_to_row(std::size_t index) {
    if (rows_.empty()) return;
    index = std::min(index, rows_.size() - 1);
    scroll_offset_ = static_cast<int>(index);
    clamp_scroll();
  }

  bool copy_selection() {
    if (rows_.empty() || !options_.on_copy) return false;
    const DiffRow& row = rows_[selected_row_];
    std::string text;
    if (row.kind == diffview_detail::DiffRowKind::kLine) {
      text = plain_text(files_[row.file].hunks[row.hunk].lines[row.line].content);
    } else {
      text = row.header;
    }
    options_.on_copy(std::move(text));
    return true;
  }

  bool has_selection() const { return !rows_.empty(); }

  Status status() const {
    Status s;
    s.file_count = files_.size();
    s.row_count = rows_.size();
    if (rows_.empty()) {
      return s;
    }
    const DiffRow& selected = rows_[selected_row_];
    s.selected_row = selected_row_;
    s.file_index = selected.file_ordinal;
    s.hunk_index = selected.hunk_ordinal;
    s.hunk_count = hunk_header_count_;
    s.first_visible = static_cast<std::size_t>(
        std::max(0, std::min(scroll_offset_, static_cast<int>(rows_.size()) - 1)));
    if (!rows_.empty()) {
      s.last_visible =
          std::min(rows_.size() - 1,
                   s.first_visible + static_cast<std::size_t>(std::max(1, box_height())) - 1);
    }
    return s;
  }

  std::size_t row_count() const { return rows_.size(); }
  bool is_empty() const { return files_.empty(); }

 private:
  using DiffRow = diffview_detail::DiffRow;
  using DiffRowKind = diffview_detail::DiffRowKind;

  void compute_gutter_width() {
    int width = 1;
    for (const diff::DiffFile& file : files_) {
      for (const diff::DiffHunk& hunk : file.hunks) {
        for (const diff::DiffLine& line : hunk.lines) {
          if (line.old_line) width = std::max(width, digit_count(*line.old_line));
          if (line.new_line) width = std::max(width, digit_count(*line.new_line));
        }
      }
    }
    gutter_width_ = width;
  }

  std::string file_header_text(const diff::DiffFile& file) const {
    if (file.old_path == file.new_path) return file.new_path;
    return file.old_path + " -> " + file.new_path;
  }

  void rebuild_rows() {
    rows_.clear();
    hunk_header_count_ = 0;
    for (std::size_t fi = 0; fi < files_.size(); ++fi) {
      const diff::DiffFile& file = files_[fi];
      const bool collapsed = collapsed_[fi];

      DiffRow header;
      header.kind = DiffRowKind::kFileHeader;
      header.file = fi;
      header.file_ordinal = fi;
      header.header = file_header_text(file);
      rows_.push_back(std::move(header));

      if (collapsed) {
        continue;  // collapsed files keep their header row but hide the body.
      }

      const bool is_new = file.old_path == "/dev/null";
      const bool is_deleted = file.new_path == "/dev/null";

      if (file.binary) {
        DiffRow notice;
        notice.kind = DiffRowKind::kBinaryNotice;
        notice.file = fi;
        notice.file_ordinal = fi;
        notice.header = "binary file: " + header_text(file) + " differs";
        rows_.push_back(std::move(notice));
        continue;
      }
      if (is_new) {
        DiffRow notice;
        notice.kind = DiffRowKind::kNewFileNotice;
        notice.file = fi;
        notice.file_ordinal = fi;
        notice.header = "new file: " + file.new_path;
        rows_.push_back(std::move(notice));
      } else if (is_deleted) {
        DiffRow notice;
        notice.kind = DiffRowKind::kDeletedFileNotice;
        notice.file = fi;
        notice.file_ordinal = fi;
        notice.header = "deleted file: " + file.old_path;
        rows_.push_back(std::move(notice));
      }

      if (file.hunks.empty()) {
        DiffRow notice;
        notice.kind = DiffRowKind::kEmptyNotice;
        notice.file = fi;
        notice.file_ordinal = fi;
        notice.header = "no changes";
        rows_.push_back(std::move(notice));
        continue;
      }

      for (std::size_t hi = 0; hi < file.hunks.size(); ++hi) {
        const diff::DiffHunk& hunk = file.hunks[hi];
        DiffRow hunk_row;
        hunk_row.kind = DiffRowKind::kHunkHeader;
        hunk_row.file = fi;
        hunk_row.hunk = hi;
        hunk_row.file_ordinal = fi;
        hunk_row.hunk_ordinal = hunk_header_count_;
        hunk_row.header = hunk.header;
        rows_.push_back(std::move(hunk_row));
        ++hunk_header_count_;

        for (std::size_t li = 0; li < hunk.lines.size(); ++li) {
          const diff::DiffLine& line = hunk.lines[li];
          DiffRow row;
          row.kind = DiffRowKind::kLine;
          row.file = fi;
          row.hunk = hi;
          row.line = li;
          row.file_ordinal = fi;
          row.hunk_ordinal = hunk_row.hunk_ordinal;
          row.old_no = format_gutter(line.old_line);
          row.new_no = format_gutter(line.new_line);
          rows_.push_back(std::move(row));
        }
      }
    }
    rebuild_matches();
  }

  std::string format_gutter(const std::optional<int>& number) const {
    const std::size_t width = static_cast<std::size_t>(gutter_width_);
    if (!number) return std::string(width, ' ');
    const std::string digits = std::to_string(*number);
    return std::string(width - digits.size(), ' ') + digits;
  }

  std::string header_text(const diff::DiffFile& file) const {
    // Same old→new separator as file_header_text so a binary notice and its
    // file header read consistently.
    if (file.old_path == file.new_path) return file.new_path;
    return file.old_path + " -> " + file.new_path;
  }

  std::string empty_message() const { return files_.empty() ? "empty diff" : ""; }

  bool gutter_visible(int width) const {
    if (!options_.show_line_numbers) return false;
    const int needed = 2 * gutter_width_ + 2 + options_.min_content_width + 1;
    return width >= needed;
  }

  int content_width(int width, bool gutter) const {
    const int gutter_cost = gutter ? 2 * gutter_width_ + 2 : 0;
    return std::max(0, width - gutter_cost - 1);
  }

  ftxui::Element render_row(std::size_t index, int width) {
    const DiffRow& row = rows_[index];
    const bool selected = (index == selected_row_);
    const bool match = is_match_row(index);
    ftxui::Element el = build_row_element(row, width);
    if (selected) {
      el = el | ftxui::inverted;
    } else if (match) {
      el = el | ftxui::underlined;
    }
    return el;
  }

  ftxui::Element build_row_element(const DiffRow& row, int width) {
    switch (row.kind) {
      case DiffRowKind::kFileHeader: {
        const bool collapsed = collapsed_[row.file];
        const std::string prefix = collapsed ? "▸ " : "▾ ";
        ftxui::Element text = ftxui::text(prefix + truncate(row.header, width - 2)) |
                              (ftxui::bold | to_decorator(theme_.primary));
        return text;
      }
      case DiffRowKind::kNewFileNotice:
        return ftxui::text(truncate(row.header, width)) | to_decorator(theme_.success);
      case DiffRowKind::kDeletedFileNotice:
        return ftxui::text(truncate(row.header, width)) | to_decorator(theme_.error);
      case DiffRowKind::kBinaryNotice:
        return ftxui::text(truncate(row.header, width)) | to_decorator(theme_.warning);
      case DiffRowKind::kEmptyNotice:
        return ftxui::text(truncate(row.header, width)) | to_decorator(theme_.muted);
      case DiffRowKind::kHunkHeader:
        return ftxui::text(truncate(row.header, width)) | to_decorator(theme_.muted);
      case DiffRowKind::kLine:
        return build_line_row(row, width);
    }
    return ftxui::text("");
  }

  ftxui::Element build_line_row(const DiffRow& row, int width) {
    const diff::DiffLine& line = files_[row.file].hunks[row.hunk].lines[row.line];
    const bool show_gutter = gutter_visible(width);
    const int content_w = content_width(width, show_gutter);

    ftxui::Decorator style;
    char marker = ' ';
    switch (line.type) {
      case diff::DiffLineType::kAdded:
        style = to_decorator(theme_.addition);
        marker = '+';
        break;
      case diff::DiffLineType::kDeleted:
        style = to_decorator(theme_.deletion);
        marker = '-';
        break;
      case diff::DiffLineType::kContext:
        style = ftxui::nothing;
        marker = ' ';
        break;
    }

    ftxui::Elements parts;
    if (show_gutter) {
      parts.push_back(ftxui::text(row.old_no) | to_decorator(theme_.muted));
      parts.push_back(ftxui::text(" "));
      parts.push_back(ftxui::text(row.new_no) | to_decorator(theme_.muted));
      parts.push_back(ftxui::text(" "));
    }
    parts.push_back(ftxui::text(std::string(1, marker)) | style);
    if (content_w > 0) {
      parts.push_back(ftxui::text(truncate(plain_text(line.content), content_w)) | style);
    }
    return ftxui::hbox(std::move(parts));
  }

  bool is_match_row(std::size_t index) const {
    if (matches_.empty()) return false;
    return std::binary_search(matches_.begin(), matches_.end(), index);
  }

  bool row_matches_query(std::size_t index) const {
    const DiffRow& row = rows_[index];
    if (row.kind != DiffRowKind::kLine) return false;
    const diff::DiffLine& line = files_[row.file].hunks[row.hunk].lines[row.line];
    return plain_text(line.content).find(search_query_) != std::string::npos;
  }

  void rebuild_matches() {
    matches_.clear();
    if (search_query_.empty()) return;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      if (row_matches_query(i)) matches_.push_back(i);
    }
    if (!matches_.empty()) {
      current_match_ = 0;
    }
  }

  std::size_t next_hunk_row(std::size_t from) const {
    for (std::size_t i = from; i < rows_.size(); ++i) {
      if (rows_[i].kind == DiffRowKind::kHunkHeader) return i;
    }
    return rows_.size();
  }

  std::size_t prev_hunk_row(std::size_t from) const {
    if (from == 0) return rows_.size();
    for (std::size_t i = from - 1; i < rows_.size(); --i) {
      if (rows_[i].kind == DiffRowKind::kHunkHeader) return i;
      if (i == 0) break;
    }
    return rows_.size();
  }

  void select_file_ordinal(std::size_t ordinal) {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      if (rows_[i].file_ordinal == ordinal && rows_[i].kind == DiffRowKind::kFileHeader) {
        select_row(i);
        return;
      }
    }
  }

  std::size_t row_of_file_header(std::size_t file_index) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      if (rows_[i].file == file_index && rows_[i].kind == DiffRowKind::kFileHeader) {
        return i;
      }
    }
    return 0;
  }

  void ensure_visible(std::size_t index) {
    if (index >= rows_.size()) return;
    const int height = box_height();
    if (static_cast<int>(index) < scroll_offset_) {
      scroll_offset_ = static_cast<int>(index);
    } else if (static_cast<int>(index) >= scroll_offset_ + height) {
      scroll_offset_ = static_cast<int>(index) - height + 1;
    }
    clamp_scroll();
  }

  void clamp_scroll() {
    if (rows_.empty()) {
      scroll_offset_ = 0;
      return;
    }
    const int max_scroll = std::max(0, static_cast<int>(rows_.size()) - box_height());
    scroll_offset_ = std::clamp(scroll_offset_, 0, max_scroll);
  }

  bool on_mouse_event(ftxui::Event& event) {
    const ftxui::Mouse& mouse = event.mouse();
    if (!box_.Contain(mouse.x, mouse.y)) {
      return false;
    }
    const int previous = scroll_offset_;
    if (mouse.button == ftxui::Mouse::WheelUp) {
      scroll_offset_ = std::max(0, scroll_offset_ - 3);
    } else if (mouse.button == ftxui::Mouse::WheelDown) {
      clamp_scroll();
      scroll_offset_ =
          std::min(std::max(0, static_cast<int>(rows_.size()) - box_height()), scroll_offset_ + 3);
    } else {
      return false;
    }
    clamp_scroll();
    return scroll_offset_ != previous;
  }

  int box_width() const { return std::max(1, box_.x_max - box_.x_min + 1); }
  int box_height() const { return std::max(1, box_.y_max - box_.y_min + 1); }

  std::vector<diff::DiffFile> files_;
  UnifiedDiffViewOptions options_;
  Theme theme_;
  std::vector<bool> collapsed_;
  std::vector<DiffRow> rows_;
  std::size_t hunk_header_count_ = 0;
  int gutter_width_ = 1;
  ftxui::Box box_;
  int scroll_offset_ = 0;
  std::size_t selected_row_ = 0;
  std::string search_query_;
  std::vector<std::size_t> matches_;
  std::size_t current_match_ = 0;
};

UnifiedDiffView::UnifiedDiffView(std::vector<diff::DiffFile> files, UnifiedDiffViewOptions options)
    : impl_(std::make_shared<Impl>(std::move(files), std::move(options))) {}

UnifiedDiffView::~UnifiedDiffView() = default;

ftxui::Component UnifiedDiffView::component() const { return impl_; }

void UnifiedDiffView::next_file() { impl_->next_file(); }
void UnifiedDiffView::prev_file() { impl_->prev_file(); }
void UnifiedDiffView::next_hunk() { impl_->next_hunk(); }
void UnifiedDiffView::prev_hunk() { impl_->prev_hunk(); }

void UnifiedDiffView::toggle_collapse_current_file() { impl_->toggle_collapse_current_file(); }
void UnifiedDiffView::collapse_file(std::size_t file_index) { impl_->collapse_file(file_index); }
void UnifiedDiffView::expand_file(std::size_t file_index) { impl_->expand_file(file_index); }
bool UnifiedDiffView::is_collapsed(std::size_t file_index) const {
  return impl_->is_collapsed(file_index);
}

void UnifiedDiffView::set_search(const std::string& query) { impl_->set_search(query); }
void UnifiedDiffView::jump_to_next_match() { impl_->jump_to_next_match(); }
void UnifiedDiffView::jump_to_prev_match() { impl_->jump_to_prev_match(); }
bool UnifiedDiffView::has_matches() const { return impl_->has_matches(); }
std::size_t UnifiedDiffView::match_count() const { return impl_->match_count(); }
std::optional<std::size_t> UnifiedDiffView::current_match() const { return impl_->current_match(); }

bool UnifiedDiffView::select_row(std::size_t row_index) { return impl_->select_row(row_index); }
void UnifiedDiffView::scroll_to_row(std::size_t row_index) { impl_->scroll_to_row(row_index); }
bool UnifiedDiffView::copy_selection() { return impl_->copy_selection(); }
bool UnifiedDiffView::has_selection() const { return impl_->has_selection(); }

UnifiedDiffView::Status UnifiedDiffView::status() const { return impl_->status(); }
std::size_t UnifiedDiffView::row_count() const { return impl_->row_count(); }
bool UnifiedDiffView::is_empty() const { return impl_->is_empty(); }

}  // namespace terminal_ui_kit