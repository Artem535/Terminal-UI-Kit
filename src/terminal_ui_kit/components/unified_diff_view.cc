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

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {

namespace {

// Concatenates the plain text of every span in a StyledText. Diff lines are
// always emitted as a single span by the parser, so this is an accessor used
// by search and copy.
std::string PlainText(const StyledText& text) {
  std::string result;
  for (const TextSpan& span : text.spans()) result += span.text;
  return result;
}

// UTF-8 encoding of U+2026 HORIZONTAL ELLIPSIS, the long-line continuation
// marker. Kept as a string (not a char literal) so it survives any execution
// character set.
constexpr std::string_view kEllipsis = "\u2026";

// Truncates a byte string to at most `max_bytes`, keeping only complete UTF-8
// code points so a multi-byte character is never split. Appends a `…`
// continuation marker when truncation occurred.
std::string TruncateUtf8(std::string text, std::size_t max_bytes) {
  if (text.size() <= max_bytes) {
    return text;
  }
  if (max_bytes == 0) {
    return std::string(kEllipsis);
  }
  // Back over bytes that are UTF-8 continuation bytes (0x10xxxxxx) so the
  // truncation point never splits a multi-byte code point. A lead byte that
  // would need a continuation beyond `max_bytes` is dropped along with its
  // (already excluded) continuations, keeping only complete code points.
  std::size_t keep = max_bytes;
  while (keep > 0 && (static_cast<std::uint8_t>(text[keep]) & 0xC0) == 0x80) {
    --keep;
  }
  text.erase(keep);
  text.append(kEllipsis);
  return text;
}

std::string RightPad(std::string_view text, std::size_t width) {
  std::string out(text);
  if (out.size() < width) {
    out.append(width - out.size(), ' ');
  }
  return out;
}

std::string RightAlign(std::string_view text, std::size_t width) {
  std::string out;
  if (text.size() < width) {
    out.append(width - text.size(), ' ');
  }
  out.append(text);
  return out;
}

// The identity decorator, used where no visual emphasis is wanted (e.g.
// context rows) while still returning a valid ftxui::Decorator.
ftxui::Decorator Identity() {
  return [](ftxui::Element element) { return element; };
}

int DigitCount(int value) {
  int digits = 1;
  while (value >= 10) {
    value /= 10;
    ++digits;
  }
  return digits;
}

}  // namespace

enum class DiffRowKind { kFileHeader, kHunkHeader, kLine };

namespace {

// A flat-row descriptor. Rows only hold indices into the retained `files_`
// model (which the view owns), so no pointers, spans, or string_views outlive
// their source and no line content is ever duplicated.
struct DiffRow {
  DiffRowKind kind = DiffRowKind::kFileHeader;
  std::size_t file_index = 0;
  std::size_t hunk_index = 0;
  std::size_t line_index = 0;
};

enum class FileKind { kModified, kNew, kDeleted, kBinary };

FileKind DescribeFile(const diff::DiffFile& file) {
  if (file.old_path == "/dev/null") return FileKind::kNew;
  if (file.new_path == "/dev/null") return FileKind::kDeleted;
  if (file.hunks.empty()) return FileKind::kBinary;
  return FileKind::kModified;
}

}  // namespace

class UnifiedDiffViewImpl {
 public:
  explicit UnifiedDiffViewImpl(UnifiedDiffViewOptions options)
      : options_(std::move(options)),
        effective_theme_(options_.enable_color ? options_.theme : without_color(options_.theme)) {
    VirtualListOptions list_opts;
    list_opts.item_count = [this] { return rows_.size(); };
    list_opts.item_height = 1;
    list_opts.render_item = [this](std::size_t index, int width) {
      return render_row(index, width);
    };
    list_opts.on_select = [this](std::size_t index) { note_selection_change(index); };

    model_ = std::make_shared<VirtualListModel>(std::move(list_opts));
    auto list = model_->component();

    component_ = ftxui::Renderer(list, [this, list] {
      rebuild_if_dirty();
      if (files_.empty()) {
        return ftxui::vbox({
            ftxui::text("  (empty diff — no changes)") | ftxui::yflex,
            render_status_bar(),
        });
      }
      return ftxui::vbox({
          list->Render() | ftxui::yflex,
          render_status_bar(),
      });
    });
    component_ |= ftxui::CatchEvent([this](ftxui::Event event) { return handle_event(event); });
  }

  ftxui::Component component() const { return component_; }

  void SetFiles(std::vector<diff::DiffFile> files) {
    files_ = std::move(files);
    collapsed_.assign(files_.size(), 0);
    search_query_.clear();
    search_input_.clear();
    searching_ = false;
    search_matches_.clear();
    search_cursor_ = 0;
    resize_gutter();
    layout_dirty_ = true;
    rebuild_if_dirty();
    model_->scroll_to_index(0);
    note_selection_change(model_->selected_index().value_or(0));
  }

  const std::vector<diff::DiffFile>& files() const { return files_; }

  std::size_t file_count() const { return files_.size(); }

  std::size_t selected_file() const { return selected_file_; }

  std::size_t selected_hunk() const { return selected_hunk_; }

  std::pair<std::size_t, std::size_t> visible_range() const {
    if (const std::optional<std::pair<std::size_t, std::size_t>> range = model_->visible_range()) {
      return *range;
    }
    return {0, 0};
  }

  std::size_t layout_build_count() const { return layout_build_count_; }

  std::string status_line() const {
    if (files_.empty()) {
      return "No changes. File 0/0";
    }
    const std::size_t total_files = file_count();
    const std::size_t hunks = files_[selected_file_].hunks.size();
    const std::size_t hunk_display = (hunks == 0) ? 0 : (selected_hunk_ + 1);
    const std::pair<std::size_t, std::size_t> vis = visible_range();
    const std::size_t vis_begin = vis.first + 1;
    const std::size_t vis_end = vis.first + vis.second;

    std::string line = "File " + std::to_string(selected_file_ + 1) + "/" +
                       std::to_string(total_files) + " · Hunk " + std::to_string(hunk_display) +
                       "/" + std::to_string(hunks) + " · Visible rows " +
                       std::to_string(vis_begin) + "–" + std::to_string(vis_end);

    if (searching_) {
      line = "Search: " + search_input_ + "▍  " + line;
    } else if (!search_query_.empty()) {
      line += " · " + std::to_string(search_matches_.size()) + " match(es)";
    }
    return line;
  }

  // --- Navigation ---
  void next_hunk() {
    rebuild_if_dirty();
    const std::optional<std::size_t> sel = model_->selected_index();
    if (!sel) return;
    for (std::size_t i = *sel + 1; i < rows_.size(); ++i) {
      if (rows_[i].kind == DiffRowKind::kFileHeader) return;
      if (rows_[i].kind == DiffRowKind::kHunkHeader && rows_[i].file_index == selected_file_) {
        select_row(i);
        return;
      }
    }
  }

  void previous_hunk() {
    rebuild_if_dirty();
    const std::optional<std::size_t> sel = model_->selected_index();
    if (!sel) return;
    for (std::size_t i = *sel; i > 0; --i) {
      if (rows_[i - 1].kind == DiffRowKind::kFileHeader) return;
      if (rows_[i - 1].kind == DiffRowKind::kHunkHeader &&
          rows_[i - 1].file_index == selected_file_) {
        select_row(i - 1);
        return;
      }
    }
  }

  void next_file() {
    rebuild_if_dirty();
    const std::optional<std::size_t> sel = model_->selected_index();
    if (!sel) return;
    for (std::size_t i = *sel + 1; i < rows_.size(); ++i) {
      if (rows_[i].kind == DiffRowKind::kFileHeader) {
        select_row(i);
        return;
      }
    }
  }

  void previous_file() {
    rebuild_if_dirty();
    const std::optional<std::size_t> sel = model_->selected_index();
    if (!sel) return;
    for (std::size_t i = *sel; i > 0; --i) {
      if (rows_[i - 1].kind == DiffRowKind::kFileHeader) {
        select_row(i - 1);
        return;
      }
    }
  }

  void toggle_collapse() {
    rebuild_if_dirty();
    if (selected_file_ >= files_.size()) return;
    const std::size_t file = selected_file_;
    // Preserve the logical position so expanding can restore it.
    const DiffRow* current = selected_row_ptr();
    const std::size_t anchor_hunk = current ? current->hunk_index : 0;
    const bool anchor_line = current && current->kind == DiffRowKind::kLine;
    const bool was_header = current && current->kind == DiffRowKind::kFileHeader;
    const std::size_t anchor_line_index = current ? current->line_index : 0;

    collapsed_[file] = collapsed_[file] ? 0 : 1;
    layout_dirty_ = true;
    rebuild_if_dirty();

    // Re-anchor selection on the equivalent row. A file anchored on its header
    // (either collapsed to it, or expanded while on it) stays on the header.
    const std::optional<std::size_t> sel = model_->selected_index();
    std::size_t target = *sel;
    if (collapsed_[file] || was_header) {
      target = first_row_of_file(file);
    } else if (anchor_line) {
      target = find_row(file, anchor_hunk, anchor_line_index);
    } else {
      target = find_row(file, anchor_hunk, 0);
    }
    if (target < rows_.size()) {
      select_row(target);
    }
  }

  bool collapsed(std::size_t file_index) const {
    if (file_index >= collapsed_.size()) return false;
    return collapsed_[file_index] != 0;
  }

  // --- Search ---
  void set_search(const std::string& query) {
    search_query_ = query;
    rebuild_if_dirty();
    recompute_search_matches();
    search_cursor_ = 0;
  }

  const std::string& search() const { return search_query_; }

  std::size_t search_result_count() const { return search_matches_.size(); }

  void next_search_result() {
    rebuild_if_dirty();
    if (search_matches_.empty()) return;
    search_cursor_ = (search_cursor_ + 1) % search_matches_.size();
    select_row(search_matches_[search_cursor_]);
  }

  void previous_search_result() {
    rebuild_if_dirty();
    if (search_matches_.empty()) return;
    search_cursor_ = (search_cursor_ + search_matches_.size() - 1) % search_matches_.size();
    select_row(search_matches_[search_cursor_]);
  }

  // --- Copy ---
  void copy_selected() {
    if (!options_.on_copy) return;
    const DiffRow* row = selected_row_ptr();
    if (row && row->kind == DiffRowKind::kLine) {
      const diff::DiffLine& line =
          files_[row->file_index].hunks[row->hunk_index].lines[row->line_index];
      options_.on_copy(PlainText(line.content));
      return;
    }
    options_.on_copy(std::string{});
  }

 private:
  void resize_gutter() {
    int old_digits = 1;
    int new_digits = 1;
    for (const diff::DiffFile& file : files_) {
      for (const diff::DiffHunk& hunk : file.hunks) {
        for (const diff::DiffLine& line : hunk.lines) {
          if (line.old_line) old_digits = std::max(old_digits, DigitCount(*line.old_line));
          if (line.new_line) new_digits = std::max(new_digits, DigitCount(*line.new_line));
        }
      }
    }
    old_digits_ = old_digits;
    new_digits_ = new_digits;
  }

  void rebuild_if_dirty() {
    if (!layout_dirty_) return;
    build_rows();
    recompute_search_matches();
    layout_dirty_ = false;
  }

  void build_rows() {
    rows_.clear();
    rows_.reserve(file_count());
    for (std::size_t f = 0; f < files_.size(); ++f) {
      rows_.push_back(DiffRow{DiffRowKind::kFileHeader, f, 0, 0});
      if (collapsed_[f]) continue;
      const diff::DiffFile& file = files_[f];
      for (std::size_t h = 0; h < file.hunks.size(); ++h) {
        rows_.push_back(DiffRow{DiffRowKind::kHunkHeader, f, h, 0});
        const diff::DiffHunk& hunk = file.hunks[h];
        for (std::size_t l = 0; l < hunk.lines.size(); ++l) {
          rows_.push_back(DiffRow{DiffRowKind::kLine, f, h, l});
        }
      }
    }
    ++layout_build_count_;
  }

  void recompute_search_matches() {
    search_matches_.clear();
    if (search_query_.empty()) return;
    std::string needle = LowerAscii(search_query_);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      if (rows_[i].kind != DiffRowKind::kLine) continue;
      const diff::DiffLine& line =
          files_[rows_[i].file_index].hunks[rows_[i].hunk_index].lines[rows_[i].line_index];
      if (LowerAscii(PlainText(line.content)).find(needle) != std::string::npos) {
        search_matches_.push_back(i);
      }
    }
    // Keep the cursor in range when the match set shrinks (e.g. after a
    // collapse rebuilds the row index).
    if (search_matches_.empty()) {
      search_cursor_ = 0;
    } else {
      search_cursor_ = std::min(search_cursor_, search_matches_.size() - 1);
    }
  }

  static std::string LowerAscii(std::string s) {
    for (char& c : s) {
      if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
  }

  void select_row(std::size_t index) {
    if (index >= rows_.size()) return;
    model_->select_index(index);
    note_selection_change(index);
  }

  void note_selection_change(std::size_t index) {
    selected_row_ = index;
    if (index >= rows_.size()) return;
    const DiffRow& row = rows_[index];
    selected_file_ = row.file_index;
    selected_hunk_ = (row.kind == DiffRowKind::kLine || row.kind == DiffRowKind::kHunkHeader)
                         ? row.hunk_index
                         : 0;
  }

  const DiffRow* selected_row_ptr() const {
    if (selected_row_ >= rows_.size()) return nullptr;
    return &rows_[selected_row_];
  }

  std::size_t first_row_of_file(std::size_t file) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      if (rows_[i].file_index == file && rows_[i].kind == DiffRowKind::kFileHeader) {
        return i;
      }
    }
    return rows_.size();
  }

  std::size_t find_row(std::size_t file, std::size_t hunk, std::size_t line) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      const DiffRow& row = rows_[i];
      if (row.file_index != file) continue;
      if (row.hunk_index != hunk) continue;
      if (row.kind == DiffRowKind::kHunkHeader) return i;
      if (row.kind == DiffRowKind::kLine && row.line_index == line) return i;
    }
    return rows_.size();
  }

  bool handle_event(ftxui::Event event) {
    if (searching_) {
      return handle_search_input(event);
    }

    if (event == ftxui::Event::Character('/')) {
      begin_search();
      return true;
    }
    if (event == ftxui::Event::Character('y')) {
      copy_selected();
      return true;
    }
    if (event == ftxui::Event::Character('n') || event == ftxui::Event::Character('N')) {
      if (!search_query_.empty()) {
        if (event.input() == "n") {
          next_search_result();
        } else {
          previous_search_result();
        }
      } else if (event.input() == "n") {
        next_hunk();
      } else {
        previous_hunk();
      }
      return true;
    }
    if (event == ftxui::Event::Character(']')) {
      next_file();
      return true;
    }
    if (event == ftxui::Event::Character('[')) {
      previous_file();
      return true;
    }
    if (event == ftxui::Event::Return) {
      toggle_collapse();
      return true;
    }
    if (event == ftxui::Event::Character('j')) {
      move_selection(1);
      return true;
    }
    if (event == ftxui::Event::Character('k')) {
      move_selection(-1);
      return true;
    }
    return false;
  }

  void begin_search() {
    search_input_.clear();
    searching_ = true;
    set_search("");
  }

  bool handle_search_input(ftxui::Event event) {
    if (event == ftxui::Event::Escape || event == ftxui::Event::Character('/')) {
      cancel_search();
      return true;
    }
    if (event == ftxui::Event::Return) {
      commit_search();
      return true;
    }
    if (event == ftxui::Event::Backspace) {
      if (!search_input_.empty()) {
        search_input_.pop_back();
        set_search(search_input_);
      }
      return true;
    }
    if (event.is_character()) {
      const std::string& input = event.input();
      // Accept printable single characters only (skip control sequences).
      if (input.size() == 1 && static_cast<unsigned char>(input[0]) >= 0x20) {
        search_input_.append(input);
        set_search(search_input_);
      }
      return true;
    }
    // Keep navigation working while typing.
    if (event == ftxui::Event::ArrowUp) return false;
    if (event == ftxui::Event::ArrowDown) return false;
    return true;
  }

  void commit_search() {
    searching_ = false;
    if (!search_matches_.empty()) {
      select_row(search_matches_[search_cursor_]);
    }
  }

  void cancel_search() {
    searching_ = false;
    set_search("");
  }

  void move_selection(std::ptrdiff_t delta) {
    const std::optional<std::size_t> sel = model_->selected_index();
    if (!sel) return;
    const std::ptrdiff_t target = static_cast<std::ptrdiff_t>(*sel) + delta;
    if (target < 0 || static_cast<std::size_t>(target) >= rows_.size()) return;
    select_row(static_cast<std::size_t>(target));
  }

  ftxui::Element render_status_bar() const {
    ftxui::Decorator style = Identity();
    if (options_.enable_color) {
      style = to_decorator(effective_theme().muted);
    }
    return ftxui::text(status_line()) | style;
  }

  ftxui::Element render_row(std::size_t index, int width) {
    if (index >= rows_.size()) {
      return ftxui::text("");
    }
    const DiffRow& row = rows_[index];
    switch (row.kind) {
      case DiffRowKind::kFileHeader:
        return render_file_header(row, width);
      case DiffRowKind::kHunkHeader:
        return render_hunk_header(row, width);
      case DiffRowKind::kLine:
        return render_line(row, width);
    }
    return ftxui::text("");
  }

  ftxui::Element render_file_header(const DiffRow& row, int width) {
    const diff::DiffFile& file = files_[row.file_index];
    const std::string collapse_glyph = collapsed_[row.file_index] ? "▸ " : "▾ ";
    const std::string badge = FileBadge(file);

    std::string text = collapse_glyph + badge + file.new_path;
    if (file.new_path != file.old_path && !IsDevNull(file.new_path) && !IsDevNull(file.old_path)) {
      text += "  ← " + file.old_path;
    }
    text = TruncateUtf8(std::move(text), ContentWidth(width));
    ftxui::Decorator style = to_decorator(effective_theme().primary);
    if (collapsed_[row.file_index] && options_.enable_color) {
      style = to_decorator(effective_theme().secondary);
    }
    return ftxui::text(RightPad(text, static_cast<std::size_t>(std::max(0, width)))) | style;
  }

  ftxui::Element render_hunk_header(const DiffRow& row, int width) {
    const std::string header = files_[row.file_index].hunks[row.hunk_index].header;
    return ftxui::text(TruncateUtf8(header, ContentWidth(width))) |
           to_decorator(effective_theme().secondary);
  }

  ftxui::Element render_line(const DiffRow& row, int width) {
    const diff::DiffLine& line = files_[row.file_index].hunks[row.hunk_index].lines[row.line_index];

    const int viewport = std::max(0, width);
    const int old_width = GutterUnit(old_digits_);
    const int new_width = GutterUnit(new_digits_);
    // Columns consumed before the content: old number + space + new number +
    // space + marker. `gutter` is only meaningful when line numbers are shown.
    const int gutter = old_width + new_width + 3;
    const bool narrow = (viewport - gutter) < 6;

    std::string old_cell;
    std::string new_cell;
    char marker = ' ';
    std::size_t content_width = ContentWidth(viewport - 1);  // marker column
    if (narrow) {
      // Narrow-terminal fallback: drop line numbers, keep only the marker.
      marker = DiffMarker(line.type);
    } else {
      old_cell = line.old_line ? RightAlign(std::to_string(*line.old_line),
                                            static_cast<std::size_t>(old_width))
                               : std::string(static_cast<std::size_t>(old_width), ' ');
      new_cell = line.new_line ? RightAlign(std::to_string(*line.new_line),
                                            static_cast<std::size_t>(new_width))
                               : std::string(static_cast<std::size_t>(new_width), ' ');
      marker = DiffMarker(line.type);
      // Content area is the viewport minus the number/marker gutter.
      content_width = ContentWidth(viewport - gutter);
    }

    ftxui::Elements parts;
    if (!narrow) {
      parts.push_back(ftxui::text(old_cell) | to_decorator(effective_theme().muted));
      parts.push_back(ftxui::text(" "));
      parts.push_back(ftxui::text(new_cell) | to_decorator(effective_theme().muted));
      parts.push_back(ftxui::text(" "));
    }
    parts.push_back(ftxui::text(std::string(1, marker)) | MarkerStyle(line.type));
    // Reserve one column for the inline "…" continuation marker so it stays
    // visible inside the viewport rather than being clipped off the edge.
    const std::size_t text_budget = content_width > 0 ? content_width - 1 : 0;
    parts.push_back(ftxui::text(LineContentTruncated(line, text_budget)) | LineStyle(line.type));

    return ftxui::hbox(std::move(parts));
  }

  std::string LineContentTruncated(const diff::DiffLine& line, std::size_t content_width) const {
    return TruncateUtf8(PlainText(line.content), content_width);
  }

  ftxui::Decorator LineStyle(diff::DiffLineType type) const {
    const Theme& theme = effective_theme();
    switch (type) {
      case diff::DiffLineType::kAdded:
        return to_decorator(theme.addition);
      case diff::DiffLineType::kDeleted:
        return to_decorator(theme.deletion);
      case diff::DiffLineType::kContext:
        return Identity();
    }
    return Identity();
  }

  ftxui::Decorator MarkerStyle(diff::DiffLineType type) const {
    const Theme& theme = effective_theme();
    switch (type) {
      case diff::DiffLineType::kAdded:
        return to_decorator(theme.addition);
      case diff::DiffLineType::kDeleted:
        return to_decorator(theme.deletion);
      case diff::DiffLineType::kContext:
        return Identity();
    }
    return Identity();
  }

  static char DiffMarker(diff::DiffLineType type) {
    switch (type) {
      case diff::DiffLineType::kAdded:
        return '+';
      case diff::DiffLineType::kDeleted:
        return '-';
      case diff::DiffLineType::kContext:
        return ' ';
    }
    return ' ';
  }

  const Theme& effective_theme() const { return effective_theme_; }

  static bool IsDevNull(const std::string& path) { return path == "/dev/null"; }

  std::string FileBadge(const diff::DiffFile& file) const {
    switch (DescribeFile(file)) {
      case FileKind::kNew:
        return "[new] ";
      case FileKind::kDeleted:
        return "[del] ";
      case FileKind::kBinary:
        return "[bin] ";
      case FileKind::kModified:
        return "";
    }
    return "";
  }

  static int GutterUnit(int digits) { return std::max(1, digits); }

  static std::size_t ContentWidth(int width) {
    return static_cast<std::size_t>(std::max(0, width));
  }

  UnifiedDiffViewOptions options_;
  Theme effective_theme_;
  std::vector<diff::DiffFile> files_;
  std::vector<std::uint8_t> collapsed_;
  std::vector<DiffRow> rows_;
  bool layout_dirty_ = true;
  std::size_t layout_build_count_ = 0;
  std::shared_ptr<VirtualListModel> model_;
  ftxui::Component component_;
  int old_digits_ = 1;
  int new_digits_ = 1;
  std::size_t selected_row_ = 0;
  std::size_t selected_file_ = 0;
  std::size_t selected_hunk_ = 0;
  std::string search_query_;
  std::string search_input_;
  bool searching_ = false;
  std::vector<std::size_t> search_matches_;
  std::size_t search_cursor_ = 0;
};

