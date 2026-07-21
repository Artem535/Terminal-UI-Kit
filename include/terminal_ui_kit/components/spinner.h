#pragma once

#include <chrono>

#include <ftxui/component/component.hpp>

namespace terminal_ui_kit {

// A frame-by-frame spinner, built on FTXUI's own ftxui::spinner() glyph
// tables. `charset_index` selects an FTXUI built-in spinner charset.
ftxui::Component Spinner(int charset_index = 2,
                         std::chrono::milliseconds frame_duration = std::chrono::milliseconds(80));

}  // namespace terminal_ui_kit
