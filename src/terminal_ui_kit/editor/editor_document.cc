#include "terminal_ui_kit/editor/editor_document.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace terminal_ui_kit {
namespace {

constexpr char kWhitespace0 = ' ';
constexpr char kWhitespace1 = '\t';

bool IsContinuationByte(unsigned char b) { return (b & 0xC0U) == 0x80U; }

bool IsWordChar(char c) {
  const unsigned char u = static_cast<unsigned char>(c);
  return !(u == static_cast<unsigned char>(kWhitespace0) ||
           u == static_cast<unsigned char>(kWhitespace1));
}

}  // namespace

EditorDocument::EditorDocument() = default;

EditorDocument::EditorDocument(std::vector<std::string> lines) {
  lines_ = std::move(lines);
  normalize();
}

const std::string& EditorDocument::line(std::size_t index) const {
  return lines_.at(std::min(index, lines_.size() - 1));
}

const std::vector<std::string>& EditorDocument::lines() const { return lines_; }

std::size_t EditorDocument::line_count() const { return lines_.size(); }

std::string EditorDocument::text() const {
  std::string result;
  const std::size_t n = lines_.size();
  result.reserve(static_cast<std::size_t>(64) * n);
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      result.push_back('\n');
    }
    result += lines_[i];
  }
  return result;
}

TextPosition EditorDocument::cursor() const { return cursor_; }

std::size_t EditorDocument::cursor_line() const { return cursor_.line; }

std::size_t EditorDocument::cursor_column() const { return cursor_.column; }

void EditorDocument::set_cursor(TextPosition position) {
  cursor_ = position;
  normalize();
  update_preferred_column();
  ensure_cursor_visible();
}

void EditorDocument::set_text(std::string text) {
  std::vector<std::string> new_lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t nl = text.find('\n', start);
    if (nl == std::string_view::npos) {
      new_lines.emplace_back(text.substr(start));
      break;
    }
    new_lines.emplace_back(text.substr(start, nl - start));
    start = nl + 1;
  }
  if (new_lines.empty()) {
    new_lines.emplace_back();
  }
  lines_ = std::move(new_lines);
  cursor_ = {0, 0};
  preferred_column_ = 0;
  scroll_top_ = 0;
  scroll_left_ = 0;
  ++revision_;
  normalize();
}

void EditorDocument::set_lines(std::vector<std::string> lines) {
  lines_ = std::move(lines);
  if (lines_.empty()) {
    lines_.emplace_back();
  }
  cursor_ = {0, 0};
  preferred_column_ = 0;
  scroll_top_ = 0;
  scroll_left_ = 0;
  ++revision_;
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::clear() { set_lines({""}); }

void EditorDocument::insert_text(std::string_view text) {
  if (text.empty()) {
    return;
  }
  std::string& current = lines_[cursor_.line];
  const std::size_t col = cursor_.column;

  // Split the pasted text on newlines.
  std::vector<std::string_view> segments;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t nl = text.find('\n', start);
    if (nl == std::string_view::npos) {
      segments.push_back(text.substr(start));
      break;
    }
    segments.push_back(text.substr(start, nl - start));
    start = nl + 1;
  }

  const std::string prefix = current.substr(0, col);
  const std::string tail = current.substr(col);

  if (segments.size() == 1) {
    // No newline: single splice into the current line.
    current = prefix;
    current += segments[0];
    current += tail;
    cursor_.column = col + segments[0].size();
    update_preferred_column();
    normalize();
    ensure_cursor_visible();
    ++revision_;
    return;
  }

  // Multi-line insertion. Build the new line block in one pass (O(n)).
  std::vector<std::string> block;
  block.reserve(segments.size() + 1);
  std::string first_line = prefix;
  first_line += segments[0];
  block.push_back(std::move(first_line));
  for (std::size_t i = 1; i + 1 < segments.size(); ++i) {
    block.emplace_back(segments[i]);
  }
  std::string last_line;
  last_line += segments.back();
  last_line += tail;
  block.push_back(std::move(last_line));

  std::vector<std::string> rebuilt;
  rebuilt.reserve(lines_.size() + block.size());
  rebuilt.insert(rebuilt.end(), lines_.begin(),
                 lines_.begin() + static_cast<std::ptrdiff_t>(cursor_.line));
  rebuilt.insert(rebuilt.end(), block.begin(), block.end());
  rebuilt.insert(rebuilt.end(), lines_.begin() + static_cast<std::ptrdiff_t>(cursor_.line) + 1,
                 lines_.end());
  lines_ = std::move(rebuilt);

  cursor_.line += segments.size() - 1;
  cursor_.column = segments.back().size();
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
  ++revision_;
}

