#pragma once

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/core/text_style.h"

namespace terminal_ui_kit {

// Converts a Theme role's TextStyle into an FTXUI Decorator, so components
// can style ftxui::Element trees from a Theme (PRD section 14). FTXUI has
// no `italic` decorator (only bold/dim/underlined/underlinedDouble/blink/
// strikethrough/color/bgcolor), so TextStyle::italic has nothing to map to
// and is not represented here.
ftxui::Decorator to_decorator(const TextStyle& style);

// Converts a StyledText sequence into an FTXUI horizontal element, applying
// each span's TextStyle via to_decorator.
ftxui::Element render_styled_text(const StyledText& text);

}  // namespace terminal_ui_kit
