#pragma once

#include <string_view>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

class SyntaxHighlighter {
 public:
  // Highlight a code string for the given language.
  // Returns StyledText with TextStyle applied to each token.
  // If language is unknown or tree-sitter is unavailable,
  // returns StyledText with a single unstyled span.
  static StyledText highlight(
      std::string_view code,
      std::string_view language,
      const Theme& theme);
};

}  // namespace terminal_ui_kit