void EditorDocument::insert_char(char c) { insert_text(std::string_view(&c, 1)); }

void EditorDocument::insert_newline() {
  std::string& current = lines_[cursor_.line];
  const std::size_t col = cursor_.column;
  std::string tail = current.substr(col);
  current.resize(col);
  lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(cursor_.line) + 1, std::move(tail));
  ++cursor_.line;
  cursor_.column = 0;
  preferred_column_ = 0;
  normalize();
  ensure_cursor_visible();
  ++revision_;
}

void EditorDocument::delete_backward() {
  if (cursor_.column > 0) {
    std::string& current = lines_[cursor_.line];
    const std::size_t prev = previous_code_point(current, cursor_.column);
    current.erase(prev, cursor_.column - prev);
    cursor_.column = prev;
    update_preferred_column();
    normalize();
    ensure_cursor_visible();
    ++revision_;
    return;
  }
  if (cursor_.line > 0) {
    // Join with the previous line: append the current line to the one above,
    // leave the cursor at the join point (start of the old line).
    std::string& prev_line = lines_[cursor_.line - 1];
    const std::size_t join_col = prev_line.size();
    prev_line += lines_[cursor_.line];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(cursor_.line));
    --cursor_.line;
    cursor_.column = join_col;
    preferred_column_ = join_col;
    normalize();
    ensure_cursor_visible();
    ++revision_;
  }
}

void EditorDocument::delete_forward() {
  std::string& current = lines_[cursor_.line];
  if (cursor_.column < current.size()) {
    const std::size_t next = next_code_point(current, cursor_.column);
    current.erase(cursor_.column, next - cursor_.column);
    normalize();
    ensure_cursor_visible();
    ++revision_;
    return;
  }
  if (cursor_.line + 1 < lines_.size()) {
    // Join with the next line at the end of the current line.
    std::string& next_line = lines_[cursor_.line + 1];
    current += next_line;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(cursor_.line) + 1);
    normalize();
    ensure_cursor_visible();
    ++revision_;
  }
}

