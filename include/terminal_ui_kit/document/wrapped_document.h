#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "terminal_ui_kit/document/streaming_document.h"

namespace terminal_ui_kit {

struct WrappedLine {
  std::string text;
  std::size_t logical_line = 0;
  std::size_t sub_line = 0;
  std::size_t byte_offset = 0;
};

class WrappedDocument {
 public:
  explicit WrappedDocument(int width = 80, int tab_width = 8);

  void rebuild_from(const StreamingDocument& doc);
  void append_from(const StreamingDocument& doc);
  void replace_tail(const StreamingDocument& doc);
  void handle_width_change(int new_width, const StreamingDocument& doc);
  void clear();

  [[nodiscard]] std::size_t display_line_count() const;
  [[nodiscard]] const WrappedLine& display_line_at(std::size_t index) const;
  [[nodiscard]] std::size_t first_display_line_for(std::size_t logical_line) const;

 private:
  void wrap_logical_line(const std::string& logical, std::size_t logical_index);

  int width_;
  int tab_width_;
  std::vector<WrappedLine> lines_;
  std::size_t last_logical_line_count_ = 0;
};

}  // namespace terminal_ui_kit