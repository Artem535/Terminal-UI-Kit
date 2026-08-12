#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace terminal_ui_kit {

// Right-aligns `text` inside a `width`-column field by left-padding it with
// ASCII spaces. When `text` is longer than `width` (including `width == 0`)
// the text is returned unchanged: it is never truncated and no padding is
// added. This is the safe replacement for the common
// `std::string(width - text.size(), ' ')` idiom, which underflows to a huge
// allocation when the text is longer than the field.
inline std::string pad_left_to_width(std::string_view text, std::size_t width) {
  if (text.size() >= width) {
    return std::string(text);
  }
  // Single allocation: reserve the full field, then pad and append.
  std::string out;
  out.reserve(width);
  out.append(width - text.size(), ' ');
  out.append(text);
  return out;
}

}  // namespace terminal_ui_kit
