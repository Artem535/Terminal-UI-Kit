#include "terminal_ui_kit/document/ansi_parser.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace terminal_ui_kit {
namespace {

constexpr std::array<Color, 16> kAnsiColors = {
    Color{0, 0, 0},       Color{205, 0, 0},   Color{0, 205, 0},   Color{205, 205, 0},
    Color{0, 0, 238},     Color{205, 0, 205}, Color{0, 205, 205}, Color{229, 229, 229},
    Color{127, 127, 127}, Color{255, 0, 0},   Color{0, 255, 0},   Color{255, 255, 0},
    Color{92, 92, 255},   Color{255, 0, 255}, Color{0, 255, 255}, Color{255, 255, 255},
};

Color indexed_color(int index) {
  if (index < 0) index = 0;
  if (index < 16) return kAnsiColors[static_cast<std::size_t>(index)];
  if (index < 232) {
    constexpr std::array<std::uint8_t, 6> levels = {0, 95, 135, 175, 215, 255};
    const int value = index - 16;
    return Color{levels[static_cast<std::size_t>(value / 36)],
                 levels[static_cast<std::size_t>((value / 6) % 6)],
                 levels[static_cast<std::size_t>(value % 6)]};
  }
  const auto gray = static_cast<std::uint8_t>(8 + (index - 232) * 10);
  return Color{gray, gray, gray};
}

std::vector<int> parse_parameters(std::string_view parameters) {
  std::vector<int> result;
  std::size_t start = 0;
  while (start <= parameters.size()) {
    const std::size_t end = parameters.find(';', start);
    const std::string_view token = parameters.substr(
        start, end == std::string_view::npos ? parameters.size() - start : end - start);
    int value = 0;
    if (!token.empty()) {
      const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
      if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size()) value = 0;
    }
    result.push_back(value);
    if (end == std::string_view::npos) break;
    start = end + 1;
  }
  return result;
}

void set_color(std::optional<Color>& destination, int index) { destination = indexed_color(index); }

void apply_sgr(std::string_view parameters, TextStyle& style) {
  const std::vector<int> values = parse_parameters(parameters);
  for (std::size_t i = 0; i < values.size(); ++i) {
    const int code = values[i];
    switch (code) {
      case 0:
        style = TextStyle{};
        break;
      case 1:
        style.bold = true;
        break;
      case 2:
        style.dim = true;
        break;
      case 4:
        style.underline = true;
        break;
      case 9:
        style.strikethrough = true;
        break;
      case 22:
        style.bold = false;
        style.dim = false;
        break;
      case 24:
        style.underline = false;
        break;
      case 29:
        style.strikethrough = false;
        break;
      case 39:
        style.foreground.reset();
        break;
      case 49:
        style.background.reset();
        break;
      default:
        if (code >= 30 && code <= 37) {
          set_color(style.foreground, code - 30);
        } else if (code >= 40 && code <= 47) {
          set_color(style.background, code - 40);
        } else if (code >= 90 && code <= 97) {
          set_color(style.foreground, code - 90 + 8);
        } else if (code >= 100 && code <= 107) {
          set_color(style.background, code - 100 + 8);
        } else if ((code == 38 || code == 48) && i + 1 < values.size()) {
          const bool foreground = code == 38;
          const int mode = values[++i];
          if (mode == 5 && i + 1 < values.size()) {
            const int index = values[++i];
            if (foreground)
              set_color(style.foreground, index);
            else
              set_color(style.background, index);
          } else if (mode == 2 && i + 3 < values.size()) {
            const Color color{static_cast<std::uint8_t>(values[i + 1]),
                              static_cast<std::uint8_t>(values[i + 2]),
                              static_cast<std::uint8_t>(values[i + 3])};
            i += 3;
            if (foreground)
              style.foreground = color;
            else
              style.background = color;
          }
        }
        break;
    }
  }
}

}  // namespace

StyledText parse_ansi(std::string_view text) {
  StyledText result;
  TextStyle style;
  std::string plain;

  const auto flush = [&] {
    if (!plain.empty()) {
      result.append(TextSpan{std::move(plain), style, std::nullopt});
      plain.clear();
    }
  };

  for (std::size_t i = 0; i < text.size();) {
    if (text[i] != '\x1b') {
      plain.push_back(text[i++]);
      continue;
    }
    if (i + 1 >= text.size()) break;
    const char introducer = text[i + 1];
    if (introducer == '[') {
      std::size_t end = i + 2;
      while (end < text.size() && (text[end] < '@' || text[end] > '~')) ++end;
      if (end == text.size()) break;
      const char final = text[end];
      if (final == 'm') {
        TextStyle next_style = style;
        apply_sgr(text.substr(i + 2, end - (i + 2)), next_style);
        if (next_style != style) {
          flush();
          style = next_style;
        }
      }
      i = end + 1;
      continue;
    }
    if (introducer == ']') {
      std::size_t end = i + 2;
      while (end < text.size() && text[end] != '\a' &&
             !(text[end] == '\x1b' && end + 1 < text.size() && text[end + 1] == '\\')) {
        ++end;
      }
      i = end < text.size() && text[end] == '\x1b' ? end + 2 : end + 1;
      continue;
    }
    i += 2;
  }
  flush();
  return result;
}

}  // namespace terminal_ui_kit
