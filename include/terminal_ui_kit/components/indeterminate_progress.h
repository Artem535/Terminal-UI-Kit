#pragma once

#include <chrono>
#include <cstddef>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/components/progress_bar.h"

namespace terminal_ui_kit {

ftxui::Component IndeterminateProgress(
    const Theme& theme, ProgressBarOptions options = {}, std::size_t segment_width = 4,
    std::chrono::milliseconds frame_duration = std::chrono::milliseconds(80));

}  // namespace terminal_ui_kit
