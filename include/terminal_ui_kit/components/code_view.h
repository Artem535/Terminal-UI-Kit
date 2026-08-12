#pragma once

#include <cstddef>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct CodeViewOptions {
  std::string language;
  bool show_line_numbers = false;
  Theme theme = default_dark_theme();
  // Width of the line-number gutter in columns. Line numbers longer than this
  // are rendered in full without truncation and without any padding; shorter
  // numbers are right-aligned within the gutter. A value of 0 is handled
  // safely (numbers render unpadded). Placed last to preserve existing
  // aggregate initialization of the earlier members.
  std::size_t gutter_width = 4;
};

ftxui::Element CodeView(std::string code, CodeViewOptions options);

}  // namespace terminal_ui_kit