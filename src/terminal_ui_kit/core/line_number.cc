#include "terminal_ui_kit/core/line_number.h"

#include <cstddef>
#include <string>

namespace terminal_ui_kit {

std::string format_line_number(std::size_t number, std::size_t width) {
  std::string num = std::to_string(number);
  if (num.size() < width) {
    num.insert(0, width - num.size(), ' ');
  }
  return num;
}

}  // namespace terminal_ui_kit