void EditorDocument::move_left() {
  const std::string& current = lines_[cursor_.line];
  cursor_.column = previous_code_point(current, cursor_.column);
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_right() {
  const std::string& current = lines_[cursor_.line];
  cursor_.column = next_code_point(current, cursor_.column);
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_up() {
  if (cursor_.line == 0) {
    move_home();
    return;
  }
  --cursor_.line;
  apply_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_down() {
  if (cursor_.line + 1 >= lines_.size()) {
    move_end();
    return;
  }
  ++cursor_.line;
  apply_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_home() {
  cursor_.column = 0;
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_end() {
  cursor_.column = lines_[cursor_.line].size();
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_word_left() {
  const std::string& current = lines_[cursor_.line];
  std::size_t pos = cursor_.column;
  while (pos > 0 && !IsWordChar(current[previous_code_point(current, pos)])) {
    pos = previous_code_point(current, pos);
  }
  while (pos > 0 && IsWordChar(current[previous_code_point(current, pos)])) {
    pos = previous_code_point(current, pos);
  }
  cursor_.column = pos;
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::move_word_right() {
  const std::string& current = lines_[cursor_.line];
  std::size_t pos = cursor_.column;
  while (pos < current.size() && !IsWordChar(current[pos])) {
    pos = next_code_point(current, pos);
  }
  while (pos < current.size() && IsWordChar(current[pos])) {
    pos = next_code_point(current, pos);
  }
  cursor_.column = pos;
  update_preferred_column();
  normalize();
  ensure_cursor_visible();
}

void EditorDocument::set_viewport_size(std::size_t width, std::size_t height) {
  if (width == 0) {
    width = 1;
  }
  if (height == 0) {
    height = 1;
  }
  viewport_width_ = width;
  viewport_height_ = height;
  ensure_cursor_visible();
}

std::size_t EditorDocument::viewport_width() const { return viewport_width_; }

std::size_t EditorDocument::viewport_height() const { return viewport_height_; }

std::size_t EditorDocument::scroll_top() const { return scroll_top_; }

std::size_t EditorDocument::scroll_left() const { return scroll_left_; }

void EditorDocument::scroll_to_cursor() { ensure_cursor_visible(); }

std::size_t EditorDocument::preferred_column() const { return preferred_column_; }

std::uint64_t EditorDocument::revision() const { return revision_; }

// --- private helpers -------------------------------------------------------

void EditorDocument::normalize() {
  // Cursor line.
  if (lines_.empty()) {
    lines_.emplace_back();
  }
  if (cursor_.line >= lines_.size()) {
    cursor_.line = lines_.size() - 1;
  }
  // Cursor column: clamp to the line length and to a code-point boundary.
  const std::size_t len = lines_[cursor_.line].size();
  cursor_.column = clamp_to_codepoint(lines_[cursor_.line], std::min(cursor_.column, len));
}

void EditorDocument::apply_preferred_column() {
  const std::string& current = lines_[cursor_.line];
  cursor_.column = clamp_to_codepoint(current, std::min(preferred_column_, current.size()));
}

void EditorDocument::ensure_cursor_visible() {
  // Vertical.
  if (cursor_.line < scroll_top_) {
    scroll_top_ = cursor_.line;
  }
  if (viewport_height_ > 0 && cursor_.line >= scroll_top_ + viewport_height_) {
    scroll_top_ = cursor_.line - viewport_height_ + 1;
  }
  // Horizontal.
  if (cursor_.column < scroll_left_) {
    scroll_left_ = cursor_.column;
  }
  if (viewport_width_ > 0 && cursor_.column >= scroll_left_ + viewport_width_) {
    const std::string& current = lines_[cursor_.line];
    const std::size_t target =
        cursor_.column >= viewport_width_ ? cursor_.column - viewport_width_ + 1 : 0;
    scroll_left_ = round_up_to_code_point(current, target);
  }
}

void EditorDocument::update_preferred_column() { preferred_column_ = cursor_.column; }

// static
std::size_t EditorDocument::clamp_to_codepoint(const std::string& line, std::size_t column) {
  while (column > 0 && IsContinuationByte(static_cast<unsigned char>(line[column]))) {
    --column;
  }
  return column;
}

// static
std::size_t EditorDocument::previous_code_point(const std::string& line, std::size_t column) {
  if (column == 0) {
    return 0;
  }
  std::size_t pos = column - 1;
  while (pos > 0 && IsContinuationByte(static_cast<unsigned char>(line[pos]))) {
    --pos;
  }
  return pos;
}

// static
std::size_t EditorDocument::next_code_point(const std::string& line, std::size_t column) {
  if (column >= line.size()) {
    return line.size();
  }
  std::size_t pos = column + 1;
  while (pos < line.size() && IsContinuationByte(static_cast<unsigned char>(line[pos]))) {
    ++pos;
  }
  return pos;
}

// static
std::size_t EditorDocument::round_up_to_code_point(const std::string& line, std::size_t column) {
  if (column >= line.size()) {
    return line.size();
  }
  while (column < line.size() && IsContinuationByte(static_cast<unsigned char>(line[column]))) {
    ++column;
  }
  return column;
}

}  // namespace terminal_ui_kit
