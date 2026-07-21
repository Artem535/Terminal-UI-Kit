#pragma once

#include "terminal_ui_kit/core/text_style.h"

namespace terminal_ui_kit {

// Semantic style roles shared across components (PRD section 14.1).
struct Theme {
  TextStyle primary;
  TextStyle secondary;
  TextStyle muted;
  TextStyle success;
  TextStyle warning;
  TextStyle error;
  TextStyle accent;
  TextStyle code;
  TextStyle addition;
  TextStyle deletion;
  TextStyle border;
  TextStyle selected;
  TextStyle focused;

  friend bool operator==(const Theme&, const Theme&) = default;
};

}  // namespace terminal_ui_kit
