#pragma once

#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct CodeViewOptions {
  std::string language;
  bool show_line_numbers = false;
  Theme theme = default_dark_theme();
};

ftxui::Element CodeView(std::string code, CodeViewOptions options);

}  // namespace terminal_ui_kit