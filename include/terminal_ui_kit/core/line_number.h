#pragma once

#include <cstddef>
#include <string>

namespace terminal_ui_kit {

// Formats `number` for a line-number gutter that is `width` columns wide.
// The number is right-aligned with leading spaces when it is shorter than
// `width`. When the digit count equals or exceeds `width` (including a width
// of zero) the number is returned unchanged, so valid long line numbers are
// never truncated and no unsigned underflow can occur.
std::string format_line_number(std::size_t number, std::size_t width);

}  // namespace terminal_ui_kit
