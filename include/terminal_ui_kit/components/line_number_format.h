#pragma once

#include <cstddef>
#include <string>

namespace terminal_ui_kit {

// Formats a 1-based line number for a line-number gutter, right-aligned to
// `width` columns with leading spaces. A number with more digits than `width`
// is returned in full with no padding (never truncated) and never causes the
// subtraction to underflow — padding is only applied when `num.size() < width`,
// so `width - num.size()` is always non-negative. A `width` of zero is safe and
// returns the bare number.
inline std::string format_line_number(std::size_t line_number, std::size_t width) {
  std::string num = std::to_string(line_number);
  if (num.size() < width) {
    num = std::string(width - num.size(), ' ') + num;
  }
  return num;
}

}  // namespace terminal_ui_kit