#pragma once

#include <compare>
#include <cstddef>

namespace terminal_ui_kit {

// Zero-based line/column coordinates within a text buffer (PRD section
// 13.2). Ordered lexicographically by line, then column.
struct TextPosition {
  std::size_t line = 0;
  std::size_t column = 0;

  friend auto operator<=>(const TextPosition&, const TextPosition&) = default;
  friend bool operator==(const TextPosition&, const TextPosition&) = default;
};

}  // namespace terminal_ui_kit
