#pragma once

#include <cstddef>

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

enum class ProgressStyle { kUnicodeBlocks, kAscii, kDots, kBraille };

struct ProgressBarOptions {
  std::size_t width = 20;
  ProgressStyle style = ProgressStyle::kUnicodeBlocks;
  bool show_percentage = true;
};

ftxui::Element ProgressBar(double fraction, const Theme& theme,
                            ProgressBarOptions options = {});

ftxui::Element ProgressBar(double value, double total, const Theme& theme,
                            ProgressBarOptions options = {});

}  // namespace terminal_ui_kit
