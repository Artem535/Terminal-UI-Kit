#pragma once

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// One key binding and the action it performs, e.g. {"esc", "cancel"} (PRD
// section 41). Callers supply semantic bindings, not a preformatted string --
// KeyHintBar owns the formatting and separator style.
struct KeyHint {
  std::string key_label;
  std::string action;
};

// Renders a row of key hints separated by " · ", e.g.
// "esc cancel · enter submit · ctrl+r history · / commands" (PRD section 41).
ftxui::Element KeyHintBar(std::vector<KeyHint> hints, const Theme& theme);

}  // namespace terminal_ui_kit
