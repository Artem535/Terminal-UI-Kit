#pragma once

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/core/text_style.h"

namespace terminal_ui_kit {

// Converts a Theme role's TextStyle into an FTXUI Decorator, so components
// can style ftxui::Element trees from a Theme (PRD section 14). FTXUI has
// no `italic` decorator (only bold/dim/underlined/underlinedDouble/blink/
// strikethrough/color/bgcolor), so TextStyle::italic has nothing to map to
// and is not represented here.
ftxui::Decorator to_decorator(const TextStyle& style);

}  // namespace terminal_ui_kit
