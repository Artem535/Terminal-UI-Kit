#pragma once

#include <string_view>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {

StyledText parse_ansi(std::string_view text);

}  // namespace terminal_ui_kit
