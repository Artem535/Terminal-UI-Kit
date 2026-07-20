#pragma once

#include <cstdint>
#include <optional>

namespace terminal_ui_kit {

// A terminal color, decoupled from any specific rendering backend so Core
// stays free of FTXUI types (PRD section 13.3). The terminal/theme layer
// maps this to concrete backend colors.
struct Color {
  std::uint8_t red = 0;
  std::uint8_t green = 0;
  std::uint8_t blue = 0;

  friend bool operator==(const Color&, const Color&) = default;
};

// Presentation attributes for a run of text. PRD section 14.1 references
// `TextStyle` as the Theme's field type without defining it; this is that
// definition.
struct TextStyle {
  bool bold = false;
  bool italic = false;
  bool underline = false;
  bool dim = false;
  bool strikethrough = false;
  std::optional<Color> foreground;
  std::optional<Color> background;

  friend bool operator==(const TextStyle&, const TextStyle&) = default;
};

}  // namespace terminal_ui_kit
