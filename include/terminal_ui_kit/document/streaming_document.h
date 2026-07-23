#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {

// Incrementally assembled UTF-8, newline-delimited text.
class StreamingDocument {
 public:
  void append(std::string_view chunk);
  void replace_tail(std::string_view tail);
  void finish();
  void clear();

  [[nodiscard]] std::size_t line_count() const { return lines_.size(); }
  [[nodiscard]] std::string_view line_at(std::size_t index) const;
  [[nodiscard]] std::uint64_t revision() const { return revision_; }

 private:
  void append_bytes(std::string_view bytes);
  void decode_pending(bool flush_incomplete);
  void finish_line();

  std::vector<std::string> lines_;
  std::string current_line_;
  std::string pending_bytes_;
  std::uint64_t revision_ = 0;
};

}  // namespace terminal_ui_kit
