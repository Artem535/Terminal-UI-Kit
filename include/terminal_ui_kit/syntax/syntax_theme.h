#pragma once

#include "terminal_ui_kit/core/text_style.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Semantic roles used by Tree-sitter syntax highlighting. Syntax styles are
// foreground-only so they do not introduce background blocks into code views.
struct SyntaxTheme {
  TextStyle keyword;
  TextStyle type;
  TextStyle function;
  TextStyle variable;
  TextStyle string;
  TextStyle number;
  TextStyle comment;
  TextStyle operator_style;
  TextStyle property;
  TextStyle namespace_style;
  TextStyle macro;
  TextStyle constant;
};

SyntaxTheme default_dark_syntax_theme(const Theme& theme);
SyntaxTheme default_light_syntax_theme(const Theme& theme);

}  // namespace terminal_ui_kit
