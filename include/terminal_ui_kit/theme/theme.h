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

// Built-in themes inspired by the GitHub Primer dark/light palettes (PRD
// section 14.2). Values are approximate, not verified byte-for-byte against
// Primer's published design tokens.
const Theme& default_dark_theme();
const Theme& default_light_theme();

// Strips foreground/background from every role, keeping bold/italic/
// underline/dim/strikethrough untouched. Consumers that need visual
// distinction without color (status icons, diff +/- markers) already carry
// that meaning outside of color.
Theme without_color(const Theme& theme);

}  // namespace terminal_ui_kit
