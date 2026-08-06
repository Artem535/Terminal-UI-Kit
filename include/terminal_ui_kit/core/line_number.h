#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

namespace terminal_ui_kit {

// Formats `number` as a right-aligned string padded to `width` columns.
//
// When the decimal representation of `number` needs more columns than
// `width`, the number is returned verbatim: it is never truncated and no
// padding is added, so a gutter width smaller than the digit count cannot
// underflow `std::size_t` or allocate an enormous padding string. A width of
// zero is safe and simply yields the bare number. The width is additionally
// clamped to a sane maximum so that an unreasonably large configured gutter
// cannot allocate a huge buffer.
inline std::string format_line_number(std::size_t number, std::size_t width) {
  constexpr std::size_t kMaxGutterWidth = 256;
  width = std::min(width, kMaxGutterWidth);
  std::string digits = std::to_string(number);
  if (digits.size() >= width) {
    return digits;
  }
  return std::string(width - digits.size(), ' ') + digits;
}

}  // namespace terminal_ui_kit
