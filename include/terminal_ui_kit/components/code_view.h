#pragma once

#include <cstddef>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct CodeViewOptions {
  std::string language;
  bool show_line_numbers = false;
  // Column width of the line-number gutter. Numbers wider than this render in
  // full without padding; numbers narrower are right-aligned within it.
  std::size_t line_number_width = 4;
  Theme theme = default_dark_theme();
};

ftxui::Element CodeView(std::string code, CodeViewOptions options);

}  // namespace terminal_ui_kit