UnifiedDiffView::UnifiedDiffView(UnifiedDiffViewOptions options)
    : impl_(std::make_shared<UnifiedDiffViewImpl>(std::move(options))) {}

ftxui::Component UnifiedDiffView::component() const { return impl_->component(); }

void UnifiedDiffView::SetFiles(std::vector<diff::DiffFile> files) {
  impl_->SetFiles(std::move(files));
}

const std::vector<diff::DiffFile>& UnifiedDiffView::files() const { return impl_->files(); }

std::size_t UnifiedDiffView::file_count() const { return impl_->file_count(); }

std::size_t UnifiedDiffView::selected_file() const { return impl_->selected_file(); }

std::size_t UnifiedDiffView::selected_hunk() const { return impl_->selected_hunk(); }

std::pair<std::size_t, std::size_t> UnifiedDiffView::visible_range() const {
  return impl_->visible_range();
}

std::size_t UnifiedDiffView::layout_build_count() const { return impl_->layout_build_count(); }

std::string UnifiedDiffView::status_line() const { return impl_->status_line(); }

void UnifiedDiffView::next_hunk() { impl_->next_hunk(); }
void UnifiedDiffView::previous_hunk() { impl_->previous_hunk(); }
void UnifiedDiffView::next_file() { impl_->next_file(); }
void UnifiedDiffView::previous_file() { impl_->previous_file(); }
void UnifiedDiffView::toggle_collapse() { impl_->toggle_collapse(); }
bool UnifiedDiffView::collapsed(std::size_t file_index) const {
  return impl_->collapsed(file_index);
}
void UnifiedDiffView::set_search(const std::string& query) { impl_->set_search(query); }
const std::string& UnifiedDiffView::search() const { return impl_->search(); }
std::size_t UnifiedDiffView::search_result_count() const { return impl_->search_result_count(); }
void UnifiedDiffView::next_search_result() { impl_->next_search_result(); }
void UnifiedDiffView::previous_search_result() { impl_->previous_search_result(); }
void UnifiedDiffView::copy_selected() { impl_->copy_selected(); }

}  // namespace terminal_ui_kit