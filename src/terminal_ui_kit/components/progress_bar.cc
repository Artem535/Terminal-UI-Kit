#include "terminal_ui_kit/components/progress_bar.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {
namespace {

double normalized(double fraction) {
  if (!std::isfinite(fraction)) {
    return 0.0;
  }
  return std::clamp(fraction, 0.0, 1.0);
}

std::string repeat(std::string_view glyph, std::size_t count) {
  std::string result;
  for (std::size_t index = 0; index < count; ++index) {
    result += glyph;
  }
  return result;
}

std::pair<std::string_view, std::string_view> glyphs(ProgressStyle style) {
  switch (style) {
    case ProgressStyle::kUnicodeBlocks:
      return {"█", "░"};
    case ProgressStyle::kAscii:
      return {"#", "-"};
    case ProgressStyle::kDots:
      return {"●", "○"};
    case ProgressStyle::kBraille:
      return {"⠿", "⠁"};
  }
  return {"█", "░"};
}

}  // namespace

ftxui::Element ProgressBar(double fraction, const Theme& theme, ProgressBarOptions options) {
  if (options.width == 0) {
    return ftxui::text("");
  }
  const double clamped = normalized(fraction);
  const std::size_t filled =
      static_cast<std::size_t>(clamped * static_cast<double>(options.width));
  const auto [filled_glyph, empty_glyph] = glyphs(options.style);
  ftxui::Elements elements = {
      ftxui::text(repeat(filled_glyph, filled)) | to_decorator(theme.accent),
      ftxui::text(repeat(empty_glyph, options.width - filled)) | to_decorator(theme.muted),
  };
  if (options.show_percentage) {
    elements.push_back(ftxui::text(" " + std::to_string(static_cast<int>(clamped * 100)) + "%") |
                       to_decorator(theme.secondary));
  }
  return ftxui::hbox(std::move(elements));
}

ftxui::Element ProgressBar(double value, double total, const Theme& theme,
                            ProgressBarOptions options) {
  return ProgressBar(total > 0.0 && std::isfinite(total) ? value / total : 0.0, theme, options);
}

}  // namespace terminal_ui_kit
