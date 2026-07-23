#include "terminal_ui_kit/document/wrapped_document.h"

#include <algorithm>
#include <stdexcept>

#include "terminal_ui_kit/core/text_wrap.h"

namespace terminal_ui_kit {

WrappedDocument::WrappedDocument(int width, int tab_width)
    : width_(std::max(1, width)), tab_width_(std::max(1, tab_width)) {}

void WrappedDocument::rebuild_from(const StreamingDocument& doc) {
  lines_.clear();
  last_logical_line_count_ = 0;
  for (std::size_t index = 0; index < doc.line_count(); ++index) {
    wrap_logical_line(std::string(doc.line_at(index)), index);
  }
  last_logical_line_count_ = doc.line_count();
}

void WrappedDocument::append_from(const StreamingDocument& doc) {
  const std::size_t count = doc.line_count();
  if (count <= last_logical_line_count_) {
    return;
  }
  for (std::size_t index = last_logical_line_count_; index < count; ++index) {
    wrap_logical_line(std::string(doc.line_at(index)), index);
  }
  last_logical_line_count_ = count;
}

void WrappedDocument::replace_tail(const StreamingDocument& doc) {
  const std::size_t count = doc.line_count();
  if (count == 0) {
    return;
  }
  const std::size_t logical_index = count - 1;
  while (!lines_.empty() && lines_.back().logical_line == logical_index) {
    lines_.pop_back();
  }
  wrap_logical_line(std::string(doc.line_at(logical_index)), logical_index);
  last_logical_line_count_ = count;
}

void WrappedDocument::handle_width_change(int new_width, const StreamingDocument& doc) {
  width_ = std::max(1, new_width);
  rebuild_from(doc);
}

void WrappedDocument::clear() {
  lines_.clear();
  last_logical_line_count_ = 0;
}

std::size_t WrappedDocument::display_line_count() const { return lines_.size(); }

const WrappedLine& WrappedDocument::display_line_at(std::size_t index) const {
  if (index >= lines_.size()) {
    throw std::out_of_range("WrappedDocument::display_line_at");
  }
  return lines_[index];
}

std::size_t WrappedDocument::first_display_line_for(std::size_t logical_line) const {
  if (lines_.empty()) {
    return 0;
  }
  auto it = std::lower_bound(
      lines_.begin(), lines_.end(), logical_line,
      [](const WrappedLine& line, std::size_t target) { return line.logical_line < target; });
  return static_cast<std::size_t>(it - lines_.begin());
}

void WrappedDocument::wrap_logical_line(const std::string& logical, std::size_t logical_index) {
  std::vector<WrappedSegment> wrapped = wrap_plain_text_with_offsets(logical, width_);
  if (wrapped.empty()) {
    lines_.push_back({std::string{}, logical_index, 0, 0});
    return;
  }
  for (std::size_t sub = 0; sub < wrapped.size(); ++sub) {
    lines_.push_back(
        {std::move(wrapped[sub].first), logical_index, sub, wrapped[sub].second});
  }
}

}  // namespace terminal_ui_kit